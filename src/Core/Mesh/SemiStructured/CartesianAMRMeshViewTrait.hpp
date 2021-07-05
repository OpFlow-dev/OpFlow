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
