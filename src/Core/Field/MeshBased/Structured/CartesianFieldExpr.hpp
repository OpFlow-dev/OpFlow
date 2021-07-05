#ifndef OPFLOW_CARTESIANFIELDEXPR_HPP
#define OPFLOW_CARTESIANFIELDEXPR_HPP

#include "CartesianFieldExprTrait.hpp"
#include "StructuredFieldExpr.hpp"

namespace OpFlow {

    template <typename Derived>
    struct CartesianFieldExpr : StructuredFieldExpr<Derived> {
        CartesianFieldExpr() = default;
        CartesianFieldExpr(const CartesianFieldExpr& other) : StructuredFieldExpr<Derived>(other) {}
        CartesianFieldExpr(CartesianFieldExpr&& other) noexcept
            : StructuredFieldExpr<Derived>(std::move(other)) {}

        template <CartesianFieldExprType Other>
        void initPropsFrom(const Other& expr) {
            static_cast<StructuredFieldExpr<Derived>*>(this)->template initPropsFrom(expr);
        }
    };
}// namespace OpFlow
#endif//OPFLOW_CARTESIANFIELDEXPR_HPP
