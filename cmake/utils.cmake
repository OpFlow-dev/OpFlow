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
#
# Utilities for building
#
# ----------------------------------------------------------------------------
#
# Modified from utils.cmake in spdlog (https://github.com/gabime/spdlog)
#
# ----------------------------------------------------------------------------

# Get spdlog version from include/spdlog/version.h and put it in SPDLOG_VERSION
function(opflow_extract_version)
    file(READ "${CMAKE_CURRENT_LIST_DIR}/include/Version" file_contents)
    string(REGEX MATCH "OPFLOW_VERSION_MAJOR ([0-9]+)" _ "${file_contents}")
    if(NOT CMAKE_MATCH_COUNT EQUAL 1)
        message(FATAL_ERROR "Could not extract major version number from include/Version")
    endif()
    set(ver_major ${CMAKE_MATCH_1})

    string(REGEX MATCH "OPFLOW_VERSION_MINOR ([0-9]+)" _ "${file_contents}")
    if(NOT CMAKE_MATCH_COUNT EQUAL 1)
        message(FATAL_ERROR "Could not extract minor version number from include/Version")
    endif()

    set(ver_minor ${CMAKE_MATCH_1})
    string(REGEX MATCH "OPFLOW_VERSION_PATCH ([0-9]+)" _ "${file_contents}")
    if(NOT CMAKE_MATCH_COUNT EQUAL 1)
        message(FATAL_ERROR "Could not extract patch version number from include/Version")
    endif()
    set(ver_patch ${CMAKE_MATCH_1})

    set(OPFLOW_VERSION_MAJOR ${ver_major} PARENT_SCOPE)
    set(OPFLOW_VERSION_MINOR ${ver_minor} PARENT_SCOPE)
    set(OPFLOW_VERSION_PATCH ${ver_patch} PARENT_SCOPE)
    set(OPFLOW_VERSION "${ver_major}.${ver_minor}.${ver_patch}" PARENT_SCOPE)
endfunction()

# Turn on warnings on the given target
function(opflow_enable_warnings target_name)
    if(OPFLOW_BUILD_WARNINGS)
        if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
            list(APPEND MSVC_OPTIONS "/W3")
            if(MSVC_VERSION GREATER 1900) # Allow non fatal security warnings for msvc 2015
                list(APPEND MSVC_OPTIONS "/WX")
            endif()
        endif()

        target_compile_options(
            ${target_name}
            PRIVATE $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
                    -Wall
                    -Wextra
                    -Wconversion
                    -pedantic
                    -Wfatal-errors>
                    $<$<CXX_COMPILER_ID:MSVC>:${MSVC_OPTIONS}>)
    endif()
endfunction()

# Enable address sanitizer (gcc/clang only)
function(opflow_enable_sanitizer target_name)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "Sanitizer supported only for gcc/clang")
    endif()
    message(STATUS "Address sanitizer enabled")
    target_compile_options(${target_name} PRIVATE -fsanitize=address,undefined)
    target_compile_options(${target_name} PRIVATE -fno-sanitize=signed-integer-overflow)
    target_compile_options(${target_name} PRIVATE -fno-sanitize-recover=all)
    target_compile_options(${target_name} PRIVATE -fno-omit-frame-pointer)
    target_link_libraries(${target_name} PRIVATE -fsanitize=address,undefined -fuse-ld=gold)
endfunction()

