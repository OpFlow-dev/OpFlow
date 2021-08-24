cmake_minimum_required(VERSION 3.10)
project(OpFlow_Cases_CONV1D)

if (NOT TARGET opflow)
    find_package(opflow REQUIRED)
endif ()

add_example(CONV1D CONV1D.cpp)