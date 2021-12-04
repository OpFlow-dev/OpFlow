# ----------------------------------------------------------------------------
#
# Copyright (c) 2019 - 2022 by the OpFlow developers
#
# This file is part of OpFlow.
# 
# OpFlow is free software and is distributed under the MPL v2.0 license. 
# The full text of the license can be found in the file LICENSE at the top
# level directory of OpFlow.
#
# ----------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.10)
project(OpFlow_Tests)

if (NOT TARGET opflow)
    find_package(opflow REQUIRED)
endif ()

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/CodeCoverage.cmake)

#define_property(GLOBAL PROPERTY all_test_srcs)
set_property(GLOBAL PROPERTY all_test_srcs)

add_custom_target(AllTests)

set(TEST_MAIN_CPP ${CMAKE_CURRENT_LIST_DIR}/test_main.cpp)

function(add_gmock testname testfile)
    add_executable(${testname} EXCLUDE_FROM_ALL ${testfile} ${ARGN} ${TEST_MAIN_CPP})
    target_link_libraries(${testname} gmock opflow::opflow gcov)
    target_compile_options(${testname} PRIVATE $<$<CXX_COMPILER_ID:GNU>:--coverage>
            PRIVATE $<$<CXX_COMPILER_ID:GNU>:-fconcepts-diagnostics-depth=5>
            PRIVATE $<$<CXX_COMPILER_ID:Clang>:-fprofile-instr-generate -fcoverage-mapping>)
    set_property(GLOBAL APPEND PROPERTY all_test_srcs ${testfile} ${ARGN})
    add_dependencies(AllTests ${testname})
    if (NOT ${testname} STREQUAL "UniTests")
        add_test(NAME ${testname} COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${testname})
    endif ()
    if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        SETUP_TARGET_FOR_COVERAGE(${testname}_coverage ${testname} ${testname}_coverage)
    endif ()
endfunction()

if (OPFLOW_WITH_MPI)
    if (NOT MPI_FOUND)
        # Find MPI
        include(FindMPI)
        find_package(MPI REQUIRED)
    endif ()
endif ()

function(add_gmock_mpi testname proccount testfile)
    add_executable(${testname} EXCLUDE_FROM_ALL ${testfile} ${ARGN} ${TEST_MAIN_CPP})
    target_link_libraries(${testname} gmock opflow::opflow gcov)
    target_compile_options(${testname} PRIVATE $<$<CXX_COMPILER_ID:GNU>:--coverage>
            PRIVATE $<$<CXX_COMPILER_ID:GNU>:-fconcepts-diagnostics-depth=5>
            PRIVATE $<$<CXX_COMPILER_ID:Clang>:-fprofile-instr-generate -fcoverage-mapping>)
    target_compile_definitions(${testname} PUBLIC OPFLOW_DISTRIBUTE_MODEL_MPI)
    set_property(GLOBAL APPEND PROPERTY all_test_srcs ${testfile} ${ARGN})
    add_dependencies(AllTests ${testname})
    if (proccount GREATER_EQUAL MPIEXEC_MAX_NUMPROCS)
        message(WARNING "Given #Proc = ${proccount} > #MaxProc = ${MPIEXEC_MAX_NUMPROCS}. Reset to ${MPIEXEC_MAX_NUMPROCS}")
        set(proccount ${MPIEXEC_MAX_NUMPROCS})
    else ()
        message(STATUS "${proccount} ${MPIEXEC_MAX_NUMPROCS}")
    endif ()
    if (NOT ${testname} STREQUAL "UniTests")
        add_test(NAME ${testname} COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} ${proccount}
                ${MPIEXEC_PREFLAGS} ${CMAKE_CURRENT_BINARY_DIR}/${testname} ${MPIEXEC_POSTFLAGS})
    endif ()
    if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        SETUP_TARGET_FOR_COVERAGE(${testname}_coverage ${testname} ${testname}_coverage)
    endif ()
endfunction()

if (OPFLOW_USE_BUNDLED_GTEST)
    # Use bundled googletest framework (v1.11.0)
    set(OPFLOW_GTEST_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../external/googletest)
    configure_file(${CMAKE_CURRENT_LIST_DIR}/../cmake/UseBundledGTest.cmake.in
            ${CMAKE_CURRENT_BINARY_DIR}/googletest-download/CMakeLists.txt)
else ()
    # Download and unpack googletest at configure time
    configure_file(../cmake/DownloadGTest.cmake.in googletest-download/CMakeLists.txt)
endif ()
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download)
if (result)
    message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif ()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download)
if (result)
    message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif ()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
if (OPFLOW_USE_BUNDLED_GTEST)
    add_subdirectory(${OPFLOW_GTEST_SOURCE_DIR}
            ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
            EXCLUDE_FROM_ALL)
else ()
    add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
            ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
            EXCLUDE_FROM_ALL)
endif ()

# The gtest/gtest_main targets carry header search path
# dependencies automatically when using CMake 2.8.11 or
# later. Otherwise we have to add them here ourselves.
if (CMAKE_VERSION VERSION_LESS 2.8.11)
    include_directories("${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
endif ()

include_directories(${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")

add_subdirectory(Core)
add_subdirectory(DataStructures)
add_subdirectory(Misc)
add_subdirectory(Utils)

# Add global tests target
get_property(all_test_srcs_list GLOBAL PROPERTY all_test_srcs)
list(REMOVE_DUPLICATES all_test_srcs_list)
add_gmock(UniTests ${all_test_srcs_list})

add_dependencies(All_CI AllTests)