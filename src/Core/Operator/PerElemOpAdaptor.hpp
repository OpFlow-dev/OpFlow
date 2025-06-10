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

#ifndef OPFLOW_PERELEMOPADAPTOR_HPP
#define OPFLOW_PERELEMOPADAPTOR_HPP

#include "Core/Expr/ExprTrait.hpp"
#include "Core/Field/MeshBased/Structured/StructuredFieldExprTrait.hpp"
#include "Core/Macros.hpp"
#include "Core/Meta.hpp"
#include "Utils/ConstexprString.hpp"
#include "Utils/NamedFunctor.hpp"

OPFLOW_MODULE_EXPORT namespace OpFlow {
    template <Utils::NamedFunctorType auto Functor>
    struct UniOpAdaptor {
        constexpr static auto bc_width = 0;

        template <StructuredFieldExprType E>
        OPFLOW_STRONG_INLINE static auto couldSafeEval(const E& expr, auto&& i) {
            return DS::inRange(expr.accessibleRange, i);
        }

        template <ExprType E>
        OPFLOW_STRONG_INLINE static auto eval_safe(const E& expr, auto&& i) {
            return Functor(expr.evalSafeAt(OP_PERFECT_FOWD(i)));
        }

        template <ExprType E>
        OPFLOW_STRONG_INLINE static auto eval(const E& expr, auto&& i) {
            return Functor(expr.evalAt(OP_PERFECT_FOWD(i)));
        }

        template <ExprType E>
        static void prepare(const Expression<UniOpAdaptor, E>& expr) {
            expr.initPropsFrom(expr.arg1);
            // name
            expr.name = std::format("{}({})", Functor.getName().to_string(), expr.arg1.name);
        }
    };

    template <Utils::NamedFunctorType auto functor, ExprType E>
    struct ResultType<UniOpAdaptor<functor>, E> {
        using type =
                typename internal::ExprTrait<E>::template twin_type<Expression<UniOpAdaptor<functor>, E>>;
        using core_type = Expression<UniOpAdaptor<functor>, E>;
    };

    namespace internal {
        template <Utils::NamedFunctorType auto functor, ExprType E>
        struct ExprTrait<Expression<UniOpAdaptor<functor>, E>> : ExprTrait<E> {
            static constexpr int access_flag = 0;
            using elem_type = decltype(functor(std::declval<const typename ExprTrait<E>::elem_type&>()));
        };
    }// namespace internal

    template <Utils::NamedFunctorType auto Functor>
    struct BinOpAdaptor {
        constexpr static auto bc_width = 0;

        template <StructuredFieldExprType E>
        OPFLOW_STRONG_INLINE static auto couldSafeEval(const E& expr1, const auto&& expr2, auto&& i) {
            return DS::inRange(expr1.accessibleRange, i);
        }

        template <ExprType E1, ExprType E2>
        OPFLOW_STRONG_INLINE static auto eval_safe(const E1& expr1, const E2& expr2, auto&& i) {
            return Functor(expr1.evalSafeAt(OP_PERFECT_FOWD(i)), expr2.evalSafeAt(OP_PERFECT_FOWD(i)));
        }

        template <ExprType E1, ExprType E2>
        OPFLOW_STRONG_INLINE static auto eval(const E1& expr1, const E2& expr2, auto&& i) {
            return Functor(expr1.evalAt(OP_PERFECT_FOWD(i)), expr2.evalAt(OP_PERFECT_FOWD(i)));
        }

        template <ExprType E1, ExprType E2>
        static void prepare(const Expression<BinOpAdaptor, E1, E2>& expr) {
            expr.initPropsFrom(expr.arg1);
            // name
            expr.name = std::format("{}({}, {})", Functor.getName().to_string(), expr.arg1.name,
                                    expr.arg2.name);
        }
    };

    template <Utils::NamedFunctorType auto functor, ExprType E1, ExprType E2>
    struct ResultType<BinOpAdaptor<functor>, E1, E2> {
        using type = typename internal::ExprTrait<E1>::template twin_type<
                Expression<BinOpAdaptor<functor>, E1, E2>>;
        using core_type = Expression<BinOpAdaptor<functor>, E1, E2>;
    };

    namespace internal {
        template <Utils::NamedFunctorType auto functor, ExprType E1, ExprType E2>
        struct ExprTrait<Expression<BinOpAdaptor<functor>, E1, E2>> : ExprTrait<E1> {
            static constexpr int access_flag = 0;
            using elem_type = decltype(functor(std::declval<const typename ExprTrait<E1>::elem_type&>(),
                                               std::declval<const typename ExprTrait<E2>::elem_type&>()));
        };
    }// namespace internal
}// namespace OpFlow
#endif//OPFLOW_PERELEMOPADAPTOR_HPP
