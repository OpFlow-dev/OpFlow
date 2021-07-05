cmake_minimum_required(VERSION 3.10)
project(OpFlow_Cases_CONV1D)

add_executable(CONV1D CONV1D.cpp)
target_link_libraries(CONV1D ${OpFlow_Libs})