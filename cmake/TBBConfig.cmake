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
# Config & build TBB from source
#
# ----------------------------------------------------------------------------

macro(CONFIG_AND_INSTALL_TBB)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tbb-build)
    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}"
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
            -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -DTBB_TEST=OFF -DTBB_STRICT=OFF
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} ${CMAKE_CURRENT_SOURCE_DIR}/external/tbb
            RESULT_VARIABLE result
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tbb-build)
    if (result)
        message(FATAL_ERROR "CMake step for TBB failed: ${result}")
    endif ()
    execute_process(COMMAND ${CMAKE_COMMAND} --build . -j
            RESULT_VARIABLE result
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tbb-build)
    if (result)
        message(FATAL_ERROR "Build step for TBB failed: ${result}")
    endif ()
    execute_process(COMMAND ${CMAKE_COMMAND} --install .
            RESULT_VARIABLE result
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tbb-build)
    if (result)
        message(FATAL_ERROR "Install step for TBB failed: ${result}")
    endif ()
endmacro()