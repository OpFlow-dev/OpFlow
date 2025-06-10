//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2025 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_SEMISTRUCTUREDMESH_HPP
#define OPFLOW_SEMISTRUCTUREDMESH_HPP

#include "Core/Mesh/MeshBase.hpp"

OPFLOW_MODULE_EXPORT namespace OpFlow {
    template <typename Derived>
    struct SemiStructuredMesh : MeshBase<Derived> {};
}// namespace OpFlow
#endif//OPFLOW_SEMISTRUCTUREDMESH_HPP
