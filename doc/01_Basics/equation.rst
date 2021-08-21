Equation
++++++++

Equation concept
----------------

Apart from doing explicit calculations, an important requirement of numerical methods is the ability to
solve implicit systems, especially linear systems. Such a problem is represented by the ``Equation``
object in OpFlow:

.. code-block:: cpp

    // an equation for 2D Poisson equation
    auto eqn = (d2x(p) + d2y(p) == div);

Construct an equation
---------------------

``Equation`` is constructed by the ``operator==`` by putting left hand side and right hand side expression
on each side. You may noticed that in the :ref:`Operator` section the equality operator is not listed in the table.
It's used for generate equations in OpFlow, which we think is the more common use case than the equality test.

.. tip::
    For equality test, you can use the NotEqualTo ``operator!=`` to do the job.

Linear solver & equation solving
--------------------------------

The code above actually cannot be solved directly, since ``p`` and ``div`` are expressions that can be directly
evaluated. The target variable must be explicitly marked out and the linear system can thus be generated.
This is accomplished by introducing a ``StencilField`` of the target field and replace the target field with
the stencil field:

.. code-block:: cpp

    // generate the stencil field object for p
    auto stField = p.getStencilField();
    auto eqn = (d2x(stField) + d2y(stField) == div);

After the equation has been constructed, it's time to pick a proper linear solver and solve the linear system.
OpFlow currently uses HYPRE as its backend for linear system solving. HYPRE provides high performance preconditioners
and linear solvers for various types of linear systems including structured, semi-structured, unstructured and
matrix form linear systems. The available solvers include:

- BiCGSTAB
- CycRed
- FGMRES
- GMRES
- Jacobi
- LGMRES
- PCG
- PFMG
- SMG

The default solver for general equations in OpFlow is GMRES. We can construct a solver as:

.. code-block:: cpp

    // params & precParams are parameters for the solver and preconditioner
    auto solver = PrecondStructSolver<type, pType>(params, precParams);
    // bind the solver, target and equation to an equation handler
    auto handler = EqnSolveHandler(eqn, p, solver);
    // call the solve method to solve the system
    handler.solve();

To simplify the solving procedure, OpFlow introduce a unified ``Solve`` function to automatically
do the work aforementioned. The above codes can be replaced by:

.. code-block:: cpp

    Solve([&](auto&& e) { return d2x(e) + d2y(e) == div; }, p, params, precParams);

``Solve`` takes a functor as the equation generator, which takes a general field and returns an
corresponding equation. The target field's ``getStencilField`` method is automatically called,
and the generated stencil field is feed into the functor to get the equation; meanwhile a solver
is constructed with the parameters provided; then the equation is finally solved and the result
is written into the target field.

.. caution::
    It's the user's responsibility to ensure that the linear system is well purposed. For details
    of the linear solvers, please refer to
    `HYPRE's documentation <https://hypre.readthedocs.io/en/latest/ch-intro.html>`_.

.. note::
    You can now checkout the :ref:`Poisson equation example<Example 02: Poisson Equation>` in the
    Example section. Try it out!