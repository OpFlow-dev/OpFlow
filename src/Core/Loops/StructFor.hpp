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

#ifndef OPFLOW_STRUCTFOR_HPP
#define OPFLOW_STRUCTFOR_HPP

#include "Core/Expr/Expr.hpp"
#include "Core/Field/FieldExprTrait.hpp"
#include "Core/Macros.hpp"
#include "Core/Meta.hpp"
#include "RangeFor.hpp"

OPFLOW_MODULE_EXPORT namespace OpFlow {
    template <FieldExprType E, typename Func>
    requires std::invocable<Func, DS::MDIndex<internal::FieldExprTrait<E>::dim>> auto structFor(
            E & expr, Func && func) {
        return structFor(expr, expr.accessibleRange, std::forward<Func>(func));
    }

    template <FieldExprType E, typename R, typename Func>
    Func structFor(E & expr, R && range, Func && func) {
        constexpr auto dim = internal::FieldExprTrait<E>::dim;
        std::array<int, dim> dims = expr.accessibleRange.getExtends();
        auto g_range = expr.accessibleRange;
        auto common_range = DS::commonRange(g_range, range);
        return rangeFor(common_range, std::forward<Func>(func));
    }

    template <FieldExprType E, typename Func>
    requires std::invocable<Func, DS::MDIndex<internal::FieldExprTrait<E>::dim>> auto structFor_s(
            E & expr, Func && func) {
        return structFor_s(expr, expr.accessibleRange, std::forward<Func>(func));
    }

    template <FieldExprType E, typename R, typename Func>
    requires std::invocable<Func, DS::MDIndex<internal::FieldExprTrait<E>::dim>> auto structFor_s(
            E & expr, R && range, Func && func) {
        constexpr auto dim = internal::FieldExprTrait<E>::dim;
        std::array<int, dim> dims = expr.accessibleRange.getExtends();
        auto g_range = expr.accessibleRange;
        auto common_range = DS::commonRange(g_range, range);
        return rangeFor_s(common_range, std::forward<Func>(func));
    }

}// namespace OpFlow
#endif//OPFLOW_STRUCTFOR_HPP
