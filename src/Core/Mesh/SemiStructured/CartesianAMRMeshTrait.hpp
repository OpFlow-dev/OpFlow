#ifndef OPFLOW_CARTESIANAMRMESHTRAIT_HPP
#define OPFLOW_CARTESIANAMRMESHTRAIT_HPP

#include "Core/BasicDataTypes.hpp"
#include "Core/Mesh/MeshTrait.hpp"
#include "Core/Mesh/SemiStructured/SemiStructuredMeshTrait.hpp"

namespace OpFlow {
    template <typename Derived>
    struct CartesianAMRMeshBase;

    namespace internal {
        template <typename Derived>
        struct CartesianAMRMeshTrait : SemiStructuredMeshTrait<Derived> {};
    }// namespace internal

    template <typename T>
    concept CartesianAMRMeshType
            = std::is_base_of_v<CartesianAMRMeshBase<Meta::RealType<T>>, Meta::RealType<T>>;

    template <typename Dim>
    struct CartesianAMRMesh;

    namespace internal {
        template <typename Dim>
        struct MeshTrait<CartesianAMRMesh<Dim>> {
            static constexpr Size dim = Dim::value;
        };
    }// namespace internal
}// namespace OpFlow
#endif//OPFLOW_CARTESIANAMRMESHTRAIT_HPP
