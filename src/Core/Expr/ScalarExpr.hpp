// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2022 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_SCALAREXPR_HPP
#define OPFLOW_SCALAREXPR_HPP

#include "Core/Expr/Expr.hpp"
#include "Core/Expr/ScalarExprTrait.hpp"
#include "Core/Meta.hpp"
#include <type_traits>

namespace OpFlow {
    template <typename T>
    struct ScalarExpr : Expr<ScalarExpr<T>> {
        T val;
        // ctors
        ScalarExpr() = default;
        ScalarExpr(const ScalarExpr&) = default;
        ScalarExpr(ScalarExpr&&) noexcept = default;
        explicit ScalarExpr(const T& t) : val(t) {}

        // const evaluators
        auto operatorT() const { return val; }
        const auto& get() const { return val; }
        const auto& evalAtImpl_final(auto&&...) const { return val; }
        auto& evalAtImpl_final(auto&&...) { return val; }

        // modifiers
        auto& get() { return val; }
        auto& getAt(auto&&...) const { return val; }
        auto& operator=(const ScalarExpr& o) {
            val = o.val;
            return *this;
        }
        auto& operator=(const T& o) {
            val = o;
            return *this;
        }
        void set(const T& t) { val = t; }

        static constexpr bool isConcrete() { return true; }
        void prepareImpl_final() const {}
        template <typename O>
        requires(!std::same_as<O, ScalarExpr>) bool containsImpl_final(const O&) const {
            return false;
        }

        bool containsImpl_final(const ScalarExpr& t) const { return this == &t; }
        bool couldEvalAtImpl_final(auto&&) const { return true; }
    };

    namespace Meta {
        template <typename T>
        using warp_if_scalar_t = std::conditional_t<(Meta::Numerical<T> || std::is_convertible_v<T, bool>),
                                                    ScalarExpr<Meta::RealType<T>>, T>;

        template <typename T>
        auto forward_unless_scalar(T&& t) -> decltype(auto) {
            if constexpr ((Meta::Numerical<T> || std::is_convertible_v<T, bool>) )
                return ScalarExpr<Meta::RealType<T>> {std::forward<T>(t)};
            else
                return std::forward<T>(t);
        }
    }// namespace Meta
}// namespace OpFlow
#endif//OPFLOW_SCALAREXPR_HPP
