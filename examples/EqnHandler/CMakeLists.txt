cmake_minimum_required(VERSION 3.10)
project(OpFlow_Cases_EqnHandler)

if (NOT TARGET opflow)
    find_package(opflow REQUIRED)
endif ()

add_example(EqnHandler EqnHandler.cpp)

add_example(EqnHandler-MPI EqnHandler-MPI.cpp)
target_compile_definitions(EqnHandler-MPI PRIVATE OPFLOW_DISTRIBUTE_MODEL_MPI)