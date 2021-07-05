#ifndef OPFLOW_CARTESIANFIELDEXPRTRAIT_HPP
#define OPFLOW_CARTESIANFIELDEXPRTRAIT_HPP

#include "Core/Field/MeshBased/Structured/StructuredFieldExprTrait.hpp"
#include "Core/Macros.hpp"
#include "Core/Meta.hpp"

namespace OpFlow {
    template <typename Derived>
    struct CartesianFieldExpr;

    template <typename T>
    concept CartesianFieldExprType
            = std::is_base_of_v<CartesianFieldExpr<Meta::RealType<T>>, Meta::RealType<T>>;

    namespace internal {
        template <typename T>
        struct CartesianFieldExprTrait : StructuredFieldExprTrait<T> {};

        DEFINE_TRAITS_CVR(CartesianFieldExpr)
    }// namespace internal
}// namespace OpFlow
#endif//OPFLOW_CARTESIANFIELDEXPRTRAIT_HPP
