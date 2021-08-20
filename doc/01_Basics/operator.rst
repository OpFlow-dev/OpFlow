Operator
++++++++

Operator classification
-----------------------

Operator is another building block of OpFlow. All the possible operations can be applied on expressions
are classified as operators. Operators are classified by the number of required expressions, i.e.,
nullary operator (operators taking no argument), unary operator (operators taking one argument),
binary operator, etc. They can also be classified by their function, e.g., arithmetic operator,
finite difference operator, interpolation operator, logical operator, etc. Currently available operators
are:

=========== =============================== ======================= ======================
Arithmetic  FDM                             Interpolator            Logical
=========== =============================== ======================= ======================
Add         D1FirstOrderBiasedDownwind      D1IntpCenterToCorner    And
Mul         D1FirstOrderBiasedUpwind        D1IntpCornerToCenter    Or
Sub         D1FirstOrderCenteredDownwind    IndexShifter            Not
Div         D1FirstOrderCenteredUpwind      IndexShifter1D          BitXor
Mod         D1WENO53Upwind                                          BitAnd
Pow         D1WENO53Downwind                                        BitOr
Neg         D2SecondOrderCentered                                   GreaterThan
Pos                                                                 GreaterThanOrEqual
Sqrt                                                                LessThan
Min                                                                 LessThanOrEqual
Max                                                                 NotEqualTo
=========== =============================== ======================= ======================

Apply an operator
-----------------

To apply an operator to an expression, you can use the general expression builder function:

.. code-block:: cpp

    auto u, v = ...; // u and v are two expressions
    auto t = makeExpression<AddOp>(u, v); // build an expression of u Add v and assign to t

Most operators provides convenient functions or operator overloads which can help you compose
expressions more naturally and easily:

.. code-block:: cpp

    auto u, v = ...; // u and v are two expressions
    auto t = u + v;  // operator+ is overloaded to produce an expression
                     // equivalent to makeExpression<AddOp>(u, v)
    auto d = dx<D1FirstOrderBiasedDownwind>(u); // FDM operators provide dx, dy & dz wrappers
                                                // which take an operator as template parameter

Operator can be applied on fields, as well as results of expressions. Therefore, it's
totally appropriate to steam an expression into another operator and construct an complex
expression:

.. code-block:: cpp

    auto u, v, w = ...; // assume u, v and w are from a 3D velocity field
    auto div = dx<D1FirstOrderBiasedUpwind>(u)
             + dy<D1FirstOrderBiasedUpwind>(v)
             + dz<D1FirstOrderBiasedUpwind>(w); // compute the divergence of the velocity field

.. tip::
    Try compose one complex expression rather than multiple intermediate assignment. This can
    max the probability of data reuse.

You may start to get the feeling of what `OpFlow` stands for. Actually, OpFlow tries to follow
the so called "Data oriented programming" paradigm. The whole program supplies as a pipeline
from source data to the results. Operators take data from different streams, modifies them,
and supplies its result into a new stream. For details about OpFlow's design, please refer to
the Design Notes section.

Special operators
-----------------

There are also some situations where we need to combine several operators together and pick
one of them to perform the actual calculation depending on the local criteria. OpFlow currently
provides two composable operators: ``CondOp`` for conditional expression and ``DecableOp`` for
operator series with priority.

``CondOp`` can be applied by using the ``conditional`` method:

.. code-block:: cpp

    // construct the flux using either the upwind scheme and the downwind scheme
    auto flux_upwind = ...;
    auto flux_downwind = ...;
    // take either flux depending on the local wind direction
    u = u + dt * u * conditional(u > 0, flux_downwind, flux_upwind);

``conditional`` takes three arguments: the condition expression, the true branch expression and
the false branch expression. Each of the three arguments can be a legal expression.

``DecableOp`` are used to handle computations on the boundaries. High order operators are often
used to provide high fidelity results. But they usually requires a wide stencil to do the computation,
which cannot be satisfied at the boundary. By using ``DecableOp``, you can pack several operators
into a priority queue forming a composed operator. During evaluation of the composed operator,
it will try to use the operator with the highest priority while assuring the computation is legal.
For example, we want to use the WENO scheme at central field, and fall back to 1st order upwind
scheme at the boundary, then we can write:

.. code-block:: cpp

    auto flux_upwind = d1<DecableOp<D1WENO53Upwind<0>, D1FirstOrderBiasedUpwind<0>>>(u);

``DecableOp`` can also construct from DecableOps, i.e.,

.. code-block:: cpp

    // Make a complex op from N ops
    using CplxOp = DecableOp<Op0, DecableOp<Op1, DecableOp<Op2, ..., OpN>>...>;

As always, the composed operator can automatically handle boundary conditions. Focus on your
masterpiece and trust the orchestra. ^_^

.. note::
    You can now checkout the 1D convection example in the Example section. Try it out!