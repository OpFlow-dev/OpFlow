//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2023 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_IDENTITYOP_HPP
#define OPFLOW_IDENTITYOP_HPP

#include "Core/Expr/ExprTrait.hpp"
#include "Core/Expr/ScalarExpr.hpp"
#include "Core/Field/FieldExprTrait.hpp"
#include "Core/Meta.hpp"

namespace OpFlow {
    /// \brief Identity op
    /// Used in the context of composed op as a fallback option
    struct IdentityOp {
        constexpr static auto bc_width = 0;

        template <typename E>
                requires FieldExprType<E> || ScalarExprType<E> OPFLOW_STRONG_INLINE static auto eval(const E& e, auto&& i) {
            return e.evalAt(OP_PERFECT_FOWD(i));
        }

        template <typename E>
        static void prepare(const Expression<IdentityOp, E>& expr) {
            expr.initPropsFrom(expr.arg1);
            // name
            expr.name = fmt::format("Identity({})", expr.arg1.name);
        }
    };

    template <ExprType T>
    struct ResultType<IdentityOp, T> {
        using type = typename internal::ExprTrait<T>::template twin_type<Expression<IdentityOp, T>>;
        using core_type = Expression<IdentityOp, T>;
    };

    namespace internal {
        template <ExprType T>
        struct ExprTrait<Expression<IdentityOp, T>> : ExprTrait<T> {
            static constexpr int access_flag = 0;
        };
    }// namespace internal
}// namespace OpFlow
#endif//OPFLOW_IDENTITYOP_HPP
