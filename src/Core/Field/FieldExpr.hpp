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

#ifndef OPFLOW_FIELDEXPR_HPP
#define OPFLOW_FIELDEXPR_HPP

#include "Core/Constants.hpp"
#include "Core/Expr/Expr.hpp"
#include "Core/Expr/ExprTrait.hpp"
#include "Core/Field/FieldExprTrait.hpp"
#include "Core/Meta.hpp"

namespace OpFlow {

    template <typename Derived>
    struct FieldExpr;

    namespace internal {

        template <typename Derived, bool rw, bool dir>
        struct FieldExprImpl;

        template <typename Derived>
        struct FieldExprImpl<Derived, true, true> {
            static constexpr bool _isConcrete() { return true; }

            // return a non-const view of the underlying field
            auto getView() { return this->derived().getView(); }

            // return the result field
            auto instantiate() const { return this->derived(); }

            // getters
            auto& operator()(auto&&... i) { return this->derived()(std::forward<decltype(i)>(i)...); }
            auto& operator[](auto&& i) { return this->derived()[std::forward<decltype(i)>(i)]; }
            // const getters
            const auto& operator()(auto&&... i) const {
                return this->derived()(std::forward<decltype(i)>(i)...);
            }
            const auto& operator[](auto&& i) const { return this->derived()[std::forward<decltype(i)>(i)]; }
            const auto& evalAt(auto&&... i) const { return this->derived()(std::forward<decltype(i)>(i)...); }
            const auto& evalSafeAt(auto&&... i) const {
                return this->derived().evalSafeAt(std::forward<decltype(i)>(i)...);
            }
            // assign to constant
            auto& operator=(const typename FieldExprTrait<Derived>::elem_type& c) {
                this->derived() = c;
                return *this;
            }

        private:
            DEFINE_CRTP_HELPERS(Derived)
        };

        template <typename Derived>
        struct FieldExprImpl<Derived, true, false> {
            static constexpr bool _isConcrete() { return false; }

            auto getView() { return this->derived().getView(); }
            auto instantiate() const { return typename FieldExprTrait<Derived>::type(this->derived()); }

            // getters
            auto operator()(auto&&... i) { return this->derived()(std::forward<decltype(i)>(i)...); }
            auto operator[](auto&& i) { return this->derived()[std::forward<decltype(i)>(i)]; }
            // const getters
            auto operator()(auto&&... i) const { return this->derived()(std::forward<decltype(i)>(i)...); }
            auto operator[](auto&& i) const { return this->derived()[std::forward<decltype(i)>(i)]; }
            auto evalAt(auto&&... i) const { return this->derived()(std::forward<decltype(i)>(i)...); }
            auto evalSafeAt(auto&&... i) const {
                return this->derived().evalSafeAt(std::forward<decltype(i)>(i)...);
            }
            // asign to const
            auto& operator=(const typename FieldExprTrait<Derived>::elem_type& c) {
                this->derived() = c;
                return *this;
            }

        private:
            DEFINE_CRTP_HELPERS(Derived)
        };

        template <typename Derived>
        struct FieldExprImpl<Derived, false, true> {
            static constexpr bool _isConcrete() { return true; }

            auto instantiate() const { return this->derived(); }

            // const getters
            const auto& operator()(auto&&... i) const {
                return this->derived()(std::forward<decltype(i)>(i)...);
            }
            const auto& operator[](auto&& i) const { return this->derived()[std::forward<decltype(i)>(i)]; }
            const auto& evalAt(auto&&... i) const { return this->derived()(std::forward<decltype(i)>(i)...); }
            const auto& evalSafeAt(auto&&... i) const {
                return this->derived().evalSafeAt(std::forward<decltype(i)>(i)...);
            }

        private:
            DEFINE_CRTP_HELPERS(Derived)
        };

        template <typename Derived>
        struct FieldExprImpl<Derived, false, false> {
            static constexpr bool _isConcrete() { return false; }

            auto instantiate() const { return typename FieldExprTrait<Derived>::type(this->derived()); }

            // const getters
            auto operator()(auto&&... i) const { return this->derived()(std::forward<decltype(i)>(i)...); }
            auto operator[](auto&& i) const { return this->derived()[std::forward<decltype(i)>(i)]; }
            auto evalAt(auto&&... i) const { return this->derived()(std::forward<decltype(i)>(i)...); }
            auto evalSafeAt(auto&&... i) const {
                return this->derived().evalSafeAt(std::forward<decltype(i)>(i)...);
            }

        private:
            DEFINE_CRTP_HELPERS(Derived)
        };
    }// namespace internal

    template <typename Derived>
    struct FieldExpr
        : Expr<Derived>,
          internal::FieldExprImpl<Derived,
                                  bool(internal::FieldExprTrait<Derived>::access_flag& HasWriteAccess),
                                  bool(internal::FieldExprTrait<Derived>::access_flag& HasDirectAccess)> {
        static constexpr int dim = internal::FieldExprTrait<Derived>::dim;
        static constexpr bool isConcrete() { return Impl::_isConcrete(); }
        static constexpr bool getDim() { return dim; }
        auto getView() const { return this->derived().getView(); }
        auto deepCopy() const { return this->derived().deepCopy(); }
        // recurse end
        void initPropsFrom(auto&& expr) {}

    private:
        constexpr static auto access_flag = internal::FieldExprTrait<Derived>::access_flag;
        using Impl = internal::FieldExprImpl<Derived, bool(access_flag& HasWriteAccess),
                                             bool(access_flag& HasDirectAccess)>;
        DEFINE_CRTP_HELPERS(Derived)
    };

}// namespace OpFlow
#endif//OPFLOW_FIELDEXPR_HPP
