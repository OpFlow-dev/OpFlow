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

#ifndef OPFLOW_EXPR_HPP
#define OPFLOW_EXPR_HPP

#include "Core/Constants.hpp"
#include "Core/Expr/ExprTrait.hpp"
#include "Core/Macros.hpp"
#include "Core/Meta.hpp"
#ifndef OPFLOW_INSIDE_MODULE
#include <string>
#endif

OPFLOW_MODULE_EXPORT namespace OpFlow {

    template <typename Derived, bool rw, bool dir>
    struct Expr;

    template <typename Derived>
    struct Expr<Derived, true, true> {
        /// \typedef The result type of the expr
        using type = typename internal::ExprTrait<Derived>::type;
        mutable std::string name;

        [[nodiscard]] static constexpr bool isConcrete() { return true; }
        [[maybe_unused]] [[nodiscard]] const auto& getName() const { return name; }
        template <typename T>
        bool contains(const T& t) const {
            return this->derived().containsImpl_final(t);
        }

        /// \brief prepare all meta infos of the expr
        void prepare() const { this->derived().prepareImpl_final(); }

        bool couldEvalAt(auto&& i) const { return this->derived().couldEvalAtImpl_final(OP_PERFECT_FOWD(i)); }

        /// init all props from another expr
        /// \param expr the src expr
        void initPropsFrom(auto&& expr) const {
            this->derived().initPropsFromImpl_final(OP_PERFECT_FOWD(expr));
        }

        template <typename Other>
                requires ExprType<Other> || Meta::Numerical<Other> auto& operator=(Other&& other) {
            this->derived().template assignImpl_final<BasicArithOp::Eq>(std::forward<Other>(other));
            return *this;
        }

        template <typename Other>
                requires ExprType<Other> || Meta::Numerical<Other> auto& operator+=(Other&& other) {
            this->derived().template assignImpl_final<BasicArithOp::Add>(OP_PERFECT_FOWD(other));
            return *this;
        }

        template <typename Other>
                requires ExprType<Other> || Meta::Numerical<Other> auto& operator-=(Other&& other) {
            this->derived().template assignImpl_final<BasicArithOp::Minus>(OP_PERFECT_FOWD(other));
            return *this;
        }

        template <typename Other>
                requires ExprType<Other> || Meta::Numerical<Other> auto& operator*=(Other&& other) {
            this->derived().template assignImpl_final<BasicArithOp::Mul>(OP_PERFECT_FOWD(other));
            return *this;
        }

        template <typename Other>
                requires ExprType<Other> || Meta::Numerical<Other> auto& operator/=(Other&& other) {
            this->derived().template assignImpl_final<BasicArithOp::Div>(OP_PERFECT_FOWD(other));
            return *this;
        }

        template <typename Other>
        requires ExprType<Other> auto& operator%=(Other&& other) {
            this->derived().template assignImpl_final<BasicArithOp::Mod>(OP_PERFECT_FOWD(other));
            return *this;
        }

        template <typename Other>
        requires ExprType<Other> auto& operator&=(Other&& other) {
            this->derived().template assignImpl_final<BasicArithOp::And>(OP_PERFECT_FOWD(other));
            return *this;
        }

        template <typename Other>
        requires ExprType<Other> auto& operator|=(Other&& other) {
            this->derived().template assignImpl_final<BasicArithOp::Or>(OP_PERFECT_FOWD(other));
            return *this;
        }

        template <typename Other>
        requires ExprType<Other> auto& operator^=(Other&& other) {
            this->derived().template assignImpl_final<BasicArithOp::Xor>(OP_PERFECT_FOWD(other));
            return *this;
        }

        template <typename Other>
        requires ExprType<Other> auto& operator<<=(Other&& other) {
            this->derived().template assignImpl_final<BasicArithOp::LShift>(OP_PERFECT_FOWD(other));
            return *this;
        }

        template <typename Other>
        requires ExprType<Other> auto& operator>>=(Other&& other) {
            this->derived().template assignImpl_final<BasicArithOp::RShift>(OP_PERFECT_FOWD(other));
            return *this;
        }

        // return a non-const view of the underlying field
        auto getView() { return this->derived().getViewImpl_final(); }

        // return the result field
        auto instantiate() const { return this->derived(); }

        // getters
        auto& operator()(auto&&... i) { return evalAt(OP_PERFECT_FOWD(i)...); }
        auto& operator[](auto&& i) { return evalAt(OP_PERFECT_FOWD(i)); }
        // const getters
        const auto& operator()(auto&&... i) const { return evalAt(OP_PERFECT_FOWD(i)...); }
        const auto& operator[](auto&& i) const { return evalAt(OP_PERFECT_FOWD(i)); }
        const auto& evalAt(auto&&... i) const {
            return this->derived().evalAtImpl_final(std::forward<decltype(i)>(i)...);
        }
        auto& evalAt(auto&&... i) {
            return this->derived().evalAtImpl_final(std::forward<decltype(i)>(i)...);
        }

    private:
        DEFINE_CRTP_HELPERS(Derived)
    };

    template <typename Derived>
    struct Expr<Derived, true, false> {
        /// \typedef The result type of the expr
        using type = typename internal::ExprTrait<Derived>::type;
        mutable std::string name;

        [[nodiscard]] static constexpr bool isConcrete() { return false; }
        [[maybe_unused]] [[nodiscard]] const auto& getName() const { return name; }
        template <typename T>
        bool contains(const T& t) const {
            return this->derived().containsImpl_final(t);
        }

        /// \brief prepare all meta infos of the expr
        void prepare() const { this->derived().prepareImpl_final(); }

        bool couldEvalAt(auto&& i) const { return this->derived().couldEvalAtImpl_final(OP_PERFECT_FOWD(i)); }

        /// init all props from another expr
        /// \param expr the src expr
        void initPropsFrom(auto&& expr) const {
            this->derived().initPropsFromImpl_final(OP_PERFECT_FOWD(expr));
        }

        template <typename Other>
                requires ExprType<Other> || Meta::Numerical<Other> auto& operator=(Other&& other) {
            this->derived().template assignImpl_final<BasicArithOp::Eq>(std::forward<Other>(other));
            return *this;
        }

        template <typename Other>
                requires ExprType<Other> || Meta::Numerical<Other> auto& operator+=(Other&& other) {
            this->derived().template assignImpl_final<BasicArithOp::Add>(OP_PERFECT_FOWD(other));
            return *this;
        }

        template <typename Other>
                requires ExprType<Other> || Meta::Numerical<Other> auto& operator-=(Other&& other) {
            this->derived().template assignImpl_final<BasicArithOp::Minus>(OP_PERFECT_FOWD(other));
            return *this;
        }

        template <typename Other>
                requires ExprType<Other> || Meta::Numerical<Other> auto& operator*=(Other&& other) {
            this->derived().template assignImpl_final<BasicArithOp::Mul>(OP_PERFECT_FOWD(other));
            return *this;
        }

        template <typename Other>
                requires ExprType<Other> || Meta::Numerical<Other> auto& operator/=(Other&& other) {
            this->derived().template assignImpl_final<BasicArithOp::Div>(OP_PERFECT_FOWD(other));
            return *this;
        }

        template <typename Other>
        requires ExprType<Other> auto& operator%=(Other&& other) {
            this->derived().template assignImpl_final<BasicArithOp::Mod>(OP_PERFECT_FOWD(other));
            return *this;
        }

        template <typename Other>
        requires ExprType<Other> auto& operator&=(Other&& other) {
            this->derived().template assignImpl_final<BasicArithOp::And>(OP_PERFECT_FOWD(other));
            return *this;
        }

        template <typename Other>
        requires ExprType<Other> auto& operator|=(Other&& other) {
            this->derived().template assignImpl_final<BasicArithOp::Or>(OP_PERFECT_FOWD(other));
            return *this;
        }

        template <typename Other>
        requires ExprType<Other> auto& operator^=(Other&& other) {
            this->derived().template assignImpl_final<BasicArithOp::Xor>(OP_PERFECT_FOWD(other));
            return *this;
        }

        template <typename Other>
        requires ExprType<Other> auto& operator<<=(Other&& other) {
            this->derived().template assignImpl_final<BasicArithOp::LShift>(OP_PERFECT_FOWD(other));
            return *this;
        }

        template <typename Other>
        requires ExprType<Other> auto& operator>>=(Other&& other) {
            this->derived().template assignImpl_final<BasicArithOp::RShift>(OP_PERFECT_FOWD(other));
            return *this;
        }

        auto getView() { return this->derived().getViewImpl_final(); }
        auto instantiate() const { return typename internal::ExprTrait<Derived>::type(this->derived()); }

        // getters
        auto operator()(auto&&... i) { return evalAt(OP_PERFECT_FOWD(i)...); }
        auto operator[](auto&& i) { return evalAt(OP_PERFECT_FOWD(i)); }
        // const getters
        auto operator()(auto&&... i) const { return evalAt(OP_PERFECT_FOWD(i)...); }
        auto operator[](auto&& i) const { return evalAt(OP_PERFECT_FOWD(i)); }
        auto evalAt(auto&&... i) const { return this->derived().evalAtImpl_final(OP_PERFECT_FOWD(i)...); }

    private:
        DEFINE_CRTP_HELPERS(Derived)
    };

    template <typename Derived>
    struct Expr<Derived, false, true> {
        /// \typedef The result type of the expr
        using type = typename internal::ExprTrait<Derived>::type;
        mutable std::string name;

        [[nodiscard]] static constexpr bool isConcrete() { return true; }
        [[maybe_unused]] [[nodiscard]] const auto& getName() const { return name; }
        template <typename T>
        bool contains(const T& t) const {
            return this->derived().containsImpl_final(t);
        }

        /// \brief prepare all meta infos of the expr
        void prepare() const { this->derived().prepareImpl_final(); }

        bool couldEvalAt(auto&& i) const { return this->derived().couldEvalAtImpl_final(OP_PERFECT_FOWD(i)); }

        /// init all props from another expr
        /// \param expr the src expr
        void initPropsFrom(auto&& expr) const {
            this->derived().initPropsFromImpl_final(OP_PERFECT_FOWD(expr));
        }

        auto instantiate() const { return this->derived(); }

        // const getters
        const auto& operator()(auto&&... i) const { return evalAt(OP_PERFECT_FOWD(i)...); }
        const auto& operator[](auto&& i) const { return evalAt(OP_PERFECT_FOWD(i)); }
        const auto& evalAt(auto&&... i) const {
            return this->derived().evalAtImpl_final(OP_PERFECT_FOWD(i)...);
        }

    private:
        DEFINE_CRTP_HELPERS(Derived)
    };

    template <typename Derived>
    struct Expr<Derived, false, false> {
        /// \typedef The result type of the expr
        using type = typename internal::ExprTrait<Derived>::type;
        mutable std::string name;

        [[nodiscard]] static constexpr bool isConcrete() { return false; }
        [[maybe_unused]] [[nodiscard]] const auto& getName() const { return name; }
        template <typename T>
        bool contains(const T& t) const {
            return this->derived().containsImpl_final(t);
        }

        /// \brief prepare all meta infos of the expr
        void prepare() const { this->derived().prepareImpl_final(); }

        bool couldEvalAt(auto&& i) const { return this->derived().couldEvalAtImpl_final(OP_PERFECT_FOWD(i)); }

        /// init all props from another expr
        /// \param expr the src expr
        void initPropsFrom(auto&& expr) const {
            this->derived().initPropsFromImpl_final(OP_PERFECT_FOWD(expr));
        }

        auto instantiate() const { return typename internal::ExprTrait<Derived>::type(this->derived()); }

        // const getters
        auto operator()(auto&&... i) const { return evalAt(OP_PERFECT_FOWD(i)...); }
        auto operator[](auto&& i) const { return evalAt(OP_PERFECT_FOWD(i)); }
        auto evalAt(auto&&... i) const { return this->derived().evalAtImpl_final(OP_PERFECT_FOWD(i)...); }

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
            // if T is a concrete expr (usually a field), take the const ref;
            // else (usually an expression) take T's copy
            using type = std::conditional_t<T::isConcrete(), const Meta::RealType<T>&, Meta::RealType<T>>;
        };
        template <Meta::Numerical T>
        struct ExprProxy<T> {
            using type = T;
        };
    }// namespace internal

}// namespace OpFlow
#endif//OPFLOW_EXPR_HPP
