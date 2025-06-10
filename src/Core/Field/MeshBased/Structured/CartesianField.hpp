// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2025 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_CARTESIANFIELD_HPP
#define OPFLOW_CARTESIANFIELD_HPP

#include "CartesianFieldExpr.hpp"
#include "CartesianFieldTrait.hpp"
#include "Core/BC/BCBase.hpp"
#include "Core/BC/DircBC.hpp"
#include "Core/BC/LogicalBC.hpp"
#include "Core/BC/NeumBC.hpp"
#include "Core/Environment.hpp"
#include "Core/Field/MeshBased/Structured/CartesianFieldTrait.hpp"
#include "Core/Loops/FieldAssigner.hpp"
#include "Core/Loops/RangeFor.hpp"
#include "Core/Loops/StructFor.hpp"
#include "Core/Macros.hpp"
#include "Core/Mesh/Structured/CartesianMesh.hpp"
#include "Core/Parallel/AbstractSplitStrategy.hpp"
#include "Core/Parallel/ParallelPlan.hpp"
#include "Math/Interpolator/Interpolator.hpp"
#ifndef OPFLOW_INSIDE_MODULE
#include <memory>
#endif

OPFLOW_MODULE_EXPORT namespace OpFlow {
    template <typename D, typename M, typename C = DS::PlainTensor<D, internal::MeshTrait<M>::dim>>
    struct CartesianField : CartesianFieldExpr<CartesianField<D, M, C>> {
        using index_type = typename internal::CartesianFieldExprTrait<CartesianField>::index_type;

    private:
        C data;
        std::array<DS::Pair<int>, internal::MeshTrait<M>::dim> ext_width;
        bool initialized = false;
        constexpr static auto dim = internal::MeshTrait<M>::dim;

    public:
        friend ExprBuilder<CartesianField>;
        friend Expr<CartesianField>;
        using Expr<CartesianField>::operator();
        using Expr<CartesianField>::operator[];
        using Expr<CartesianField>::operator=;

        std::array<DS::Pair<std::unique_ptr<BCBase<CartesianField>>>, internal::MeshTrait<M>::dim> bc;

        CartesianField() = default;
        CartesianField(const CartesianField& other)
            : CartesianFieldExpr<CartesianField<D, M, C>>(other), data(other.data),
              ext_width(other.ext_width), initialized(true) {
            for (auto i = 0; i < internal::ExprTrait<CartesianField>::dim; ++i) {
                bc[i].start = other.bc[i].start ? other.bc[i].start->getCopy() : nullptr;
                if (bc[i].start && isLogicalBC(bc[i].start->getBCType()))
                    dynamic_cast<LogicalBCBase<CartesianField>*>(bc[i].start.get())->rebindField(*this);
                bc[i].end = other.bc[i].end ? other.bc[i].end->getCopy() : nullptr;
                if (bc[i].end && isLogicalBC(bc[i].end->getBCType()))
                    dynamic_cast<LogicalBCBase<CartesianField>*>(bc[i].end.get())->rebindField(*this);
            }
        }
        CartesianField(CartesianField&& other) noexcept
            : CartesianFieldExpr<CartesianField<D, M, C>>(std::move(other)), data(std::move(other.data)),
              initialized(true), bc(std::move(other.bc)), ext_width(std::move(other.ext_width)) {}

        CartesianField& operator=(const CartesianField& other) {
            assignImpl_final(other);
            return *this;
        }

        CartesianField& operator=(CartesianField&& other) noexcept {
            assignImpl_final(std::move(other));
            return *this;
        }

        template <typename T>
        requires std::same_as<T, CartesianField> void
        resplitWithStrategy(AbstractSplitStrategy<T>* strategy) {
            // this method only acts when MPI is enabled
#if defined(OPFLOW_WITH_MPI) && defined(OPFLOW_DISTRIBUTE_MODEL_MPI)
            if (!strategy) return;
            auto old_local_range = this->localRange;
            auto old_splitMap = this->splitMap;
            auto new_local_range = strategy->splitRange(this->mesh.getRange(), getGlobalParallelPlan());
            auto new_splitMap = strategy->getSplitMap(this->mesh.getRange(), getGlobalParallelPlan());
            for (int i = 0; i < dim; ++i) {
                auto _loc = this->loc[i];
                if (_loc == LocOnMesh::Corner && new_local_range.end[i] == this->mesh.getRange().end[i] - 1)
                    new_local_range.end[i]
                            = std::min(new_local_range.end[i] + 1, this->accessibleRange.end[i]);
                for (auto& r : new_splitMap) {
                    if (_loc == LocOnMesh::Corner && r.end[i] == this->mesh.getRange().end[i] - 1)
                        r.end[i] = std::min(r.end[i] + 1, this->accessibleRange.end[i]);
                }
            }

            // find each potential intersections with each rank
            std::vector<DS::Range<dim>> intersections(getWorkerCount());
            for (int i = 0; i < this->splitMap.size(); ++i) {
                intersections[i] = DS::commonRange(old_local_range, new_splitMap[i]);
            }
            std::vector<std::vector<D>> send_buff;
            std::vector<int> dest_ranks;
            for (int i = 0; i < intersections.size(); ++i) {
                if (!intersections[i].empty()) {
                    dest_ranks.push_back(i);
                    std::vector<D> buff;
                    buff.reserve(intersections[i].count());
                    rangeFor_s(intersections[i], [&](auto&& k) { buff.push_back(this->evalAt(k)); });
                    send_buff.push_back(std::move(buff));
                    if constexpr (std::is_trivial_v<D> && std::is_standard_layout_v<D>) {
                        MPI_Request request;
                        MPI_Isend(send_buff.back().data(), send_buff.back().size() * sizeof(D), MPI_BYTE,
                                  dest_ranks.back(), getWorkerId(), MPI_COMM_WORLD, &request);
                        MPI_Request_free(&request);// status tested on receiver's side
                    } else {
                        OP_NOT_IMPLEMENTED;
                    }
                }
            }

            std::vector<std::vector<D>> recv_buff;
            std::vector<MPI_Request> recv_requests;
            recv_buff.reserve(getWorkerCount());
            recv_requests.reserve(getWorkerCount());
            for (int i = 0; i < old_splitMap.size(); ++i) {
                intersections[i] = DS::commonRange(new_local_range, old_splitMap[i]);
            }
            std::vector<int> src_ranks;
            for (int i = 0; i < intersections.size(); ++i) {
                if (!intersections[i].empty()) {
                    src_ranks.push_back(i);
                    recv_buff.emplace_back();
                    recv_buff.back().resize(intersections[i].count());
                    recv_requests.emplace_back();
                    if constexpr (std::is_trivial_v<D> && std::is_standard_layout_v<D>) {
                        MPI_Irecv(recv_buff.back().data(), intersections[i].count() * sizeof(D), MPI_BYTE, i,
                                  i, MPI_COMM_WORLD, &recv_requests.back());
                    }
                }
            }
            // reshape data array
            this->data.reShape(new_local_range.getInnerRange(-this->padding).getExtends());
            this->offset = typename internal::CartesianFieldExprTrait<CartesianField>::index_type(
                    new_local_range.getInnerRange(-this->padding).getOffset());
            this->localRange = new_local_range;
            this->splitMap = new_splitMap;

            // loop over all requests
            int finished_count = 0;
            std::vector<bool> finished_bit(src_ranks.size(), false);
            while (finished_count != src_ranks.size()) {
                for (int i = 0; i < finished_bit.size(); ++i) {
                    if (!finished_bit[i]) {
                        int flag;
                        MPI_Test(&recv_requests[i], &flag, MPI_STATUS_IGNORE);
                        if (flag) {
                            finished_bit[i] = true;
                            finished_count++;
                            auto iter = recv_buff[i].begin();
                            rangeFor_s(intersections[src_ranks[i]],
                                       [&](auto&& k) { this->operator[](k) = *iter++; });
                        }
                    }
                }
            }
            this->updateNeighbors();
            updatePaddingImpl_final();
#endif
        }

        template <BasicArithOp Op = BasicArithOp::Eq>
        auto& assignImpl_final(const CartesianField& other) {
            if (!initialized) {
                OP_ASSERT_MSG(Op == BasicArithOp::Eq,
                              "Incremental assignment to uninitialized field is illegal");
                this->initPropsFrom(other);
                data = other.data;
                ext_width = other.ext_width;
                initialized = true;
            } else if (this != &other) {
                internal::FieldAssigner::assign<Op>(other, *this);
                this->updatePadding();
            }
            return *this;
        }

        template <BasicArithOp Op = BasicArithOp::Eq, CartesianFieldExprType T>
        auto&
        assignImpl_final(T&& other) {// T is not const here for that we need to call other.prepare() later
            other.prepare();
            if (!initialized) {
                OP_ASSERT_MSG(Op == BasicArithOp::Eq,
                              "Incremental assignment to uninitialized field is illegal");
                this->initPropsFrom(other);
                if constexpr (CartesianFieldType<T>) {
                    data = other.data;
                    ext_width = other.ext_width;
                    if constexpr (std::same_as<Meta::RealType<T>, CartesianField>)
                        for (int i = 0; i < dim; ++i) {
                            this->bc[i].start = other.bc[i].start ? other.bc[i].start->getCopy() : nullptr;
                            this->bc[i].end = other.bc[i].end ? other.bc[i].end->getCopy() : nullptr;
                        }
                    else
                        for (int i = 0; i < dim; ++i) {
                            this->bc[i].start = other.bc[i].start
                                                        ? genProxyBC<CartesianField>(*other.bc[i].start)
                                                        : nullptr;
                            this->bc[i].end = other.bc[i].end ? genProxyBC<CartesianField>(*other.bc[i].end)
                                                              : nullptr;
                        }
                } else {
                    this->data.reShape(this->localRange.getInnerRange(-this->padding).getExtends());
                    this->offset = typename internal::CartesianFieldExprTrait<CartesianField>::index_type(
                            this->localRange.getInnerRange(-this->padding).getOffset());
                }
                // invoke the assigner
                internal::FieldAssigner::assign<Op>(other, *this);
                this->updatePadding();
                initialized = true;
            } else if ((void*) this != (void*) &other) {
                // assign all values from T to assignable range
                internal::FieldAssigner::assign<Op>(other, *this);
                this->updatePadding();
            }
            return *this;
        }

        template <BasicArithOp Op = BasicArithOp::Eq>
        auto& assignImpl_final(const D& c) {
            if (!initialized) {
                OP_CRITICAL("CartesianField not initialized. Cannot assign constant to it.");
                OP_ABORT;
            }
            if constexpr (Op == BasicArithOp::Eq)
                rangeFor(DS::commonRange(this->assignableRange, this->localRange),
                         [&](auto&& i) { this->operator[](i) = c; });
            else if constexpr (Op == BasicArithOp::Add)
                rangeFor(DS::commonRange(this->assignableRange, this->localRange),
                         [&](auto&& i) { this->operator[](i) += c; });
            else if constexpr (Op == BasicArithOp::Minus)
                rangeFor(DS::commonRange(this->assignableRange, this->localRange),
                         [&](auto&& i) { this->operator[](i) -= c; });
            else if constexpr (Op == BasicArithOp::Mul)
                rangeFor(DS::commonRange(this->assignableRange, this->localRange),
                         [&](auto&& i) { this->operator[](i) *= c; });
            else if constexpr (Op == BasicArithOp::Div)
                rangeFor(DS::commonRange(this->assignableRange, this->localRange),
                         [&](auto&& i) { this->operator[](i) /= c; });
            else if constexpr (Op == BasicArithOp::Mod)
                rangeFor(DS::commonRange(this->assignableRange, this->localRange),
                         [&](auto&& i) { this->operator[](i) %= c; });
            else if constexpr (Op == BasicArithOp::And)
                rangeFor(DS::commonRange(this->assignableRange, this->localRange),
                         [&](auto&& i) { this->operator[](i) &= c; });
            else if constexpr (Op == BasicArithOp::Or)
                rangeFor(DS::commonRange(this->assignableRange, this->localRange),
                         [&](auto&& i) { this->operator[](i) |= c; });
            else if constexpr (Op == BasicArithOp::Xor)
                rangeFor(DS::commonRange(this->assignableRange, this->localRange),
                         [&](auto&& i) { this->operator[](i) ^= c; });
            else if constexpr (Op == BasicArithOp::LShift)
                rangeFor(DS::commonRange(this->assignableRange, this->localRange),
                         [&](auto&& i) { this->operator[](i) <<= c; });
            else if constexpr (Op == BasicArithOp::RShift)
                rangeFor(DS::commonRange(this->assignableRange, this->localRange),
                         [&](auto&& i) { this->operator[](i) >>= c; });
            else
                OP_NOT_IMPLEMENTED;

            this->updatePadding();
            return *this;
        }

        auto&
        initBy(const std::function<D(const std::array<Real, internal::CartesianMeshTrait<M>::dim>&)>& f) {
            rangeFor(DS::commonRange(this->assignableRange, this->localRange), [&](auto&& i) {
                std::array<Real, internal::CartesianMeshTrait<M>::dim> cords;
                for (auto k = 0; k < internal::CartesianMeshTrait<M>::dim; ++k)
                    cords[k] = this->loc[k] == LocOnMesh::Corner
                                       ? this->mesh.x(k, i)
                                       : this->mesh.x(k, i) + .5 * this->mesh.dx(k, i);
                this->operator[](i) = f(cords);
            });
            this->updatePadding();
            return *this;
        }

        void prepareImpl_final() const {}

        void updateNeighbors() {
            if (this->splitMap.size() == 1) {
                // single node mode
                this->neighbors.clear();
            } else {
#if defined(OPFLOW_WITH_MPI) && defined(OPFLOW_DISTRIBUTE_MODEL_MPI)
                int rank = getWorkerId();
                this->neighbors.clear();
                std::array<bool, dim> is_periodic;
                for (int d = 0; d < dim; ++d) {
                    is_periodic[d] = this->bc[d].start && this->bc[d].start->getBCType() == BCType::Periodic;
                }
                int range_count = Math::int_pow(3, std::count(is_periodic.begin(), is_periodic.end(), true));
                for (auto i = 0; i < this->splitMap.size(); ++i) {
                    auto mesh_range_extends = this->mesh.getRange().getExtends();
                    for (int k = 0; k < range_count; ++k) {
                        auto r = this->splitMap[i];
                        for (int d = 0; d < dim; ++d) {
                            int direction
                                    = (k % Math::int_pow(3, d + 1)) / Math::int_pow(3, d);// 0, 1(+), 2(-)
                            switch (direction) {
                                case 2:
                                    r.start[d] -= mesh_range_extends[d] - 1;
                                    r.end[d] -= mesh_range_extends[d] - 1;
                                    break;
                                default:
                                case 0:
                                    break;
                                case 1:
                                    r.start[d] += mesh_range_extends[d] - 1;
                                    r.end[d] += mesh_range_extends[d] - 1;
                                    break;
                            }
                        }
                        if (!(i == rank && r == this->localRange)) {
                            auto send_range
                                    = DS::commonRange(this->localRange, r.getInnerRange(-this->padding));
                            auto recv_range
                                    = DS::commonRange(this->localRange.getInnerRange(-this->padding), r);
                            if (send_range.count() > 0) {
                                this->neighbors.emplace_back(i, send_range, recv_range, k);
                            }
                        }
                    }
                }
#else
                OP_CRITICAL("MPI not provided.");
#endif
            }
        }

        void updatePaddingImpl_final() {
            // step 0: update dirc bc for corner case
            for (int i = 0; i < dim; ++i) {
                // lower side
                if (this->localRange.start[i] == this->accessibleRange.start[i] && this->bc[i].start
                    && this->bc[i].start->getBCType() == BCType::Dirc && this->loc[i] == LocOnMesh::Corner) {
                    auto r = this->localRange.slice(i, this->localRange.start[i]);
                    rangeFor(r, [&](auto&& idx) { this->operator()(idx) = this->bc[i].start->evalAt(idx); });
                }
                // upper side
                if (this->localRange.end[i] == this->accessibleRange.end[i] && this->bc[i].end
                    && this->bc[i].end->getBCType() == BCType::Dirc && this->loc[i] == LocOnMesh::Corner) {
                    auto r = this->localRange.slice(i, this->localRange.end[i] - 1);
                    rangeFor(r, [&](auto&& idx) { this->operator()(idx) = this->bc[i].end->evalAt(idx); });
                }
            }
            // step 1: update paddings by bc extension
            // start/end record the start/end index of last padding operation
            // the latter padding op pads the outer range of the former padding zone
            std::array<int, dim> start, end;
            if constexpr (requires(D v) {
                              { v + v }
                              ->std::same_as<D>;
                              { v - v }
                              ->std::same_as<D>;
                              { v * 1.0 }
                              ->std::same_as<D>;
                              { v / 1.0 }
                              ->std::same_as<D>;
                          }) {
                for (int i = 0; i < dim; ++i) {
                    // lower side
                    if (this->localRange.start[i] == this->accessibleRange.start[i] && this->bc[i].start
                        && this->bc[i].start->getBCType() != BCType::Periodic) {
                        start[i] = this->logicalRange.start[i];
                        // current padding zone
                        auto r = this->localRange;
                        for (int j = 0; j < i; ++j) {
                            r.start[j] = start[j];
                            r.end[j] = end[j];
                        }
                        r.start[i] = start[i];
                        r.end[i] = this->localRange.start[i];
                        if (this->loc[i] == LocOnMesh::Corner) {
                            switch (this->bc[i].start->getBCType()) {
                                case BCType::Dirc:
                                    // mid-point rule
                                    rangeFor(r, [&](auto&& idx) {
                                        auto bc_v = this->bc[i].start->evalAt(idx);
                                        auto mirror_idx = idx;
                                        mirror_idx[i] = 2 * this->localRange.start[i] - idx[i];
                                        this->operator()(idx) = Math::Interpolator1D::intp(
                                                this->mesh.x(i, this->localRange.start[i]), bc_v,
                                                this->mesh.x(i, mirror_idx[i]), this->evalAt(mirror_idx),
                                                this->mesh.x(i, idx[i]));
                                    });
                                    break;
                                case BCType::Neum:
                                    // mid-diff = bc
                                    rangeFor(r, [&](auto&& idx) {
                                        auto bc_v = this->bc[i].start->evalAt(idx);
                                        auto mirror_idx = idx;
                                        mirror_idx[i] = 2 * this->localRange.start[i] - idx[i];
                                        this->operator()(idx) = this->evalAt(mirror_idx)
                                                                + bc_v
                                                                          * (this->mesh.x(i, idx)
                                                                             - this->mesh.x(i, mirror_idx));
                                    });
                                    break;
                                case BCType::Symm:
                                    rangeFor(r, [&](auto&& idx) {
                                        auto mirror_idx = idx;
                                        mirror_idx[i] = 2 * this->localRange.start[i] - idx[i];
                                        this->operator()(idx) = this->evalAt(mirror_idx);
                                    });
                                    break;
                                case BCType::ASymm:
                                    rangeFor(r, [&](auto&& idx) {
                                        auto mirror_idx = idx;
                                        mirror_idx[i] = 2 * this->localRange.start[i] - idx[i];
                                        this->operator()(idx) = -this->evalAt(mirror_idx);
                                    });
                                    break;
                                default:
                                    OP_ERROR("Cannot handle current bc padding: bc type {}",
                                             this->bc[i].start->getTypeName());
                                    OP_ABORT;
                            }
                        } else {
                            // center case
                            switch (this->bc[i].start->getBCType()) {
                                case BCType::Dirc:
                                    // mid-point rule
                                    rangeFor(r, [&](auto&& idx) {
                                        auto bc_v = this->bc[i].start->evalAt(idx);
                                        auto mirror_idx = idx;
                                        mirror_idx[i] = 2 * this->localRange.start[i] - 1 - idx[i];
                                        this->operator()(idx) = Math::Interpolator1D::intp(
                                                this->mesh.x(i, this->localRange.start[i]), bc_v,
                                                this->mesh.x(i, mirror_idx[i])
                                                        + this->mesh.dx(i, mirror_idx) / 2.,
                                                this->evalAt(mirror_idx),
                                                this->mesh.x(i, idx[i]) + this->mesh.dx(i, idx) / 2.);
                                    });
                                    break;
                                case BCType::Neum:
                                    // mid-diff = bc
                                    rangeFor(r, [&](auto&& idx) {
                                        auto bc_v = this->bc[i].start->evalAt(idx);
                                        auto mirror_idx = idx;
                                        mirror_idx[i] = 2 * this->localRange.start[i] - 1 - idx[i];
                                        this->operator()(idx)
                                                = this->evalAt(mirror_idx)
                                                  + bc_v
                                                            * (this->mesh.x(i, idx)
                                                               + this->mesh.dx(i, idx) / 2.
                                                               - this->mesh.x(i, mirror_idx)
                                                               - this->mesh.dx(i, mirror_idx) / 2.);
                                    });
                                    break;
                                case BCType::Symm:
                                    rangeFor(r, [&](auto&& idx) {
                                        auto mirror_idx = idx;
                                        mirror_idx[i] = 2 * this->localRange.start[i] - 1 - idx[i];
                                        this->operator()(idx) = this->evalAt(mirror_idx);
                                    });
                                    break;
                                case BCType::ASymm:
                                    rangeFor(r, [&](auto&& idx) {
                                        auto mirror_idx = idx;
                                        mirror_idx[i] = 2 * this->localRange.start[i] - 1 - idx[i];
                                        this->operator()(idx) = -this->evalAt(mirror_idx);
                                    });
                                    break;
                                default:
                                    OP_ERROR("Cannot handle current bc padding: bc type {}",
                                             this->bc[i].start->getTypeName());
                                    OP_ABORT;
                            }
                        }
                    } else {
                        start[i] = this->localRange.start[i];
                    }

                    // upper side
                    if (this->localRange.end[i] == this->accessibleRange.end[i] && this->bc[i].end
                        && this->bc[i].end->getBCType() != BCType::Periodic) {
                        end[i] = this->logicalRange.end[i];
                        // current padding zone
                        auto r = this->localRange;
                        for (int j = 0; j < i; ++j) {
                            r.start[j] = start[j];
                            r.end[j] = end[j];
                        }
                        r.start[i] = this->localRange.end[i];
                        r.end[i] = this->logicalRange.end[i];
                        if (this->loc[i] == LocOnMesh::Corner) {
                            switch (this->bc[i].end->getBCType()) {
                                case BCType::Dirc:
                                    // mid-point rule
                                    rangeFor(r, [&](auto&& idx) {
                                        auto bc_v = this->bc[i].end->evalAt(idx);
                                        auto mirror_idx = idx;
                                        mirror_idx[i] = 2 * this->localRange.end[i] - 2 - idx[i];
                                        this->operator()(idx) = Math::Interpolator1D::intp(
                                                this->mesh.x(i, this->localRange.end[i] - 1), bc_v,
                                                this->mesh.x(i, mirror_idx[i]), this->evalAt(mirror_idx),
                                                this->mesh.x(i, idx[i]));
                                    });
                                    break;
                                case BCType::Neum:
                                    // mid-diff = bc
                                    rangeFor(r, [&](auto&& idx) {
                                        auto bc_v = this->bc[i].end->evalAt(idx);
                                        auto mirror_idx = idx;
                                        mirror_idx[i] = 2 * this->localRange.end[i] - 2 - idx[i];
                                        this->operator()(idx) = this->evalAt(mirror_idx)
                                                                + bc_v
                                                                          * (this->mesh.x(i, idx)
                                                                             - this->mesh.x(i, mirror_idx));
                                    });
                                    break;
                                case BCType::Symm:
                                    rangeFor(r, [&](auto&& idx) {
                                        auto mirror_idx = idx;
                                        mirror_idx[i] = 2 * this->localRange.end[i] - 2 - idx[i];
                                        this->operator()(idx) = this->evalAt(mirror_idx);
                                    });
                                    break;
                                case BCType::ASymm:
                                    rangeFor(r, [&](auto&& idx) {
                                        auto mirror_idx = idx;
                                        mirror_idx[i] = 2 * this->localRange.end[i] - 2 - idx[i];
                                        this->operator()(idx) = -this->evalAt(mirror_idx);
                                    });
                                    break;
                                default:
                                    OP_ERROR("Cannot handle current bc padding: bc type {}",
                                             this->bc[i].end->getTypeName());
                                    OP_ABORT;
                            }
                        } else {
                            // center case
                            switch (this->bc[i].end->getBCType()) {
                                case BCType::Dirc:
                                    // mid-point rule
                                    rangeFor(r, [&](auto&& idx) {
                                        auto bc_v = this->bc[i].end->evalAt(idx);
                                        auto mirror_idx = idx;
                                        mirror_idx[i] = 2 * this->localRange.end[i] - 1 - idx[i];
                                        this->operator()(idx) = Math::Interpolator1D::intp(
                                                this->mesh.x(i, this->localRange.end[i]), bc_v,
                                                this->mesh.x(i, mirror_idx[i])
                                                        + this->mesh.dx(i, mirror_idx) / 2.,
                                                this->evalAt(mirror_idx),
                                                this->mesh.x(i, idx[i]) + this->mesh.dx(i, idx) / 2.);
                                    });
                                    break;
                                case BCType::Neum:
                                    // mid-diff = bc
                                    rangeFor(r, [&](auto&& idx) {
                                        auto bc_v = this->bc[i].end->evalAt(idx);
                                        auto mirror_idx = idx;
                                        mirror_idx[i] = 2 * this->localRange.end[i] - 1 - idx[i];
                                        this->operator()(idx)
                                                = this->evalAt(mirror_idx)
                                                  + bc_v
                                                            * (this->mesh.x(i, idx)
                                                               + this->mesh.dx(i, idx) / 2.
                                                               - this->mesh.x(i, mirror_idx)
                                                               - this->mesh.dx(i, mirror_idx) / 2.);
                                    });
                                    break;
                                case BCType::Symm:
                                    rangeFor(r, [&](auto&& idx) {
                                        auto mirror_idx = idx;
                                        mirror_idx[i] = 2 * this->localRange.end[i] - 1 - idx[i];
                                        this->operator()(idx) = this->evalAt(mirror_idx);
                                    });
                                    break;
                                case BCType::ASymm:
                                    rangeFor(r, [&](auto&& idx) {
                                        auto mirror_idx = idx;
                                        mirror_idx[i] = 2 * this->localRange.end[i] - 1 - idx[i];
                                        this->operator()(idx) = -this->evalAt(mirror_idx);
                                    });
                                    break;
                                default:
                                    OP_ERROR("Cannot handle current bc padding: bc type {}",
                                             this->bc[i].end->getTypeName());
                                    OP_ABORT;
                            }
                        }
                    } else {
                        end[i] = this->localRange.end[i];
                    }
                }
            }

            // step 2: update paddings by MPI communication
            if (this->splitMap.size() == 1) {// no MPI or local field
                // update along periodic dims
                for (int i = 0; i < dim; ++i) {
                    if (this->bc[i].start && this->bc[i].start->getBCType() == BCType::Periodic) {
                        // issue an update
                        auto rl = this->logicalRange.slice(i, this->logicalRange.start[i],
                                                           this->accessibleRange.start[i]);
                        rangeFor(rl, [&](auto&& idx) {
                            auto mirror_idx = idx;
                            mirror_idx[i] += this->accessibleRange.end[i] - this->accessibleRange.start[i];
                            this->operator()(idx) = this->operator()(mirror_idx);
                        });
                        auto rh = this->logicalRange.slice(i, this->accessibleRange.end[i],
                                                           this->logicalRange.end[i]);
                        rangeFor(rh, [&](auto&& idx) {
                            auto mirror_idx = idx;
                            mirror_idx[i] -= this->accessibleRange.end[i] - this->accessibleRange.start[i];
                            this->operator()(idx) = this->operator()(mirror_idx);
                        });
                    }
                }
            } else {
#if defined(OPFLOW_WITH_MPI) && defined(OPFLOW_DISTRIBUTE_MODEL_MPI)
                int rank = getWorkerId();
                // one of the following pairs of buffer is used
                std::vector<std::vector<D>> send_buff, recv_buff;
                std::vector<std::vector<std::byte>> send_buff_byte, recv_buff_byte;
                std::vector<std::vector<int>> send_buff_offsets, recv_buff_offsets;
                std::vector<MPI_Request> requests;
                auto padded_my_range = this->localRange.getInnerRange(-this->padding);
                auto mesh_range_extends = this->mesh.getRange().getExtends();

                for (const auto& [other_rank, send_range, recv_range, code] : this->neighbors) {
                    // calculate the intersect range to be send
                    if constexpr (std::is_trivial_v<D> && std::is_standard_layout_v<D>) {
                        // pack data into send buffer
                        send_buff.emplace_back();
                        requests.emplace_back();
                        rangeFor_s(send_range, [&](auto&& i) {
                            OP_DEBUG("Send {} = {}", i, this->operator()(i));
                            send_buff.back().push_back(this->evalAt(i));
                        });
                    } else if constexpr (Serializable<D>) {
                        // pack data into send buffer
                        send_buff_byte.emplace_back();
                        send_buff_offsets.emplace_back();
                        send_buff_offsets.back().push_back(0);
                        requests.emplace_back();
                        rangeFor_s(send_range, [&](auto&& i) {
                            OP_DEBUG("Send {} = {}", i, this->operator()(i));
                            std::vector<std::byte> tmp = this->operator()(i).serialize();
                            send_buff_byte.back().insert(send_buff_byte.back().end(), tmp.begin(), tmp.end());
                            send_buff_offsets.back().push_back((int) send_buff_byte.back().size());
                        });
                    } else {
                        OP_ERROR("Datatype cannot be serialized.");
                        OP_ABORT;
                    }
                    OP_DEBUG("Send range {} from rank {} to rank {}", send_range.toString(), rank,
                             other_rank);
                    auto o_recv_range = send_range;
                    // inverse the shift to get the recv range on the receiver side
                    for (int d = 0; d < dim; ++d) {
                        int direction
                                = (code % Math::int_pow(3, d + 1)) / Math::int_pow(3, d);// 0, 1(+), 2(-)
                        switch (direction) {
                            case 1:
                                o_recv_range.start[d] -= mesh_range_extends[d] - 1;
                                o_recv_range.end[d] -= mesh_range_extends[d] - 1;
                                break;
                            default:
                            case 0:
                                break;
                            case 2:
                                o_recv_range.start[d] += mesh_range_extends[d] - 1;
                                o_recv_range.end[d] += mesh_range_extends[d] - 1;
                                break;
                        }
                    }
                    if constexpr (std::is_trivial_v<D> && std::is_standard_layout_v<D>)
                        MPI_Isend(send_buff.back().data(), send_buff.back().size() * sizeof(D) / sizeof(char),
                                  MPI_CHAR, other_rank,
                                  std::hash<Meta::RealType<decltype(o_recv_range)>> {}(o_recv_range)
                                          % (1 << 24),
                                  MPI_COMM_WORLD, &requests.back());
                    else if constexpr (Serializable<D>) {
                        MPI_Isend(send_buff_offsets.back().data(), send_buff_offsets.back().size(), MPI_INT,
                                  other_rank, rank, MPI_COMM_WORLD, &requests.back());
                        requests.template emplace_back();
                        MPI_Isend(send_buff_byte.back().data(), send_buff_byte.back().size(), MPI_BYTE,
                                  other_rank,
                                  std::hash<Meta::RealType<decltype(o_recv_range)>> {}(o_recv_range)
                                          % (1 << 24),
                                  MPI_COMM_WORLD, &requests.back());
                    }

                    // calculate the intersect range to be received
                    requests.emplace_back();
                    // issue receive request
                    OP_DEBUG("Recv range {} from rank {} to rank {}", recv_range.toString(), other_rank,
                             rank);
                    if constexpr (std::is_trivial_v<D> && std::is_standard_layout_v<D>) {
                        recv_buff.emplace_back();
                        recv_buff.back().resize(recv_range.count());
                        MPI_Irecv(recv_buff.back().data(), recv_buff.back().size() * sizeof(D) / sizeof(char),
                                  MPI_CHAR, other_rank,
                                  std::hash<Meta::RealType<decltype(recv_range)>> {}(recv_range) % (1 << 24),
                                  MPI_COMM_WORLD, &requests.back());
                    } else if constexpr (Serializable<D>) {
                        recv_buff_offsets.emplace_back(recv_range.count() + 1);
                        MPI_Recv(recv_buff_offsets.back().data(), recv_buff_offsets.back().size(), MPI_INT,
                                 other_rank, other_rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        recv_buff_byte.emplace_back(recv_buff_offsets.back().back());
                        MPI_Irecv(recv_buff_byte.back().data(), recv_buff_byte.back().size(), MPI_BYTE,
                                  other_rank,
                                  std::hash<Meta::RealType<decltype(recv_range)>> {}(recv_range) % (1 << 24),
                                  MPI_COMM_WORLD, &requests.back());
                    }
                }
                // wait all communication done
                std::vector<MPI_Status> status(requests.size());
                MPI_Waitall(requests.size(), requests.data(), status.data());
                // check status
                for (const auto& s : status) {
                    if (s.MPI_ERROR != MPI_SUCCESS)
                        OP_CRITICAL("Field {}'s updatePadding failed.", this->getName());
                }
                // unpack receive buffer
                if constexpr (std::is_trivial_v<D> && std::is_standard_layout_v<D>) {
                    auto recv_iter = recv_buff.begin();
                    for (const auto& [other_rank, send_range, recv_range, code] : this->neighbors) {
                        auto _iter = recv_iter->begin();
                        OP_DEBUG("Unpacking range {}", recv_range.toString());
                        rangeFor_s(recv_range, [&](auto&& i) {
                            OP_DEBUG("Unpack {} = {}", i, *_iter);
                            this->operator()(i) = *_iter++;
                        });
                        ++recv_iter;
                    }
                } else if constexpr (Serializable<D>) {
                    auto recv_iter = recv_buff_byte.begin();
                    auto recv_offset_iter = recv_buff_offsets.begin();
                    for (const auto& [other_rank, send_range, recv_range, code] : this->neighbors) {
                        auto _iter = recv_iter->begin();
                        auto _offset_iter = recv_offset_iter->begin();
                        OP_DEBUG("Unpacking range {}", recv_range.toString());
                        rangeFor_s(recv_range, [&](auto&& i) {
                            //OP_INFO("Unpack {} = {}", i, *(char*)&*_iter);
                            this->operator()(i).deserialize(&(*_iter) + *_offset_iter,
                                                            *(_offset_iter + 1) - *_offset_iter);
                            _offset_iter++;
                        });
                        ++recv_iter;
                        ++recv_offset_iter;
                    }
                }
#else
                OP_CRITICAL("MPI not provided.");
#endif
            }
        }

        auto getViewImpl_final() {
            OP_NOT_IMPLEMENTED;
            return 0;
        }
        const auto& evalAtImpl_final(const index_type& i) const {
            OP_ASSERT_MSG(DS::inRange(this->getLocalReadableRange(), i),
                          "Cannot eval {} at {}: out of range {}", this->getName(), i,
                          this->getLocalReadableRange().toString());
            return data[i - this->offset];
        }
        auto& evalAtImpl_final(const index_type& i) {
            OP_ASSERT_MSG(DS::inRange(this->getLocalReadableRange(), i),
                          "Cannot eval {} at {}: out of range {}", this->getName(), i,
                          this->getLocalWritableRange().toString());
            return data[i - this->offset];
        }

        template <typename Other>
        requires(!std::same_as<Other, CartesianField>) bool containsImpl_final(const Other& o) const {
            return false;
        }

        bool containsImpl_final(const CartesianField& other) const { return this == &other; }
    };// namespace OpFlow

    template <typename D, typename M, typename C>
    struct ExprBuilder<CartesianField<D, M, C>> {
        using Field = CartesianField<D, M, C>;
        using Mesh = M;
        static constexpr auto dim = internal::CartesianFieldExprTrait<Field>::dim;
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
                    targetBC = std::make_unique<SymmBC<CartesianField<D, M, C>>>(f, d, pos);
                    break;
                case BCType::ASymm:
                    targetBC = std::make_unique<ASymmBC<CartesianField<D, M, C>>>(f, d, pos);
                    break;
                case BCType::Periodic:
                    targetBC = std::make_unique<PeriodicBC<CartesianField<D, M, C>>>(f, d, pos);
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
            if (isLogicalBC(type)) return setBC(d, pos, type);
            auto& targetBC = pos == DimPos::start ? f.bc[d].start : f.bc[d].end;
            switch (type) {
                case BCType::Dirc:
                    targetBC = std::make_unique<ConstDircBC<CartesianField<D, M, C>>>(val);
                    break;
                case BCType::Neum:
                    targetBC = std::make_unique<ConstNeumBC<CartesianField<D, M, C>>>(val);
                    break;
                default:
                    OP_ERROR("BC Type not supported.");
                    OP_ABORT;
            }
            return *this;
        }

        // set a functor bc
        template <typename F>
        requires requires(F f) {
            { f(std::declval<typename internal::ExprTrait<CartesianField<D, M, C>>::index_type>()) }
            ->std::convertible_to<typename internal::ExprTrait<CartesianField<D, M, C>>::elem_type>;
        }
        auto& setBC(int d, DimPos pos, BCType type, F&& functor) {
            OP_ASSERT(d < dim);
            auto& targetBC = pos == DimPos::start ? f.bc[d].start : f.bc[d].end;
            switch (type) {
                case BCType::Dirc:
                    targetBC = std::make_unique<FunctorDircBC<CartesianField<D, M, C>>>(functor);
                    break;
                case BCType::Neum:
                    targetBC = std::make_unique<FunctorNeumBC<CartesianField<D, M, C>>>(functor);
                    break;
                default:
                    OP_ERROR("BC Type not supported.");
                    OP_ABORT;
            }
            return *this;
        }

        // set an externally built bc
        auto& setBC(int d, DimPos pos, std::unique_ptr<BCBase<CartesianField<D, M, C>>>&& bc) {
            OP_ASSERT(d < dim);
            auto& targetBC = pos == DimPos::start ? f.bc[d].start : f.bc[d].end;
            targetBC = std::move(bc);
            return *this;
        }

        auto& setExt(int d, DimPos pos, int width) {
            if (pos == DimPos::start) {
                f.ext_width[d].start = width;
            } else {
                f.ext_width[d].end = width;
            }
            return *this;
        }

        auto& setExt(int width) {
            // a uniform ext
            for (auto& e : f.ext_width) {
                e.start = width;
                e.end = width;
            }
            return *this;
        }

        auto& setPadding(int p) {
            f.padding = p;
            return *this;
        }

        auto& setSplitStrategy(std::shared_ptr<AbstractSplitStrategy<CartesianField<D, M, C>>> s) {
            strategy = s;
            return *this;
        }

        auto& build() {
            calculateRanges();
            validateRanges();
            OP_ASSERT(f.localRange.check() && f.accessibleRange.check() && f.assignableRange.check());
            f.data.reShape(f.localRange.getInnerRange(-f.padding).getExtends());
            f.offset = typename internal::CartesianFieldExprTrait<Field>::index_type(
                    f.localRange.getInnerRange(-f.padding).getOffset());
            f.updatePadding();
            return f;
        }

    private:
        void validateRanges() {
            // accessibleRange <= logicalRange
            f.accessibleRange = commonRange(f.accessibleRange, f.logicalRange);
            // localRange <= logicalRange
            f.localRange = commonRange(f.localRange, f.logicalRange);
            // assignableRange <= accessibleRange
            f.assignableRange = commonRange(f.assignableRange, f.accessibleRange);
        }

        void calculateRanges() {
            // padding take the max of {padding, ext_width}
            for (const auto& w : f.ext_width) f.padding = std::max({f.padding, w.start, w.end});
            // init ranges to mesh range
            f.logicalRange = f.assignableRange = f.localRange = f.accessibleRange = f.mesh.getRange();
            // trim ranges according to bcs
            for (auto i = 0; i < dim; ++i) {
                auto loc = f.loc[i];
                // start side
                auto type = f.bc[i].start ? f.bc[i].start->getBCType() : BCType::Undefined;
                switch (type) {
                    case BCType::Dirc:
                        if (loc == LocOnMesh::Corner) f.assignableRange.start[i]++;
                        break;
                    case BCType::Neum:
                    case BCType::Undefined:
                    case BCType::Symm:
                    case BCType::ASymm:
                    case BCType::Periodic:
                        break;
                    default:
                        OP_NOT_IMPLEMENTED;
                }
                // end side
                type = f.bc[i].end ? f.bc[i].end->getBCType() : BCType::Undefined;
                switch (type) {
                    case BCType::Dirc:
                        if (loc == LocOnMesh::Corner) {
                            f.assignableRange.end[i]--;
                        } else if (loc == LocOnMesh::Center) {
                            f.accessibleRange.end[i]--;
                            f.assignableRange.end[i]--;
                        }
                        break;
                    case BCType::Neum:
                    case BCType::Undefined:
                    case BCType::Symm:
                    case BCType::ASymm:
                        if (loc == LocOnMesh::Center) {
                            f.accessibleRange.end[i]--;
                            f.assignableRange.end[i]--;
                        }
                        break;
                    case BCType::Periodic:
                        // same as center case
                        f.accessibleRange.end[i]--;
                        f.assignableRange.end[i]--;
                        break;
                    default:
                        OP_NOT_IMPLEMENTED;
                }
                f.logicalRange.start[i] = f.accessibleRange.start[i] - f.ext_width[i].start;
                f.logicalRange.end[i] = f.accessibleRange.end[i] + f.ext_width[i].end;
            }
            // calculate localRange
            if (strategy) {
                // always split the mesh range to make sure associated fields shares the same split
                // note: the returned local range is in centered mode
                f.localRange = strategy->splitRange(f.mesh.getRange(), getGlobalParallelPlan());
                f.splitMap = strategy->getSplitMap(f.mesh.getRange(), getGlobalParallelPlan());
                // adjust the local range according to the location & bc
                for (auto i = 0; i < dim; ++i) {
                    auto loc = f.loc[i];
                    // we only need to consider the end side for whether taken the right boundary into account
                    // only +1 if the block is at the right end
                    // periodic bc can be trimmed by the min operation
                    if (loc == LocOnMesh::Corner && f.localRange.end[i] == f.mesh.getRange().end[i] - 1)
                        f.localRange.end[i] = std::min(f.localRange.end[i] + 1, f.accessibleRange.end[i]);
                    for (auto& range : f.splitMap) {
                        if (loc == LocOnMesh::Corner && range.end[i] == f.mesh.getRange().end[i] - 1)
                            range.end[i] = std::min(range.end[i] + 1, f.accessibleRange.end[i]);
                    }
                }
            } else {
                f.localRange = f.accessibleRange;
                f.splitMap.clear();
                f.splitMap.push_back(f.localRange);
            }
            f.updateNeighbors();
        }

        CartesianField<D, M, C> f;
        std::shared_ptr<AbstractSplitStrategy<CartesianField<D, M, C>>> strategy;
    };

}// namespace OpFlow

namespace std {
    template <typename D, typename M, typename C>
    void swap(OpFlow::CartesianField<D, M, C>& a, OpFlow::CartesianField<D, M, C>& b) {
        std::swap(a.data, b.data);
    }
}// namespace std

#endif//OPFLOW_CARTESIANFIELD_HPP
