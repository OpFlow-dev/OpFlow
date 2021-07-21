cmake_minimum_required(VERSION 3.10)
project(OpFlow_Cases_LevelSet)

if (NOT TARGET opflow)
    find_package(opflow REQUIRED)
endif ()

add_executable(LevelSet EXCLUDE_FROM_ALL LevelSet.cpp AMRLS.cpp UniLS.cpp)
target_link_libraries(LevelSet opflow::opflow)
if (APPLE)
    target_link_libraries(LevelSet c++)
endif ()
target_compile_options(LevelSet PRIVATE ${OPFLOW_EXAMPLE_FLAGS})