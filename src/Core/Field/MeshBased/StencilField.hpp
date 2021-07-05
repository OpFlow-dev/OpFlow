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

#ifndef OPFLOW_STENCILFIELD_HPP
#define OPFLOW_STENCILFIELD_HPP

#include "Core/BC/BCBase.hpp"
#include "Core/Field/MeshBased/MeshBasedFieldExprTrait.hpp"
#include "Core/Field/MeshBased/SemiStructured/CartAMRFieldExprTrait.hpp"
#include "Core/Field/MeshBased/SemiStructured/CartAMRFieldTrait.hpp"
#include "Core/Field/MeshBased/SemiStructured/SemiStructuredFieldExprTrait.hpp"
#include "Core/Field/MeshBased/StencilFieldTrait.hpp"
#include "Core/Field/MeshBased/Structured/StructuredFieldExprTrait.hpp"
#include "DataStructures/StencilPad.hpp"

namespace OpFlow {

    template <StructuredFieldExprType T>
    struct StencilField<T> : internal::StructuredFieldExprTrait<T>::template twin_type<StencilField<T>> {
        StencilField() = default;
        StencilField(const StencilField&) = default;
        StencilField(StencilField&&) noexcept = default;
        explicit StencilField(const T& base) : base(&base) {
            this->name = fmt::format("StencilField({})", base.name);
            if constexpr (StructuredFieldExprType<T>) this->loc = base.loc;
            this->mesh = base.mesh.getView();
            this->localRange = base.localRange;
            this->assignableRange = base.assignableRange;
            this->accessibleRange = base.accessibleRange;
            for (auto i = 0; i < internal::MeshBasedFieldExprTrait<T>::dim; ++i) {
                this->bc[i].start
                        = genProxyBC<typename internal::MeshBasedFieldExprTrait<StencilField>::type>(
                                *(base.bc[i].start));
                this->bc[i].end = genProxyBC<typename internal::MeshBasedFieldExprTrait<StencilField>::type>(
                        *(base.bc[i].end));
            }
        }

        using index_type = typename internal::MeshBasedFieldExprTrait<T>::index_type;

        void pin(bool p) { pinned = p; }

        auto operator()(const index_type& index) const {
            if (DS::inRange(this->assignableRange, index)) [[likely]] {
                    auto ret = DS::StencilPad<index_type>(0);
                    if (pinned && index == index_type(this->assignableRange.start))
                        [[unlikely]] ret.pad[index] = 0.;
                    else
                        [[likely]] ret.pad[index] = 1.0;
                    return ret;
                }
            else
                [[unlikely]] { return DS::StencilPad<index_type>(base->evalSafeAt(index)); }
        }
        void prepare() const {}
        auto operator[](const index_type& index) const { return this->operator()(index); }
        auto evalAt(const index_type& index) const { return this->operator()(index); }
        auto evalSafeAt(const index_type& index) const { return this->operator()(index); }

        template <typename Other>
        requires(!std::same_as<Other, StencilField>) bool contains(const Other& o) const {
            return false;
        }
        bool contains(const StencilField& o) const { return this == &o; }

    private:
        const T* base;
        bool pinned = false;
        constexpr static auto dim = internal::MeshBasedFieldExprTrait<T>::dim;
    };

    template <CartAMRFieldType T>
    struct StencilField<T> : CartAMRFieldExpr<StencilField<T>> {
        using index_type = typename internal::ExprTrait<T>::index_type;

    private:
        std::vector<std::vector<typename internal::ExprTrait<typename internal::ExprTrait<
                T>::template other_type<DS::StencilPad<index_type>>>::container_type>>
                data;
        std::vector<std::vector<typename internal::ExprTrait<
                typename internal::ExprTrait<T>::template other_type<bool>>::container_type>>
                block_mark;
        std::vector<std::vector<index_type>> offset;

    public:
        StencilField() = default;
        StencilField(const StencilField& other)
            : CartAMRFieldExpr<StencilField<T>>(other), data(other.data), block_mark(other.block_mark),
              offset(other.offset) {}
        StencilField(StencilField&& other) noexcept
            : CartAMRFieldExpr<StencilField<T>>(std::move(other)), data(std::move(other.data)),
              block_mark(std::move(other.block_mark)), offset(std::move(other.offset)) {}
        explicit StencilField(const T& base) {
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
                        st.pad[i] = 1.0;
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

        void prepare() {}

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
        auto& operator()(const index_type& i) { return data[i.l][i.p][i - offset[i.l][i.p]]; }
        auto& operator[](const index_type& i) { return data[i.l][i.p][i - offset[i.l][i.p]]; }
        const auto& operator()(const index_type& i) const { return data[i.l][i.p][i - offset[i.l][i.p]]; }
        const auto& operator[](const index_type& i) const { return data[i.l][i.p][i - offset[i.l][i.p]]; }
        const auto& evalAt(const index_type& i) const { return data[i.l][i.p][i - offset[i.l][i.p]]; }
        const auto& evalSafeAt(const index_type& i) const { return data[i.l][i.p][i - offset[i.l][i.p]]; }
        auto& blocked(const index_type& i) { return block_mark[i.l][i.p][i - offset[i.l][i.p]]; }
        const auto& blocked(const index_type& i) const { return block_mark[i.l][i.p][i - offset[i.l][i.p]]; }

        template <typename Other>
        requires(!std::same_as<Other, StencilField>) bool contains(const Other& o) const {
            return false;
        }
        bool contains(const StencilField& o) const { return this == &o; }

    private:
        bool pinned = false;
        constexpr static auto dim = internal::MeshBasedFieldExprTrait<T>::dim;
    };
}// namespace OpFlow
#endif//OPFLOW_STENCILFIELD_HPP
