cmake_minimum_required(VERSION 3.10)
project(OpFlow_Cases_LidDriven)

add_executable(LidDriven2D LidDriven2D.cpp)
target_link_libraries(LidDriven2D ${OpFlow_Libs})

add_executable(LidDriven3D LidDriven3D.cpp)
target_link_libraries(LidDriven3D  ${OpFlow_Libs})