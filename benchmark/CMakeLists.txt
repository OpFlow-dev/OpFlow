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
project(OpFlow_Benchmarks)

if (NOT TARGET opflow)
    find_package(opflow REQUIRED)
endif ()

add_custom_target(AllBenchmarks)

function(add_bench name)
    add_executable(BENCH_${name} EXCLUDE_FROM_ALL ${ARGN})
    target_link_libraries(BENCH_${name} opflow::opflow benchmark::benchmark)
    add_dependencies(AllBenchmarks BENCH_${name})
endfunction()

add_subdirectory(Core)
