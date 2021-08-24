cmake_minimum_required(VERSION 3.10)
project(OpFlow_Cases_LidDriven)

if (NOT TARGET opflow)
    find_package(opflow REQUIRED)
endif ()

add_example(LidDriven2D LidDriven2D.cpp)

add_example(LidDriven3D LidDriven3D.cpp)

add_example(LidDriven2D-MPI LidDriven2D-MPI.cpp)
target_compile_definitions(LidDriven2D-MPI PRIVATE OPFLOW_DISTRIBUTE_MODEL_MPI)