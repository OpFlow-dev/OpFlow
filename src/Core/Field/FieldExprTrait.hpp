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
