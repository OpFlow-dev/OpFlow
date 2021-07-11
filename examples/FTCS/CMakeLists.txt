cmake_minimum_required(VERSION 3.10)
project(OpFlow_Cases_FTCS)

if (NOT TARGET opflow)
    find_package(opflow REQUIRED)
endif ()

add_executable(FTCS EXCLUDE_FROM_ALL FTCS.cpp)
target_link_libraries(FTCS opflow::opflow)
target_compile_options(FTCS PRIVATE ${OPFLOW_EXAMPLE_FLAGS})
target_compile_options(FTCS PRIVATE "-save-temps")