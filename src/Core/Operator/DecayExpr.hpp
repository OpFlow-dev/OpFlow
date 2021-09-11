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
#include "Core/Macros.hpp"

namespace OpFlow {

    template <std::size_t bc_width = 0>
    struct DecayOp {
        template <ExprType... Ts>
        OPFLOW_STRONG_INLINE static auto couldSafeEval(const Ts&... ts, auto&& i) {
            return (DS::inRange(ts.accessibleRange, i) || ...);
        }

        template <ExprType T>
        OPFLOW_STRONG_INLINE static auto eval_safe(const T& t, auto&& i) {
            return t.evalSafeAt(OP_PERFECT_FOWD(i));
        }

        template <ExprType T1, ExprType... Ts>
        requires(sizeof...(Ts) > 0) OPFLOW_STRONG_INLINE
                static auto eval_safe(const T1& t1, const Ts&... ts, auto&& i) {
            return (DS::inRange(t1.accessibleRange, i) ? eval_safe(t1, OP_PERFECT_FOWD(i))
                                                       : eval_safe(ts..., OP_PERFECT_FOWD(i)));
        }

        template <ExprType T>
        OPFLOW_STRONG_INLINE static auto eval(const T& t, auto&& i) {
            return t.evalAt(OP_PERFECT_FOWD(i));
        }

        template <ExprType T1, ExprType... Ts>
        requires(sizeof...(Ts) > 0) OPFLOW_STRONG_INLINE
                static auto eval(const T1& t1, const Ts&... ts, auto&& i) {
            return DS::inRange(t1.acessibleRange, i) ? eval(t1, OP_PERFECT_FOWD(i))
                                                     : eval(ts..., OP_PERFECT_FOWD(i));
        }

        template <FieldExprType... Ts>
        static void prepare(Expression<DecayOp, Ts...>& expr) {
            expr.initPropsFrom(expr.arg1);
        }
    };
}// namespace OpFlow

#endif//OPFLOW_DECAYEXPR_HPP