Meta Programming
++++++++++++++++

Meta programming is used extensively in OpFlow. It has already been proved that C++'s template is Turing complete,
which means that you can compute anything computable during the compiling phase. For freshers of C++'s template
system, I strongly recommended the book `C++ Template Metaprogramming: Concepts, Tools, and Techniques from
Boost and Beyond <https://www.amazon.com/Template-Metaprogramming-Techniques-Documents-Depth-ebook/dp/B003XNTTBW>`_
by David Abrahams and Aleksey Gurtovoy, who are also the authors of the Boost MPL library. Briefly, meta programming
is used in OpFlow for 4 major purposes:

- **Static dispatching**. The entire type system of OpFlow is designed to be static (except some utilities and boundary
  condition related stuffs). By using curiously recurring template pattern (CRTP), the exact type of an object
  is well known at compile time. By using constrains for template parameters, operators can make targeted assumptions
  & optimizations for a specific typed (or class of typed) expressions. This is on contrary to the dynamic dispatch
  where operators can only use common interfaces provided by the base class thus losing chances for further optimization.

- **Dimension independent programming**. It's often the case where numerical developers first compose their methods on 1d
  or 2d cases, and turn to 3d cases when the debugging work is done. Re-writing a 2d algorithm into 3d can be boring,
  tedious, and error-prone. Since space symmetry is one of the fundamentals of physics, the fields & operators should
  have such symmetry, at least within the same dimension. Therefore, all meshes & fields in OpFlow have a template
  parameter of int indicating the dimension of that object. Furthermore, many operators are designed in 1d-form with
  a template parameter indicating the dimension to be operated on. Besides, the index & range system are also designed
  with a dimension template parameter. With these facilities, it's natural to write dimensionless code and instantiate
  the whole algorithm with your specific dimension number.

- **Expression templates**. The design of OpFlow's expression system is borrowed largely from Eigen, where they call it
  the `expression templates technique <http://eigen.tuxfamily.org/index.php?title=Expression_templates>`_. The core
  idea of this technique is: every intermediate expressions generated from legal operations should be treated
  **as-if** they were concrete expressions of the corresponding types. For example, the addition of two Cartesian fields
  ``u + v`` should be treated just as a Cartesian field, sharing the same APIs and being capable for anywhere requiring
  a Cartesian field. This can greatly reduce the amount of code needed to distinguish the intermediate results from
  concrete fields and maintain a clean API. From another aspect, intermediate expressions only **act-as** concrete
  expressions, but **not truly are**. This means that they are only light-weighted proxies which can be used extensively
  without performance impacts. The computation only happens on the assignment to a concrete expression. This is called
  `lazy evaluation` which is used extensively in modern compilers & frameworks.

- **Performance optimizing with compiler support**. Since the whole program is static, it's more possible for the
  compiler to do more aggressive optimization, such as inlining & auto vectorization. Although not implemented yet,
  explicit vectorization is also possible to be achieved with zero runtime overhead since blocking & instruction mapping
  can be done at the compile time.

To explain each of the advantages in more detail, we'll show some examples of how these features are achieved in OpFlow.

Static dispatching
------------------

As demonstrated in the :ref:`Expr` section, all expressions are organized into a tree by CRTP. For example, the branch
for ``CartesianField`` is:

.. code-block:: cpp

    CartesianField<Data, Mesh>
        : CartesianFieldExpr<CartesianField<Data, Mesh>>
            : StructuredFieldExpr<CartesianField<Data, Mesh>>
                : MeshBasedFieldExpr<CartesianField<Data, Mesh>>
                    : FieldExpr<CartesianField<Data, Mesh>>
                        : Expr<CartesianField<Data, Mesh>>

This inheritance allows a ``CartesianField`` to participate into any scenarios requiring a parent's typed object. Operators
can correspondingly require an argument to live on some level of abstraction. For example, element-wise operators such
as ``AddOp`` may require the operands to just be of ``FieldExpr`` s:

.. code-block:: cpp

    struct AddOp {
        template <FieldExprType T, FieldExprType U>
        OPFLOW_STRONG_INLINE static auto eval_at(const T& t, const U& u, auto&& i) {
            return t.evalAt(OP_PERFECT_FOWD(i)) + u.evalAt(OP_PERFECT_FOWD(i));
        }
    };

Static dispatch is involved when an operator can handle different types of expressions. For example, the first order
differential operator must use different scheme for Cartesian fields and CurvedLinear structured fields:

.. code-block:: cpp

    template <int d>
    struct FirstOrderDiffOp {
        template <CartesianFieldExprType T>
        OPFLOW_STRONG_INLINE static auto eval_at(const T& t, auto&& i) {
            return (t.evalAt(i.next<d>()) - t.evalAt(i)) / t.mesh.dx(d, i);
        }

        template <CurvedLinearFieldExprType T>
        OPFLOW_STRONG_INLINE static auto eval_at(const T& t, auto&& i) {
            return ...; // some other scheme
        }
    };

In such cases, we can use the same frontend API to operate on both types of fields, while switching to the fine-tuned
version of implementation at the compile time. Note that the operators operate on one element at one time. Dynamic
dispatch will have severe performance issue under this scenario.

Dimension independent programming
---------------------------------

We have partially explained this issue in the :ref:`index & range<Index, Range & RangeModel>` section. By using unified
indexes & ranges, we can loop over the entire field without explicitly specifying the dimension of it. Besides, dimension
infos are supplied as template parameters when build meshes and fields:

.. code-block:: cpp

    using Mesh = CartesianMesh<Meta::int_<2>>; // 2d mesh
    using Field = CartesianField<Real, Mesh>;  // dimension defined by Mesh

The dimension info can also be get via traits classes of these types:

.. code-block:: cpp

    constexpr int dim = internal::FieldExprTrait<Field>::dim;
    constexpr int m_dim = internal::MeshTrait<Mesh>::dim;

These are useful inside operators' implementations to know the input field's dimension. Also, many operators are designed
to be dimension splitted and take a dimension template parameter:

.. code-block:: cpp

    template <int d>
    struct FirstOrderDiff;

    auto d0_f = d1<FirstOrderDiff<0>>(f); // calculate along the first dimension
    auto d1_f = d1<FirstOrderDiff<1>>(f); // calculate along the second dimension

This avoids writing the same scheme for each combination of dimensions.

Expression templates
--------------------

Briefly, this is another achievement of the CRTP type system. We only need to show here how intermediate expression
are defined. Aside from ``Expr``, we define a templated type called ``Expression``:

.. code-block:: cpp

    template <typename ... Ts>
    struct Expression;

And it's partial specialized by the count of arguments:

.. code-block:: cpp

    template <typename Op> struct Expression<Op> {...}; // nullary operator
    template <typename Op, typename Arg1> struct Expression<Op, Arg1> {...}; // unary operator
    // etc.

The maximum of possible arguments for now is 6. For each of these ``Expression``, they are defined as:

.. code-block:: cpp

    template <typename Op, typename Arg1>
    struct Expression<Op, Arg1> : ResultType<Op, Arg>::type {
        typename internal::ExprProxy<Arg1>::type arg1;
    };

where the ``ExprProxy`` is defined as

.. code-block:: cpp

    template <typename T>
    struct ExprProxy;

    template <ExprType T>
    struct ExprProxy<T> {
        // if T is a concrete expr (usually a field), take the ref;
        // else (usually an expression) take T's copy
        using type = std::conditional_t<T::isConcrete(), Meta::RealType<T>&, Meta::RealType<T>>;
    };
    template <Meta::Numerical T>
    struct ExprProxy<T> {
        using type = T;
    };

``ResultType`` is defined alongside each operator. It tells what type of expression should the operator return
with the current argument. This completes the definition of an intermediate proxy expression type. As you can see,
the ``Expression`` only takes a reference to concrete field, or a copy of another proxy expression, which takes rarely
no extra memory space. Therefore, it's recommended to construct sub-expression first as intermediates, and assemble
them together later to get the whole expression. This can make your code clear and easy to read.

Performance optimizing with compiler support
--------------------------------------------

It'll be hard to demonstrate this issue with OpFlow directly, as it's already a complex & large project involving
too much unrelated stuffs. Instead, I shall give some snippets here to show how brilliant modern compilers are
such they can do amazing optimizations for our code:

- `Dynamic expr <https://godbolt.org/z/9YTxqG>`_ vs `Static expr <https://godbolt.org/z/E18hvY>`_. This pair of
  examples shows how far the compiler can optimize dynamic expressions vs static expressions. Both code implements
  an expression type and a corresponding add operator. The test function ``foo`` adds up 4 such expressions of ``int``.
  You can see the generated assembly for the dynamic case (at around line 1067) still contains calls to virtual
  functions. On the contrary, the static expr case (at the beginning) eliminates all function calls. All instructions
  are successfully inlined. Also note that the compile flag for the static case is set to ``-Og``. Switch it to ``-O3``
  leaves only add & pop instructions.

- `Auto vectorization <https://godbolt.org/z/Pv7q1c>`_. This case further shows how the compiler optimize static expressions
  of vector fields. With AVX512F enabled, the compiler automatically generate code for vectorized operations of 8, 4, 2 & 1
  doubles. Notice that there is no manually specified vector instructions in the code. Therefore, it's totally possible
  to leave the vectorization work to the compiler as long as enough information is provided.