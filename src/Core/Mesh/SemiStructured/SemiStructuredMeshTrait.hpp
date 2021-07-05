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
