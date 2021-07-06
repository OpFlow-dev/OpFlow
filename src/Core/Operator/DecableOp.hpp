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

#ifndef OPFLOW_DECABLEOP_HPP
#define OPFLOW_DECABLEOP_HPP

#include "Core/Expr/ExprTrait.hpp"
#include "Core/Expr/Expression.hpp"
#include "Core/Macros.hpp"

namespace OpFlow {
    template <typename Op, typename DecayedOp>
    struct DecableOp {
        constexpr static auto bc_width = Op::bc_width;
        OPFLOW_STRONG_INLINE static auto couldSafeEval(auto&&... i) {
            return Op::couldSafeEval(OP_PERFECT_FOWD(i)...)
                   || DecayedOp::couldSafeEval(OP_PERFECT_FOWD(i)...);
        }
        OPFLOW_STRONG_INLINE static auto eval_safe(auto&&... e) {
            if (Op::couldSafeEval(OP_PERFECT_FOWD(e)...)) return Op::eval_safe(OP_PERFECT_FOWD(e)...);
            else
                return DecayedOp::eval_safe(OP_PERFECT_FOWD(e)...);
        }
        OPFLOW_STRONG_INLINE static auto eval(auto&&... e) { return Op::eval(OP_PERFECT_FOWD(e)...); }
        static void prepare(auto&& expr) { DecayedOp::prepare(OP_PERFECT_FOWD(expr)); }
    };

    template <typename T>
    concept DecableOpType = Meta::isTemplateInstance<DecableOp, T>::value;

    template <typename T>
    struct LastOpOfDecableOp;

    template <typename T>
    requires(!DecableOpType<T>) struct LastOpOfDecableOp<T> {
        using type = T;
    };

    template <typename T, typename U>
    struct LastOpOfDecableOp<DecableOp<T, U>> {
        using type = typename LastOpOfDecableOp<U>::type;
    };

    template <typename Op, typename DecayedOp, typename... Args>
    struct ResultType<DecableOp<Op, DecayedOp>, Args...> {
        using type = typename internal::ExprTrait<typename ResultType<DecayedOp, Args...>::core_type>::
                template twin_type<Expression<DecableOp<Op, DecayedOp>, Args...>>;
    };

    namespace internal {
        template <typename Op, typename DecayedOp, typename... Args>
        struct ExprTrait<Expression<DecableOp<Op, DecayedOp>, Args...>>
            : ExprTrait<Expression<DecayedOp, Args...>> {
            static constexpr auto bc_width = Op::bc_width + std::max({ExprTrait<Args>::bc_width...});
        };
    }// namespace internal
}// namespace OpFlow
#endif//OPFLOW_DECABLEOP_HPP
