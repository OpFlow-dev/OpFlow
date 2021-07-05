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

#ifndef OPFLOW_CONDITIONAL_HPP
#define OPFLOW_CONDITIONAL_HPP

#include "Core/Expr/Expr.hpp"
#include "Core/Expr/Expression.hpp"
#include "Core/Field/MeshBased/MeshBasedFieldExprTrait.hpp"
#include "Core/Meta.hpp"
#include "Core/Operator/Operator.hpp"

namespace OpFlow {
    struct CondOp {
        constexpr static auto bc_width = 0;

        template <ExprType C, ExprType T1, ExprType T2>
        OPFLOW_STRONG_INLINE static auto couldSafeEval(const C& c, const T1& t1, const T2& t2, auto&& i) {
            return DS::inRange(c.accessibleRange, i) && DS::inRange(t1.accessibleRange, i)
                   && DS::inRange(t2.accessibleRange, i);
        }

        template <ExprType C, ExprType T1, ExprType T2>
        OPFLOW_STRONG_INLINE static auto eval_safe(const C& c, const T1& t1, const T2& t2, auto&& i) {
            return c.evalSafeAt(OP_PERFECT_FOWD(i)) ? t1.evalSafeAt(OP_PERFECT_FOWD(i))
                                                    : t2.evalSafeAt(OP_PERFECT_FOWD(i));
        }

        template <ExprType C, ExprType T1, ExprType T2>
        OPFLOW_STRONG_INLINE static auto eval(const C& c, const T1& t1, const T2& t2, auto&& i) {
            return c.evalAt(OP_PERFECT_FOWD(i)) ? t1.evalAt(OP_PERFECT_FOWD(i))
                                                : t2.evalAt(OP_PERFECT_FOWD(i));
        }

        template <FieldExprType C, FieldExprType T1, FieldExprType T2>
        static void prepare(Expression<CondOp, C, T1, T2>& expr) {
            expr.initPropsFrom(expr.arg2);
            expr.name = fmt::format("{} ? {} : {}", expr.arg1.name, expr.arg2.name, expr.arg3.name);
            if constexpr (MeshBasedFieldExprType<
                                  T1> && MeshBasedFieldExprType<T2> && MeshBasedFieldExprType<C>) {
                OP_ASSERT(expr.arg1.mesh == expr.arg2.mesh);
                OP_ASSERT(expr.arg1.mesh == expr.arg3.mesh);
                if constexpr (StructuredFieldExprType<T1> || SemiStructuredFieldExprType<T1>) {
                    OP_ASSERT(expr.arg1.loc == expr.arg2.loc);
                    OP_ASSERT(expr.arg1.loc == expr.arg3.loc);
                }
            }
            if constexpr (StructuredFieldExprType<T1>) {
                expr.accessibleRange = DS::commonRange(expr.arg1.accessibleRange, expr.arg2.accessibleRange);
                expr.accessibleRange = DS::commonRange(expr.accessibleRange, expr.arg3.accessibleRange);
                expr.assignableRange.setEmpty();
                expr.localRange = DS::commonRange(expr.arg1.localRange, expr.arg2.localRange);
                expr.localRange = DS::commonRange(expr.localRange, expr.arg3.localRange);
            } else if constexpr (SemiStructuredFieldExprType<T1>) {
                for (auto i = 0; i < expr.getLevels(); ++i) {
                    expr.accessibleRanges[i]
                            = DS::commonRanges(expr.arg1.accessibleRanges[i], expr.arg2.accessibleRanges[i]);
                    expr.accessibleRanges[i]
                            = DS::commonRanges(expr.accessibleRanges[i], expr.arg3.accessibleRanges[i]);
                    expr.localRanges[i]
                            = DS::commonRanges(expr.arg1.localRanges[i], expr.arg2.localRanges[i]);
                    expr.localRanges[i] = DS::commonRanges(expr.localRanges[i], expr.arg3.localRanges[i]);
                    for (auto& r : expr.assignableRanges[i]) r.setEmpty();
                }
            }
        }
        template <typename C, FieldExprType T1, FieldExprType T2>
        static void prepare(Expression<CondOp, ScalarExpr<C>, T1, T2>& expr) {
            if constexpr (MeshBasedFieldExprType<T1> && MeshBasedFieldExprType<T2>)
                OP_ASSERT(expr.arg2.mesh == expr.arg3.mesh);
            expr.initPropsFrom(expr.arg2);
            expr.name = fmt::format("{} ? {} : {}", expr.arg1.name, expr.arg2.name, expr.arg3.name);
            if constexpr (StructuredFieldExprType<T1>) {
                expr.accessibleRange = DS::commonRange(expr.arg2.accessibleRange, expr.arg3.accessibleRange);
                expr.assignableRange.setEmpty();
                expr.localRange = DS::commonRange(expr.arg2.localRange, expr.arg3.localRange);
            } else if constexpr (SemiStructuredFieldExprType<T1>) {
                for (auto i = 0; i < expr.getLevels(); ++i) {
                    expr.accessibleRanges[i]
                            = DS::commonRanges(expr.arg2.accessibleRanges[i], expr.arg3.accessibleRanges[i]);
                    expr.localRanges[i]
                            = DS::commonRanges(expr.arg2.localRanges[i], expr.arg3.localRanges[i]);
                    for (auto& r : expr.assignableRanges[i]) r.setEmpty();
                }
            }
        }
    };

    template <FieldExprType C, FieldExprType T, FieldExprType U>
    struct ResultType<CondOp, C, T, U> {
        using type = typename internal::FieldExprTrait<T>::template twin_type<Expression<CondOp, C, T, U>>;
        using core_type = Expression<CondOp, C, T, U>;
    };

    template <typename C, FieldExprType T, FieldExprType U>
    struct ResultType<CondOp, ScalarExpr<C>, T, U> {
        using type = typename internal::FieldExprTrait<T>::template twin_type<
                Expression<CondOp, ScalarExpr<C>, T, U>>;
        using core_type = Expression<CondOp, ScalarExpr<C>, T, U>;
    };

    namespace internal {
        template <FieldExprType C, FieldExprType T, FieldExprType U>
        struct ExprTrait<Expression<CondOp, C, T, U>> : ExprTrait<T> {
            static constexpr int bc_width = std::max(
                    {FieldExprTrait<C>::bc_width, FieldExprTrait<T>::bc_width, FieldExprTrait<U>::bc_width});
            static constexpr int access_flag = 0;
            using mesh_type = typename ViewOrVoid<T>::type;
        };
        template <typename C, FieldExprType T, FieldExprType U>
        struct ExprTrait<Expression<CondOp, ScalarExpr<C>, T, U>> : ExprTrait<T> {
            static constexpr int bc_width
                    = std::max({CartesianFieldExprTrait<T>::bc_width, CartesianFieldExprTrait<U>::bc_width});
            static constexpr int access_flag = 0;
            using mesh_type = typename ViewOrVoid<T>::type;
        };
    }// namespace internal

    template <ExprType C, ExprType T, ExprType U>
    auto conditional(C&& c, T&& t, U&& u) {
        return makeExpression<CondOp>(OP_PERFECT_FOWD(c), OP_PERFECT_FOWD(t), OP_PERFECT_FOWD(u));
    }
    template <typename C, ExprType T, ExprType U>
    requires(!ExprType<C>) auto conditional(C&& c, T&& t, U&& u) {
        // here c must be kept rather than directly return t or u because c's value may change during exec
        return makeExpression<CondOp>(ScalarExpr<C>(c), OP_PERFECT_FOWD(t), OP_PERFECT_FOWD(u));
    }
}// namespace OpFlow
#endif//OPFLOW_CONDITIONAL_HPP
