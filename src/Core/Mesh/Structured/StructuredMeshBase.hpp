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

#ifndef OPFLOW_STRUCTUREDMESHBASE_HPP
#define OPFLOW_STRUCTUREDMESHBASE_HPP

#include "Core/Macros.hpp"
#include "Core/Mesh/MeshBase.hpp"

namespace OpFlow {
    template <typename Derived>
    struct StructuredMeshBase : MeshBase<Derived> {};

}// namespace OpFlow
#endif//OPFLOW_STRUCTUREDMESHBASE_HPP
