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

#ifndef OPFLOW_SEMISTRUCTUREDMESHTRAIT_HPP
#define OPFLOW_SEMISTRUCTUREDMESHTRAIT_HPP

#include "Core/Mesh/MeshTrait.hpp"

namespace OpFlow {
    template <typename Derived>
    struct SemiStructuredMesh;

    namespace internal {
        template <typename Derived>
        struct SemiStructuredMeshTrait : MeshTrait<Derived> {};
    }// namespace internal

    template <typename T>
    concept SemiStructuredMeshType
            = std::is_base_of_v<SemiStructuredMesh<Meta::RealType<T>>, Meta::RealType<T>>;
}// namespace OpFlow
#endif//OPFLOW_SEMISTRUCTUREDMESHTRAIT_HPP
