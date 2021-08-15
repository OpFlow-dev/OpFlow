cmake_minimum_required(VERSION 3.10)
project(OpFlow_Cases_EqnHandler)

if (NOT TARGET opflow)
    find_package(opflow REQUIRED)
endif ()

add_executable(EqnHandler EXCLUDE_FROM_ALL EqnHandler.cpp)
target_link_libraries(EqnHandler opflow::opflow)
target_compile_options(EqnHandler PRIVATE ${OPFLOW_EXAMPLE_FLAGS})

add_executable(EqnHandler-MPI EXCLUDE_FROM_ALL EqnHandler-MPI.cpp)
target_link_libraries(EqnHandler-MPI opflow::opflow)
target_compile_options(EqnHandler-MPI PRIVATE ${OPFLOW_EXAMPLE_FLAGS})
target_compile_definitions(EqnHandler-MPI PRIVATE OPFLOW_DISTRIBUTE_MODEL_MPI)