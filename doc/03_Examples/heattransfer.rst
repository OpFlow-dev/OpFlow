Example 03: Heat transfer
+++++++++++++++++++++++++

Description
-----------

This section will introduce the heat transfer example, showing how to use different parallelization plans
to run the case. The governing equation of the problem is

.. math::
    \frac{\partial u}{\partial t} = \alpha (\frac{\partial^2 u}{\partial x^2} + \frac{\partial^2 u}{\partial y^2})

We further propose the following boundary & initial conditions:

.. math::
    u\bigg|_{\partial\Omega}=1, u(x, 0)\bigg|_{\Omega\setminus\partial\Omega}=0, \Omega=[0, 1]\times[0,1]

We use the Forward Time Centered Space (FTCS) scheme to discretize the equation:

.. math::
    u^{n+1} = u^{n} + \Delta t \alpha (\frac{u_{i-1, j} - 2u_{i, j} + u_{i+1, j}}{\Delta x^2} +
        \frac{u_{i,j-1}-2u_{i,j}+u_{i,j+1}}{\Delta y^2})^n

Implementation
--------------

Just as the former examples, we create the mesh & field first:

.. code-block:: cpp

    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;

    constexpr auto n = 1025;
    auto mesh = MeshBuilder<Mesh>().newMesh(n, n).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();
    auto u = ExprBuilder<Field>()
                     .setName("u")
                     .setMesh(mesh)
                     .setBC(0, DimPos::start, BCType::Dirc, 1.)
                     .setBC(0, DimPos::end, BCType::Dirc, 1.)
                     .setBC(1, DimPos::start, BCType::Dirc, 1.)
                     .setBC(1, DimPos::end, BCType::Dirc, 1.)
                     .build();
    u = 0;

We initialize a ``HDF5Stream`` for data output:

.. code-block:: cpp

    Utils::H5Stream uf("./sol.h5"); uf.fixedMesh();
    uf << Utils::TimeStamp(0.) << u;

.. tip::
    Use ``fixedMesh`` to avoid writing meshes repeatedly.

Then we compose the main algorithm:

.. code-block:: cpp

    for (auto i = 1; i <= 50000; ++i) {
        if (i % 100 == 0) OP_INFO("Current step {}", i);
        u = u + dt * alpha * (d2x<D2SecondOrderCentered>(u) + d2y<D2SecondOrderCentered>(u));
        if (i % 1000 == 0) uf << Utils::TimeStamp(i * dt) << u;
    }
    uf.close();

.. caution::
    HDF5Stream needs to be closed explicitly by ``close`` method. This call must be placed before ``FinalizeEnvironment``,
    especially in MPI environment.

It's find to write ``u`` on each side of the assignment since an temporary field will be created automatically
by the backend engine.

To make the program MPI-Ready, you only have to add a few lines during the field building stage:

.. code-block:: cpp

    auto info = makeParallelInfo();
    setGlobalParallelInfo(info);
    setGlobalParallelPlan(makeParallelPlan(getGlobalParallelInfo(), ParallelIdentifier::DistributeMem));
    std::shared_ptr<AbstractSplitStrategy<Field>> strategy = std::make_shared<EvenSplitStrategy<Field>>();

    auto u = ExprBuilder<Field>()
            // ...
            .setPadding(1)
            .setSplitStrategy(strategy)
            .build();

On the cmake side, add the required macros to enable MPI support:

.. code-block:: cmake

    target_compile_definitions(FTCS-MPI PRIVATE OPFLOW_DISTRIBUTE_MODEL_MPI)

That's it! After build the program, run it with ``mpirun -np <nproc> ./FTCS-MPI``.

.. caution::
    The auto detection of system resources will get the maximum possible thread count for a single process.
    This means if you want to run on a 16-core machine with 4 MPI processes each with 4 OpenMP threads
    you must specify this configuration **manually**. Otherwise, the default setting will run 4 MPI processes
    each with 16 threads, which is not what we want.

Visualization
-------------

You can load the solution data file ``sol.h5`` to any HDF5-capable post-processing software. The data is
organized by ``/T=<time step>/u`` structure. You may get a time series plot like:

.. image:: assets/heattransfer.gif
    :width: 800
    :alt: heat transfer