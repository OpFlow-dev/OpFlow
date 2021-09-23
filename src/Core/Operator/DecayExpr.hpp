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

#ifndef OPFLOW_DECAYEXPR_HPP
#define OPFLOW_DECAYEXPR_HPP
#include "Core/Expr/ExprTrait.hpp"
#include "Core/Expr/Expression.hpp"
#include "Core/Field/FieldExprTrait.hpp"
#include "Core/Field/MeshBased/MeshBasedFieldExprTrait.hpp"
#include "Core/Field/MeshBased/Structured/StructuredFieldExprTrait.hpp"
#include "Core/Macros.hpp"

namespace OpFlow {

    template <std::size_t bc_width = 0>
    struct DecayOp {
        template <ExprType T1, ExprType T2>
        OPFLOW_STRONG_INLINE static auto couldSafeEval(const T1& t1, const T2& t2, auto&& i) {
            return t1.couldEvalAt(OP_PERFECT_FOWD(i)) || t2.couldEvalAt(OP_PERFECT_FOWD(i));
        }

        template <ExprType T1, ExprType T2>
        OPFLOW_STRONG_INLINE static auto eval_safe(const T1& t1, const T2& t2, auto&& i) {
            return (t1.couldEvalAt(OP_PERFECT_FOWD(i)) ? t1.evalSafeAt(OP_PERFECT_FOWD(i))
                                                       : t2.evalSafeAt(OP_PERFECT_FOWD(i)));
        }

        template <ExprType T1, ExprType T2>
        OPFLOW_STRONG_INLINE static auto eval(const T1& t1, const T2& t2, auto&& i) {
            return t1.couldEvalAt(OP_PERFECT_FOWD(i)) ? t1.evalAt(OP_PERFECT_FOWD(i))
                                                      : t2.evalAt(OP_PERFECT_FOWD(i));
        }

        template <FieldExprType T1, FieldExprType T2>
        static void prepare(Expression<DecayOp, T1, T2>& expr) {
            if constexpr (MeshBasedFieldExprType<T1>) {
                static_assert(MeshBasedFieldExprType<T2>,
                              "DecayOp Error: mismatch types of decay expression");
                OP_ASSERT_MSG(expr.arg1.getMesh() == expr.arg2.getMesh(),
                              "DecayOp Error: expressions must have the same mesh.");
                if constexpr (StructuredFieldExprType<T1>) {
                    static_assert(StructuredFieldExprType<T2>,
                                  "DecayOp Error: mismatch types of decay expression");
                    OP_ASSERT_MSG(expr.arg1.loc == expr.arg2.loc,
                                  "DecayOp Error: expressions must have the same location on mesh.");
                }
            }
            expr.initPropsFrom(expr.arg2);
            // name
            expr.name = fmt::format("decayExpr({}, {})", expr.arg1.name, expr.arg2.name);
        }
    };

    template <std::size_t bc_width, typename Arg1, typename Arg2>
    struct ResultType<DecayOp<bc_width>, Arg1, Arg2> {
        using type = typename internal::ExprTrait<Arg2>::template twin_type<
                Expression<DecayOp<bc_width>, Arg1, Arg2>>;
        using core_type = Expression<DecayOp<bc_width>, Arg1, Arg2>;
    };

    namespace internal {
        template <std::size_t bc_w, typename Arg1, typename Arg2>
        struct ExprTrait<Expression<DecayOp<bc_w>, Arg1, Arg2>> : ExprTrait<Arg2> {
            static constexpr int bc_width = bc_w;
            static constexpr auto access_flag = 0;
        };
    }// namespace internal

    template <std::size_t bc_width = 0, ExprType T1, ExprType T2>
    OPFLOW_STRONG_INLINE static auto decay(T1&& t1, T2&& t2) {
        return makeExpression<DecayOp<bc_width>>(OP_PERFECT_FOWD(t1), OP_PERFECT_FOWD(t2));
    }

    template <std::size_t bc_width = 0, ExprType T1, ExprType T2, ExprType T3, ExprType... Ts>
    OPFLOW_STRONG_INLINE static auto decay(T1&& t1, T2&& t2, T3&& t3, Ts&&... ts) {
        return decay<bc_width>(OP_PERFECT_FOWD(t1), decay<bc_width>(OP_PERFECT_FOWD(t2), OP_PERFECT_FOWD(t3),
                                                                    OP_PERFECT_FOWD(ts)...));
    }
}// namespace OpFlow

#endif//OPFLOW_DECAYEXPR_HPP