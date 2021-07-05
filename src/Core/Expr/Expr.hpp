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

#ifndef OPFLOW_EXPR_HPP
#define OPFLOW_EXPR_HPP

#include "Core/Constants.hpp"
#include "Core/Expr/ExprTrait.hpp"
#include "Core/Macros.hpp"
#include "Core/Meta.hpp"
#include <string>

namespace OpFlow {

    template <typename Derived>
    struct Expr;

    namespace internal {

        template <typename Derived, bool rw>
        struct ExprImpl;

        template <typename Derived>
        struct ExprImpl<Derived, true> {
            template <typename Other>
            auto& operator=(Expr<Other>& other) {
                this->derived() = other;
                return *this;
            }

            template <typename Other>
            auto& operator=(Expr<Other>&& other) {
                this->derived() = std::move(other);
                return *this;
            }

            auto& operator=(const Derived& other) {
                if (&other != this) { this->derived() = other; }
                return *this;
            }

        private:
            DEFINE_CRTP_HELPERS(Derived)
        };

        template <typename Derived>
        struct ExprImpl<Derived, false> {};
    }// namespace internal

    template <typename Derived>
    struct Expr
        : internal::ExprImpl<Derived, bool(internal::ExprTrait<Derived>::access_flag& HasWriteAccess)> {
        /// \typedef The result type of the expr
        using type = typename internal::ExprTrait<Derived>::type;
        std::string name;

        [[nodiscard]] static constexpr bool isConcrete() { return Derived::isConcrete(); }
        [[maybe_unused]] [[nodiscard]] const auto& getName() const { return name; }
        template <typename T>
        bool contains(const T& t) const {
            return this->derived().contains(t);
        }

        /// \brief prepare all meta infos of the expr
        void prepare() { derived().prepare(); }

        /// init all props from another expr
        /// \param expr the src expr
        void initPropsFrom(auto&& expr) { derived().initPropsFrom(OP_PERFECT_FOWD(expr)); }

    private:
        DEFINE_CRTP_HELPERS(Derived)
    };

    template <typename T>
    struct ExprBuilder;

    namespace internal {
        template <typename T>
        struct ExprProxy;

        template <ExprType T>
        struct ExprProxy<T> {
            // if T is a concrete expr (usually a field), take the ref;
            // else (usually an expression) take T's copy
            using type = std::conditional_t<T::isConcrete(), Meta::RealType<T>&, Meta::RealType<T>>;
        };
        template <Meta::Numerical T>
        struct ExprProxy<T> {
            using type = T;
        };
    }// namespace internal

}// namespace OpFlow
#endif//OPFLOW_EXPR_HPP
