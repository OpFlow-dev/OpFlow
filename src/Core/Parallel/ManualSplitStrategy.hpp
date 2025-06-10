//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2025 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_MANUALSPLITSTRATEGY_HPP
#define OPFLOW_MANUALSPLITSTRATEGY_HPP

#include "Core/Field/FieldExprTrait.hpp"
#include "Core/Parallel/AbstractSplitStrategy.hpp"
#ifdef OPFLOW_WITH_MPI
#ifndef OPFLOW_INSIDE_MODULE
#include <mpi.h>
#endif
#endif
#ifndef OPFLOW_INSIDE_MODULE
#include <array>
#include <string>
#include <vector>
#endif

OPFLOW_MODULE_EXPORT namespace OpFlow {
    /// \brief Manually specified split ranges strategy
    /// (usually used for debug or adapt to external parallel strategies)
    /// \tparam F FieldType
    template <FieldExprType F>
    struct ManualSplitStrategy : AbstractSplitStrategy<F> {
    public:
        static constexpr int dim = internal::ExprTrait<F>::dim;

        ManualSplitStrategy() = default;
        ManualSplitStrategy(std::vector<typename internal::ExprTrait<F>::range_type> ranges)
            : splitMap(std::move(ranges)) {}

        [[nodiscard]] std::string strategyName() const override { return "Manual split strategy"; }

        typename internal::ExprTrait<F>::range_type
        splitRange(const typename internal::ExprTrait<F>::range_type& range,
                   const ParallelPlan& plan) override {
            return splitMap[getWorkerId()];
        }

        std::vector<typename internal::ExprTrait<F>::range_type>
        getSplitMap(const typename internal::ExprTrait<F>::range_type& range,
                    const ParallelPlan& plan) override {
            return splitMap;
        }

        std::vector<typename internal::ExprTrait<F>::range_type> splitMap;
    };
}// namespace OpFlow

#endif//OPFLOW_MANUALSPLITSTRATEGY_HPP
