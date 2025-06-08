//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2023 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_CARTAMRFIELD_HPP
#define OPFLOW_CARTAMRFIELD_HPP

#include "Core/BC/DircBC.hpp"
#include "Core/BC/LogicalBC.hpp"
#include "Core/BC/NeumBC.hpp"
#include "Core/Field/MeshBased/SemiStructured/CartAMRFieldExpr.hpp"
#include "Core/Field/MeshBased/SemiStructured/CartAMRFieldExprTrait.hpp"
#include "Core/Field/MeshBased/SemiStructured/CartAMRFieldTrait.hpp"
#include "Core/Loops/FieldAssigner.hpp"
#include "Core/Macros.hpp"
#include "Core/Mesh/SemiStructured/CartesianAMRMesh.hpp"
#include "Core/Meta.hpp"
#include "DataStructures/Arrays/Tensor/PlainTensor.hpp"

OPFLOW_MODULE_EXPORT namespace OpFlow {

    /// The Cartesian AMR field type
    /// \warning This is a special impl version. It relies on the sparse storage trait of the data container
    /// \tparam D The element type
    /// \tparam M The AMR mesh type
    /// \tparam C The data container unit
    template <typename D, typename M, typename C = DS::PlainTensor<D, internal::MeshTrait<M>::dim>>
    struct CartAMRField : CartAMRFieldExpr<CartAMRField<D, M, C>> {
        using index_type = typename internal::CartAMRFieldExprTrait<CartAMRField>::index_type;

    private:
        constexpr static auto dim = internal::CartAMRFieldExprTrait<CartAMRField>::dim;
        std::vector<std::vector<C>> data;
        std::vector<std::vector<index_type>> offset;

    public:
        friend ExprBuilder<CartAMRField>;
        friend Expr<CartAMRField>;
        friend SemiStructuredFieldExpr<CartAMRField>;
        using Expr<CartAMRField>::operator[];
        using Expr<CartAMRField>::operator();
        using Expr<CartAMRField>::operator=;

        CartAMRField() = default;
        CartAMRField(const CartAMRField& other)
            : CartAMRFieldExpr<CartAMRField<D, M, C>>(other), data(other.data), offset(other.offset) {}
        CartAMRField(CartAMRField&& other) noexcept
            : CartAMRFieldExpr<CartAMRField<D, M, C>>(std::move(other)), data(std::move(other.data)),
              offset(std::move(other.offset)) {}

        CartAMRField& operator=(const CartAMRField& other) {
            assignImpl_final(other);
            return *this;
        }

        CartAMRField& operator=(CartAMRField&& other) noexcept {
            assignImpl_final(std::move(other));
            return *this;
        }

        template <BasicArithOp Op = BasicArithOp::Eq>
        auto& assignImpl_final(const CartAMRField& other) {
            if (this != &other) {
                // only data is assigned
                internal::FieldAssigner::assign<Op>(other, *this);
                updateCovering();
                updatePadding();
            }
            return *this;
        }

        template <BasicArithOp Op = BasicArithOp::Eq, CartAMRFieldExprType T>
        auto& assignImpl_final(T&& other) {
            if ((void*) this != (void*) &other) {
                internal::FieldAssigner::assign<Op>(other, *this);
                updateCovering();
                updatePadding();
            }
            return *this;
        }

        template <BasicArithOp Op = BasicArithOp::Eq>
        auto& assignImpl_final(const D& c) {
            auto levels = data.size();
#pragma omp parallel
            for (auto l = 0; l < levels; ++l) {
                auto parts = data[l].size();
#pragma omp for schedule(dynamic) nowait
                for (auto p = 0; p < parts; ++p) {
                    if constexpr (Op == BasicArithOp::Eq)
                        rangeFor_s(this->assignableRanges[l][p], [&](auto&& i) { this->operator[](i) = c; });
                    else if constexpr (Op == BasicArithOp::Add)
                        rangeFor_s(this->assignableRanges[l][p], [&](auto&& i) { this->operator[](i) += c; });
                    else if constexpr (Op == BasicArithOp::Minus)
                        rangeFor_s(this->assignableRanges[l][p], [&](auto&& i) { this->operator[](i) -= c; });
                    else if constexpr (Op == BasicArithOp::Mul)
                        rangeFor_s(this->assignableRanges[l][p], [&](auto&& i) { this->operator[](i) *= c; });
                    else if constexpr (Op == BasicArithOp::Div)
                        rangeFor_s(this->assignableRanges[l][p], [&](auto&& i) { this->operator[](i) /= c; });
                    else if constexpr (Op == BasicArithOp::Mod)
                        rangeFor_s(this->assignableRanges[l][p], [&](auto&& i) { this->operator[](i) %= c; });
                    else if constexpr (Op == BasicArithOp::And)
                        rangeFor_s(this->assignableRanges[l][p], [&](auto&& i) { this->operator[](i) &= c; });
                    else if constexpr (Op == BasicArithOp::Or)
                        rangeFor_s(this->assignableRanges[l][p], [&](auto&& i) { this->operator[](i) |= c; });
                    else if constexpr (Op == BasicArithOp::Xor)
                        rangeFor_s(this->assignableRanges[l][p], [&](auto&& i) { this->operator[](i) ^= c; });
                    else if constexpr (Op == BasicArithOp::LShift)
                        rangeFor_s(this->assignableRanges[l][p],
                                   [&](auto&& i) { this->operator[](i) <<= c; });
                    else if constexpr (Op == BasicArithOp::RShift)
                        rangeFor_s(this->assignableRanges[l][p],
                                   [&](auto&& i) { this->operator[](i) >>= c; });
                    else
                        OP_NOT_IMPLEMENTED;
                }
            }
            updateCovering();
            updatePadding();
            return *this;
        }

        template <typename F>
        requires requires(F f) {
            { f(std::declval<std::array<Real, internal::CartesianAMRMeshTrait<M>::dim>>()) }
            ->std::convertible_to<D>;
        }
        auto& initBy(F&& f) {
            auto levels = data.size();
#pragma omp parallel
            for (auto l = 0; l < levels; ++l) {
                auto parts = data[l].size();
#pragma omp for schedule(dynamic) nowait
                for (auto p = 0; p < parts; ++p) {
                    rangeFor_s(this->assignableRanges[l][p], [&](auto&& i) {
                        std::array<Real, internal::CartesianAMRMeshTrait<M>::dim> cords;
                        for (auto k = 0; k < internal::CartesianAMRMeshTrait<M>::dim; ++k)
                            cords[k]
                                    = this->loc[k] == LocOnMesh::Corner
                                              ? this->mesh.x(k, i.l, i[k])
                                              : this->mesh.x(k, i.l, i[k]) + .5 * this->mesh.dx(k, i.l, i[k]);
                        this->operator[](i) = f(cords);
                    });
                }
            }
            updateCovering();
            updatePadding();
            return *this;
        }

        auto& initBy(Meta::Numerical auto v) { return *this = v; }

    protected:
        void prepareImpl_final() const {}
        void updateBCImpl_final() {
            if (this->localRanges[0][0] == this->assignableRanges[0][0]) return;
            else {
                for (auto i = 0; i < dim; ++i) {
                    if (this->loc[i] == LocOnMesh::Center) continue;// only nodal dims needs to be update
                    auto type = this->bc[i].start ? this->bc[i].start->getBCType() : BCType::Undefined;
                    switch (type) {
                        case BCType::Dirc:
                            rangeFor(this->accessibleRanges[0][0].slice(
                                             i, this->accessibleRanges[0][0].start[i]),
                                     [&](auto&& pos) {
                                         data[pos.l][pos.p][pos - offset[pos.l][pos.p]]
                                                 = this->bc[i].start->evalAt(pos);
                                     });
                            break;
                        case BCType::Neum:
                        case BCType::Undefined:
                            break;
                        default:
                            OP_NOT_IMPLEMENTED;
                    }

                    type = this->bc[i].end ? this->bc[i].end->getBCType() : BCType::Undefined;
                    switch (type) {
                        case BCType::Dirc:
                            rangeFor(this->accessibleRanges[0][0].slice(i, this->accessibleRanges[0][0].end[i]
                                                                                   - 1),
                                     [&](auto&& pos) {
                                         data[pos.l][pos.p][pos - offset[pos.l][pos.p]]
                                                 = this->bc[i].end->evalAt(pos);
                                     });
                            break;
                        case BCType::Neum:
                        case BCType::Undefined:
                            break;
                        default:
                            OP_NOT_IMPLEMENTED;
                    }
                }
            }
        }

    public:
        void updatePadding() {
            // step 1: fill all halo regions covered by parents
#pragma omp parallel
            for (auto l = 1; l < this->accessibleRanges.size(); ++l) {
#pragma omp for schedule(dynamic)
                for (auto p = 0; p < this->accessibleRanges[l].size(); ++p) {
                    // here to avoid the accessibleRanges[l][p] is already been trimmed by the maxLogicalRange[l]
                    auto bc_ranges = this->localRanges[l][p]
                                             .getInnerRange(-this->mesh.buffWidth)
                                             .getBCRanges(this->mesh.buffWidth);
                    for (auto r_p : this->mesh.parents[l][p]) {
                        // convert the parent range into this level
                        auto p_range = this->localRanges[l - 1][r_p];
                        for (auto i = 0; i < dim; ++i) {
                            p_range.start[i] *= this->mesh.refinementRatio;
                            p_range.end[i] *= this->mesh.refinementRatio;
                        }
                        // for each potential intersections
                        for (auto& bc_r : bc_ranges) {
                            rangeFor_s(DS::commonRange(bc_r, p_range), [&](auto&& i) {
                                // use piecewise constant interpolation
                                auto i_base = i.toLevel(l - 1, this->mesh.refinementRatio);
                                i_base.p = r_p;
                                this->operator[](i) = this->operator[](i_base);
                            });
                        }
                    }
                }
            }
            // step 2: fill all halo regions covered by neighbors
#pragma omp parallel
            for (auto l = 1; l < this->accessibleRanges.size(); ++l) {
#pragma omp for nowait schedule(dynamic)
                for (auto p = 0; p < this->accessibleRanges[l].size(); ++p) {
                    auto bc_ranges = this->localRanges[l][p]
                                             .getInnerRange(-this->mesh.buffWidth)
                                             .getBCRanges(this->mesh.buffWidth);
                    for (auto r_n : this->mesh.neighbors[l][p]) {
                        // for each potential intersections
                        for (auto& bc_r : bc_ranges) {
                            auto _r = DS::commonRange(bc_r, this->localRanges[l][r_n]);
                            rangeFor_s(DS::commonRange(bc_r, this->localRanges[l][r_n]), [&](auto&& i) {
                                // copy from other fine cells
                                auto other_i = i;
                                other_i.p = r_n;
                                this->operator[](i) = this->operator[](other_i);
                            });
                        }
                    }
                }
            }
        }
        void updateCovering() {
            auto ratio = this->mesh.refinementRatio;
#pragma omp parallel
            for (auto l = this->localRanges.size() - 1; l > 0; --l) {
#pragma omp for schedule(dynamic)
                for (auto p = 0; p < this->localRanges[l].size(); ++p) {
                    for (auto& i_p : this->mesh.parents[l][p]) {
                        auto rp = this->localRanges[l - 1][i_p];
                        auto rc = this->localRanges[l][p];
                        for (auto i = 0; i < dim; ++i) {
                            rc.start[i] /= ratio;
                            rc.end[i] /= ratio;
                        }
                        rc.level = l - 1;
                        rangeFor_s(DS::commonRange(rp, rc), [&](auto&& i) {
                            auto rt = rc;
                            for (auto k = 0; k < dim; ++k) {
                                rt.start[k] = i[k] * ratio;
                                rt.end[k] = (i[k] + 1) * ratio;
                            }
                            rt.level = l;
                            this->operator[](i) = rangeReduce_s(
                                                          rt, [](auto&& a, auto&& b) { return a + b; },
                                                          [&](auto&& k) { return this->operator[](k); })
                                                  / Math::int_pow(ratio, dim);
                        });
                    }
                }
            }
        }
        void replaceMeshBy(auto&& m) {
            // build a new local field
            auto builder = ExprBuilder<CartAMRField>().setMesh(m).setName(this->getName()).setLoc(this->loc);
            for (auto i = 0; i < dim; ++i) {
                builder.setBC(i, DimPos::start, this->bc[i].start);
                builder.setBC(i, DimPos::end, this->bc[i].end);
            }
            auto f = builder.build();
#pragma omp parallel
            for (auto l = 0; l < this->accessibleRanges.size(); ++l) {
                if (l > 0) {
                    // copy all coarser data from new to new
#pragma omp for schedule(dynamic)
                    for (auto p_new = 0; p_new < f.accessibleRanges[l].size(); ++p_new) {
                        for (auto p = 0; p < f.accessibleRanges[l - 1].size(); ++p) {
                            auto r_upcast = f.localRanges[l - 1][p];
                            for (auto i = 0; i < dim; ++i) {
                                r_upcast.start[i] *= this->mesh.refinementRatio;
                                r_upcast.end[i] *= this->mesh.refinementRatio;
                            }
                            r_upcast.level = l;
                            rangeFor_s(DS::commonRange(r_upcast, f.localRanges[l][p_new]), [&](auto&& i) {
                                auto i_new = i;
                                i_new.p = p_new;
                                auto i_old = i.toLevel(l - 1, this->mesh.refinementRatio);
                                f[i_new] = f[i_old];
                            });
                        }
                    }
                }
#pragma omp for schedule(dynamic)
                for (auto p_new = 0; p_new < f.accessibleRanges[l].size(); ++p_new) {
                    for (auto p = 0; p < this->accessibleRanges[l].size(); ++p) {
                        // copy each overlapping region of (p, p_new)
                        rangeFor_s(DS::commonRange(this->localRanges[l][p], f.localRanges[l][p_new]),
                                   [&](auto&& i) {
                                       auto i_old = i;
                                       i_old.p = p;
                                       auto i_new = i;
                                       i_new.p = p_new;
                                       f[i_new] = this->operator[](i_old);
                                   });
                    }
                }
            }
            std::swap(this->data, f.data);
            std::swap(this->accessibleRanges, f.accessibleRanges);
            std::swap(this->localRanges, f.localRanges);
            std::swap(this->assignableRanges, f.assignableRanges);
            std::swap(this->maxLogicalRanges, f.maxLogicalRanges);
            std::swap(this->offset, f.offset);
            this->mesh = f.mesh;
            updatePadding();
            updateCovering();
        }

    protected:
        auto getViewImpl_final() {
            OP_NOT_IMPLEMENTED;
            return 0;
        }
        const auto& evalAtImpl_final(const index_type& i) const {
            return data[i.l][i.p][i - offset[i.l][i.p]];
        }
        const auto& evalSafeAtImpl_final(const index_type& i) const {
            return data[i.l][i.p][i - offset[i.l][i.p]];
        }
        auto& evalAtImpl_final(const index_type& i) { return data[i.l][i.p][i - offset[i.l][i.p]]; }
        auto& evalSafeAtImpl_final(const index_type& i) { return data[i.l][i.p][i - offset[i.l][i.p]]; }

        template <typename Other>
        requires(!std::same_as<Other, CartAMRField>) bool containsImpl_final(const Other& o) const {
            return false;
        }

        bool containsImpl_final(const CartAMRField& other) const { return this == &other; }
    };

    template <typename D, typename M, typename C>
    struct ExprBuilder<CartAMRField<D, M, C>> {
        using Field = CartAMRField<D, M, C>;
        using Mesh = M;
        static constexpr auto dim = internal::CartAMRFieldExprTrait<Field>::dim;
        ExprBuilder() = default;

        auto& setName(const std::string& n) {
            f.name = n;
            return *this;
        }

        auto& setMesh(const Mesh& m) {
            f.mesh = m;
            return *this;
        }

        auto& setLoc(const std::array<LocOnMesh, dim>& loc) {
            f.loc = loc;
            return *this;
        }

        auto& setLoc(LocOnMesh loc) {
            f.loc.fill(loc);
            return *this;
        }

        auto& setLocOfDim(int i, LocOnMesh loc) {
            f.loc[i] = loc;
            return *this;
        }

        // set a logical bc
        auto& setBC(int d, DimPos pos, BCType type) {
            OP_ASSERT(d < dim);
            OP_ASSERT(type != BCType::Dirc && type != BCType::Neum);
            auto& targetBC = pos == DimPos::start ? f.bc[d].start : f.bc[d].end;
            switch (type) {
                case BCType::Symm:
                    targetBC = std::make_unique<SymmBC<CartAMRField<D, M, C>>>();
                    break;
                case BCType::ASymm:
                    targetBC = std::make_unique<ASymmBC<CartAMRField<D, M, C>>>();
                    break;
                default:
                    OP_ERROR("BC Type not supported.");
                    OP_ABORT;
            }
            return *this;
        }

        // set a constant bc
        template <Meta::Numerical T>
        auto& setBC(int d, DimPos pos, BCType type, T val) {
            OP_ASSERT(d < dim);
            auto& targetBC = pos == DimPos::start ? f.bc[d].start : f.bc[d].end;
            switch (type) {
                case BCType::Dirc:
                    targetBC = std::make_unique<ConstDircBC<CartAMRField<D, M, C>>>(val);
                    break;
                case BCType::Neum:
                    targetBC = std::make_unique<ConstNeumBC<CartAMRField<D, M, C>>>(val);
                    break;
                default:
                    OP_ERROR("BC Type not supported.");
                    OP_ABORT;
            }
            return *this;
        }

        auto& setBC(int d, DimPos pos, auto&& bc) {
            auto& targetBC = pos == DimPos::start ? f.bc[d].start : f.bc[d].end;
            targetBC = bc->getCopy();
            return *this;
        }

        auto& build() {
            calculateRanges();
            f.data.resize(f.localRanges.size());
            for (auto i = 0; i < f.data.size(); ++i) {
                f.data[i].resize(f.localRanges[i].size());
                for (auto j = 0; j < f.data[i].size(); ++j) {
                    f.data[i][j].reShape(f.accessibleRanges[i][j].getExtends());
                }
            }
            f.offset.resize(f.accessibleRanges.size());
            for (auto i = 0; i < f.accessibleRanges.size(); ++i) {
                f.offset[i].resize(f.accessibleRanges[i].size());
                for (auto j = 0; j < f.accessibleRanges[i].size(); ++j) {
                    f.offset[i][j] = typename internal::CartAMRFieldExprTrait<Field>::index_type(
                            i, j, f.accessibleRanges[i][j].getOffset());
                }
            }
            f.updateBC();
            return f;
        }

    private:
        void calculateRanges() {
            // init ranges to mesh range
            f.assignableRanges = f.localRanges = f.accessibleRanges = f.mesh.getRanges();
            for (auto i = 0; i < f.mesh.meshes.size(); ++i) {
                f.maxLogicalRanges.emplace_back(i, 0, f.mesh.meshes[i].getRange());
            }
            // extend accessibleRange according to localRange + buffWidth
            for (auto l = 1; l < f.localRanges.size(); ++l) {
                for (auto p = 0; p < f.localRanges[l].size(); ++p) {
                    auto& r = f.accessibleRanges[l][p];
                    for (auto i = 0; i < dim; ++i) {
                        r.start[i] -= f.mesh.buffWidth;
                        r.end[i] += f.mesh.buffWidth;
                        // trim accessibleRange to logical range
                        r.start[i] = std::max(f.maxLogicalRanges[l].start[i], r.start[i]);
                        r.end[i] = std::min(f.maxLogicalRanges[l].end[i], r.end[i]);
                    }
                }
            }
            // trim all ranges according to locs
            for (auto i = 0; i < dim; ++i) {
                if (f.loc[i] == LocOnMesh::Center) {
                    for (auto& _ : f.localRanges)
                        for (auto& r : _) r.end[i]--;
                    for (auto& _ : f.accessibleRanges)
                        for (auto& r : _) r.end[i]--;
                    for (auto& _ : f.assignableRanges)
                        for (auto& r : _) r.end[i]--;
                    for (auto& r : f.maxLogicalRanges) r.end[i]--;
                }
            }

            // trim ranges according to bcs
            for (auto i = 0; i < dim; ++i) {
                auto loc = f.loc[i];
                // start side
                auto type = f.bc[i].start ? f.bc[i].start->getBCType() : BCType::Undefined;
                switch (type) {
                    case BCType::Dirc:
                        if (loc == LocOnMesh::Corner) f.assignableRanges[0][0].start[i]++;
                        break;
                    case BCType::Neum:
                    case BCType::Undefined:
                        break;
                    default:
                        OP_NOT_IMPLEMENTED;
                }
                // end side
                type = f.bc[i].end ? f.bc[i].end->getBCType() : BCType::Undefined;
                switch (type) {
                    case BCType::Dirc:
                        if (loc == LocOnMesh::Corner) { f.assignableRanges[0][0].end[i]--; }
                        break;
                    case BCType::Neum:
                    case BCType::Undefined:
                        break;
                    default:
                        OP_NOT_IMPLEMENTED;
                }
            }
        }

        CartAMRField<D, M, C> f;
    };

}// namespace OpFlow
#endif//OPFLOW_CARTAMRFIELD_HPP
