#ifndef OPFLOW_CARTAMRFIELDEXPRTRAIT_HPP
#define OPFLOW_CARTAMRFIELDEXPRTRAIT_HPP

#include "Core/Macros.hpp"
#include "Core/Meta.hpp"
#include "SemiStructuredFieldExprTrait.hpp"

namespace OpFlow {
    template <typename Derived>
    struct CartAMRFieldExpr;

    template <typename T>
    concept CartAMRFieldExprType = std::is_base_of_v<CartAMRFieldExpr<Meta::RealType<T>>, Meta::RealType<T>>;

    namespace internal {
        template <typename T>
        struct CartAMRFieldExprTrait : SemiStructuredFieldExprTrait<T> {};

        DEFINE_TRAITS_CVR(CartAMRFieldExpr)
    }// namespace internal
}// namespace OpFlow

#endif//OPFLOW_CARTAMRFIELDEXPRTRAIT_HPP
