cmake_minimum_required(VERSION 3.10)
project(OpFlow_Cases_EqnHandler)

add_executable(EqnHandler EqnHandler.cpp)
target_link_libraries(EqnHandler ${OpFlow_Libs})