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
# Config & build FMT from source
#
# ----------------------------------------------------------------------------

macro(CONFIG_AND_INSTALL_FMT)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/fmt-build)
    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}"
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
            -DFMT_DOC=OFF -DFMT_TEST=OFF
            -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} ${CMAKE_CURRENT_LIST_DIR}/../external/fmt
            RESULT_VARIABLE result
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/fmt-build)
    if (result)
        message(FATAL_ERROR "CMake step for fmt failed: ${result}")
    endif ()
    execute_process(COMMAND ${CMAKE_COMMAND} --build . -j
            RESULT_VARIABLE result
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/fmt-build)
    if (result)
        message(FATAL_ERROR "Build step for fmt failed: ${result}")
    endif ()
    execute_process(COMMAND ${CMAKE_COMMAND} --install .
            RESULT_VARIABLE result
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/fmt-build)
    if (result)
        message(FATAL_ERROR "Install step for fmt failed: ${result}")
    endif ()

endmacro()