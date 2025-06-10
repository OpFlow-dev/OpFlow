// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2025 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_CARTESIANMESHVIEWTRAIT_HPP
#define OPFLOW_CARTESIANMESHVIEWTRAIT_HPP

#include "Core/Mesh/Structured/StructuredMeshTrait.hpp"

OPFLOW_MODULE_EXPORT namespace OpFlow {
    template <typename T>
    requires CartesianMeshType<T>&&
            Meta::isTemplateInstance<CartesianMesh, T>::value struct CartesianMeshView;

    namespace internal {
        template <typename T>
        struct MeshTrait<CartesianMeshView<T>> {
            static constexpr auto dim = MeshTrait<T>::dim;
        };
    }// namespace internal
}// namespace OpFlow
#endif//OPFLOW_CARTESIANMESHVIEWTRAIT_HPP
