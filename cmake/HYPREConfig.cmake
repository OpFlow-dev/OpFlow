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
# Config & build HYPRE from source
#
# ----------------------------------------------------------------------------

macro(CONFIG_AND_INSTALL_HYPRE)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/hypre-build)
    string(JOIN " " HYPRE_OPTIONS
            "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
            "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
            "-DHYPRE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
            "-DHYPRE_ENABLE_SINGLE=${OPFLOW_SINGLE_PRECISION}"
            "-DHYPRE_WITH_OPENMP=${OPFLOW_WITH_OPENMP}"
            "-DHYPRE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}")
    string(REPLACE " " "\t\n" _hypre_options ${HYPRE_OPTIONS})
    message(STATUS "HYPRE is to be configured with:\t\n${_hypre_options}")

    set(OPFLOW_HYPRE_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/external/hypre)

    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}"
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DHYPRE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DHYPRE_ENABLE_SINGLE=${OPFLOW_SINGLE_PRECISION}
            -DHYPRE_WITH_OPENMP=${OPFLOW_WITH_OPENMP}
            -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
            ${CMAKE_CURRENT_SOURCE_DIR}/external/hypre/src
            RESULT_VARIABLE _result
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/hypre-build)
    if (_result)
        message(FATAL_ERROR "CMake step for HYPRE failed: ${_result}")
    endif ()

    execute_process(COMMAND ${CMAKE_COMMAND} --build . -j
            RESULT_VARIABLE _result
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/hypre-build)
    if (_result)
        message(FATAL_ERROR "Build step for HYPRE failed: ${_result}")
    endif ()

    execute_process(COMMAND ${CMAKE_COMMAND} --install .
            RESULT_VARIABLE _result
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/hypre-build)
    if (_result)
        message(FATAL_ERROR "Install step for HYPRE failed: ${_result}")
    endif ()

endmacro()