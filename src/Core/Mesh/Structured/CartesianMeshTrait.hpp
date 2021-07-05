#ifndef OPFLOW_CARTESIANMESHTRAIT_HPP
#define OPFLOW_CARTESIANMESHTRAIT_HPP

#include "Core/Mesh/Structured/StructuredMeshTrait.hpp"

namespace OpFlow {
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
