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
