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

#ifndef OPFLOW_EVENSPLITSTRATEGY_HPP
#define OPFLOW_EVENSPLITSTRATEGY_HPP

#include "Core/Field/FieldExprTrait.hpp"
#include "Core/Field/MeshBased/Structured/StructuredFieldExprTrait.hpp"
#include "Core/Parallel/AbstractSplitStrategy.hpp"
#include <algorithm>
#include <array>
#include <string>
#include <utility>
#include <vector>
#ifdef OPFLOW_WITH_MPI
#include <mpi.h>
#endif

namespace OpFlow {
    template <FieldExprType F>
    struct EvenSplitStrategy : AbstractSplitStrategy<F> {
        [[nodiscard]] std::string strategyName() const override { return "Even split"; }
        typename internal::ExprTrait<F>::range_type
        splitRange(const typename internal::ExprTrait<F>::range_type& range,
                   const ParallelPlan& plan) override {
            return split_impl(range, 0, plan);
        }
        typename internal::ExprTrait<F>::range_type
        splitRange(const typename internal::ExprTrait<F>::range_type& range, int padding,
                   const ParallelPlan& plan) override {
            return split_impl(range, padding, plan);
        }

    private:
        /// Implementations of split for different types of ranges
        /// Note that we only split the global range into #procs here

        /// \brief Split algorithm for rectangle range
        /// \tparam d Dimension of range
        /// \param range The input range
        /// \param plan Parallel plan
        /// \return The local range for current process
        template <std::size_t d>
        static auto split_impl(const DS::Range<d>& range, int padding, const ParallelPlan& plan) {
            if (plan.singleNodeMode()) return range;
            else {
                // Minimize the total comm cost
                // For dim i, n_i = pow(N_total / (N1 * N2 * ... Nd), 1/d) * Ni
                // We try three strategy here:
                // 1. from the min size dim to the max size dim, n_i always round up
                // 2. from the min size dim to the max size dim, n_i always round down
                // Then we take the smaller cost one. Cost function:
                // Cost = sum ( n_i / Ni )

                // assume the input range is nodal range, convert to central range
                auto _range = range;
                for (auto i = 0; i < d; ++i) _range.end[i]--;
                auto extends = _range.getExtends();
                std::vector<std::pair<int, int>> labeled_extends;
                for (auto i = 0; i < d; ++i) { labeled_extends.template emplace_back(i, extends[i]); }
                std::sort(labeled_extends.begin(), labeled_extends.end(), [](auto&& a, auto&& b) {
                    // sort according to dim size
                    return a.second < b.second;
                });
                // Strategy 1:
                std::array<int, d> splits_1;
                int remain_vol = _range.count();
                int remain_proc = plan.distributed_workers_count;
                for (auto i = 0; i < d; ++i) {
                    // find all factors of the remained proc
                    std::vector<int> factors;
                    for (auto j = 1; j <= remain_proc; ++j) {
                        if (remain_proc % j == 0) factors.push_back(j);
                    }
                    const auto& c = labeled_extends[i];
                    auto p = std::lower_bound(factors.begin(), factors.end(),
                                              std::pow(remain_proc * 1.0 / remain_vol, 1. / (d - i))
                                                      * c.second);
                    int n = (p != factors.end()) ? *p : plan.distributed_workers_count;
                    splits_1[c.first] = n;
                    remain_proc /= n;
                    remain_vol /= _range.end[c.first] - _range.start[c.first];
                }
                auto cost_1 = 0.;
                for (auto i = 0; i < d; ++i) cost_1 += splits_1[i] / (_range.end[i] - _range.start[i]);
                // Strategy 2:
                std::array<int, d> splits_2;
                remain_vol = _range.count();
                remain_proc = plan.distributed_workers_count;
                for (auto i = 0; i < d; ++i) {
                    // find all factors of the remained proc
                    std::vector<int> factors;
                    for (auto j = 1; j <= remain_proc; ++j) {
                        if (remain_proc % j == 0) factors.push_back(j);
                    }
                    const auto& c = labeled_extends[i];
                    auto p = std::lower_bound(factors.begin(), factors.end(),
                                              std::pow(remain_proc * 1.0 / remain_vol, 1. / (d - i))
                                              * c.second);
                    int n = (p != factors.begin()) ? *(p - 1) : 1;
                    splits_2[c.first] = n;
                    remain_proc /= n;
                    remain_vol /= _range.end[c.first] - _range.start[c.first];
                }
                auto cost_2 = 0.;
                for (auto i = 0; i < d; ++i) cost_2 += splits_2[i] / (_range.end[i] - _range.start[i]);

                // choose the smaller cost plan
                std::array<int, d> final_split;
                if (cost_1 <= cost_2) final_split = splits_1;
                else
                    final_split = splits_2;

#ifdef OPFLOW_WITH_MPI
                int proc_rank;
                MPI_Comm_rank(MPI_COMM_WORLD, &proc_rank);
                DS::Range<d> proc_range(final_split);
                DS::RangedIndex<d> idx(proc_range);
                idx += proc_rank;
                DS::Range<d> ret;
                for (auto i = 0; i < d; ++i) {
                    ret.start[i]
                            = _range.start[i] + (_range.end[i] - _range.start[i]) / final_split[i] * idx[i];
                    if (idx[i] < proc_range.end[i] - 1)
                        ret.end[i] = ret.start[i] + (_range.end[i] - _range.start[i]) / final_split[i];
                    else
                        ret.end[i] = _range.end[i];
                    // append padding
                    if (idx[i] > 0) {
                        // pad the start side
                        OP_ASSERT(ret.start[i] - padding >= _range.start[i]);
                        ret.start[i] -= padding;
                    }
                    if (idx[i] < proc_range.end[i] - 1) {
                        // pad the end side
                        OP_ASSERT(ret.end[i] + padding <= _range.end[i]);
                        ret.end[i] += padding;
                    }
                    // recover to nodal range
                    ret.end[i]++;
                }
                return ret;
#else
                OP_ERROR("Distributed parallel mode enabled without MPI library. Stop.")
                OP_ABORT;
#endif
            }
        }
    };
}// namespace OpFlow
#endif//OPFLOW_EVENSPLITSTRATEGY_HPP
