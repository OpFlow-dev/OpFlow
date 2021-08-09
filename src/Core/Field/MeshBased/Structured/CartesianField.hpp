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
        index_type offset;

    public:
        friend ExprBuilder<CartesianField>;
        CartesianField() = default;
        CartesianField(const CartesianField& other)
            : CartesianFieldExpr<CartesianField<D, M, C>>(other), data(other.data), offset(other.offset) {}
        CartesianField(CartesianField&& other) noexcept
            : CartesianFieldExpr<CartesianField<D, M, C>>(std::move(other)), data(std::move(other.data)),
              offset(std::move(other.offset)) {}

        auto& operator=(const CartesianField& other) {
            if (this != &other && this->accessibleRange == other.accessibleRange) {
                // only data is assigned
                data = other.data;
            } else {
                internal::FieldAssigner::assign(other, *this);
            }
            return *this;
        }

        template <CartesianFieldExprType T>
        auto& operator=(T&& other) {// T is not const here for that we need to call other.prepare() later
            if ((void*) this != (void*) &other) {
                // assign all values from T to assignable range
                internal::FieldAssigner::assign(other, *this);
            }
            return *this;
        }
        auto& operator=(const D& c) {
            rangeFor(this->assignableRange, [&](auto&& i) { this->operator[](i) = c; });
            return *this;
        }

        auto&
        initBy(const std::function<D(const std::array<Real, internal::CartesianMeshTrait<M>::dim>&)>& f) {
            rangeFor(this->assignableRange, [&](auto&& i) {
                std::array<Real, internal::CartesianMeshTrait<M>::dim> cords;
                for (auto k = 0; k < internal::CartesianMeshTrait<M>::dim; ++k)
                    cords[k] = this->loc[k] == LocOnMesh::Corner
                                       ? this->mesh.x(k, i)
                                       : this->mesh.x(k, i) + .5 * this->mesh.dx(k, i);
                this->operator[](i) = f(cords);
            });
            return *this;
        }
        void prepare() {}
        void updateBC() {
            if (this->localRange == this->assignableRange) return;
            else {
                for (auto i = 0; i < CartesianField::dim; ++i) {
                    if (this->loc[i] == LocOnMesh::Center) continue;// only nodal dims needs to be update
                    auto type = this->bc[i].start ? this->bc[i].start->getBCType() : BCType::Undefined;
                    switch (type) {
                        case BCType::Dirc:
                            rangeFor_s(
                                    DS::commonRange(
                                            this->accessibleRange.slice(i, this->accessibleRange.start[i]),
                                            this->localRange),
                                    [&](auto&& pos) { data[pos - offset] = this->bc[i].start->evalAt(pos); });
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
                            rangeFor_s(
                                    DS::commonRange(
                                            this->accessibleRange.slice(i, this->accessibleRange.end[i] - 1),
                                            this->localRange),
                                    [&](auto&& pos) { data[pos - offset] = this->bc[i].end->evalAt(pos); });
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

        auto getView() {
            OP_NOT_IMPLEMENTED;
            return 0;
        }
        auto& operator()(const index_type& i) { return data[i - offset]; }
        auto& operator[](const index_type& i) { return data[i - offset]; }
        const auto& operator()(const index_type& i) const { return data[i - offset]; }
        const auto& operator[](const index_type& i) const { return data[i - offset]; }
        const auto& evalAt(const index_type& i) const { return data[i - offset]; }
        const auto& evalSafeAt(const index_type& i) const { return data[i - offset]; }

        template <typename Other>
        requires(!std::same_as<Other, CartesianField>) bool contains(const Other& o) const { return false; }

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
                    targetBC = std::make_unique<SymmBC<CartesianField<D, M, C>>>();
                    break;
                case BCType::ASymm:
                    targetBC = std::make_unique<ASymmBC<CartesianField<D, M, C>>>();
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

        auto& setParallelPlan(ParallelPlan parallelPlan) {
            this->plan = parallelPlan;
            return *this;
        }

        auto& setPadding(int p) {
            padding = p;
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
            f.data.reShape(f.localRange.getExtends());
            f.offset =
                    typename internal::CartesianFieldExprTrait<Field>::index_type(f.localRange.getOffset());
            f.updateBC();
            return f;
        }

    private:
        void validateRanges() {
            f.accessibleRange = commonRange(f.accessibleRange, f.mesh.getRange());
            f.localRange = commonRange(f.localRange, f.accessibleRange);
            f.assignableRange = commonRange(f.assignableRange, f.localRange);
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
                        if (loc == LocOnMesh::Center) {
                            f.accessibleRange.end[i]--;
                            f.assignableRange.end[i]--;
                        }
                        break;
                    default:
                        OP_NOT_IMPLEMENTED;
                }
            }
            // calculate localRange
            f.localRange = strategy->splitRange(f.accessibleRange, padding, plan);
        }

        CartesianField<D, M, C> f;
        ParallelPlan plan;
        int padding = 0;
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
