// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2026 by the OpFlow developers
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
#include "Math/Function/Integral.hpp"
#ifndef OPFLOW_INSIDE_MODULE
#include <array>
#include <concepts>
#include <cstdarg>
#endif

OPFLOW_MODULE_EXPORT namespace OpFlow {
    enum class MeshExtMode { Undefined, Symm, Periodic, Uniform };

    template <typename Dim>
    struct CartesianMesh : CartesianMeshBase<CartesianMesh<Dim>> {
        static constexpr Size dim = internal::MeshTrait<CartesianMesh>::dim;

        std::array<int, dim> _dims;
        DS::Range<dim> _range, _ext_range;
        std::array<std::vector<OpFlow::Real>, dim> _x;
        std::array<std::vector<OpFlow::Real>, dim> _dx, _idx;

        auto x(int d, int i) const { return _x[d][i - _ext_range.start[d]]; }
        auto x(int d, const DS::MDIndex<dim>& i) const { return _x[d][i[d] - _ext_range.start[d]]; }
        auto dx(int d, int i) const { return _dx[d][i - _ext_range.start[d]]; }
        auto dx(int d, const DS::MDIndex<dim>& i) const { return _dx[d][i[d] - _ext_range.start[d]]; }
        auto idx(int d, int i) const { return _idx[d][i - _ext_range.start[d]]; }
        auto idx(int d, const DS::MDIndex<dim>& i) const { return _idx[d][i[d] - _ext_range.start[d]]; }

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
        const auto& getExtRange() const { return _ext_range; }

        auto operator==(const CartesianMesh& other) const {
            if (this == &other) return true;
            else
                return commonRangeEqualTo(other);
        }
        template <CartesianMeshType Other>
        bool operator==(const Other& other) const {
            return commonRangeEqualTo(other);
        }

        template <typename Other>
        requires(internal::MeshTrait<Other>::dim == dim) auto&
        operator=(const CartesianMeshView<Other>& view) {
            _dims = view.getDims();
            _range = view.getRange();
            _ext_range = view.getExtRange();
            for (auto i = 0; i < dim; ++i) {
                for (auto j = _ext_range.start[i]; j < _ext_range.end[i]; ++j) {
                    _x[i][j - _ext_range.start[i]] = view.x(i, j);
                }
                for (auto j = _ext_range.start[i]; j < _ext_range.end[i] - 1; ++j) {
                    _dx[i][j - _ext_range.start[i]] = view.dx(i, j);
                    _idx[i][j - _ext_range.start[i]] = view.idx(i, j);
                }
            }
            return *this;
        }

    private:
        bool commonRangeEqualTo(const CartesianMesh& other) const {
            bool ret = true;
            for (auto i = 0; i < dim; ++i) {
                auto start = std::max(_ext_range.start[i], other._ext_range.start[i]);
                auto end = std::min(_ext_range.end[i], other._ext_range.end[i]);
                for (auto j = start; j < end; ++j) {
                    ret &= _x[i][j - _ext_range.start[i]] == other._x[i][j - other._ext_range.start[i]];
                    if (!ret) return false;
                }
            }
            return true;
        }
        template <CartesianMeshType Other>
        bool commonRangeEqualTo(const Other& other) const {
            bool ret = true;
            for (auto i = 0; i < dim; ++i) {
                auto start = std::max(_ext_range.start[i], other.getExtRange().start[i]);
                auto end = std::min(_ext_range.end[i], other.getExtRange().end[i]);
                for (auto j = start; j < end; ++j) {
                    ret &= _x[i][j - _ext_range.start[i]] == other.x(i, j);
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
        int padding_width = 5;
        std::array<int, dim> start;
        std::array<MeshExtMode, dim> ext_mode;

    public:
        auto& newMesh(Index dim1, ...) {
            va_list ap;
            va_start(ap, dim1);
            mesh._dims[0] = dim1;
            for (Index i = 1; i < dim; ++i) { mesh._dims[i] = va_arg(ap, Index); }
            va_end(ap);
            return *this;
        }

        template <Meta::BracketIndexable Array>
        auto& newMesh(const Array& d) {
            for (Index i = 0; i < dim; ++i) { mesh._dims[i] = d[i]; }
            return *this;
        }

        auto& newMesh(const CartesianMesh<Dim>& m) {
            mesh = m;
            return *this;
        }

        auto& setPadWidth(int w) {
            padding_width = w;
            return *this;
        }

        auto& setStart(auto&& s) {
            start = s;
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

        auto& setExtMode(MeshExtMode m) {
            for (auto& mode : ext_mode) { mode = m; }
            return *this;
        }

        auto& setExtModeOfDim(Index k, MeshExtMode m) {
            ext_mode[k] = m;
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
        void set1DRange(Index k) {
            mesh._range.start[k] = start[k];
            mesh._range.end[k] = start[k] + mesh._dims[k];
            mesh._ext_range.start[k] = mesh._range.start[k] - padding_width;
            mesh._ext_range.end[k] = mesh._range.end[k] + padding_width;
            mesh._x[k].resize(mesh._ext_range.end[k] - mesh._ext_range.start[k]);
            mesh._dx[k].resize(mesh._ext_range.end[k] - mesh._ext_range.start[k] - 1);
            mesh._idx[k].resize(mesh._ext_range.end[k] - mesh._ext_range.start[k] - 1);
        }

        void setExtMesh(Index k) {
            // ext mesh
            switch (ext_mode[k]) {
                    // default mode is Symm
                case MeshExtMode::Undefined:
                case MeshExtMode::Symm:
                    for (Index i = mesh._ext_range.start[k]; i < mesh._range.start[k]; ++i) {
                        mesh._dx[k][i - mesh._ext_range.start[k]]
                                = mesh._dx[k][2 * mesh._range.start[k] - 1 - i - mesh._ext_range.start[k]];
                        mesh._idx[k][i - mesh._ext_range.start[k]]
                                = 1. / mesh._dx[k][i - mesh._ext_range.start[k]];
                    }
                    for (Index i = mesh._range.end[k] - 1; i < mesh._ext_range.end[k] - 1; ++i) {
                        mesh._dx[k][i - mesh._ext_range.start[k]]
                                = mesh._dx[k][2 * mesh._range.end[k] - 3 - i - mesh._ext_range.start[k]];
                        mesh._idx[k][i - mesh._ext_range.start[k]]
                                = 1. / mesh._dx[k][i - mesh._ext_range.start[k]];
                    }
                    break;
                case MeshExtMode::Periodic:
                    for (Index i = mesh._ext_range.start[k]; i < mesh._range.start[k]; ++i) {
                        mesh._dx[k][i - mesh._ext_range.start[k]]
                                = mesh._dx[k][mesh._range.end[k] - (mesh._range.start[k] - i)
                                              - mesh._ext_range.start[k]];
                        mesh._idx[k][i - mesh._ext_range.start[k]]
                                = 1. / mesh._dx[k][i - mesh._ext_range.start[k]];
                    }
                    for (Index i = mesh._range.end[k] - 1; i < mesh._ext_range.end[k] - 1; ++i) {
                        mesh._dx[k][i - mesh._ext_range.start[k]]
                                = mesh._dx[k][mesh._range.start[k] + i - mesh._range.end[k] + 1
                                              - mesh._ext_range.start[k]];
                        mesh._idx[k][i - mesh._ext_range.start[k]]
                                = 1. / mesh._dx[k][i - mesh._ext_range.start[k]];
                    }
                    break;
                case MeshExtMode::Uniform:
                    for (Index i = mesh._ext_range.start[k]; i < mesh._range.start[k]; ++i) {
                        mesh._dx[k][i - mesh._ext_range.start[k]]
                                = mesh._dx[k][mesh._range.start[k] - mesh._ext_range.start[k]];
                        mesh._idx[k][i - mesh._ext_range.start[k]]
                                = 1. / mesh._dx[k][i - mesh._ext_range.start[k]];
                    }
                    for (Index i = mesh._range.end[k] - 1; i < mesh._ext_range.end[k] - 1; ++i) {
                        mesh._dx[k][i - mesh._ext_range.start[k]]
                                = mesh._dx[k][mesh._range.end[k] - 2 - mesh._ext_range.start[k]];
                        mesh._idx[k][i - mesh._ext_range.start[k]]
                                = 1. / mesh._idx[k][i - mesh._ext_range.start[k]];
                    }
            }
            // integrate x
            for (Index i = mesh._range.start[k] - 1; i >= mesh._ext_range.start[k]; --i) {
                mesh._x[k][i - mesh._ext_range.start[k]] = mesh._x[k][i + 1 - mesh._ext_range.start[k]]
                                                           - mesh._dx[k][i - mesh._ext_range.start[k]];
            }
            for (Index i = mesh._range.end[k]; i < mesh._ext_range.end[k]; ++i) {
                mesh._x[k][i - mesh._ext_range.start[k]] = mesh._x[k][i - 1 - mesh._ext_range.start[k]]
                                                           + mesh._dx[k][i - 1 - mesh._ext_range.start[k]];
            }
        }

        void set1DMesh(const MeshSetter& f, Index k) {
            set1DRange(k);
            for (Index i = mesh._range.start[k]; i < mesh._range.end[k]; ++i) {
                mesh._x[k][i - mesh._ext_range.start[k]] = f(i);
            }
            for (Index j = mesh._range.start[k]; j < mesh._range.end[k] - 1; ++j) {
                mesh._dx[k][j - mesh._ext_range.start[k]] = (mesh._x[k][j + 1 - mesh._ext_range.start[k]]
                                                             - mesh._x[k][j - mesh._ext_range.start[k]]);
                mesh._idx[k][j - mesh._ext_range.start[k]] = 1. / mesh._dx[k][j - mesh._ext_range.start[k]];
            }
            setExtMesh(k);
        }

        void set1DMesh(Real min, Real max, Index k) {
            set1DRange(k);
            for (Index i = mesh._range.start[k]; i < mesh._range.end[k]; ++i) {
                mesh._x[k][i - mesh._ext_range.start[k]]
                        = (max - min) / (mesh._dims[k] - 1) * (i - mesh._range.start[k]) + min;
            }
            for (Index j = mesh._range.start[k]; j < mesh._range.end[k] - 1; ++j) {
                mesh._dx[k][j - mesh._ext_range.start[k]] = (max - min) / (mesh._dims[k] - 1);
                mesh._idx[k][j - mesh._ext_range.start[k]] = 1. / mesh._dx[k][j - mesh._ext_range.start[k]];
            }
            setExtMesh(k);
        }
    };
}// namespace OpFlow
#endif//OPFLOW_CARTESIANMESH_HPP
