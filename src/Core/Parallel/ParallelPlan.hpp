// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2021 by the OpFlow developers
//
// This file is part of OpFlow.
// 
// OpFlow is free software and is distributed under the MPL v2.0 license. 
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_PARALLELPLAN_HPP
#define OPFLOW_PARALLELPLAN_HPP

#include <vector>

namespace OpFlow {
    enum class ParallelType : unsigned int { OpenMP = 0x1, MPI = 0x2, CUDA = 0x4 };

    struct ParallelPlan {
        ParallelType parallelType;
        int thread_per_proc;
        std::vector<int> dim_split;
    };
}// namespace OpFlow
#endif//OPFLOW_PARALLELPLAN_HPP
