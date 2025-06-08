//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2023 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_CARTAMRFIELDEXPRTRAIT_HPP
#define OPFLOW_CARTAMRFIELDEXPRTRAIT_HPP

#include "Core/Macros.hpp"
#include "Core/Meta.hpp"
#include "SemiStructuredFieldExprTrait.hpp"

OPFLOW_MODULE_EXPORT namespace OpFlow {
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
