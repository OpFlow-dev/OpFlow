// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2023 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_CARTESIANMESHVIEW_HPP
#define OPFLOW_CARTESIANMESHVIEW_HPP

#include "Core/Mesh/Structured/CartesianMeshBase.hpp"
#include "Core/Mesh/Structured/CartesianMeshTrait.hpp"
#ifndef OPFLOW_INSIDE_MODULE
#include <memory>
#endif

OPFLOW_MODULE_EXPORT namespace OpFlow {
    template <typename T>
        requires CartesianMeshType<T> && Meta::isTemplateInstance<CartesianMesh, T>::value
    struct CartesianMeshView : CartesianMeshBase<CartesianMeshView<T>> {
    private:
        std::add_pointer_t<typename std::add_const<T>::type> base = nullptr;
        static constexpr auto dim = internal::MeshTrait<T>::dim;
        DS::Range<dim> range, ext_range;
        std::array<int, dim> dims;
        DS::MDIndex<dim> offset {0};

    public:
        CartesianMeshView() = default;
        explicit CartesianMeshView(const T& mesh)
            : base(mesh.getPtr()), range(mesh.getRange()), dims(mesh.getDims()) {}

        CartesianMeshView& operator=(const CartesianMeshView&) = default;
        auto& operator=(const T& mesh) {
            base = mesh.getPtr();
            range = mesh.getRange();
            ext_range = mesh.getExtRange();
            dims = mesh.getDims();
            return *this;
        }

        auto getView() const { return CartesianMeshView(*this); }

        auto getDims() const -> decltype(auto) { return dims; }
        auto getDimOf(int i) const { return dims[i]; }
        auto getStart() const -> decltype(auto) { return DS::MDIndex<dim>(range.start); }
        auto getStartOf(int i) const { return range.start[i]; }
        auto getEnd() const { return DS::MDIndex<dim> {range.end}; }
        auto getEndOf(int i) const { return range.end[i]; }
        auto getRange() const -> decltype(auto) { return range; }
        auto getExtRange() const -> decltype(auto) { return ext_range; }
        void setRange(const DS::Range<dim>& r) {
            range = r;
            for (auto i = 0; i < dim; ++i) dims[i] = range.end[i] - range.start[i];
        }
        void setOffset(const DS::MDIndex<dim>& off) {
            offset = off;
            for (auto i = 0; i < dim; ++i) {
                range.end[i] += off[i];
                range.start[i] += off[i];
            }
        }
        void appendOffset(const DS::MDIndex<dim>& off) {
            offset += off;
            for (auto i = 0; i < dim; ++i) {
                range.end[i] += off[i];
                range.start[i] += off[i];
            }
        }
        void appendOffsetOf(int i, int off) {
            offset[i] += off;
            range.end[i] += off;
            range.start[i] += off;
        }

        auto x(int d, int i) const { return base->x(d, i - offset[d]); }
        auto x(int d, const DS::MDIndex<dim>& i) const { return x(d, i[d]); }
        auto dx(int d, int i) const { return base->dx(d, i - offset[d]); }
        auto dx(int d, const DS::MDIndex<dim>& i) const { return dx(d, i[d]); }
        auto idx(int d, int i) const { return base->idx(d, i - offset[d]); }
        auto idx(int d, const DS::MDIndex<dim>& i) const { return idx(d, i[d]); }

        bool operator==(const CartesianMeshView& other) const {
            if (base == other.base) return true;
            else
                return commonRangeEqual(other);
        }
        template <CartesianMeshType Other>
        bool operator==(const Other& other) const {
            return commonRangeEqual(other);
        }

    private:
        template <CartesianMeshType Other>
        bool commonRangeEqual(const Other& other) const {
            bool ret = true;
            for (auto i = 0; i < dim; ++i) {
                auto start = std::max(range.start[i], other.getStartOf(i));
                auto end = std::min(range.end[i], other.getEndOf(i));
                for (auto j = start; j < end; ++j) {
                    ret &= x(i, j) == other.x(i, j);
                    if (!ret) return false;
                }
            }
            return true;
        }
    };
}// namespace OpFlow
#endif//OPFLOW_CARTESIANMESHVIEW_HPP
