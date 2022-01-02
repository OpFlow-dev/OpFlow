// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2022 by the OpFlow developers
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
        template <template <typename, typename> typename map_impl = DS::fake_map>
        auto getStencilField(int color = 0) const {
            return StencilField<Derived, map_impl>(this->derived(), color);
        }

    protected:
        template <MeshBasedFieldExprType Other>
        void initPropsFromImpl_MeshBasedFieldExpr(const Other& other) {
            this->initPropsFromImpl_FieldExpr(other);
            this->mesh = other.mesh;
        }

    private:
        DEFINE_CRTP_HELPERS(Derived)
    };
}// namespace OpFlow
#endif//OPFLOW_MESHBASEDFIELDEXPR_HPP
