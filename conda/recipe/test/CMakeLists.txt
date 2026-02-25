cmake_minimum_required(VERSION 3.20)
project(opflow_consumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(opflow CONFIG REQUIRED)

add_executable(opflow_smoke main.cpp)
target_link_libraries(opflow_smoke PRIVATE opflow::opflow)

