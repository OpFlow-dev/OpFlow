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

#ifndef OPFLOW_STENCILFIELD_HPP
#define OPFLOW_STENCILFIELD_HPP

#include "Core/BC/BCBase.hpp"
#include "Core/BC/LogicalBC.hpp"
#include "Core/BC/ProxyBC.hpp"
#include "Core/Field/MeshBased/MeshBasedFieldExprTrait.hpp"
#include "Core/Field/MeshBased/SemiStructured/CartAMRFieldExprTrait.hpp"
#include "Core/Field/MeshBased/SemiStructured/CartAMRFieldTrait.hpp"
#include "Core/Field/MeshBased/SemiStructured/SemiStructuredFieldExprTrait.hpp"
#include "Core/Field/MeshBased/StencilFieldTrait.hpp"
#include "Core/Field/MeshBased/Structured/StructuredFieldExprTrait.hpp"
#include "DataStructures/StencilPad.hpp"
#include "Math/Interpolator/Interpolator.hpp"

namespace OpFlow {

    template <StructuredFieldExprType T, template <typename, typename> typename map_impl>
    struct StencilField<T, map_impl>
        : internal::StructuredFieldExprTrait<T>::template twin_type<StencilField<T, map_impl>> {
        std::array<DS::Pair<std::unique_ptr<BCBase<StencilField>>>, internal::ExprTrait<StencilField>::dim>
                bc;
        int color = 0;

        StencilField() = default;
        StencilField(const StencilField& other)
            : internal::StructuredFieldExprTrait<T>::template twin_type<StencilField>(other),
              base(other.base), pinned(other.pinned), color(other.color) {
            for (auto i = 0; i < internal::ExprTrait<StencilField>::dim; ++i) {
                bc[i].start = other.bc[i].start ? other.bc[i].start->getCopy() : nullptr;
                if (isLogicalBC(bc[i].start->getBCType()))
                    dynamic_cast<LogicalBCBase<StencilField>*>(bc[i].start.get())->rebindField(*this);
                bc[i].end = other.bc[i].end ? other.bc[i].end->getCopy() : nullptr;
                if (isLogicalBC(bc[i].end->getBCType()))
                    dynamic_cast<LogicalBCBase<StencilField>*>(bc[i].end.get())->rebindField(*this);
            }
        }
        StencilField(StencilField&&) noexcept = default;
        explicit StencilField(const T& base, int color = 0) : base(&base), color(color) {
            this->name = fmt::format("StencilField({})", base.name);
            if constexpr (StructuredFieldExprType<T>) this->loc = base.loc;
            this->mesh = base.mesh.getView();
            this->localRange = base.localRange;
            this->assignableRange = base.assignableRange;
            this->accessibleRange = base.accessibleRange;
            this->logicalRange = base.logicalRange;
            for (auto i = 0; i < internal::MeshBasedFieldExprTrait<T>::dim; ++i) {
                if (base.bc[i].start && isLogicalBC(base.bc[i].start->getBCType())) {
                    // if base.bc[i].start is a logical bc, we build a new instance of the same type bc
                    switch (base.bc[i].start->getBCType()) {
                        case BCType::Symm:
                            this->bc[i].start = genLogicalBC<BCType::Symm>(*this, i, DimPos::start);
                            break;
                        case BCType::ASymm:
                            this->bc[i].start = genLogicalBC<BCType::ASymm>(*this, i, DimPos::start);
                            break;
                        case BCType::Periodic:
                            this->bc[i].start = genLogicalBC<BCType::Periodic>(*this, i, DimPos::start);
                            break;
                        default:
                            OP_CRITICAL("{} is not a logical bc type", base.bc[i].start->getTypeName());
                            OP_ABORT;
                    }
                } else {
                    // other cases we build a proxy bc to convert original bc to the same bc returning stencilpads
                    this->bc[i].start
                            = genProxyBC<typename internal::MeshBasedFieldExprTrait<StencilField>::type>(
                                    *(base.bc[i].start));
                }
                if (base.bc[i].end && isLogicalBC(base.bc[i].end->getBCType())) {
                    // if base.bc[i].start is a logical bc, we build a new instance of the same type bc
                    switch (base.bc[i].end->getBCType()) {
                        case BCType::Symm:
                            this->bc[i].end = genLogicalBC<BCType::Symm>(*this, i, DimPos::end);
                            break;
                        case BCType::ASymm:
                            this->bc[i].end = genLogicalBC<BCType::ASymm>(*this, i, DimPos::end);
                            break;
                        case BCType::Periodic:
                            this->bc[i].end = genLogicalBC<BCType::Periodic>(*this, i, DimPos::end);
                            break;
                        default:
                            OP_CRITICAL("{} is not a logical bc type", base.bc[i].end->getTypeName());
                            OP_ABORT;
                    }
                } else {
                    this->bc[i].end
                            = genProxyBC<typename internal::MeshBasedFieldExprTrait<StencilField>::type>(
                                    *(base.bc[i].end));
                }
            }
        }

        using index_type = typename internal::MeshBasedFieldExprTrait<T>::index_type;
        using colored_index_type = DS::ColoredIndex<index_type>;

        void pin(bool p) { pinned = p; }

        // only used for HYPRE solvers to get the exact offset stencil pad
        void ignorePeriodicBC() {
            for (auto i = 0; i < dim; ++i) {
                if (base->bc[i].start->getBCType() == BCType::Periodic) {
                    this->assignableRange.start[i] = base->logicalRange.start[i];
                    this->accessibleRange.start[i] = base->logicalRange.start[i];
                    this->assignableRange.end[i] = base->logicalRange.end[i];
                    this->accessibleRange.end[i] = base->logicalRange.end[i];
                }
            }
        }

        auto evalAtImpl_final(const index_type& index) const {
            OP_ASSERT_MSG(base, "base ptr of stencil field is nullptr");
            if (DS::inRange(this->assignableRange, index)) [[likely]] {
                    auto ret = typename internal::ExprTrait<StencilField>::elem_type {0};
                    // note: here solution is pinned at base->assignableRange.start
                    // rather than this->assignableRange.start; This is because
                    // for periodic case the assignableRange of this will be changed
                    // to logicalRange for HYPRE solver to get exact offset.
                    if (!(pinned && index == index_type(base->assignableRange.start)))
                        [[likely]] ret.pad[colored_index_type {index, color}] = 1.0;
                    return ret;
                }
            else if (DS::inRange(this->accessibleRange, index)) {
                // index lay on dirc bc
                return typename internal::ExprTrait<StencilField>::elem_type {base->evalAt(index)};
            } else if (!DS::inRange(this->logicalRange, index)) {
                OP_ERROR("Index {} out of range {}", index, this->logicalRange.toString());
                OP_ABORT;
            } else {
                // index has to be extended by bc
                for (int i = 0; i < dim; ++i) {
                    if (this->accessibleRange.start[i] <= index[i] && index[i] < this->accessibleRange.end[i])
                        continue;
                    if (this->loc[i] == LocOnMesh::Corner) {
                        // corner case
                        if (index[i] < this->accessibleRange.start[i]) {
                            // lower case
                            switch (this->bc[i].start->getBCType()) {
                                case BCType::Dirc: {
                                    // mid point rule
                                    auto bc_v = this->bc[i].start->evalAt(index);
                                    auto mirror_idx = index;
                                    mirror_idx[i] = 2 * this->accessibleRange.start[i] - index[i];
                                    return Math::Interpolator1D::intp(
                                            base->mesh.x(i, this->accessibleRange.start[i]), bc_v,
                                            base->mesh.x(i, mirror_idx), this->evalAtImpl_final(mirror_idx),
                                            base->mesh.x(i, index));
                                } break;
                                case BCType::Neum: {
                                    // mid-diff = bc
                                    auto bc_v = this->bc[i].start->evalAt(index);
                                    auto mirror_idx = index;
                                    mirror_idx[i] = 2 * this->accessibleRange.start[i] - index[i];
                                    return this->evalAtImpl_final(mirror_idx)
                                           + bc_v * (base->mesh.x(i, index) - base->mesh.x(i, mirror_idx));
                                } break;
                                case BCType::Symm: {
                                    auto mirror_idx = index;
                                    mirror_idx[i] = 2 * this->accessibleRange.start[i] - index[i];
                                    return this->evalAtImpl_final(mirror_idx);
                                } break;
                                case BCType::ASymm: {
                                    auto mirror_idx = index;
                                    mirror_idx[i] = 2 * this->accessibleRange.start[i] - index[i];
                                    return -1. * this->evalAtImpl_final(mirror_idx);
                                } break;
                                case BCType::Periodic: {
                                    auto mirror_idx = index;
                                    mirror_idx[i]
                                            += this->accessibleRange.end[i] - this->accessibleRange.start[i];
                                    return this->evalAtImpl_final(mirror_idx);
                                }
                                default:
                                    OP_ERROR("Cannot handle current bc padding for stencil field: bc type {}",
                                             this->bc[i].start->getTypeName());
                                    OP_ABORT;
                            }
                        } else {
                            // upper case
                            switch (this->bc[i].end->getBCType()) {
                                case BCType::Dirc: {
                                    // mid-point rule
                                    auto bc_v = this->bc[i].end->evalAt(index);
                                    auto mirror_idx = index;
                                    mirror_idx[i] = 2 * this->accessibleRange.end[i] - 2 - index[i];
                                    return Math::Interpolator1D::intp(
                                            base->mesh.x(i, this->accessibleRange.end[i] - 1), bc_v,
                                            base->mesh.x(i, mirror_idx), this->evalAtImpl_final(mirror_idx),
                                            base->mesh.x(i, index));
                                } break;
                                case BCType::Neum: {
                                    // mid-diff = bc
                                    auto bc_v = this->bc[i].end->evalAt(index);
                                    auto mirror_idx = index;
                                    mirror_idx[i] = 2 * this->accessibleRange.end[i] - 2 - index[i];
                                    return this->evalAtImpl_final(mirror_idx)
                                           + bc_v * (base->mesh.x(i, index) - base->mesh.x(i, mirror_idx));
                                } break;
                                case BCType::Symm: {
                                    auto mirror_idx = index;
                                    mirror_idx[i] = 2 * this->accessibleRange.end[i] - 2 - index[i];
                                    return this->evalAtImpl_final(mirror_idx);
                                } break;
                                case BCType::ASymm: {
                                    auto mirror_idx = index;
                                    mirror_idx[i] = 2 * this->accessibleRange.end[i] - 2 - index[i];
                                    return -1. * this->evalAtImpl_final(mirror_idx);
                                } break;
                                case BCType::Periodic: {
                                    auto mirror_idx = index;
                                    mirror_idx[i]
                                            -= this->accessibleRange.end[i] - this->accessibleRange.start[i];
                                    return this->evalAtImpl_final(mirror_idx);
                                } break;
                                default:
                                    OP_ERROR("Cannot handle current bc padding for stencil field: bc type {}",
                                             this->bc[i].end->getTypeName());
                                    OP_ABORT;
                            }
                        }
                    } else {
                        // center case
                        if (index[i] < this->accessibleRange.start[i]) {
                            // lower case
                            switch (this->bc[i].start->getBCType()) {
                                case BCType::Dirc: {
                                    // mid-point rule
                                    auto bc_v = this->bc[i].start->evalAt(index);
                                    auto mirror_index = index;
                                    mirror_index[i] = 2 * this->accessibleRange.start[i] - 1 - index[i];
                                    return Math::Interpolator1D::intp(
                                            base->mesh.x(i, this->accessibleRange.start[i]), bc_v,
                                            base->mesh.x(i, mirror_index[i])
                                                    + base->mesh.dx(i, mirror_index) / 2.,
                                            this->evalAtImpl_final(mirror_index),
                                            base->mesh.x(i, index[i]) + base->mesh.dx(i, index) / 2.);
                                } break;
                                case BCType::Neum: {
                                    // mid-diff = bc
                                    auto bc_v = this->bc[i].start->evalAt(index);
                                    auto mirror_index = index;
                                    mirror_index[i] = 2 * this->accessibleRange.start[i] - 1 - index[i];
                                    return this->evalAtImpl_final(mirror_index)
                                           + bc_v
                                                     * (base->mesh.x(i, index) + base->mesh.dx(i, index) / 2.
                                                        - base->mesh.x(i, mirror_index)
                                                        - base->mesh.dx(i, mirror_index) / 2.);
                                } break;
                                case BCType::Symm: {
                                    auto mirror_index = index;
                                    mirror_index[i] = 2 * this->accessibleRange.start[i] - 1 - index[i];
                                    return this->evalAtImpl_final(mirror_index);

                                } break;
                                case BCType::ASymm: {
                                    auto mirror_index = index;
                                    mirror_index[i] = 2 * this->accessibleRange.start[i] - 1 - index[i];
                                    return -1.0 * this->evalAtImpl_final(mirror_index);

                                } break;
                                case BCType::Periodic: {
                                    auto mirror_idx = index;
                                    mirror_idx[i]
                                            += this->accessibleRange.end[i] - this->accessibleRange.start[i];
                                    return this->evalAtImpl_final(mirror_idx);
                                } break;
                                default:
                                    OP_ERROR("Cannot handle current bc padding: bc type {}",
                                             this->bc[i].start->getTypeName());
                                    OP_ABORT;
                            }
                        } else {
                            // upper case
                            switch (this->bc[i].end->getBCType()) {
                                case BCType::Dirc: {
                                    auto bc_v = this->bc[i].end->evalAt(index);
                                    auto mirror_index = index;
                                    mirror_index[i] = 2 * this->accessibleRange.end[i] - 1 - index[i];
                                    return Math::Interpolator1D::intp(
                                            base->mesh.x(i, this->accessibleRange.end[i]), bc_v,
                                            base->mesh.x(i, mirror_index[i])
                                                    + base->mesh.dx(i, mirror_index) / 2.,
                                            this->evalAtImpl_final(mirror_index),
                                            base->mesh.x(i, index[i]) + base->mesh.dx(i, index) / 2.);

                                } break;
                                case BCType::Neum: {
                                    auto bc_v = this->bc[i].end->evalAt(index);
                                    auto mirror_index = index;
                                    mirror_index[i] = 2 * this->accessibleRange.end[i] - 1 - index[i];
                                    return this->evalAtImpl_final(mirror_index)
                                           + bc_v
                                                     * (base->mesh.x(i, index) + base->mesh.dx(i, index) / 2.
                                                        - base->mesh.x(i, mirror_index)
                                                        - base->mesh.dx(i, mirror_index) / 2.);
                                } break;
                                case BCType::Symm: {
                                    auto mirror_index = index;
                                    mirror_index[i] = 2 * this->accessibleRange.end[i] - 1 - index[i];
                                    return this->evalAtImpl_final(mirror_index);

                                } break;
                                case BCType::ASymm: {
                                    auto mirror_index = index;
                                    mirror_index[i] = 2 * this->accessibleRange.end[i] - 1 - index[i];
                                    return -1. * this->evalAtImpl_final(mirror_index);

                                } break;
                                case BCType::Periodic: {
                                    auto mirror_idx = index;
                                    mirror_idx[i]
                                            -= this->accessibleRange.end[i] - this->accessibleRange.start[i];
                                    return this->evalAtImpl_final(mirror_idx);
                                } break;
                                default:
                                    OP_ERROR("Cannot handle current bc padding: bc type {}",
                                             this->bc[i].end->getTypeName());
                                    OP_ABORT;
                            }
                        }
                    }
                }
                // should not reach here
                OP_ERROR("Could not handle current case: i = {}", index);
                OP_ABORT;
            }
        }
        void prepareImpl_final() const {}

        template <typename Other>
        requires(!std::same_as<Other, StencilField>) bool containsImpl_final(const Other& o) const {
            return false;
        }
        bool containsImpl_final(const StencilField& o) const { return this == &o; }

    private:
        const T* base;
        bool pinned = false;
        constexpr static auto dim = internal::MeshBasedFieldExprTrait<T>::dim;
    };

    template <CartAMRFieldType T, template <typename, typename> typename map_impl>
    struct StencilField<T, map_impl> : CartAMRFieldExpr<StencilField<T, map_impl>> {
        using index_type = typename internal::ExprTrait<T>::index_type;
        using colored_index_type = DS::ColoredIndex<index_type>;

    private:
        // todo: why need explicit storage here?
        std::vector<std::vector<typename internal::ExprTrait<typename internal::ExprTrait<
                T>::template other_type<DS::StencilPad<colored_index_type, map_impl>>>::container_type>>
                data;
        std::vector<std::vector<typename internal::ExprTrait<
                typename internal::ExprTrait<T>::template other_type<bool>>::container_type>>
                block_mark;
        std::vector<std::vector<index_type>> offset;
        int color = 0;

    public:
        StencilField() = default;
        StencilField(const StencilField& other)
            : CartAMRFieldExpr<StencilField>(other), data(other.data), block_mark(other.block_mark),
              offset(other.offset) {}
        StencilField(StencilField&& other) noexcept
            : CartAMRFieldExpr<StencilField>(std::move(other)), data(std::move(other.data)),
              block_mark(std::move(other.block_mark)), offset(std::move(other.offset)) {}
        explicit StencilField(const T& base, int color) : color(color) {
            this->name = fmt::format("StencilField({})", base.name);
            this->loc = base.loc;
            this->mesh = base.mesh;
            this->localRanges = base.localRanges;
            this->assignableRanges = base.assignableRanges;
            this->accessibleRanges = base.accessibleRanges;
            this->maxLogicalRanges = base.maxLogicalRanges;
            for (auto i = 0; i < internal::SemiStructuredFieldExprTrait<T>::dim; ++i) {
                this->bc[i].start
                        = genProxyBC<typename internal::SemiStructuredFieldExprTrait<StencilField>::type>(
                                *(base.bc[i].start));
                this->bc[i].end
                        = genProxyBC<typename internal::SemiStructuredFieldExprTrait<StencilField>::type>(
                                *(base.bc[i].end));
            }
            // allocate data
            data.resize(this->localRanges.size());
            for (auto i = 0; i < data.size(); ++i) {
                data[i].resize(this->localRanges[i].size());
                for (auto j = 0; j < data[i].size(); ++j) {
                    data[i][j].reShape(this->accessibleRanges[i][j].getExtends());
                }
            }
            block_mark.resize(this->localRanges.size());
            for (auto i = 0; i < block_mark.size(); ++i) {
                block_mark[i].resize(this->localRanges[i].size());
                for (auto j = 0; j < block_mark[i].size(); ++j) {
                    block_mark[i][j].reShape(this->accessibleRanges[i][j].getExtends());
                }
            }
            offset.resize(this->accessibleRanges.size());
            for (auto i = 0; i < this->accessibleRanges.size(); ++i) {
                offset[i].resize(this->accessibleRanges[i].size());
                for (auto j = 0; j < this->accessibleRanges[i].size(); ++j) {
                    offset[i][j] = index_type(i, j, this->accessibleRanges[i][j].getOffset());
                }
            }
            // init all stencils
            for (auto l = 0; l < this->localRanges.size(); ++l) {
                for (auto p = 0; p < this->localRanges[l].size(); ++p) {
                    rangeFor(this->localRanges[l][p], [&](auto&& i) {
                        auto& st = this->operator[](i);
                        st.pad[colored_index_type {i}] = 1.0;
                        st.bias = 0;
                        this->blocked(i) = false;
                    });
                }
            }
            // update padding & covering
            updatePadding();
            updateCovering();
        }

        void pin(bool p) { pinned = p; }

        void prepareImpl_final() {}

        void updatePadding() {
            // step 1: fill all halo regions covered by parents
            for (auto l = 1; l < this->accessibleRanges.size(); ++l) {
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
                            rangeFor(DS::commonRange(bc_r, p_range), [&](auto&& i) {
                                // use piecewise constant interpolation
                                auto i_base = i.toLevel(l - 1, this->mesh.refinementRatio);
                                i_base.p = r_p;
                                this->operator[](i) = this->operator[](i_base);
                                this->blocked(i) = true;
                            });
                        }
                    }
                }
            }
            // step 2: fill all halo regions covered by neighbors
            for (auto l = 1; l < this->accessibleRanges.size(); ++l) {
                for (auto p = 0; p < this->accessibleRanges[l].size(); ++p) {
                    auto bc_ranges = this->localRanges[l][p]
                                             .getInnerRange(-this->mesh.buffWidth)
                                             .getBCRanges(this->mesh.buffWidth);
                    for (auto r_n : this->mesh.neighbors[l][p]) {
                        // for each potential intersections
                        for (auto& bc_r : bc_ranges) {
                            auto _r = DS::commonRange(bc_r, this->localRanges[l][r_n]);
                            rangeFor(DS::commonRange(bc_r, this->localRanges[l][r_n]), [&](auto&& i) {
                                // copy from other fine cells
                                auto other_i = i;
                                other_i.p = r_n;
                                this->operator[](i) = this->operator[](other_i);
                                this->blocked(i) = true;
                            });
                        }
                    }
                }
            }
        }
        void updateCovering() {
            auto ratio = this->mesh.refinementRatio;
            for (auto l = 1; l < this->localRanges.size(); ++l) {
                for (auto p = 0; p < this->localRanges[l].size(); ++p) {
                    for (auto& i_p : this->mesh.parents[l][p]) {
                        auto rp = this->localRanges[l - 1][i_p];
                        auto rc = this->localRanges[l][p];
                        for (auto i = 0; i < dim; ++i) {
                            rc.start[i] /= ratio;
                            rc.end[i] /= ratio;
                        }
                        rc.level = l - 1;
                        rangeFor(DS::commonRange(rp, rc), [&](auto&& i) {
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
                            this->blocked(i) = true;
                        });
                    }
                }
            }
        }
        auto getView() {
            OP_NOT_IMPLEMENTED;
            return 0;
        }
        const auto& evalAtImpl_final(const index_type& i) const {
            return data[i.l][i.p][i - offset[i.l][i.p]];
        }
        auto& evalAtImpl_final(const index_type& i) { return data[i.l][i.p][i - offset[i.l][i.p]]; }
        auto& blocked(const index_type& i) { return block_mark[i.l][i.p][i - offset[i.l][i.p]]; }
        const auto& blocked(const index_type& i) const { return block_mark[i.l][i.p][i - offset[i.l][i.p]]; }

        template <typename Other>
        requires(!std::same_as<Other, StencilField>) bool containsImpl_final(const Other& o) const {
            return false;
        }
        bool containsImpl_final(const StencilField& o) const { return this == &o; }

    private:
        bool pinned = false;
        constexpr static auto dim = internal::MeshBasedFieldExprTrait<T>::dim;
    };
}// namespace OpFlow
#endif//OPFLOW_STENCILFIELD_HPP
