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
    execute_process(
            COMMAND git -C ${CMAKE_CURRENT_SOURCE_DIR} rev-parse
            RESULT_VARIABLE _result
            OUTPUT_VARIABLE _output
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
    )
    if (_result EQUAL "0")
        execute_process(
                COMMAND git describe --tags --abbrev=0
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                RESULT_VARIABLE _tag_result
                OUTPUT_VARIABLE _tag_output
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
        )
        if (_tag_result EQUAL 0 AND NOT "${_tag_output}" STREQUAL "")
            string(REGEX MATCH "v([0-9]+)\.([0-9]+)\.([0-9]+)" _ "${_tag_output}")
            set(OPFLOW_VERSION_STRING "${_tag_output}" PARENT_SCOPE)
            if (NOT CMAKE_MATCH_COUNT EQUAL 3)
                message(FATAL_ERROR "Could not extract version number from ${_tag_output}")
            endif ()
            set(ver_major ${CMAKE_MATCH_1})
            set(ver_minor ${CMAKE_MATCH_2})
            set(ver_patch ${CMAKE_MATCH_3})

            set(OPFLOW_VERSION_MAJOR ${ver_major} CACHE STRING "OPFLOW_VERSION_MAJOR" FORCE)
            set(OPFLOW_VERSION_MINOR ${ver_minor} CACHE STRING "OPFLOW_VERSION_MINOR" FORCE)
            set(OPFLOW_VERSION_PATCH ${ver_patch} CACHE STRING "OPFLOW_VERSION_PATCH" FORCE)
            set(OPFLOW_VERSION "${ver_major}.${ver_minor}.${ver_patch}" CACHE STRING "OPFLOW_VERSION" FORCE)
        else ()
            message(STATUS "Git tags unavailable. Use include/Version instead")
            opflow_extract_version_from_file()
            set(OPFLOW_VERSION_STRING "${OPFLOW_VERSION}" PARENT_SCOPE)
        endif ()

        execute_process(
                COMMAND git describe --tags
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                RESULT_VARIABLE _describe_result
                OUTPUT_VARIABLE _describe_output
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
        )
        if (_describe_result EQUAL 0 AND NOT "${_describe_output}" STREQUAL "")
            set(OPFLOW_COMMIT_STRING "${_describe_output}" PARENT_SCOPE)
        else ()
            execute_process(
                    COMMAND git rev-parse --short HEAD
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    RESULT_VARIABLE _head_result
                    OUTPUT_VARIABLE _head_output
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_QUIET
            )
            if (_head_result EQUAL 0 AND NOT "${_head_output}" STREQUAL "")
                set(OPFLOW_COMMIT_STRING "${_head_output}" PARENT_SCOPE)
            endif ()
        endif ()
    else ()
        message(STATUS "Code not organized by git. Use default version file instead")
        opflow_extract_version_from_file()
    endif ()
endfunction()

function(opflow_extract_version_from_file)
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/include/Version" file_contents)
    string(REGEX MATCH "OPFLOW_VERSION_MAJOR ([0-9]+)" _ "${file_contents}")
    if (NOT ${CMAKE_MATCH_COUNT} EQUAL 1)
        message(FATAL_ERROR "Could not extract major version number from include/Version")
    endif ()
    set(ver_major ${CMAKE_MATCH_1})

    string(REGEX MATCH "OPFLOW_VERSION_MINOR ([0-9]+)" _ "${file_contents}")
    if (NOT ${CMAKE_MATCH_COUNT} EQUAL 1)
        message(FATAL_ERROR "Could not extract minor version number from include/Version")
    endif ()

    set(ver_minor ${CMAKE_MATCH_1})
    string(REGEX MATCH "OPFLOW_VERSION_PATCH ([0-9]+)" _ "${file_contents}")
    if (NOT ${CMAKE_MATCH_COUNT} EQUAL 1)
        message(FATAL_ERROR "Could not extract patch version number from include/Version")
    endif ()
    set(ver_patch ${CMAKE_MATCH_1})

    set(OPFLOW_VERSION_MAJOR ${ver_major} CACHE STRING "OPFLOW_VERSION_MAJOR" FORCE)
    set(OPFLOW_VERSION_MINOR ${ver_minor} CACHE STRING "OPFLOW_VERSION_MINOR" FORCE)
    set(OPFLOW_VERSION_PATCH ${ver_patch} CACHE STRING "OPFLOW_VERSION_PATCH" FORCE)
    set(OPFLOW_VERSION "${ver_major}.${ver_minor}.${ver_patch}" CACHE STRING "OPFLOW_VERSION" FORCE)
endfunction()

# Turn on warnings on the given target
function(opflow_enable_warnings target_name)
    if (OPFLOW_BUILD_WARNINGS)
        if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
            list(APPEND MSVC_OPTIONS "/W3")
            if (MSVC_VERSION GREATER 1900) # Allow non fatal security warnings for msvc 2015
                list(APPEND MSVC_OPTIONS "/WX")
            endif ()
        endif ()

        target_compile_options(
                ${target_name}
                PRIVATE $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
                -Wall
                -Wextra
                -Wconversion
                -pedantic
                -Wfatal-errors>
                $<$<CXX_COMPILER_ID:MSVC>:${MSVC_OPTIONS}>)
    endif ()
endfunction()

# Enable address sanitizer (gcc/clang only)
function(opflow_enable_sanitizer target_name)
    if (NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "Sanitizer supported only for gcc/clang")
    endif ()
    message(STATUS "Address sanitizer enabled")
    target_compile_options(${target_name} PRIVATE -fsanitize=address,undefined)
    target_compile_options(${target_name} PRIVATE -fno-sanitize=signed-integer-overflow)
    target_compile_options(${target_name} PRIVATE -fno-sanitize-recover=all)
    target_compile_options(${target_name} PRIVATE -fno-omit-frame-pointer)
    target_link_libraries(${target_name} PRIVATE -fsanitize=address,undefined -fuse-ld=gold)
endfunction()
