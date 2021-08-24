cmake_minimum_required(VERSION 3.10)
project(OpFlow_Cases_FTCS-MPI)

if (NOT TARGET opflow)
    find_package(opflow REQUIRED)
endif ()

add_example(FTCS-OMP FTCS-OMP.cpp)
target_compile_definitions(FTCS-OMP PRIVATE OPFLOW_THREAD_MODEL_OPENMP)

add_example(FTCS-MPI FTCS-MPI.cpp)
target_compile_definitions(FTCS-MPI PRIVATE OPFLOW_DISTRIBUTE_MODEL_MPI)