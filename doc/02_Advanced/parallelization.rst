Parallelization
+++++++++++++++

Parallelization is crucial to scientific computing, especially for large cases involving
millions or even trillions of elements. OpFlow provides two levels of parallelization,
namely the multithreading & multiprocessing parallelization. Currently, they are implemented
with OpenMP and MPI, respectively. This section will illustrate how to properly setup
the parallel configuration for a program, as well as some utilities for workload
balancing.

.. note::
    To enable the multiprocessing parallelization, OPFLOW_WITH_MPI must be specified
    during the cmake configuration phase. Also, a ``OPFLOW_DISTRIBUTE_MODEL_MPI``
    macro needs to be defined before including OpFlow. (Hint: this can be set by the
    cmake command ``target_compile_definitions(<proj_name> PRIVATE OPFLOW_DISTRIBUTE_MODEL_MPI)``

Environment setup
-----------------

Currently, OpFlow shares the parallel configuration throughout the program. Therefore,
a global configuration of environment is needed before declare any OpFlow objects:

.. code-block:: cpp

    int main(int argc, char* argv[]) {
        // pass the command line args to the initializer
        InitEnvironment(argc, argv);

        // ...
        // call the finalizer at the end of program
        FinalizeEnvironment();
        return 0;
    }

To configure the number & type of workers, a ``ParallelPlan`` needs to be generated and
registered:

.. code-block:: cpp

    // collect the available parallel resources on the current system
    auto info = makeParallelInfo();
    // you can modify the info to use specified number of processors
    // register the global parallel info
    setGlobalParallelInfo(info);
    // construct a parallel plan & register to the environment
    setGlobalParallelPlan(makeParallelPlan(getGlobalParallelInfo(), ParallelIdentifier::DistributeMem));

``makeParallelPlan`` takes a ``ParallelInfo`` object and a bit flag describing the parallel mode to be
used. Possible masks are: ``None``, ``DistributeMem``, ``SharedMem`` and ``Heterogeneous``. The
constructed ``ParallelPlan`` object records the number of workers for each level of parallelism.
It can be accessed globally via ``getGlobalParallelPlan``.

Range based loops
-----------------

Although OpFlow is designed to let users write loop-free codes as much as possible, there are cases
where a simple loop does the job, e.g., printing the data to the console or interacting with
non-OpFlow data structures. Therefore, two range based loops, i.e., ``rangeFor`` and ``rangeFor_s``,
are introduced to do the job. Both of them do the same work, despite that ``rangeFor_s`` will run
in serial mode while ``rangeFor`` will try to use multithreading as much as possible. For example,

.. code-block:: cpp

    CartesianField<Real, Mesh> f;
    // print f's value to the console
    rangeFor_s(f.accessibleRange, [&](auto&& i) {
        OP_INFO("f[{}] = {}", i.toString(), f[i]);
    });

    // copy the data of f to a C array
    Real buffer[nx][ny];
    rangeFor(f.accessibleRange, [&](auto&& i) {
        buffer[i[0]][i[1]] = f[i];
    });

Both of them takes a range and a functor as arguments. The functor takes an index type corresponding to
the range's type, which indicates the current position of the loop.

SplitStrategy & Distributed data maintenance
--------------------------------------------

The first thing to do when using distributed memory parallelization is to properly divide the total
work to each node as evenly as possible. To make this process automatically, OpFlow introduces the
concept of `Split Strategy`. For example, you can create an ``EvenSplitStrategy`` object

.. code-block:: cpp

    std::shared_ptr<AbstractSplitStrategy<Field>> strategy
            = std::make_shared<EvenSplitStrategy<Field>>();

and pass it to the builder while building the fields

.. code-block:: cpp

    auto u = builder.setPadding(1).setSplitStrategy(strategy).build();

The builder will automatically calculate the split and store the split range into ``u``'s ``localRange``.
The ``setPadding`` method is used to reserve halo zones for internal data communication.

.. caution::
    Due to the use-after-declaration constraint by the embedded language, OpFlow currently
    cannot deduce the required padding width from the algorithms composed later. It's the
    user's responsibility to set an appropriate width of padding.

Typically there is no other changes to make for parallelization. All fields are automatically
updated after assignment. If you need to make some local changes, do remember to invoke
``updatePadding()`` after your modification:

.. code-block:: cpp

    f[DS::MDIndex<2>{10, 10}] = 1.;
    // call updatePadding to exchange the padding zones
    f.updatePadding();

.. note::
    You can now checkout the :ref:`Heat transfer example<Example 03: Heat transfer>`_ and compare the
    performance with different parallelization configurations. Check it out!
