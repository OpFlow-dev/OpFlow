# ----------------------------------------------------------------------------
#
# Copyright (c) 2019 - 2025 by the OpFlow developers
#
# This file is part of OpFlow.
# 
# OpFlow is free software and is distributed under the MPL v2.0 license. 
# The full text of the license can be found in the file LICENSE at the top
# level directory of OpFlow.
#
# ----------------------------------------------------------------------------

project(OpFlow_DS_Tests)

# MDArray
add_gmock(PlainTensorTest ${CMAKE_CURRENT_SOURCE_DIR}/Arrays/PlainTensorTest.cpp)

# MDIndex
add_gmock(MDIndexTest ${CMAKE_CURRENT_SOURCE_DIR}/Index/MDIndexTest.cpp)

# StencilPad
add_gmock(StencilPadTest ${CMAKE_CURRENT_LIST_DIR}/StencilPadTest.cpp)

# Mappers
if(NOT OPFLOW_ENABLE_MODULE)
    add_gmock(BlockedMDRangeMapperTest ${CMAKE_CURRENT_LIST_DIR}/Index/BlockedMDRangeMapperTest.cpp)
    add_gmock(ColoredBlockedMDRangeMapperTest ${CMAKE_CURRENT_LIST_DIR}/Index/ColoredBlockedMDRangeMapperTest.cpp)
endif()
