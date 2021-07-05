# ----------------------------------------------------------------------------
#
# Copyright (c) 2019 - 2021 by the OpFlow developers
#
# This file is part of OpFlow.
#
# OpFlow is free software and is distributed under the MPL v2.0 license.
# The full text of the license can be found in the file LICENSE at the top
# level directory of OpFlow.
#
# ----------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.10)
project(OpFlow)
set(CMAKE_CXX_STANDARD 20)

option(OpFlow_build_tests "Build OpFlow's test suit." OFF)
option(OpFlow_build_examples "Build OpFlow's example cases." OFF)
option(OpFlow_build_docs "Build OpFlow's documentation." OFF)
option(OpFlow_single_precision "Use float for real data as default." OFF)
option(OpFlow_with_openmp "Enable multi-thread with OpenMP." ON)
option(OpFlow_with_MPI "Enable distributed parallelization with MPI." ON)
option(OpFlow_cross_compile "Compile for another architecture." OFF)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/external/morse_cmake/modules")
if (APPLE)
    list(APPEND CMAKE_PREFIX_PATH "/usr/local/opt/qt@5")
endif ()

set(OpFlow_Libs)

if (OpFlow_with_MPI)
    find_package(MPI REQUIRED)
    include_directories(${MPI_CXX_INCLUDE_PATH})
    list(APPEND OpFlow_Libs ${MPI_CXX_LIBRARIES})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MPI_CXX_COMPILE_FLAGS}")
    if (MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DOPFLOW_WITH_MPI")
    else ()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOPFLOW_WITH_MPI")
    endif ()
endif ()

if (OpFlow_with_openmp)
    find_package(OpenMP REQUIRED)
    list(APPEND OpFlow_Libs OpenMP::OpenMP_CXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    if (MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DOPFLOW_WITH_OPENMP")
    else ()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOPFLOW_WITH_OPENMP")
    endif ()
endif ()

# detect highest avx flags
if (NOT OpFlow_cross_compile)
    if (MSVC)
        include(FindAVX)
        CHECK_FOR_AVX()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${AVX_FLAGS}")
    else ()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
    endif ()
else ()
    if (TARGET_ARCH STREQUAL "")
        message(FATAL_ERROR "TARGET_ARCH must be specified for cross-compilation.")
    else ()
        if (MSVC)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:${TARGET_ARCH}")
        else ()
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=${TARGET_ARCH}")
        endif ()
    endif ()
endif ()

if (OpFlow_single_precision)
    if (MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DOPFLOW_SINGLE_REAL")
    else ()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOPFLOW_SINGLE_REAL")
    endif ()
endif ()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mprefer-vector-width=512 -mfma -Wno-narrowing -ftemplate-backtrace-limit=0")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 --param inline-unit-growth=200 --param large-function-growth=3000 \
        -fopenmp-simd -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE} -g")
    if (OpFlow_build_tests AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
        include(CodeCoverage)
    endif ()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    message(FATAL_ERROR "Intel compiler doesn't support C++20 standard.")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS_DEBUG "/Od")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfma -Wno-narrowing -Wno-deprecated-anon-enum-enum-conversion")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -fopenmp-simd -DNDEBUG")
    if (OpFlow_build_tests AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fprofile-instr-generate -fcoverage-mapping")
        include(CodeCoverage)
    endif ()
endif ()

include_directories(external)
include_directories($ENV{MKLROOT}/include)
include_directories(src)
include_directories(include)
include_directories(external/amgcl)
include_directories(external/tecio)
include_directories(external/spdlog/include)
include_directories(external/xxHash)
include_directories(external/fmt/include)
add_subdirectory(external/fmt)

list(APPEND OpFlow_Libs fmt::fmt)

include(HYPREConfig)
CONFIG_HYPRE()
list(APPEND OpFlow_Libs ${HYPRE_LIBRARIES})
include(VTKConfig)
CONFIG_VTK()
list(APPEND OpFlow_Libs ${VTK_LIBRARIES})

if (OpFlow_build_tests)
    add_subdirectory(test)
endif ()

if (OpFlow_build_examples)
    add_subdirectory(examples)
endif ()

if (OpFlow_build_docs)
    add_custom_target(doc
                      COMMAND sphinx-build ${CMAKE_CURRENT_SOURCE_DIR}/doc ${CMAKE_CURRENT_BINARY_DIR}/doc
                      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endif ()