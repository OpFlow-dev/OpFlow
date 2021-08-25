cmake_minimum_required(VERSION 3.10)
project(OpFlow_Cases_FTCS)

if (NOT TARGET opflow)
    find_package(opflow REQUIRED)
endif ()

add_example(FTCS FTCS.cpp)