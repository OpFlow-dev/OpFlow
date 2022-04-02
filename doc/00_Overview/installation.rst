Installation
++++++++++++

OpFlow is itself a header-only library, yet the backend of linear solver and data writer is delegated
to third-party libraries for now. Therefore, these two libraries need to be pre-installed to make OpFlow
fully functional. We recommend two ways to introduce OpFlow to your existing project:

.. note::
    OpFlow is targeting at modern C++ infrastructures and rely on CMake for overall management.
    Currently we only support using CMake to configure OpFlow. For other build system users, please
    find an alternative approach to convert the build process from CMake to your build system.

.. caution::
    OpFlow is currently only tested on Linux and macOS. Windows users are recommended to use WSL for now.

Pre-requirement
---------------------------------------------------------------------

There are some packages needs to be installed before you can successfully build OpFlow with your projects.
Depending on the components you need and OS you work with, there are several different conditions:

- **Dependencies for all** A C++20 compatible compiler and the boost library are needed for all configurations:

.. code-block:: bash

    # macOS with homebrew
    brew install gcc boost
    # Ubuntu with apt
    sudo apt install gcc-10 g++-10 libboost-all-dev

- **Build with MPI** The MPI library is needed:

.. code-block:: bash

    # macOS with homebrew
    brew install open-mpi
    # Ubuntu with apt
    sudo apt install libopenmpi-dev

- **Build with doc** The doxygen, sphinx and other supporting python libraries are needed:

.. code-block:: bash

    # macOS with homebrew
    brew install doxygen python3 python3-pip
    # Ubuntu with apt (needs doxygen >= v1.9.2, currently not included in Ubuntu 20.04's repo)
    sudo apt install doxygen python3-pip python3-sphinx
    # All platforms
    python3 -m pip install sphinx sphinx-rtd-dark-mode breathe

TL;DR: Copy & paste the command in your shell and you're ready to go :)

macOS:

.. code-block:: bash

    brew install gcc doxygen tbb llvm lcov boost open-mpi hdf5-mpi python3 && python3 -m pip install sphinx sphinx-rtd-dark-mode breathe

Ubuntu:

.. code-block:: bash

    sudo apt install -y gcc-10 g++-10 doxygen python3 python3-pip python3-sphinx lcov libboost-all-dev libomp-12-dev clang-12 libopenmpi-dev libhdf5-mpi-dev libhdf5-dev && python3 -m pip install sphinx sphinx-rtd-dark-mode breathe

Approach 1: Via ``add_subdirectory()``
---------------------------------------------------------------------

You can copy the source code of OpFlow in a subdirectory of your project either by git submodule:
(we assume the destination directory is ``external/OpFlow``)

.. code-block:: bash

    git submodule add https://github.com/OpFlow-dev/OpFlow external/OpFlow

or by directly download & unpack the source code into your project. Then in your project's major
``CMakeLists.txt``, you can add the line

.. code-block:: cmake

    add_subdirectory(external/OpFlow)

to introduce the OpFlow project. The config file for OpFlow will generate the linkable target
``opflow::opflow`` for later usage, e.g.:

.. code-block:: cmake

    add_executable(your-exe <your-src-files>)
    target_link_libraries(your-exe opflow::opflow [<your-other-libs>...])

That's it! You have made the first project with OpFlow support.

.. note::
    This is also the recommended approach to introduce OpFlow into your project as it can share
    the same compiler flags with your project to avoid unexpected linkage errors, while assuming
    the same layout models (e.g. flags for SIMD & alignment) throughout the project.

Approach 2: Via ``find_package()``
----------------------------------

If you want to maintain a standalone installation of OpFlow, there is also an approach to achieve that.
First, clone the main repository of OpFlow by

.. code-block:: bash

    git clone https://github.com/OpFlow-dev/OpFlow

And then make a new directory for build files, e.g., ``./build``

.. code-block:: bash

    cd OpFlow && mkdir build && cd build

And then run cmake to config the build

.. code-block:: bash

    cmake .. <options>

Here are some options you can specify to control the build process:

=========================== =============================================== ======================= =======================================
Options (pass by -D)        Description                                     Default Value           Notes
=========================== =============================================== ======================= =======================================
CMAKE_C_COMPILER            The C compiler used for compilation             cc                      Optional
CMAKE_CXX_COMPILER          The C++ compiler used for compilation           cxx                     Optional, needs to be C++20 compatible
CMAKE_BUILD_TYPE            The build type for OpFlow                       Release                 Optional
CMAKE_CXX_STANDARD          The standard of C++ used for compilation        20                      C++20 is required for OpFlow to compile
CMAKE_INSTALL_PREFIX        The install prefix                              System default
OPFLOW_BUILD_ALL            Build all targets (tests, examples, docs)       OFF
OPFLOW_BUILD_TESTS          Build OpFlow's test sets                        OFF
OPFLOW_BUILD_EXAMPLES       Build OpFlow's examples                         OFF
OPFLOW_BUILD_BENCHMARKS     Build OpFlow's benchmarks                       OFF
OPFLOW_BUILD_DOCS           Build OpFlow's document                         OFF
OPFLOW_SINGLE_PRECISION     Use ``float`` for default Real type             OFF                     Default Real type is ``double``
OPFLOW_WITH_OPENMP          Enable OpenMP for shared memory parallelization ON                      Keep ON for now
OPFLOW_WITH_MPI             Enable MPI for distributed parallelization      ON                      Keep ON for now
OPFLOW_WITH_HDF5            Enable HDF5 for distributed parallel I/O        OFF                     MPI version of HDF5 is needed to enable
OPFLOW_SANITIZE_ADDRESS     Enable address sanitizer                        OFF
OPFLOW_SANITIZE_LEAK        Enable memory leak sanitizer                    OFF
OPFLOW_SANITIZE_THREAD      Enable thread sanitizer                         OFF
OPFLOW_SANITIZE_UB          Enable undefined behavior sanitizer             OFF
OPFLOW_BUILD_WARNINGS       Enable compiler warnings                        OFF
OPFLOW_ENABLE_COVERAGE      Enable coverage for tests                       OFF
OPFLOW_INSTALL              Generate the install target                     OPFLOW_MASTER_PROJECT
OPFLOW_HYPRE_EXTERNAL       Use external HYPRE library                      OFF                     Must also set HYPRE_DIR if enabled
OPFLOW_VTK_EXTERNAL         Use external VTK library                        OFF                     Must also set VTK_DIR if enabled
OPFLOW_VTK_PRE_DOWNLOAD     Use pre-downloaded VTK src for build            OFF                     Must also set OPFLOW_VTK_SOURCE_DIR
                                                                                                    to the path of the source
OPFLOW_TBB_EXTERNAL         Use external TBB library                        OFF                     Must also set TBB_DIR if enabled
OPFLOW_NO_EXCEPTIONS        Compile with ``-fno-exceptions``                OFF
=========================== =============================================== ======================= =======================================

Don't be frightened by the available options. Typically, you can build OpFlow with the default settings:

.. code-block:: bash

    cmake .. -DCMAKE_C_COMPILER=gcc-10 -DCMAKE_CXX_COMPILER=g++-10
             -DCMAKE_INSTALL_PREFIX=<your-preferred-dir>

As before, you can add options for tests, benchmarks, examples and docs at your demand.

.. note::
    You can use any C++20 compatible (or specifically, concept-ready) compiler for compile. The author
    has tested with `gcc` >= 10.0 and `clang` >= 12.

After configuration, type ``make -j && make install`` to issue the build & deployment. After that,
turn to your own project, add the following line to your ``CMakeLists.txt``

.. code-block:: cmake

    find_package(opflow CONFIG REQUIRED)
    target_link_libraries(your-exe opflow::opflow [<your-other-libs>...])

and config your project with

.. code-block:: bash

    cmake .. -Dopflow_DIR=<path-to-opflowConfig.cmake> [<your-other-options>...]

Your project should now compile correctly with OpFlow.