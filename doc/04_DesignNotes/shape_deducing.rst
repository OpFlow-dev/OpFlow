Shape deducing & Boundary handling
++++++++++++++++++++++++++++++++++

The shape of the expressions is critical for operators to behave properly. As mentioned in the :ref:`range model
<Range model>` section, we use 4-range model to describe the shape of an expression. This property is initialized
during the building phase of concrete fields, and during the ``prepare()`` phase of an intermediate expression.
These ranges must be initialized before any computations are made.

For the boundary conditions of intermediate expressions, we try to calculate the boundary conditions as much as
possible. For untouched boundaries, we keep the original boundary conditions and copy them to the new expression.
For boundaries cannot be determined, we leave the boundary condition to ``Undefined``. Besides, to avoid introduce
branches by boundaries into the main loop, we separate the boundary calculation from the inner zone. This is achieved
by three facilities:

- Each operator will hold a constexpr value of int named ``bc_width`` indicating the potential boundary affected
  zone's width:

.. code-block:: cpp

    template <...>
    struct FirstOrderBiasedDiff {
        constexpr static int bc_width = 1;
        // ...
    };

- Each expression also has a ``bc_width`` parameter indicating the boundary affected zone's width, which is recorded
  in the trait class:

.. code-block:: cpp

    template <...>
    struct Expression<Op, Arg1>;

    template <...>
    struct ExprTrait<Expression<Op, Arg1>> {
        constexpr static int bc_width = Op::bc_width + ExprTrait<Arg1>::bc_width;
    };

- During the computation, the targeted range is split into inner & outer ranges. For each of the outer ranges
  (usually multiple slim slices), the ``evalSafeAt()`` is used with all branches considered. For the inner range,
  the ``evalAt()`` is used which only considers the trivial path:

.. code-block:: cpp

    auto bc_ranges = range.getBCRanges(width);
    for (const auto& r : bc_ranges)
        // all branches enabled

    auto inner_range = range.getInnerRange(width);
    rangeFor(inner_range, [&](auto&& i) {
        // only consider the trivial path
    });

By this means, we extract the most time consuming computation of the inner range, which can be readily optimized with
aggressive optimizations, while keeping the calculations near the boundary correct & safe.