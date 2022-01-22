//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2022 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_PARALLELINFO_HPP
#define OPFLOW_PARALLELINFO_HPP

#include "Core/Parallel/ParallelType.hpp"
#ifdef OPFLOW_WITH_MPI
#include <mpi.h>
#endif
#ifdef OPFLOW_WITH_OPENMP
#include <omp.h>
#endif
#include <oneapi/tbb.h>

namespace OpFlow {
    struct NodeInfo {
        DistributeMemType type;
        int node_count;
    };
    struct ThreadInfo {
        SharedMemType type;
        int thread_count;
    };
    struct DeviceInfo {
        HeterogeneousType type;
        int device_count;
    };
    struct ParallelInfo {
        ParallelType parallelType = 0;
        NodeInfo nodeInfo {};
        ThreadInfo threadInfo {};
        DeviceInfo deviceInfo {};
    };

    static inline auto makeParallelInfo() {
        ParallelInfo ret;
#if defined(OPFLOW_WITH_MPI) && defined(OPFLOW_DISTRIBUTE_MODEL_MPI)
        ret.parallelType |= ParallelIdentifier::DistributeMem;
        ret.nodeInfo.type = DistributeMemType::MPI;
        MPI_Comm_size(MPI_COMM_WORLD, &ret.nodeInfo.node_count);
#else
        ret.nodeInfo.type = DistributeMemType::None;
        ret.nodeInfo.node_count = 1;
#endif

#ifdef OPFLOW_THREAD_MODEL_OPENMP
        ret.threadInfo.type = SharedMemType::OpenMP;
#elif defined(OPFLOW_THREAD_MODEL_TBB)
        ret.threadInfo.type = SharedMemType::TBB;
#else
        ret.threadInfo.type = SharedMemType::None;
#endif
#if defined(OPFLOW_WITH_OPENMP)
        ret.parallelType |= ParallelIdentifier::SharedMem;
        ret.threadInfo.thread_count = omp_get_max_threads();
#elif defined(OPFLOW_THREAD_MODEL_TBB)
        ret.parallelType |= ParallelIdentifier::SharedMem;
        // tbb will handle the thread count to use so we don't calculate it here
#else
        ret.threadInfo.type = SharedMemType::None;
        ret.threadInfo.thread_count = 1;
#endif

#ifdef OPFLOW_WITH_CUDA
        ret.parallelType |= ParallelIdentifier::Heterogeneous;
        OP_NOT_IMPLEMENTED;
#elifdef OPFLOW_WITH_ROCM
        ret.parallelType |= ParallelIdentifier::Heterogeneous;
        OP_NOT_IMPLEMENTED;
#else
        ret.deviceInfo.type = HeterogeneousType::None;
#endif
        return ret;
    }

}// namespace OpFlow
#endif//OPFLOW_PARALLELINFO_HPP
