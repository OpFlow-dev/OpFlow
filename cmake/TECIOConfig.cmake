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
# Config & build TECIO from source
#
# ----------------------------------------------------------------------------

macro(CONFIG_AND_INSTALL_TECIO)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tecio-build)
    if (OPFLOW_WITH_MPI)
        execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}"
                -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} ${CMAKE_CURRENT_SOURCE_DIR}/external/tecio/teciompisrc
                RESULT_VARIABLE result
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tecio-build)

    else ()
        execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}"
                -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} ${CMAKE_CURRENT_SOURCE_DIR}/external/tecio/teciosrc
                RESULT_VARIABLE result
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tecio-build)
    endif ()
    if (result)
        message(FATAL_ERROR "CMake step for TECIO failed: ${result}")
    endif ()
    execute_process(COMMAND ${CMAKE_COMMAND} --build . -j
            RESULT_VARIABLE result
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tecio-build)
    if (result)
        message(FATAL_ERROR "Build step for AMGCL failed: ${result}")
    endif ()
    # Manually install
    file(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/include/tecio)
    if (OPFLOW_WITH_MPI)
        file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/external/tecio/teciompisrc/TECIO.h
                ${CMAKE_CURRENT_SOURCE_DIR}/external/tecio/teciompisrc/tecio_Exports.h
                ${CMAKE_CURRENT_SOURCE_DIR}/external/tecio/teciompisrc/tecio.inc
                ${CMAKE_CURRENT_SOURCE_DIR}/external/tecio/teciompisrc/tecio.for
                ${CMAKE_CURRENT_SOURCE_DIR}/external/tecio/teciompisrc/tecio.f90
                ${CMAKE_CURRENT_SOURCE_DIR}/external/tecio/teciompisrc/tecio_license_agreement.txt
                DESTINATION ${CMAKE_INSTALL_PREFIX}/include/tecio)

        file(COPY ${CMAKE_CURRENT_BINARY_DIR}/tecio-build/libteciompi.a DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
    else ()
        file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/external/tecio/teciosrc/TECIO.h
                ${CMAKE_CURRENT_SOURCE_DIR}/external/tecio/teciosrc/tecio_Exports.h
                ${CMAKE_CURRENT_SOURCE_DIR}/external/tecio/teciosrc/tecio.inc
                ${CMAKE_CURRENT_SOURCE_DIR}/external/tecio/teciosrc/tecio.for
                ${CMAKE_CURRENT_SOURCE_DIR}/external/tecio/teciosrc/tecio.f90
                ${CMAKE_CURRENT_SOURCE_DIR}/external/tecio/teciosrc/tecio_license_agreement.txt
                DESTINATION ${CMAKE_INSTALL_PREFIX}/include/tecio)

        file(COPY ${CMAKE_CURRENT_BINARY_DIR}/tecio-build/libtecio.a DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
    endif ()
endmacro()