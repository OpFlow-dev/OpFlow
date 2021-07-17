cmake_minimum_required(VERSION 3.10)
project(OpFlow_Cases_AMR)

add_executable(AMR EXCLUDE_FROM_ALL AMR.cpp SSHypre.cpp HYPRE_test.cpp
        GridInit.cpp GridUpdate.cpp GridUpdate.hpp Poisson.cpp Poisson.hpp)
target_link_libraries(AMR opflow::opflow)
target_compile_options(AMR PRIVATE ${OPFLOW_EXAMPLE_FLAGS})