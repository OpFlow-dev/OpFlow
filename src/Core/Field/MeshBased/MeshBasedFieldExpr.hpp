// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2021 by the OpFlow developers
//
// This file is part of OpFlow.
// 
// OpFlow is free software and is distributed under the MPL v2.0 license. 
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_MESHBASEDFIELDEXPR_HPP
#define OPFLOW_MESHBASEDFIELDEXPR_HPP

#include "Core/Constants.hpp"
#include "Core/Field/FieldExpr.hpp"
#include "Core/Field/MeshBased/StencilField.hpp"
#include "Core/Macros.hpp"
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
