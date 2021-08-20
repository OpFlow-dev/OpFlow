Field
+++++

``Field`` is the build block of OpFlow's evaluation system. Fields are conceptually Exprs,
but they differ from general Exprs in that they are concrete objects owning allocated
data memory. All computations' source operands are eventually get from Fields.

Field concept
-------------

Field's concept is much alike Expr's, except that they provide special concepts for determining
if an expression is a ``Field`` but not a ``FieldExpr``:

.. code-block:: cpp

    // u, v are type of Field
    auto u = ExprBuilder<CartesianField>().build(); auto v = u;
    // t is declared as an intermediate expression
    auto t = u + v;

    static_assert(CartesianFieldExprType<decltype(u)>); // pass, u is a CartesianFieldExpr
    static_assert(CartesianFieldType<decltype(u)>);     // pass, u is a concrete CartesianField
    static_assert(CartesianFieldType<decltype(t)>);     // fail, t doesn't have allocated data memory

Using this series of concepts can constraint the input arguments where direct memory access
or writen access is required.

Build a Field
-------------

A ``Field`` is also constructed by the ``ExprBuilder``. For mesh based fields, there are usually
three steps for the build procedure: setting the name, mesh and the boundary condition.

Name is an optional identifier to a field, and are set by:

.. code-block:: cpp

    auto u = ExprBuilder<Field>().setName("u") //...

This identifier will be shown in the expressions involving the current field, e.g., by calling the
``getName()`` method on any expression:

.. code-block:: cpp

    auto n1 = u.getName();  // n1 == "u"
    auto t = u + 2 * u; t.prepare()
    auto n2 = t.getName();  // n2 = "u + 2 * u"

and in the output file as dataset's identifier. Although name is optional, picking a unique & proper
name can help you with debugging and postprocessing.

Mesh is a vital information for describing a field. After building the mesh, you can set the field's
mesh by:

.. code-block:: cpp

    auto m = MeshBuilder<Mesh>(). ... .build(); // built mesh
    auto u = ExprBuilder<Field>().setMesh(m)    // ...

Mesh determines the field's dimension, therefore it's usually a template parameter for the field.

The third and the most complex part is the boundary condition. OpFlow's field and expressions are
designed to be the union of all the internal data & boundary conditions. Therefore with a simple
expression the background engine knows where the boundary is, what the boundary type is and how
to calculate on the boundary. It's hard to find a universal boundary condition representation.
Here we classify the possible boundary conditions into three categories: Dirichlet boundary,
Neumann boundary and Logical boundary. [#f1]_ The first two types are further implemented in
two approaches: ``ConstBC`` and ``FunctorBC``. ``ConstBC`` stands for a constant
boundary condition of the corresponding type; ``FunctorBC`` takes a functor which takes an index and return
a boundary value. Logical boundary condition refers to conditions which are defined by its relation
between inner fields, e.g., the symmetric and asymmetric boundary conditions. To set a corresponding
boundary condition, you can use the ``setBC()`` method provided by the ``ExprBuilder``:

.. code-block:: cpp

    auto builder = ExprBuilder<Field>();
    // set a constant Dirichlet boundary condition of 1 at the start side of dim 0
    builder.setBC(0, DimPos::start, BCType::Dirc, 1);
    // set a functor Neumann boundary condition of y * y at the end side of dim 0
    builder.setBC(0, DimPos::end, BCType::Neum,
                  [mesh](auto&& i) { return Math::pow2(mesh.x(1, i[1])); });
    // set a periodic boundary condition at dim 1
    builder.setBC(1, DimPos::start, BCType::Periodic)
           .setBC(1, DimPos::end, BCType::Periodic);

.. note::
    Although the periodic boundary condition binds a pair of boundaries together at the same time, we still
    requires the user to provide the ``pos`` argument explicitly to simplify the inner logic for building
    the field.

.. rubric:: Footnotes
.. [#f1] Robin boundaries are currently not considered since they rarely come into my field :)

Assign & Eval
-------------

To initialize a newly built field, you can use the ``initBy()`` method:

.. code-block:: cpp

    // initialize the field to u(x, y) = x * x + y * y
    u.initBy([](auto&& x) { return x[0] * x[0] + x[1] * x[1]; });

or by a scalar, existing field or a general expression:

.. code-block:: cpp

    u = 0.;     // set the inner of u to 0
    u = v;      // set u's inner value to v
    u = v * v;  // set u's inner value to v's square

.. note::
    The statement ``auto u = v;`` will construct a copy of ``v`` instead of an intermediate expression.
    This feature is often used to create a series of fields shares the same configuration, e.g.,
    multiple middle versions of a field during a time integration step.

To evaluate at a specific index, you can use ``evalAt()`` and ``evalSafeAt()`` methods just as with an
``Expr``. To change the value at an index, you can use the ``operator[]`` and ``operator()`` to get
the reference to the data object (see Expr section). Nevertheless, the most common approach to assign
to a field is by assign an ``Expr`` to it, e.g.:

.. code-block:: cpp

    Field u, v, w;
    u = v + w;      // evaluate v + w and store the result to u
    v = d2x(w);     // evaluate the 2nd order derivative of w
                    // along x and store the result to v
    u = u + v;      // u itself can also appear on the right hand side
                    // A temporal copy will be created automatically

By using unified assignment syntax, the users can focus on composing the algorithm, while OpFlow can
automatically choose the best parallelism method and evaluation sequence of the rhs expression.
Boundary conditions are also carefully handled by the backend assignment engine. Therefore, it's
strongly recommended to write your code in this fashion instead of pointwise operations.

Field IO
--------

OpFlow provides streams for field IO. All available streams can be found at ``Utils/Writers``.
A stream is usually constructed by a filepath, e.g.:

.. code-block:: cpp

    RawBinaryOStream os("./");

and fields are written info file with time stamps as tags:

.. code-block:: cpp

    os << Utils::TimeStamp(0) << u; // record u at t = 0

Currently available streams' capabilities are list as following:

======================= =========== =========== =========== ============
Stream \ Feature        Input       Output      Encoding    Parallel I/O
======================= =========== =========== =========== ============
TecplotASCIIStream      No          Yes         ASCII       No
RawBinaryOStream        No          Yes         Binary      Yes
RawBinaryIStream        Yes         No          Binary      Yes
HDF5Stream              Yes         Yes         Binary      Yes
======================= =========== =========== =========== ============

For detailed usage of each stream, please to the API section and examples of OpFlow.