// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2023 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_EQUATION_HPP
#define OPFLOW_EQUATION_HPP

#include "Core/BasicDataTypes.hpp"
#include "Core/Expr/Expr.hpp"
#include "Core/Expr/ExprTrait.hpp"
#include "Core/Expr/ScalarExpr.hpp"
#include "Core/Field/FieldExprTrait.hpp"
#include "DataStructures/Range/Ranges.hpp"
#ifndef OPFLOW_INSIDE_MODULE
#include <tuple>
#endif

namespace OpFlow {
    template <ExprType Lhs, ExprType Rhs>
    struct Equation {
        using LhsType = Lhs;
        using RhsType = Rhs;
        Equation(auto&& lhs, auto&& rhs) : lhs(OP_PERFECT_FOWD(lhs)), rhs(OP_PERFECT_FOWD(rhs)) {}
        typename internal::ExprProxy<Lhs>::type lhs;
        typename internal::ExprProxy<Rhs>::type rhs;

        operator bool() const { return isIdentical(lhs, rhs); }
    };

    template <typename T>
    concept EquationType = Meta::isTemplateInstance<Equation, Meta::RealType<T>>::value;

    template <ExprType Lhs, ExprType Rhs>
    auto operator==(Lhs&& lhs, Rhs&& rhs) {
        return Equation<Meta::RealType<Lhs>, Meta::RealType<Rhs>>(std::forward<Lhs>(lhs),
                                                                  std::forward<Rhs>(rhs));
    }

    template <ExprType Lhs, Meta::Numerical Rhs>
    auto operator==(Lhs&& lhs, Rhs&& rhs) {
        return Equation<Meta::RealType<Lhs>, ScalarExpr<Meta::RealType<Rhs>>>(std::forward<Lhs>(lhs),
                                                                              std::forward<Rhs>(rhs));
    }
    template <Meta::Numerical Lhs, ExprType Rhs>
    auto operator==(Lhs&& lhs, Rhs&& rhs) {
        return Equation<ScalarExpr<Meta::RealType<Lhs>>, Meta::RealType<Rhs>> {lhs, rhs};
    }

    template <ExprType Lhs, ExprType Rhs>
    auto operator!=(const Lhs& lhs, const Rhs& rhs) {
        return !isIdentical(lhs, rhs);
    }

    template <typename... Eqns>
    struct EquationSet {
        explicit EquationSet(const Eqns&... e) : eqns(std::make_tuple(OP_PERFECT_FOWD(e)...)) {}

        template <int i>
        using eqn_type = std::tuple_element_t<i, std::tuple<Eqns...>>;

        std::tuple<Eqns...> eqns;
    };

    template <typename... Ts>
    struct TargetSet {
        template <int i>
        using target_type = std::tuple_element_t<i, std::tuple<Meta::RealType<Ts>...>>;
    };
}// namespace OpFlow
#endif//OPFLOW_EQUATION_HPP
