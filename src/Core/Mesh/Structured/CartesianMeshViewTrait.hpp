#ifndef OPFLOW_CARTESIANMESHVIEWTRAIT_HPP
#define OPFLOW_CARTESIANMESHVIEWTRAIT_HPP

#include "Core/Mesh/Structured/StructuredMeshTrait.hpp"

namespace OpFlow {
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
