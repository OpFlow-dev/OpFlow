Example 02: Poisson equation
++++++++++++++++++++++++++++

Definition
----------

This section will show you how to solve an implicit PDE - the Poisson equation within OpFlow.
The equation to be solved is:

.. math::
    \frac{\partial^2 u}{\partial x^2} + \frac{\partial^2 u}{\partial y^2} = 1.

together with the boundary condition:

.. math::
    u\bigg|_{\partial \Omega} = 0. \Omega=[0,1]\times[0,1]

Construct the solver
--------------------

We begin with defining the mesh & field to use:

.. code-block:: cpp

    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;

    // build a 11 x 11 nodal mesh & field
    auto m = MeshBuilder<Mesh>().newMesh(11, 11).setMeshOfDim(0, 0., 10.).setMeshOfDim(1, 0., 10.).build();
    auto u = ExprBuilder<Field>()
                     .setMesh(m)
                     .setBC(0, OpFlow::DimPos::start, BCType::Dirc, 0.)
                     .setBC(0, OpFlow::DimPos::end, BCType::Dirc, 0.)
                     .setBC(1, OpFlow::DimPos::start, BCType::Dirc, 0.)
                     .setBC(1, OpFlow::DimPos::end, BCType::Dirc, 0.)
                     .build();

Then we construct the solver to be used:

.. code-block:: cpp

    StructSolverParams<StructSolverType::GMRES> params;
    params.dumpPath = "u";
    params.printLevel = 2;
    params.staticMat = true;
    StructSolverParams<StructSolverType::PFMG> p_params;
    p_params.maxIter = 1;
    p_params.tol = 0.0;
    p_params.useZeroGuess = true;
    p_params.rapType = 0;
    p_params.relaxType = 1;
    p_params.numPreRelax = 1;
    p_params.numPostRelax = 1;
    p_params.skipRelax = 0;

    auto solver = PrecondStructSolver<StructSolverType::GMRES, StructSolverType::PFMG>(params, p_params);
    auto handler = makeEqnSolveHandler(
            [&](auto&& e) { return d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e) == 1.0; }, u,
            solver);

``params`` and ``p_params`` are parameters for the solver and preconditioner, respectively.
Options of these two parameters can be found in the :ref:`API section<APIs>` or `HYPRE's documentation
<https://hypre.readthedocs.io/en/latest/ch-intro.html>`_. Finally we solve the system by

.. code-block:: cpp

    handler.solve();

You may notice that we didn't use the unified solve interface here. By explicitly constructing
the solver handler, it can be reused in later computations, especially when the matrix is static and
only the rhs will be generated on each invoke of ``solve``. For one-time solving, we still recommend
the unified interface.

The above code should give you some output like

.. code-block::

    [2021-08-22 10:44:34.348] [info] [EqnHandler.cpp:main@37] Built solver handler.
    L2 norm of b: 9.000000e+00
    Initial L2 norm of residual: 9.000000e+00
    =============================================

    Iters     resid.norm     conv.rate  rel.res.norm
    -----    ------------    ---------- ------------
        1    3.130160e+00    0.347796   3.477956e-01
        2    3.683536e-01    0.117679   4.092818e-02
        3    4.864029e-02    0.132048   5.404477e-03
        4    2.117097e-03    0.043526   2.352330e-04
        5    1.183356e-04    0.055895   1.314840e-05
        6    1.814100e-05    0.153301   2.015667e-06
        7    1.964011e-06    0.108264   2.182235e-07


    Final L2 norm of residual: 1.964011e-06



    [2021-08-22 10:44:35.044] [info] [EqnHandler.cpp:main@39] Solver finished.

And you will find the dumped matrix & rhs files ``u_A.mat.00000`` & ``u_b.vec.00000``
under the execution path. These files show the coefficients of the matrix & rhs in detail,
and is helpful for debugging your algorithm.