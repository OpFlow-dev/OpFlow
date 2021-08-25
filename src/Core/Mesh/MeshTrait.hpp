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

#ifndef OPFLOW_MESHTRAIT_HPP
#define OPFLOW_MESHTRAIT_HPP

#include "Core/Meta.hpp"
#include <type_traits>

namespace OpFlow {
    template <typename Derived>
    struct MeshBase;

    namespace internal {
        /// Trait struct of meshes
        /// \tparam M The examined mesh type
        /// \var dim The dim of the space the mesh lives
        template <typename M>
        struct MeshTrait;
    }// namespace internal

    template <typename T>
    concept MeshType = std::is_base_of_v<MeshBase<Meta::RealType<T>>, Meta::RealType<T>>;
}// namespace OpFlow
#endif//OPFLOW_MESHTRAIT_HPP
