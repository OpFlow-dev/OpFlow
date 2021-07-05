#ifndef OPFLOW_SEMISTRUCTUREDMESH_HPP
#define OPFLOW_SEMISTRUCTUREDMESH_HPP

#include "Core/Mesh/MeshBase.hpp"

namespace OpFlow {
    template <typename Derived>
    struct SemiStructuredMesh : MeshBase<Derived> {};
}// namespace OpFlow
#endif//OPFLOW_SEMISTRUCTUREDMESH_HPP
