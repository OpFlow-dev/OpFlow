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

#ifndef OPFLOW_STRUCTUREDFIELDEXPRTRAIT_HPP
#define OPFLOW_STRUCTUREDFIELDEXPRTRAIT_HPP

#include "Core/Field/MeshBased/MeshBasedFieldExprTrait.hpp"
#include "Core/Macros.hpp"
#include "Core/Meta.hpp"

namespace OpFlow {
    template <typename Derived>
    struct StructuredFieldExpr;

    template <typename T>
    concept StructuredFieldExprType
            = std::is_base_of_v<StructuredFieldExpr<Meta::RealType<T>>, Meta::RealType<T>>;

    namespace internal {
        template <typename T>
        struct StructuredFieldExprTrait : MeshBasedFieldExprTrait<T> {};

        DEFINE_TRAITS_CVR(StructuredFieldExpr)
    }// namespace internal
}// namespace OpFlow
#endif//OPFLOW_STRUCTUREDFIELDEXPRTRAIT_HPP
