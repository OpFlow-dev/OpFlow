//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2023 by the OpFlow developers
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
    using ParallelType = unsigned int;
    struct ParallelIdentifier {
        constexpr unsigned int static None = 0;
        constexpr unsigned int static SharedMem = 0x1;
        constexpr unsigned int static DistributeMem = 0x2;
        constexpr unsigned int static Heterogeneous = 0x4;
    };
    enum class SharedMemType { None, OpenMP, TBB };
    enum class DistributeMemType { None, MPI };
    enum class HeterogeneousType { None, CUDA, ROCM };
}// namespace OpFlow
#endif//OPFLOW_PARALLELTYPE_HPP
