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

        template <CartAMRFieldExprType Other>
        void initPropsFrom(const Other& other) {
            maxLogicalRanges = other.maxLogicalRanges;
            static_cast<SemiStructuredFieldExpr<Derived>*>(this)->initPropsFrom(other);
        }
    };
}// namespace OpFlow
#endif//OPFLOW_CARTAMRFIELDEXPR_HPP
