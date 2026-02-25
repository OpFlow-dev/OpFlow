cmake_minimum_required(VERSION 3.10)
project(OpFlow_Cases_AMR)

set(OPFLOW_AMR_SOURCES
        AMR.cpp
        GridInit.cpp
        GridUpdate.cpp
        GridUpdate.hpp
        Poisson.cpp
        Poisson.hpp)

if (OPFLOW_HYPRE_HAS_SSTRUCT_FAC)
    list(APPEND OPFLOW_AMR_SOURCES SSHypre.cpp HYPRE_test.cpp)
endif ()

add_example(AMR ${OPFLOW_AMR_SOURCES})
