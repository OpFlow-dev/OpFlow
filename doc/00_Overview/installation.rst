Installation
++++++++++++

OpFlow is itself a header-only library, yet the backend of linear solver and data writer is delegated
to HYPRE and VTK for now. Therefore, these two libraries need to be pre-installed to make OpFlow
fully functional. We recommend two ways to introduce OpFlow to your existing project:

.. note::
    OpFlow is targeting at modern C++ infrastructures and rely on CMake for overall management.
    Currently we only support using CMake to configure OpFlow. For other build system users, please
    find an alternative approach to convert the build process from CMake to your build system.

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
OPFLOW_BUILD_DOCS           Build OpFlow's document                         OFF
OPFLOW_SINGLE_PRECISION     Use ``float`` for default Real type             OFF                     Default Real type is ``double``
OPFLOW_WITH_OPENMP          Enable OpenMP for shared memory parallelization ON                      Keep ON for now
OPFLOW_WITH_MPI             Enable MPI for distributed parallelization      ON                      Keep ON for now
OPFLOW_SANITIZE_ADDRESS     Enable address sanitizer in tests               OFF
OPFLOW_BUILD_WARNINGS       Enable compiler warnings                        OFF
OPFLOW_INSTALL              Generate the install target                     OPFLOW_MASTER_PROJECT
OPFLOW_FMT_EXTERNAL         Use external fmt library                        OFF                     Must also set FMT_DIR if enabled
OPFLOW_FMT_EXTERNAL_HO      Use external fmt header-only library            OFF                     Must also set FMT_DIR if enabled
OPFLOW_SPDLOG_EXTERNAL      Use external spdlog library                     OFF                     Must also set SPDLOG_DIR if enabled
OPFLOW_SPDLOG_EXTERNAL_HO   Use external spdlog header-only library         OFF                     Must also set SPDLOG_DIR if enabled
OPFLOW_HYPRE_EXTERNAL       Use external HYPRE library                      OFF                     Must also set HYPRE_DIR if enabled
OPFLOW_HYPRE_PRE_DOWNLOAD   Use pre-downloaded HYPRE src for build          OFF                     Must also set OPFLOW_HYPRE_SOURCE_DIR
                                                                                                    to the path of the source
OPFLOW_VTK_EXTERNAL         Use external VTK library                        OFF                     Must also set VTK_DIR if enabled
OPFLOW_VTK_PRE_DOWNLOAD     Use pre-downloaded VTK src for build            OFF                     Must also set OPFLOW_VTK_SOURCE_DIR
                                                                                                    to the path of the source
OPFLOW_NO_EXCEPTIONS        Compile with ``-fno-exceptions``                OFF
=========================== =============================================== ======================= =======================================

Don't be frightened by the available options. Typically, you can build OpFlow in two styles:

- **Fully automatic style**

  If you haven't install any of the support libraries and just want to try out OpFlow,
  you can let OpFlow download & install all its dependencies for you (although it's
  still necessary and beneficial for you to manually install the compiler and MPI
  library systematically). You can then configure OpFlow as:

.. code-block:: bash

    cmake .. -DCMAKE_C_COMPILER=gcc-10 -DCMAKE_CXX_COMPILER=g++-10
             -DCMAKE_INSTALL_PREFIX=<your-preferred-dir>

Add options for tests, examples and docs at your demand.

- **Developer/Offline style**

  If you want to play with the code of OpFlow, or trying to deploy OpFlow to an offline environment,
  you can configure OpFlow in this more detailed mode. This can save you large amount of time of
  configuring & building external dependencies. Firstly, you need to install the HYPRE & VTK libraries
  manually, either through package manager on your system or build from source. Then, config OpFlow as:

.. code-block:: bash

    cmake .. -DCMAKE_C_COMPILER=gcc-10 -DCMAKE_CXX_COMPILER=g++-10
             -DCMAKE_INSTALL_PREFIX=<your-preferred-dir>
             -DOPFLOW_HYPRE_EXTERNAL=ON -DHYPRE_DIR=<path-to-HYPREConfig.cmake>
             -DOPFLOW_VTK_EXTERNAL=ON -DVTK_DIR=<path-to-vtk-config.cmake>

As before, you can add options for tests, examples and docs at your demand.

.. note::
    You can use any C++20 compatible (or specifically, concept-ready) compiler for compile. The author
    has tested `gcc-10` and `gcc-11` to be working properly.

After configuration, type ``make -j && make install`` to issue the build & deployment. After that,
turn to your own project, add the following line to your ``CMakeLists.txt``

.. code-block:: cmake

    find_package(opflow CONFIG REQUIRED)
    target_link_libraries(your-exe opflow::opflow [<your-other-libs>...])

and config your project with

.. code-block:: bash

    cmake .. -Dopflow_DIR=<path-to-opflowConfig.cmake> [<your-other-options>...]

Your project should now compile correctly with OpFlow.