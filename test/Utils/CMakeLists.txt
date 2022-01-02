# ----------------------------------------------------------------------------
#
# Copyright (c) 2019 - 2022 by the OpFlow developers
#
# This file is part of OpFlow.
# 
# OpFlow is free software and is distributed under the MPL v2.0 license. 
# The full text of the license can be found in the file LICENSE at the top
# level directory of OpFlow.
#
# ----------------------------------------------------------------------------

project(OpFlow_Utils_Tests)

add_gmock(HDF5StreamTest ${CMAKE_CURRENT_LIST_DIR}/HDF5StreamTest.cpp)