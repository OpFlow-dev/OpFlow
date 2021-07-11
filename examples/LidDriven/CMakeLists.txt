cmake_minimum_required(VERSION 3.10)
project(OpFlow_Cases_LidDriven)

if (NOT TARGET opflow)
    find_package(opflow REQUIRED)
endif ()

add_executable(LidDriven2D EXCLUDE_FROM_ALL LidDriven2D.cpp)
target_link_libraries(LidDriven2D PRIVATE opflow::opflow)
target_compile_options(LidDriven2D PRIVATE ${OPFLOW_EXAMPLE_FLAGS})

add_executable(LidDriven3D EXCLUDE_FROM_ALL LidDriven3D.cpp)
target_link_libraries(LidDriven3D PRIVATE opflow::opflow)
target_compile_options(LidDriven3D PRIVATE ${OPFLOW_EXAMPLE_FLAGS})