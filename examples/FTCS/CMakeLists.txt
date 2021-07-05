cmake_minimum_required(VERSION 3.10)
project(OpFlow_Cases_FTCS)

add_executable(FTCS FTCS.cpp)
target_link_libraries(FTCS ${OpFlow_Libs})

set_target_properties(FTCS PROPERTIES COMPILE_FLAGS "-save-temps")