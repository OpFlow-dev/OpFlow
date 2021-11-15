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
add_gmock(MetaTest ${CMAKE_CURRENT_LIST_DIR}/MetaTest.cpp)

# Meshes
add_gmock(CartesianMeshTest ${CMAKE_CURRENT_LIST_DIR}/Mesh/CartesianMeshTest.cpp)

# Fields
add_gmock(CartesianFieldTest ${CMAKE_CURRENT_LIST_DIR}/Field/CartesianFieldTest.cpp)

# BCs
add_gmock(DircBCTest ${CMAKE_CURRENT_LIST_DIR}/BC/DircBCTest.cpp)
add_gmock(NeumBCTest ${CMAKE_CURRENT_LIST_DIR}/BC/NeumBCTest.cpp)
add_gmock(PeriodicBCTest ${CMAKE_CURRENT_LIST_DIR}/BC/PeriodicBCTest.cpp)

# Operators
add_gmock(InterpolatorTest ${CMAKE_CURRENT_LIST_DIR}/Operator/InterpolatorTest.cpp)
add_gmock(ConvolutionTest ${CMAKE_CURRENT_LIST_DIR}/Operator/ConvolutionTest.cpp)
add_gmock(DecayExprTest ${CMAKE_CURRENT_LIST_DIR}/Operator/DecayExprTest.cpp)

# RangeFor
add_gmock(RangeForTest ${CMAKE_CURRENT_LIST_DIR}/Loops/RangeForTest.cpp)

# Parallel
add_gmock(EvenSplitStrategyTest ${CMAKE_CURRENT_LIST_DIR}/Parallel/EvenSplitStrategyTest.cpp)

# Equation
add_gmock(EqnSolveHandlerTest ${CMAKE_CURRENT_LIST_DIR}/Equation/EqnSolveHandlerTest.cpp)