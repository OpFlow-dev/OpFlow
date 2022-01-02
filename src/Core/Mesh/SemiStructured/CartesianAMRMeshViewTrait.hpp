//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2021 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_CARTESIANAMRMESHVIEWTRAIT_HPP
#define OPFLOW_CARTESIANAMRMESHVIEWTRAIT_HPP

#include "Core/Mesh/SemiStructured/SemiStructuredMeshTrait.hpp"

namespace OpFlow {
    template <typename T>
    requires CartesianAMRMeshType<T>&&
            Meta::isTemplateInstance<CartesianAMRMesh, T>::value struct CartesianAMRMeshView;

    namespace internal {
        template <typename T>
        struct MeshTrait<CartesianAMRMeshView<T>> {
            static constexpr auto dim = MeshTrait<T>::dim;
        };
    }// namespace internal
}// namespace OpFlow
#endif//OPFLOW_CARTESIANAMRMESHVIEWTRAIT_HPP
