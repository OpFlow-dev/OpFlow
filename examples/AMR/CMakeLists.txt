cmake_minimum_required(VERSION 3.10)
project(OpFlow_Cases_AMR)

add_example(AMR AMR.cpp SSHypre.cpp HYPRE_test.cpp
        GridInit.cpp GridUpdate.cpp GridUpdate.hpp Poisson.cpp Poisson.hpp)