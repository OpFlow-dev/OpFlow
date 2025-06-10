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

#ifndef OPFLOW_MINMAX_HPP
#define OPFLOW_MINMAX_HPP
#include "Core/Expr/Expr.hpp"
#include "Core/Expr/Expression.hpp"
#include "Core/Field/FieldExpr.hpp"
#include "Core/Field/MeshBased/MeshBasedFieldExprTrait.hpp"
#include "Core/Field/MeshBased/Structured/CartesianFieldExprTrait.hpp"
#include "Core/Meta.hpp"
#include "Core/Operator/BinOpDefMacros.hpp.in"
#include "Core/Operator/Operator.hpp"
#include "Core/Operator/UniOpDefMacros.hpp.in"
#ifndef OPFLOW_INSIDE_MODULE
#include <cmath>
#include <type_traits>
#include <utility>
#endif

OPFLOW_MODULE_EXPORT namespace OpFlow {

    namespace internal {
        template <typename T, typename U>
        auto min(const T& a, const U& b) {
            if constexpr (std::is_same_v<T, U>) {
                return std::min<T>(a, b);
            } else {
                return std::min<std::common_type_t<T, U>>(a, b);
            }
        }
        template <typename T, typename U>
        auto max(const T& a, const U& b) {
            if constexpr (std::is_same_v<T, U>) {
                return std::max<T>(a, b);
            } else {
                return std::max<std::common_type_t<T, U>>(a, b);
            }
        }
    }// namespace internal

    DEFINE_BINFUNC(Max, internal::max, max)
    DEFINE_BINFUNC(Min, internal::min, min)

#undef DEFINE_BINOP
#undef DEFINE_BINFUNC
#undef DEFINE_UNIOP
#undef DEFINE_UNIFUNC
}// namespace OpFlow
#endif//OPFLOW_MINMAX_HPP
