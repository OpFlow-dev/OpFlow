cmake_minimum_required(VERSION 3.10)
project(OpFlow_Cases_EqnHandler)

if (NOT TARGET opflow)
    find_package(opflow REQUIRED)
endif ()

add_example(EqnHandler EqnHandler.cpp)

add_mpi_example(EqnHandler-MPI EqnHandler-MPI.cpp)

add_example(EqnHandlerAMGCL EqnHandlerAMGCL.cpp)

add_mpi_example(EqnHandlerAMGCL-MPI EqnHandlerAMGCL-MPI.cpp)