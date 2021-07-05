#ifndef OPFLOW_FIELDEXPRTRAIT_HPP
#define OPFLOW_FIELDEXPRTRAIT_HPP

#include "Core/Expr/ExprTrait.hpp"
#include "Core/Macros.hpp"
#include "Core/Meta.hpp"

namespace OpFlow {
    template <typename Derived>
    struct FieldExpr;

    template <typename T>
    concept FieldExprType = std::is_base_of_v<FieldExpr<Meta::RealType<T>>, Meta::RealType<T>>;

    namespace internal {
        template <typename T>
        struct FieldExprTrait : ExprTrait<T> {};

        DEFINE_TRAITS_CVR(FieldExpr)

    }// namespace internal
}// namespace OpFlow
#endif//OPFLOW_FIELDEXPRTRAIT_HPP
