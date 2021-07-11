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
# Config & build VTK from source
#
# ----------------------------------------------------------------------------

macro(CONFIG_VTK)
    # try build VTK from source
    if (APPLE)
        # use apple-clang instead of specified compiler for vtk to avoid error
        set(VTK_C_COMPILER "cc")
        set(VTK_CXX_COMPILER "c++")
    else ()
        set(VTK_C_COMPILER ${CMAKE_C_COMPILER})
        set(VTK_CXX_COMPILER ${CMAKE_CXX_COMPILER})
    endif ()
    string(JOIN " " VTK_OPTIONS
            "-DCMAKE_C_COMPILER=${VTK_C_COMPILER}"
            "-DCMAKE_CXX_COMPILER=${VTK_CXX_COMPILER}"
            "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
            "-DBUILD_SHARED_LIBS=OFF"
            "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/vtk")
    string(REPLACE " " "\t\n" _vtk_options ${VTK_OPTIONS})
    message(STATUS "VTK is to be configured with:\t\n${_vtk_options}")

    if (${OPFLOW_VTK_PRE_DOWNLOAD})
        # use pre-downloaded source file
        if (NOT OPFLOW_VTK_SOURCE_DIR)
            message(FATAL_ERROR "OPFLOW_VTK_SOURCE_DIR must be defined when 
            OPFLOW_VTK_PRE_DOWNLOAD is enabled")
        endif ()

        configure_file(cmake/UseExistingVTK.txt.in
                ${CMAKE_CURRENT_BINARY_DIR}/vtk-download/CMakeLists.txt)
    else ()
        configure_file(cmake/DownloadVTK.txt.in
                ${CMAKE_CURRENT_BINARY_DIR}/vtk-download/CMakeLists.txt)
    endif ()
    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
            RESULT_VARIABLE _result
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/vtk-download)
    if (_result)
        message(FATAL_ERROR "CMake step for VTK failed: ${_result}")
    endif ()

    execute_process(COMMAND ${CMAKE_COMMAND} --build . -j
            RESULT_VARIABLE _result
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/vtk-download)
    if (_result)
        message(FATAL_ERROR "Build step for VTK failed: ${_result}")
    endif ()

    find_package(VTK CONFIG REQUIRED
            PATHS ${CMAKE_CURRENT_BINARY_DIR}/vtk/lib/cmake)
endmacro()