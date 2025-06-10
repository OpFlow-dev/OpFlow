//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2025 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_CARTESIANAMRMESHVIEW_HPP
#define OPFLOW_CARTESIANAMRMESHVIEW_HPP

#include "Core/Mesh/SemiStructured/CartesianAMRMeshBase.hpp"
#include "Core/Mesh/SemiStructured/CartesianAMRMeshTrait.hpp"
#include "DataStructures/Index/LevelMDIndex.hpp"
#ifndef OPFLOW_INSIDE_MODULE
#include <memory>
#endif

OPFLOW_MODULE_EXPORT namespace OpFlow {
    template <typename T>
    requires CartesianAMRMeshType<T>&&
            Meta::isTemplateInstance<CartesianAMRMesh, T>::value struct CartesianAMRMeshView
        : CartesianAMRMeshBase<CartesianAMRMeshView<T>> {
    private:
        std::add_pointer_t<typename std::add_const<T>::type> base = nullptr;
        static constexpr auto dim = internal::MeshTrait<T>::dim;

    public:
        int refinementRatio = -1;
        CartesianAMRMeshView() = default;
        explicit CartesianAMRMeshView(const T& mesh)
            : base(mesh.getPtr()), refinementRatio(mesh.refinementRatio) {}

        CartesianAMRMeshView& operator=(const CartesianAMRMeshView&) = default;
        auto& operator=(const T& mesh) {
            base = mesh.getPtr();
            refinementRatio = mesh.refinementRatio;
            return *this;
        }

        auto getView() const { return CartesianAMRMeshView(*this); }

        auto getRanges() const -> decltype(auto) { return base->getRanges(); }

        auto x(int d, int l, int i) const { return base->x(d, l, i); }
        auto x(int d, const DS::LevelMDIndex<dim>& i) const { return base->x(d, i); }
        auto dx(int d, int l, int i) const { return base->dx(d, l, i); }
        auto dx(int d, const DS::LevelMDIndex<dim>& i) const { return base->dx(d, i); }
        auto idx(int d, int l, int i) const { return base->idx(d, l, i); }
        auto idx(int d, const DS::LevelMDIndex<dim>& i) const { return base->idx(d, i); }

        bool operator==(const CartesianAMRMeshView& other) const {
            return base == other.base || *base == *other.base;
        }
        template <CartesianAMRMeshType Other>
        bool operator==(const Other& other) const {
            return operator==(other.getView());
        }
    };
}// namespace OpFlow
#endif//OPFLOW_CARTESIANAMRMESHVIEW_HPP
