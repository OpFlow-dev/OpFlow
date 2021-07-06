// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2021 by the OpFlow developers
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
#include "DataStructures/StencilPad.hpp"
#include <HYPRE.h>
#include <HYPRE_struct_ls.h>

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
}// namespace OpFlow
#endif//OPFLOW_EQUATION_HPP
