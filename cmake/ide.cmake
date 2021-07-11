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
# IDE support for headers
#
# ----------------------------------------------------------------------------
#
# Modified from ide.cmake in spdlog (https://github.com/gabime/spdlog)
#
# ----------------------------------------------------------------------------

set(OPFLOW_HEADERS_DIR "${CMAKE_CURRENT_LIST_DIR}/../include")
set(OPFLOW_SOURCES_DIR "${CMAKE_CURRENT_LIST_DIR}/../src")

file(GLOB OPFLOW_TOP_HEADERS "${OPFLOW_HEADERS_DIR}/*")
file(GLOB_RECURSE OPFLOW_CORE_HEADERS   "${OPFLOW_SOURCES_DIR}/Core/*.hpp")
file(GLOB_RECURSE OPFLOW_DS_HEADERS     "${OPFLOW_SOURCES_DIR}/DataStructures/*.hpp")
file(GLOB_RECURSE OPFLOW_MATH_HEADERS   "${OPFLOW_SOURCES_DIR}/Math/*.hpp")
file(GLOB_RECURSE OPFLOW_UTILS_HEADERS  "${OPFLOW_SOURCES_DIR}/Utils/*.hpp")
set(OPFLOW_ALL_HEADERS ${OPFLOW_TOP_HEADERS} ${OPFLOW_CORE_HEADERS}
        ${OPFLOW_DS_HEADERS} ${OPFLOW_MATH_HEADERS} ${OPFLOW_UTILS_HEADERS})

source_group("Header Files\\OpFlow" FILES ${OPFLOW_TOP_HEADERS})
source_group("Header Files\\OpFlow\\Core" FILES ${OPFLOW_CORE_HEADERS})
source_group("Header Files\\OpFlow\\DS" FILES ${OPFLOW_DS_HEADERS})
source_group("Header Files\\OpFlow\\Math" FILES ${OPFLOW_MATH_HEADERS})
source_group("Header Files\\OpFlow\\Utils" FILES ${OPFLOW_UTILS_HEADERS})
