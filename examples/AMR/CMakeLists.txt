cmake_minimum_required(VERSION 3.10)
project(OpFlow_Cases_AMR)

add_executable(AMR AMR.cpp SSHypre.cpp HYPRE_test.cpp UniLS.cpp GridInit.cpp AMRLS.cpp AMRLS.hpp GridUpdate.cpp GridUpdate.hpp Poisson.cpp Poisson.hpp)
#target_precompile_headers(AMR PRIVATE pch.hpp)
target_link_libraries(AMR ${OpFlow_Libs})