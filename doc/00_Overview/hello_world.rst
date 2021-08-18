Hello world
+++++++++++

This section will show you how to build & run your first "Hello world" program with OpFlow. Here we
suppose you have followed the previous section and successfully installed OpFlow at ``<install_dir>``.
Then pick a nice place and create our project folder, let's call it ``<project_dir>``. We first compose
the project's CMake configuration file, ``<project_dir>/CMakeLists.txt``:

.. code-block:: cmake

    cmake_minimum_required(VERSION 3.16)
    project(HelloWorld)

    set(CMAKE_CXX_STANDARD 20)

    find_package(opflow REQUIRED)
    add_executable(HelloWorld main.cpp)
    target_link_libraries(HelloWorld opflow::opflow)

And then the source file, ``<project_dir>/main.cpp``:

.. code-block:: cpp

    #include <OpFlow>

    int main(int argc, char* argv[]) {
        OP_INFO("Hello {} from OpFlow!", "world");
        return 0;
    }

Then make a build directory, ``<project_dir>/build``, and config under it by:

.. code-block:: bash

    cmake .. -Dopflow_DIR=<install_dir>/lib/cmake/opflow

Finally, build & run the project with:

.. code-block:: bash

    make -j && ./HelloWorld

Congratulations! You've made your first OpFlow-embedded program!

.. note::
    You can either introduce OpFlow as a subproject of your project. In this way installation can
    be omitted and a direct link to OpFlow with ``target_link_libraries(<target> opflow::opflow)``
    should be adequate.