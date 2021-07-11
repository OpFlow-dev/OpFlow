cmake_minimum_required(VERSION 3.10)
project(OpFlow_Cases_CONV1D)

if (NOT TARGET opflow)
    find_package(opflow REQUIRED)
endif ()

add_executable(CONV1D EXCLUDE_FROM_ALL CONV1D.cpp)
target_link_libraries(CONV1D opflow::opflow)
target_compile_options(CONV1D PRIVATE ${OPFLOW_EXAMPLE_FLAGS})