//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2021  by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_PARALLELTYPE_HPP
#define OPFLOW_PARALLELTYPE_HPP

namespace OpFlow {
    enum class ParallelType : unsigned int { None = 0, SharedMem = 0x1, DistributeMem = 0x2, Heterogeneous = 0x4 };
    enum class SharedMemType { None, OpenMP, TBB };
    enum class DistributeMemType { None, MPI };
    enum class HeterogeneousType { None, CUDA, ROCM };
}
#endif//OPFLOW_PARALLELTYPE_HPP
