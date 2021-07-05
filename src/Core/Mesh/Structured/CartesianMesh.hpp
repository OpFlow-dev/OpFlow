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

#ifndef OPFLOW_CARTESIANMESH_HPP
#define OPFLOW_CARTESIANMESH_HPP

#include "Core/BasicDataTypes.hpp"
#include "Core/Mesh/Structured/CartesianMeshBase.hpp"
#include "Core/Mesh/Structured/CartesianMeshTrait.hpp"
#include "Core/Mesh/Structured/CartesianMeshView.hpp"
#include "Core/Meta.hpp"
#include "DataStructures/Arrays/CoordVector.hpp"
#include "DataStructures/Arrays/OffsetVector.hpp"
#include "DataStructures/Pair.hpp"
#include "DataStructures/Range/Ranges.hpp"
#include <array>
#include <concepts>
#include <cstdarg>

namespace OpFlow {
    template <typename Dim>
    struct CartesianMesh : CartesianMeshBase<CartesianMesh<Dim>> {
        static constexpr Size dim = internal::MeshTrait<CartesianMesh>::dim;

        std::array<int, dim> _dims;
        DS::Range<dim> _range;
        std::array<std::vector<OpFlow::Real>, dim> _x;
        std::array<std::vector<OpFlow::Real>, dim> _dx, _idx;

        auto x(int d, int i) const { return _x[d][i - _range.start[d]]; }
        auto x(int d, const DS::MDIndex<dim>& i) const { return _x[d][i[d] - _range.start[d]]; }
        auto dx(int d, int i) const { return _dx[d][i - _range.start[d]]; }
        auto dx(int d, const DS::MDIndex<dim>& i) const { return _dx[d][i[d] - _range.start[d]]; }
        auto idx(int d, int i) const { return _idx[d][i - _range.start[d]]; }
        auto idx(int d, const DS::MDIndex<dim>& i) const { return _idx[d][i[d] - _range.start[d]]; }

        auto getPtr() const { return this; }

        auto getPtr() { return this; }

        auto getView() const { return CartesianMeshView<CartesianMesh>(*this); }

        const auto& getDims() const { return _dims; }
        auto getDimOf(int i) const { return _dims[i]; }
        auto getStart() const { return DS::MDIndex<dim>(_range.start); }
        auto getStartOf(int i) const { return _range.start[i]; }
        auto getEnd() const { return DS::MDIndex<dim>(_range.end); }
        auto getEndOf(int i) const { return _range.end[i]; }
        const auto& getRange() const { return _range; }

        auto operator==(const CartesianMesh& other) const {
            if (this == &other) return true;
            else
                return commonRangeEqualTo(other);
        }
        template <CartesianMeshType Other>
        bool operator==(const Other& other) const {
            return commonRangeEqualTo(other);
        }

    private:
        bool commonRangeEqualTo(const CartesianMesh& other) const {
            bool ret = true;
            for (auto i = 0; i < dim; ++i) {
                auto start = std::max(_range.start[i], other._range.start[i]);
                auto end = std::min(_range.end[i], other._range.end[i]);
                for (auto j = start; j < end; ++j) {
                    ret &= _x[i][j] == other._x[i][j];
                    if (!ret) return false;
                }
            }
            return true;
        }
        template <CartesianMeshType Other>
        bool commonRangeEqualTo(const Other& other) const {
            bool ret = true;
            for (auto i = 0; i < dim; ++i) {
                auto start = std::max(_range.start[i], other.getRange().start[i]);
                auto end = std::min(_range.end[i], other.getRange().end[i]);
                for (auto j = start; j < end; ++j) {
                    ret &= _x[i][j] == other.x(i, j);
                    if (!ret) return false;
                }
            }
            return true;
        }
    };

    template <typename Dim>
    struct MeshBuilder<CartesianMesh<Dim>> {
    private:
        static constexpr auto dim = internal::MeshTrait<CartesianMesh<Dim>>::dim;
        CartesianMesh<Dim> mesh;

    public:
        auto& newMesh(Index dim1, ...) {
            va_list ap;
            va_start(ap, dim1);
            mesh._dims[0] = dim1;
            mesh._x[0].resize(mesh._dims[0]);
            mesh._dx[0].resize(mesh._dims[0] - 1);
            mesh._idx[0].resize(mesh._dims[0] - 1);
            for (Index i = 1; i < dim; ++i) {
                mesh._dims[i] = va_arg(ap, Index);
                mesh._x[i].resize(mesh._dims[i]);
                mesh._dx[i].resize(mesh._dims[i] - 1);
                mesh._idx[i].resize(mesh._dims[i] - 1);
            }
            va_end(ap);
            for (auto i = 0; i < dim; ++i) {
                mesh._range.start[i] = 0;
                mesh._range.end[i] = mesh._dims[i];
            }
            return *this;
        }

        template <Meta::BracketIndexable Array>
        auto& newMesh(const Array& d) {
            for (Index i = 0; i < dim; ++i) {
                mesh._dims[i] = d[i];
                mesh._x[i].resize(mesh._dims[i]);
                mesh._dx[i].resize(mesh._dims[i] - 1);
                mesh._idx[i].resize(mesh._dims[i] - 1);
            }
            for (auto i = 0; i < dim; ++i) {
                mesh._range.start[i] = 0;
                mesh._range.end[i] = mesh._dims[i];
            }
            return *this;
        }

        auto& newMesh(const CartesianMesh<Dim>& m) {
            mesh = m;
            return *this;
        }

        auto& resize1D(int d, int size) {
            mesh._dims[d] = size;
            mesh._x[d].resize(size);
            mesh._dx[d].resize(size);
            mesh._idx[d].resize(size);
            mesh._range.end[d] = mesh._range.start[d] + size;
            return *this;
        }

        auto& setStart(auto&& start) {
            for (auto i = 0; i < dim; ++i) {
                mesh._range.end[i] += start[i] - mesh._range.start[i];
                mesh._range.start[i] = start[i];
            }
            return *this;
        }

        using MeshSetter = std::function<Real(Index)>;

        auto& setMeshOfDim(Index k, const MeshSetter& f) {
            set1DMesh(f, k);
            return *this;
        }

        auto& setMeshOfDim(Index k, Real min, Real max) {
            set1DMesh(min, max, k);
            return *this;
        }

        auto& refine(int ratio) {
            for (auto i = 0; i < dim; ++i) {
                std::vector<Real> x(ratio * (mesh._dims[i] - 1) + 1);
                x[0] = mesh._x[i][0];
                for (auto j = 0; j < mesh._dims[i] - 1; ++j) {
                    for (auto k = 1; k <= ratio; ++k)
                        x[ratio * j + k] = mesh._x[i][j] + mesh._dx[i][j] / ratio * k;
                }
                mesh._dims[i] = x.size();
                mesh._x[i] = x;
                mesh._dx[i].resize(x.size() - 1);
                mesh._idx[i].resize(x.size() - 1);
                for (auto j = 0; j < mesh._dims[i] - 1; ++j) {
                    mesh._dx[i][j] = mesh._x[i][j + 1] - mesh._x[i][j];
                    mesh._idx[i][j] = 1. / mesh._dx[i][j];
                }
                mesh._range.start[i] *= ratio;
                mesh._range.end[i] = (mesh._range.end[i] - 1) * ratio + 1;
            }
            return *this;
        }

        auto&& build() { return std::move(mesh); }

    private:
        void set1DMesh(const MeshSetter& f, Index k) {
            for (Index i = 0; i < mesh._dims[k]; ++i) { mesh._x[k][i] = f(i + mesh._range.start[i]); }
            for (Index j = 0; j < mesh._dims[k] - 1; ++j) {
                mesh._dx[k][j] = (mesh._x[k][j + 1] - mesh._x[k][j]);
                mesh._idx[k][j] = 1. / mesh._dx[k][j];
            }
        }

        void set1DMesh(Real min, Real max, Index k) {
            for (Index i = 0; i < mesh._dims[k]; ++i) {
                mesh._x[k][i] = (max - min) / (mesh._dims[k] - 1) * i + min;
            }
            for (Index j = 0; j < mesh._dims[k] - 1; ++j) {
                mesh._dx[k][j] = (max - min) / (mesh._dims[k] - 1);
                mesh._idx[k][j] = 1. / mesh._dx[k][j];
            }
        }
    };
}// namespace OpFlow
#endif//OPFLOW_CARTESIANMESH_HPP
