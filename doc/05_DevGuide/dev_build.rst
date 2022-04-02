Development build
+++++++++++++++++

To build & test OpFlow itself, we suggest to toggle the example, tests and benchmarks options on during configuration:

.. code-block:: bash

    cmake .. -DOPFLOW_BUILD_EXAMPLES=ON -DOPFLOW_BUILD_TESTS=ON -DOPFLOW_BUILD_BENCHMARKS=ON [other options]

This configuration will generate three groups of targets:

- **Examples**  All examples targets will be generated, together with a unified target ``AllExamples`` to build all
  examples. Some examples have different configurations, e.g. ``FTCS`` and ``FTCS-OMP``.

- **Tests**  All tests targets will be generated, named with ``<TestName>Test``, together with a unified target
  ``AllTests`` to build all tests. If coverage is enabled, each test will conjugate with a coverage-enabled version
  ``<TestName>Test_coverage``. To run all tests, you can use CTest with ``ctest --parallel <n>`` in the build directory,
  where ``n`` is the count of parallel tests.

- **Benchmarks**  All benchmarks targets will be generated, named with ``BENCH_<BenchName>``, together with a unified
  target ``AllBenchmarks`` to build all benchmarks.

Besides, a target ``All_CI`` will be generated as a unified target of the three. Be sure to make build with ``All_CI`` pass
and all tests clear before submitting your PR!

For developers who want to contribute to the documentation, you may enable the ``OPFLOW_BUILD_DOC=ON`` option during
configuration. A ``doc`` target will be generated. Each time you finished your modification, you can run ``make doc``
and preview the generated html pages in the ``doc`` folder under the build directory.