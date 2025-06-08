// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2023 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_CARTESIANMESHTRAIT_HPP
#define OPFLOW_CARTESIANMESHTRAIT_HPP

#include "Core/Mesh/Structured/StructuredMeshTrait.hpp"

OPFLOW_MODULE_EXPORT namespace OpFlow {
    template <typename Derived>
    struct CartesianMeshBase;

    namespace internal {
        template <typename Derived>
        struct CartesianMeshTrait : StructuredMeshTrait<Derived> {};
    }// namespace internal

    template <typename T>
    concept CartesianMeshType = std::is_base_of_v<CartesianMeshBase<Meta::RealType<T>>, Meta::RealType<T>>;

    template <typename Dim>
    struct CartesianMesh;

    namespace internal {
        template <typename Dim>
        struct MeshTrait<CartesianMesh<Dim>> {
            static constexpr auto dim = Dim::value;
        };
    }// namespace internal
}// namespace OpFlow
#endif//OPFLOW_CARTESIANMESHTRAIT_HPP
