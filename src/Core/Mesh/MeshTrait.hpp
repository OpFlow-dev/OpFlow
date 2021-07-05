#ifndef OPFLOW_MESHTRAIT_HPP
#define OPFLOW_MESHTRAIT_HPP

#include "Core/Meta.hpp"
#include <type_traits>

namespace OpFlow {
    template <typename Derived>
    class MeshBase;

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
