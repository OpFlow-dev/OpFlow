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

project(OpFlow_Misc_Tests)

add_gmock(FmtlibTest ${CMAKE_CURRENT_SOURCE_DIR}/FmtlibTest.cpp)
target_link_libraries(FmtlibTest fmt::fmt)

add_gmock(SpdlogTest ${CMAKE_CURRENT_SOURCE_DIR}/SpdlogTest.cpp)