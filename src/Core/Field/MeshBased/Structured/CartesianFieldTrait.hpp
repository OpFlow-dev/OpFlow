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

#ifndef OPFLOW_CARTESIANFIELDTRAIT_HPP
#define OPFLOW_CARTESIANFIELDTRAIT_HPP

#include "Core/Field/MeshBased/Structured/CartesianFieldExprTrait.hpp"
#include "Core/Meta.hpp"
#include "DataStructures/Arrays/Tensor/PlainTensor.hpp"

namespace OpFlow {
    template <typename D, typename M, typename C>
    struct CartesianField;

    namespace internal {
        template <typename D, typename M, typename C>
        struct ExprTrait<CartesianField<D, M, C>> {
            static constexpr int dim = internal::MeshTrait<M>::dim;
            static constexpr int bc_width = 0;
            using type = CartesianField<D, M, C>;
            template <typename T>
            using other_type
                    = CartesianField<T, M, typename DS::internal::TensorTrait<C>::template other_type<T>>;
            template <typename T>
            using twin_type = CartesianFieldExpr<T>;
            using elem_type = D;
            using mesh_type = M;
            using range_type = DS::Range<dim>;
            using index_type = DS::MDIndex<dim>;
            static constexpr int access_flag = HasDirectAccess | HasWriteAccess;
        };
    }// namespace internal

    template <typename T>
    concept CartesianFieldType = Meta::isTemplateInstance<CartesianField, Meta::RealType<T>>::value;
}// namespace OpFlow
#endif//OPFLOW_CARTESIANFIELDTRAIT_HPP
