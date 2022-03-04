cmake_minimum_required(VERSION 3.10)
project(OpFlow_Cases_LidDriven)

if (NOT TARGET opflow)
    find_package(opflow REQUIRED)
endif ()

add_example(LidDriven2D LidDriven2D.cpp)

add_example(LidDriven3D LidDriven3D.cpp)

add_mpi_example(LidDriven2D-MPI LidDriven2D-MPI.cpp)