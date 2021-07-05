#ifndef OPFLOW_STRUCTUREDMESHTRAIT_HPP
#define OPFLOW_STRUCTUREDMESHTRAIT_HPP

#include "Core/Mesh/MeshTrait.hpp"
#include "Core/Meta.hpp"
#include <type_traits>

namespace OpFlow {
    template <typename Derived>
    struct StructuredMeshBase;

    namespace internal {
        template <typename Derived>
        struct StructuredMeshTrait : MeshTrait<Derived> {};
    }// namespace internal

    template <typename T>
    concept StructuredMeshType = std::is_base_of_v<StructuredMeshBase<Meta::RealType<T>>, Meta::RealType<T>>;
}// namespace OpFlow
#endif//OPFLOW_STRUCTUREDMESHTRAIT_HPP
