// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2026 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_STRUCTREDUCE_HPP
#define OPFLOW_STRUCTREDUCE_HPP

#include "Core/Expr/Expr.hpp"
#include "Core/Field/FieldExprTrait.hpp"
#include "Core/Macros.hpp"
#include "Core/Meta.hpp"
#include "RangeFor.hpp"
#ifndef OPFLOW_INSIDE_MODULE
#endif

OPFLOW_MODULE_EXPORT namespace OpFlow {
    template <FieldExprType E, typename Func, typename ReOp>
    auto structReduce(E & expr, ReOp && op, Func && func) {
        return structReduce(expr, expr.props.accessibleRange, std::forward<ReOp>(op),
                            std::forward<Func>(func));
    }

    template <FieldExprType E, typename R, typename Func, typename ReOp>
    auto structReduce(E & expr, R && range, ReOp && op, Func && func) {
        constexpr auto dim = internal::FieldExprTrait<E>::dim;
        std::array<int, dim> dims = expr.props.accessibleRange.getExtends();
        auto g_range = expr.props.accessibleRange;
        auto common_range = DS::commonRange(g_range, range);
        return rangeReduce(common_range, std::forward<ReOp>(op), std::forward<Func>(func));
    }
}// namespace OpFlow
#endif//OPFLOW_STRUCTREDUCE_HPP
