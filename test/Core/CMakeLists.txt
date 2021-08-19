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

project(OpFlow_Core_Tests)

# Metas
add_gmock(MetaTest ${CMAKE_CURRENT_SOURCE_DIR}/MetaTest.cpp)

# Meshes
add_gmock(CartesianMeshTest ${CMAKE_CURRENT_SOURCE_DIR}/Mesh/CartesianMeshTest.cpp)

# Fields
add_gmock(CartesianFieldBuilderTest ${CMAKE_CURRENT_LIST_DIR}/CartesianFieldBuilderTest.cpp)

# Interpolators
add_gmock(InterpolatorTest ${CMAKE_CURRENT_SOURCE_DIR}/InterpolatorTest.cpp)

# RangeFor
add_gmock(RangeForTest ${CMAKE_CURRENT_SOURCE_DIR}/RangeForTest.cpp)

# MDIndex
add_gmock(MDIndexTest ${CMAKE_CURRENT_SOURCE_DIR}/MDIndexTest.cpp)
