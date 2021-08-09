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

#ifndef OPFLOW_PARALLELINFO_HPP
#define OPFLOW_PARALLELINFO_HPP

#include "Core/Parallel/ParallelType.hpp"
#ifdef OPFLOW_WITH_MPI
#include <mpi.h>
#endif
#ifdef OPFLOW_WITH_OPENMP
#include <omp.h>
#endif

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
        unsigned int parallelType = 0;
        NodeInfo nodeInfo{};
        ThreadInfo threadInfo{};
        DeviceInfo deviceInfo{};

        ParallelInfo() {
#if defined(OPFLOW_WITH_MPI) && defined(OPFLOW_DISTRIBUTE_MODEL_MPI)
            parallelType |= (unsigned int)ParallelType::DistributeMem;
            nodeInfo.type = DistributeMemType::MPI;
            MPI_Comm_size(MPI_COMM_WORLD, &nodeInfo.node_count);
#else
            nodeInfo.type = DistributeMemType::None;
            nodeInfo.node_count = 1;
#endif

#if defined(OPFLOW_WITH_OPENMP) && defined(OPFLOW_THREAD_MODEL_OPENMP)
            parallelType |= (unsigned int)ParallelType::SharedMem;
            threadInfo.type = SharedMemType::OpenMP;
            threadInfo.thread_count = omp_get_max_threads();
#elifdef OPFLOW_THREAD_MODEL_TBB
            parallelType |= (unsigned int)ParallelType::SharedMem;
            threadInfo.type = SharedMemType::TBB;
            // tbb will handle the thread count to use so we don't calculate it here
#else
            threadInfo.type = SharedMemType::None;
            threadInfo.thread_count = 1;
#endif

#ifdef OPFLOW_WITH_CUDA
            parallelType |= (unsigned int)ParallelType::Heterogeneous;
            OP_NOT_IMPLEMENTED;
#elifdef OPFLOW_WITH_ROCM
            parallelType |= (unsigned int)ParallelType::Heterogeneous;
            OP_NOT_IMPLEMENTED;
#else
            deviceInfo.type = HeterogeneousType::None;
#endif
        }
    };
}
#endif//OPFLOW_PARALLELINFO_HPP
