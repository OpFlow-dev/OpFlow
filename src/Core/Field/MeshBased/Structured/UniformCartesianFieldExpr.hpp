#ifndef OPFLOW_UNIFORMCARTESIANFIELDEXPR_HPP
#define OPFLOW_UNIFORMCARTESIANFIELDEXPR_HPP

#include "CartesianFieldExpr.hpp"

namespace OpFlow {
    template <typename Derived>
    struct UniformCartesianFieldExpr;

    template <typename T>
    concept UniformCartesianFieldExprType = std::is_base_of_v<UniformCartesianFieldExpr<T>, T>;

    template <UniformCartesianFieldExprType T>
    struct UniformCartesianFieldExprTrait : CartesianFieldExprTrait<T> {};

    DEFINE_TRAITS_CVR(UniformCartesianFieldExpr)

    template <typename Derived>
    struct UniformCartesianFieldExpr : CartesianFieldExpr<Derived> {};
}// namespace OpFlow
#endif//OPFLOW_UNIFORMCARTESIANFIELDEXPR_HPP
