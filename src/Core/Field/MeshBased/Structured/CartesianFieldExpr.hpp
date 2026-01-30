// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2026 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_CARTESIANFIELDEXPR_HPP
#define OPFLOW_CARTESIANFIELDEXPR_HPP

#include "CartesianFieldExprTrait.hpp"
#include "StructuredFieldExpr.hpp"

OPFLOW_MODULE_EXPORT namespace OpFlow {

    template <typename Derived>
    struct CartesianFieldExpr : StructuredFieldExpr<Derived> {
        CartesianFieldExpr() = default;
        CartesianFieldExpr(const CartesianFieldExpr& other) : StructuredFieldExpr<Derived>(other) {}
        CartesianFieldExpr(CartesianFieldExpr&& other) noexcept
            : StructuredFieldExpr<Derived>(std::move(other)) {}

    protected:
        template <CartesianFieldExprType Other>
        void initPropsFromImpl_CartesianFieldExpr(const Other& expr) const {
            this->initPropsFromImpl_StructuredFieldExpr(expr);
        }

        template <CartesianFieldExprType Other>
        void initPropsFromImpl_final(const Other& expr) const {
            initPropsFromImpl_CartesianFieldExpr(expr);
        }
    };
}// namespace OpFlow
#endif//OPFLOW_CARTESIANFIELDEXPR_HPP
