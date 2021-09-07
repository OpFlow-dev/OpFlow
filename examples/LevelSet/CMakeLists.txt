cmake_minimum_required(VERSION 3.10)
project(OpFlow_Cases_LevelSet)

if (NOT TARGET opflow)
    find_package(opflow REQUIRED)
endif ()

add_example(LevelSet LevelSet.cpp AMRLS.cpp UniLS.cpp)
if (APPLE)
    target_link_libraries(LevelSet c++)
endif ()
