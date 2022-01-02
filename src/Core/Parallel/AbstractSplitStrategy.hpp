//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2021 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_ABSTRACTSPLITSTRATEGY_HPP
#define OPFLOW_ABSTRACTSPLITSTRATEGY_HPP

#include "Core/Field/FieldExprTrait.hpp"
#include "Core/Parallel/ParallelPlan.hpp"
#include <string>
#include <vector>

namespace OpFlow {
    template <FieldExprType F>
    struct AbstractSplitStrategy {
        [[nodiscard]] virtual std::string strategyName() const = 0;
        virtual std::vector<typename internal::ExprTrait<F>::range_type>
        getSplitMap(const typename internal::ExprTrait<F>::range_type& range, const ParallelPlan& plan) = 0;
        virtual typename internal::ExprTrait<F>::range_type
        splitRange(const typename internal::ExprTrait<F>::range_type& range, const ParallelPlan& plan)
                = 0;
    };
}// namespace OpFlow

#endif//OPFLOW_ABSTRACTSPLITSTRATEGY_HPP
