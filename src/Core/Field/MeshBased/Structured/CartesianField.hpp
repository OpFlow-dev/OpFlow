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

#ifndef OPFLOW_CARTESIANFIELD_HPP
#define OPFLOW_CARTESIANFIELD_HPP

#include "CartesianFieldExpr.hpp"
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
#include <memory>

namespace OpFlow {

    template <typename D, typename M, typename C = DS::PlainTensor<D, internal::MeshTrait<M>::dim>>
    struct CartesianField : CartesianFieldExpr<CartesianField<D, M, C>> {
        using index_type = typename internal::CartesianFieldExprTrait<CartesianField>::index_type;

    private:
        C data;

    public:
        friend ExprBuilder<CartesianField>;
        CartesianField() = default;
        CartesianField(const CartesianField& other)
            : CartesianFieldExpr<CartesianField<D, M, C>>(other), data(other.data) {}
        CartesianField(CartesianField&& other) noexcept
            : CartesianFieldExpr<CartesianField<D, M, C>>(std::move(other)), data(std::move(other.data)) {}

        auto& operator=(const CartesianField& other) {
            if (this != &other) {
                internal::FieldAssigner::assign(other, *this);
                updatePadding();
            }
            return *this;
        }

        template <CartesianFieldExprType T>
        auto& operator=(T&& other) {// T is not const here for that we need to call other.prepare() later
            if ((void*) this != (void*) &other) {
                // assign all values from T to assignable range
                internal::FieldAssigner::assign(other, *this);
                updatePadding();
            }
            return *this;
        }
        auto& operator=(const D& c) {
            rangeFor(DS::commonRange(this->assignableRange, this->localRange),
                     [&](auto&& i) { this->operator[](i) = c; });
            updatePadding();
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
            updatePadding();
            return *this;
        }
        void prepare() {}
        void updateNeighbors() {
            if (this->splitMap.size() == 1) {
                // single node mode
                this->neighbors.clear();
            } else {
#if defined(OPFLOW_WITH_MPI) && defined(OPFLOW_DISTRIBUTE_MODEL_MPI)
                int rank;
                MPI_Comm_rank(MPI_COMM_WORLD, &rank);
                this->neighbors.clear();
                for (auto i = 0; i < this->splitMap.size(); ++i) {
                    if (i == rank) continue;
                    // test if two range intersect
                    auto padded_my_range = this->localRange.getInnerRange(-this->padding);
                    auto padded_other_range = this->splitMap[i].getInnerRange(-this->padding);
                    auto p_my_intersect_other = DS::commonRange(padded_my_range, this->splitMap[i]);
                    auto p_other_intersect_my = DS::commonRange(padded_other_range, this->localRange);
                    OP_ASSERT(p_my_intersect_other.count() == p_other_intersect_my.count());
                    if (p_my_intersect_other.count() > 0) {
                        this->neighbors.emplace_back(i, this->splitMap[i]);
                    }
                }
#else
                OP_CRITICAL("MPI not provided.");
#endif
            }
        }
        void updateBC() {
            if (this->localRange == this->assignableRange) return;
            else {
                for (auto i = 0; i < CartesianField::dim; ++i) {
                    if (this->loc[i] == LocOnMesh::Center) continue;// only nodal dims needs to be update
                    auto type = this->bc[i].start ? this->bc[i].start->getBCType() : BCType::Undefined;
                    switch (type) {
                        case BCType::Dirc:
                            rangeFor_s(DS::commonRange(
                                               this->accessibleRange.slice(i, this->accessibleRange.start[i]),
                                               this->localRange),
                                       [&](auto&& pos) {
                                           data[pos - this->offset] = this->bc[i].start->evalAt(pos);
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
                            rangeFor_s(DS::commonRange(this->accessibleRange.slice(
                                                               i, this->accessibleRange.end[i] - 1),
                                                       this->localRange),
                                       [&](auto&& pos) {
                                           data[pos - this->offset] = this->bc[i].end->evalAt(pos);
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
        void updatePadding() {
            auto plan = getGlobalParallelPlan();
            if (plan.singleNodeMode()) return;
            else {
#if defined(OPFLOW_WITH_MPI) && defined(OPFLOW_DISTRIBUTE_MODEL_MPI)
                int rank;
                MPI_Comm_rank(MPI_COMM_WORLD, &rank);
                std::vector<std::vector<D>> send_buff, recv_buff;
                std::vector<MPI_Request> requests;
                auto padded_my_range = this->localRange.getInnerRange(-this->padding);
                for (const auto& [other_rank, range] : this->neighbors) {
                    // calculate the intersect range to be send
                    auto send_range = DS::commonRange(range.getInnerRange(-this->padding), this->localRange);
                    // pack data into send buffer
                    send_buff.emplace_back();
                    requests.emplace_back();
                    rangeFor_s(send_range, [&](auto&& i) { send_buff.back().push_back(evalAt(i)); });
                    MPI_Isend(send_buff.back().data(), send_buff.back().size() * sizeof(D) / sizeof(char),
                              MPI_CHAR, other_rank, rank, MPI_COMM_WORLD, &requests.back());

                    // calculate the intersect range to be received
                    auto recv_range = DS::commonRange(range, this->localRange.getInnerRange(-this->padding));
                    recv_buff.emplace_back();
                    requests.emplace_back();
                    recv_buff.back().resize(recv_range.count());
                    // issue receive request
                    MPI_Irecv(recv_buff.back().data(), recv_buff.back().size() * sizeof(D) / sizeof(char),
                              MPI_CHAR, other_rank, other_rank, MPI_COMM_WORLD, &requests.back());
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
                auto recv_iter = recv_buff.begin();
                for (const auto& [other_rank, range] : this->neighbors) {
                    auto _iter = recv_iter->begin();
                    auto recv_range = DS::commonRange(this->localRange.getInnerRange(-this->padding), range);
                    rangeFor_s(recv_range, [&](auto&& i) { this->operator()(i) = *_iter++; });
                    ++recv_iter;
                }
#else
                OP_CRITICAL("MPI not provided.");
#endif
            }
        }

        auto getView() {
            OP_NOT_IMPLEMENTED;
            return 0;
        }
        auto& operator()(const index_type& i) { return data[i - this->offset]; }
        auto& operator[](const index_type& i) { return data[i - this->offset]; }
        const auto& operator()(const index_type& i) const { return data[i - this->offset]; }
        const auto& operator[](const index_type& i) const { return data[i - this->offset]; }
        const auto& evalAt(const index_type& i) const { return data[i - this->offset]; }
        const auto& evalSafeAt(const index_type& i) const { return data[i - this->offset]; }

        template <typename Other>
        requires(!std::same_as<Other, CartesianField>) bool contains(const Other& o) const {
            return false;
        }

        bool contains(const CartesianField& other) const { return this == &other; }
    };

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
            f.updateBC();
            return f;
        }

    private:
        void validateRanges() {
            f.accessibleRange = commonRange(f.accessibleRange, f.mesh.getRange());
            f.localRange = commonRange(f.localRange, f.accessibleRange);
        }

        void calculateRanges() {
            // init ranges to mesh range
            f.assignableRange = f.localRange = f.accessibleRange = f.mesh.getRange();
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
