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

project(OpFlow_Core_Tests)

# Metas
add_gmock(MetaTest ${CMAKE_CURRENT_LIST_DIR}/MetaTest.cpp)

# Meshes
add_gmock(CartesianMeshTest ${CMAKE_CURRENT_LIST_DIR}/Mesh/CartesianMeshTest.cpp)

# Fields
add_gmock(CartesianFieldTest ${CMAKE_CURRENT_LIST_DIR}/Field/CartesianFieldTest.cpp)
add_gmock_mpi(CartesianFieldMPITest 4 ${CMAKE_CURRENT_LIST_DIR}/Field/CartesianFieldMPITest.cpp)

# BCs
add_gmock(DircBCTest ${CMAKE_CURRENT_LIST_DIR}/BC/DircBCTest.cpp)
add_gmock(NeumBCTest ${CMAKE_CURRENT_LIST_DIR}/BC/NeumBCTest.cpp)
add_gmock(PeriodicBCTest ${CMAKE_CURRENT_LIST_DIR}/BC/PeriodicBCTest.cpp)

# Operators
add_gmock(InterpolatorTest ${CMAKE_CURRENT_LIST_DIR}/Operator/InterpolatorTest.cpp)
add_gmock(ConvolutionTest ${CMAKE_CURRENT_LIST_DIR}/Operator/ConvolutionTest.cpp)
add_gmock(ConditionalTest ${CMAKE_CURRENT_LIST_DIR}/Operator/ConditionalTest.cpp)

# Loops
add_gmock(RangeForTest ${CMAKE_CURRENT_LIST_DIR}/Loops/RangeForTest.cpp)
add_gmock(RangeReduceTest ${CMAKE_CURRENT_LIST_DIR}/Loops/RangeReduceTest.cpp)

# Parallel
add_gmock(EvenSplitStrategyTest ${CMAKE_CURRENT_LIST_DIR}/Parallel/EvenSplitStrategyTest.cpp)
add_gmock_mpi(MPITest 16 ${CMAKE_CURRENT_LIST_DIR}/Parallel/MPITest.cpp)

# Equation
add_gmock(DircEqnTest ${CMAKE_CURRENT_LIST_DIR}/Equation/DircEqnTest.cpp)
add_gmock(NeumEqnTest ${CMAKE_CURRENT_LIST_DIR}/Equation/NeumEqnTest.cpp)
add_gmock(PeriodicEqnTest ${CMAKE_CURRENT_LIST_DIR}/Equation/PeriodicEqnTest.cpp)
add_gmock(AMGCLTest ${CMAKE_CURRENT_LIST_DIR}/Equation/AMGCLTest.cpp)
add_gmock(EqnHolderTest ${CMAKE_CURRENT_LIST_DIR}/Equation/EqnHolderTest.cpp)
add_gmock(CSRMatrixGeneratorTest ${CMAKE_CURRENT_LIST_DIR}/Equation/CSRMatrixGeneratorTest.cpp)
add_gmock(EqnSetTest ${CMAKE_CURRENT_LIST_DIR}/Equation/EqnSetTest.cpp)
add_gmock_mpi(AMGCLMPITest 4 ${CMAKE_CURRENT_LIST_DIR}/Equation/AMGCLMPITest.cpp)