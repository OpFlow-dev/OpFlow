# ----------------------------------------------------------------------------
# CMakeLists.txt for the opflow library
# ----------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.16)

# ----------------------------------------------------------------------------
# Header only version
# ----------------------------------------------------------------------------
add_library(opflow INTERFACE)
add_library(opflow::opflow ALIAS opflow)

target_compile_options(opflow INTERFACE -std=c++26)
set_property(TARGET opflow PROPERTY CXX_SCAN_FOR_MODULES ON)

# Create & include the version header
configure_file(${CMAKE_CURRENT_LIST_DIR}/../cmake/Version.hpp.in ${CMAKE_BINARY_DIR}/Version.hpp @ONLY)

target_include_directories(opflow INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../src>"
    "$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/src>")

# ----------------------------------------------------------------------------
# Platform detection
# ----------------------------------------------------------------------------
if(UNIX)
    target_compile_definitions(opflow INTERFACE OPFLOW_PLATFORM_UNIX)
elseif(APPLE)
    target_compile_definitions(opflow INTERFACE OPFLOW_PLATFORM_APPLE)
elseif(WIN32)
    target_compile_definitions(opflow INTERFACE OPFLOW_PLATFORM_WIN)
endif()

# ----------------------------------------------------------------------------
# Language & library features detection
# ----------------------------------------------------------------------------
include(CheckIncludeFiles)
check_include_files("sys/mman.h" HAS_SYS_MMAN_H)
if(HAS_SYS_MMAN_H)
    target_compile_definitions(opflow INTERFACE OPFLOW_HAS_MMAN_H)
endif()

# ----------------------------------------------------------------------------
# Misc definitions according to tweak options
# ----------------------------------------------------------------------------
foreach(
    OPFLOW_OPTION
    OPFLOW_SINGLE_PRECISION
    OPFLOW_WITH_MPI
    OPFLOW_WITH_OPENMP)
    if(${OPFLOW_OPTION})
        target_compile_definitions(opflow INTERFACE ${OPFLOW_OPTION})
    endif()
endforeach()
if(OPFLOW_WITH_MPI)
    target_compile_definitions(opflow INTERFACE OPFLOW_DISTRIBUTE_MODEL_MPI)
endif()

if(OPFLOW_NO_EXCEPTIONS AND NOT MSVC)
    target_compile_options(opflow INTERFACE -fno-exceptions)
endif()

# sanitizer flags
target_compile_options(opflow
    INTERFACE $<$<BOOL:${OPFLOW_SANITIZE_ADDRESS}>: -fsanitize=address>
    INTERFACE $<$<BOOL:${OPFLOW_SANITIZE_LEAK}>: -fsanitize=leak>
    INTERFACE $<$<BOOL:${OPFLOW_SANITIZE_THREAD}>: -fsanitize=thread>
    INTERFACE $<$<BOOL:${OPFLOW_SANITIZE_UB}>: -fsanitize=undefined>)
target_link_options(opflow
    INTERFACE $<$<BOOL:${OPFLOW_SANITIZE_ADDRESS}>: -fsanitize=address>
    INTERFACE $<$<BOOL:${OPFLOW_SANITIZE_LEAK}>: -fsanitize=leak>
    INTERFACE $<$<BOOL:${OPFLOW_SANITIZE_THREAD}>: -fsanitize=thread>
    INTERFACE $<$<BOOL:${OPFLOW_SANITIZE_UB}>: -fsanitize=undefined>)
