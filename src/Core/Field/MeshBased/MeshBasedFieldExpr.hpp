#ifndef OPFLOW_MESHBASEDFIELDEXPR_HPP
#define OPFLOW_MESHBASEDFIELDEXPR_HPP

#include "Core/Field/FieldExpr.hpp"
#include "Core/Field/MeshBased/StencilField.hpp"
#include "Core/Macros.hpp"
#include "Core/Constants.hpp"
#include "MeshBasedFieldExprTrait.hpp"

namespace OpFlow {
    template <typename Derived>
    struct MeshBasedFieldExpr : FieldExpr<Derived> {
        using MeshType = typename internal::MeshBasedFieldExprTrait<Derived>::mesh_type;

        MeshType mesh;

        MeshBasedFieldExpr() = default;
        MeshBasedFieldExpr(const MeshBasedFieldExpr& other) : FieldExpr<Derived>(other), mesh(other.mesh) {}
        MeshBasedFieldExpr(MeshBasedFieldExpr&& other) noexcept
            : FieldExpr<Derived>(std::move(other)), mesh(std::move(other.mesh)) {}

        [[maybe_unused]] const auto& getMesh() const { return mesh; }
        void setMesh(const MeshType& m) { mesh = m; }
        auto getStencilField() const { return StencilField<Derived>(this->derived()); }

        template <MeshBasedFieldExprType Other>
        void initPropsFrom(const Other& other) {
            static_cast<FieldExpr<Derived>*>(this)->template initPropsFrom(other);
            this->mesh = other.mesh;
        }

    private:
        DEFINE_CRTP_HELPERS(Derived)
    };
}// namespace OpFlow
#endif//OPFLOW_MESHBASEDFIELDEXPR_HPP
