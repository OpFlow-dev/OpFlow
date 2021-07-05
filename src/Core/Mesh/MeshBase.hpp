#ifndef OPFLOW_MESHBASE_HPP
#define OPFLOW_MESHBASE_HPP

#include "Core/Macros.hpp"
#include "Core/Mesh/MeshTrait.hpp"

namespace OpFlow {

    template <typename Derived>
    struct MeshBase {
        template <typename Other>
        bool operator==(const MeshBase<Other>& o) const {
            return false;
        }
        bool operator==(const MeshBase& o) const { return this->derived().operator==(o); }

    private:
        DEFINE_CRTP_HELPERS(Derived)
    };

    template <MeshType T>
    struct MeshBuilder;
}// namespace OpFlow
#endif//OPFLOW_MESHBASE_HPP
