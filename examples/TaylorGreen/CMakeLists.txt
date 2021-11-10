cmake_minimum_required(VERSION 3.10)
project(OpFlow_Cases_TG)

if (NOT TARGET opflow)
    find_package(opflow REQUIRED)
endif ()

add_example(TG TG.cpp)
target_compile_definitions(TG PRIVATE OPFLOW_THREAD_MODEL_OPENMP)
