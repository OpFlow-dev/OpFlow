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

#ifndef OPFLOW_ENVIRONMENT_HPP
#define OPFLOW_ENVIRONMENT_HPP

#ifdef OPFLOW_WITH_MPI
#include <mpi.h>
#endif
#include <omp.h>

#include "Core/Parallel/ParallelInfo.hpp"
#include "Core/Parallel/ParallelPlan.hpp"

namespace OpFlow {
    void static inline InitEnvironment(int argc, char** argv) {
#ifdef OPFLOW_WITH_MPI
        MPI_Init(&argc, &argv);
#endif
    }

    void static inline FinalizeEnvironment() {
#ifdef OPFLOW_WITH_MPI
        MPI_Finalize();
#endif
    }

    namespace internal {
        static inline ParallelInfo GLOBAL_PARALLELINFO;
        static inline ParallelPlan GLOBAL_PARALLELPLAN;
    }// namespace internal

    inline static auto& getGlobalParallelInfo() { return internal::GLOBAL_PARALLELINFO; }

    inline static auto& getGlobalParallelPlan() { return internal::GLOBAL_PARALLELPLAN; }

    inline static void setGlobalParallelInfo(const ParallelInfo& info) {
        internal::GLOBAL_PARALLELINFO = info;
    }

    inline static void setGlobalParallelPlan(const ParallelPlan& plan) {
        internal::GLOBAL_PARALLELPLAN = plan;
#ifdef OPFLOW_WITH_OPENMP
#ifdef OPFLOW_THREAD_MODEL_OPENMP
        omp_set_num_threads(plan.shared_memory_workers_count);
#else
        omp_set_num_threads(1);
#endif
#endif
    }

#if defined(OPFLOW_WITH_MPI) && defined(OPFLOW_DISTRIBUTE_MODEL_MPI)
    inline static auto getWorkerId(MPI_Comm comm = MPI_COMM_WORLD) {
#else
    inline static auto getWorkerId() {
#endif
#if defined(OPFLOW_WITH_MPI) && defined(OPFLOW_DISTRIBUTE_MODEL_MPI)
        int rank;
        MPI_Comm_rank(comm, &rank);
        return rank;
#else
        return 0;
#endif
    }
}// namespace OpFlow
#endif//OPFLOW_ENVIRONMENT_HPP
