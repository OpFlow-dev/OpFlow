#ifndef OPFLOW_STRUCTUREDMESHBASE_HPP
#define OPFLOW_STRUCTUREDMESHBASE_HPP

#include "Core/Macros.hpp"
#include "Core/Mesh/MeshBase.hpp"

namespace OpFlow {
    template <typename Derived>
    struct StructuredMeshBase : MeshBase<Derived> {};

}// namespace OpFlow
#endif//OPFLOW_STRUCTUREDMESHBASE_HPP
