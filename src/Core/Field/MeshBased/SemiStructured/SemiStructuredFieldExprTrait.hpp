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

#ifndef OPFLOW_SEMISTRUCTUREDFIELDEXPRTRAIT_HPP
#define OPFLOW_SEMISTRUCTUREDFIELDEXPRTRAIT_HPP

#include "Core/Field/MeshBased/MeshBasedFieldExprTrait.hpp"
#include "Core/Macros.hpp"
#include "Core/Meta.hpp"

namespace OpFlow {
    template <typename Derived>
    struct SemiStructuredFieldExpr;

    template <typename T>
    concept SemiStructuredFieldExprType
            = std::is_base_of_v<SemiStructuredFieldExpr<Meta::RealType<T>>, Meta::RealType<T>>;

    namespace internal {
        template <typename T>
        struct SemiStructuredFieldExprTrait : MeshBasedFieldExprTrait<T> {};

        DEFINE_TRAITS_CVR(SemiStructuredFieldExpr)
    }// namespace internal
}// namespace OpFlow
#endif//OPFLOW_SEMISTRUCTUREDFIELDEXPRTRAIT_HPP
