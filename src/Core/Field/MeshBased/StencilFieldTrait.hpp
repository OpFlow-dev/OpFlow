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

#ifndef OPFLOW_STENCILFIELDTRAIT_HPP
#define OPFLOW_STENCILFIELDTRAIT_HPP

#include "Core/Field/MeshBased/MeshBasedFieldExprTrait.hpp"
#include "Core/Field/MeshBased/SemiStructured/SemiStructuredFieldExprTrait.hpp"
#include "Core/Field/MeshBased/Structured/StructuredFieldExprTrait.hpp"
#include "DataStructures/StencilPad.hpp"

namespace OpFlow {
    template <typename T>
    struct StencilField;

    namespace internal {
        template <StructuredFieldExprType T>
        struct ExprTrait<StencilField<T>>
            : ExprTrait<typename StructuredFieldExprTrait<T>::template other_type<
                      DS::StencilPad<typename StructuredFieldExprTrait<T>::index_type>>> {
            static constexpr auto access_flag = 0;
            using type = StencilField<typename StructuredFieldExprTrait<T>::type>;
            using mesh_type
                    = decltype(std::declval<typename StructuredFieldExprTrait<T>::mesh_type&>().getView());
        };

        template <SemiStructuredFieldExprType T>
        struct ExprTrait<StencilField<T>>
            : ExprTrait<typename SemiStructuredFieldExprTrait<T>::template other_type<
                      DS::StencilPad<typename SemiStructuredFieldExprTrait<T>::index_type>>> {
            static constexpr auto access_flag = 0;
            using type = StencilField<typename SemiStructuredFieldExprTrait<T>::type>;
        };
    }// namespace internal
}// namespace OpFlow
#endif//OPFLOW_STENCILFIELDTRAIT_HPP
