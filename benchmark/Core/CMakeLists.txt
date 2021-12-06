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

project(OpFlow_Core_Benchmarks)

add_bench(UniFieldAssign UniFieldAssign.cpp)

add_bench(AMRFieldAssign AMRFieldAssign.cpp)

add_bench(AMRMeshBuild AMRMeshBuild.cpp)

add_bench(AMRFieldInit AMRFieldInit.cpp)

add_bench(LaplaceOp LaplaceOp.cpp)

add_bench(EqnSolve EqnSolve.cpp)