//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2021 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_CARTAMRFIELDEXPR_HPP
#define OPFLOW_CARTAMRFIELDEXPR_HPP

#include "Core/Field/MeshBased/SemiStructured/CartAMRFieldExprTrait.hpp"
#include "Core/Macros.hpp"
#include "SemiStructuredFieldExprTrait.hpp"
#include <vector>

namespace OpFlow {

    template <typename Derived>
    struct CartAMRFieldExpr : SemiStructuredFieldExpr<Derived> {
        std::vector<typename internal::CartAMRFieldExprTrait<Derived>::range_type> maxLogicalRanges;
        CartAMRFieldExpr() = default;
        CartAMRFieldExpr(const CartAMRFieldExpr& other)
            : SemiStructuredFieldExpr<Derived>(other), maxLogicalRanges(other.maxLogicalRanges) {}
        CartAMRFieldExpr(CartAMRFieldExpr&& other) noexcept
            : SemiStructuredFieldExpr<Derived>(std::move(other)),
              maxLogicalRanges(std::move(other.maxLogicalRanges)) {}

    protected:
        template <CartAMRFieldExprType Other>
        void initPropsFromImpl_CartAMRFieldExpr(const Other& other) {
            this->initPropsFromImpl_SemiStructuredFieldExpr(other);
            maxLogicalRanges = other.maxLogicalRanges;
        }

        template <CartAMRFieldExprType Other>
        void initPropsFromImpl_final(const Other& other) {
            initPropsFromImpl_CartAMRFieldExpr(other);
        }
    };
}// namespace OpFlow
#endif//OPFLOW_CARTAMRFIELDEXPR_HPP
