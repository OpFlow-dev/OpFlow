// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2025 by the OpFlow developers
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

#include "Core/Field/FieldExprTrait.hpp"
#include "Core/Parallel/ParallelInfo.hpp"

OPFLOW_MODULE_EXPORT namespace OpFlow {
    struct ParallelPlan {
        ParallelInfo info;
        int distributed_workers_count = 1;
        int shared_memory_workers_count = 1;
        int heterogeneous_workers_count = 0;

        [[nodiscard]] bool serialMode() const {
            return distributed_workers_count == 1 && shared_memory_workers_count == 1
                   && heterogeneous_workers_count == 0;
        }
        [[nodiscard]] bool singleNodeMode() const { return distributed_workers_count == 1; }
        [[nodiscard]] bool multiThreadMode() const {
            return distributed_workers_count == 1 && shared_memory_workers_count > 1
                   && heterogeneous_workers_count == 0;
        }
        [[nodiscard]] bool deviceMode() const { return heterogeneous_workers_count > 0; }
    };

    constexpr inline ParallelPlan makeParallelPlan(ParallelInfo info, ParallelType pbit) {
        bool dist_bit = pbit & ParallelIdentifier::DistributeMem,
             sm_bit = pbit & ParallelIdentifier::SharedMem,
             device_bit = pbit & ParallelIdentifier::Heterogeneous;

        ParallelPlan ret;
        ret.info = info;
        ret.distributed_workers_count = dist_bit ? info.nodeInfo.node_count : 1;
        ret.shared_memory_workers_count = sm_bit ? info.threadInfo.thread_count : 1;
        ret.heterogeneous_workers_count = device_bit ? info.deviceInfo.device_count : 0;

        return ret;
    }
}// namespace OpFlow
#endif//OPFLOW_PARALLELPLAN_HPP
