cmake_minimum_required(VERSION 3.10)
project(OpFlow_Cases_EqnHandler)

if (NOT TARGET opflow)
    find_package(opflow REQUIRED)
endif ()

add_executable(EqnHandler EXCLUDE_FROM_ALL EqnHandler.cpp)
target_link_libraries(EqnHandler opflow::opflow)
target_compile_options(EqnHandler PRIVATE ${OPFLOW_EXAMPLE_FLAGS})