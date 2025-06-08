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

#ifndef OPFLOW_STRUCTUREDFIELDEXPR_HPP
#define OPFLOW_STRUCTUREDFIELDEXPR_HPP

#include "Core/Field/MeshBased/MeshBasedFieldExpr.hpp"
#include "DataStructures/Pair.hpp"
#include "StructuredFieldExprTrait.hpp"
#ifndef OPFLOW_INSIDE_MODULE
#include <array>
#include <memory>
#include <utility>
#include <vector>
#endif

OPFLOW_MODULE_EXPORT namespace OpFlow {
    namespace internal {
        template <typename R>
        struct NeighborInfo {
            NeighborInfo() = default;
            NeighborInfo(int rank, R send, R recv, int code)
                : rank(rank), send_range(std::move(send)), recv_range(std::move(recv)), shift_code(code) {}
            int rank;
            R send_range, recv_range;
            int shift_code;
        };
    }// namespace internal

    template <typename Derived>
    struct StructuredFieldExpr : MeshBasedFieldExpr<Derived> {
        mutable std::array<LocOnMesh, internal::StructuredFieldExprTrait<Derived>::dim> loc;
        using RangeType = typename internal::StructuredFieldExprTrait<Derived>::range_type;
        using IndexType = typename internal::StructuredFieldExprTrait<Derived>::index_type;
        mutable RangeType localRange;           ///< local accessible range, unextended range
        mutable RangeType assignableRange;      ///< global assignable range
        mutable RangeType accessibleRange;      ///< global accessible range, unextended range
        mutable RangeType logicalRange;         ///< logical accessible range, extended range
        mutable IndexType offset;               ///< index offset for distributed parallelization
        mutable int padding = 0;                ///< padding width for distributed parallelization
        mutable std::vector<RangeType> splitMap;///< Map of rank to range for distributed parallelization
        mutable std::vector<internal::NeighborInfo<RangeType>>
                neighbors;///< Neighbor patches rank & range info

        StructuredFieldExpr() = default;
        StructuredFieldExpr(const StructuredFieldExpr& other)
            : MeshBasedFieldExpr<Derived>(other), loc(other.loc), localRange(other.localRange),
              assignableRange(other.assignableRange), accessibleRange(other.accessibleRange),
              logicalRange(other.logicalRange), offset(other.offset), padding(other.padding),
              splitMap(other.splitMap), neighbors(other.neighbors) {}
        StructuredFieldExpr(StructuredFieldExpr&& other) noexcept
            : MeshBasedFieldExpr<Derived>(std::move(other)), loc(std::move(other.loc)),
              localRange(std::move(other.localRange)), assignableRange(std::move(other.assignableRange)),
              accessibleRange(std::move(other.accessibleRange)), logicalRange(std::move(other.logicalRange)),
              offset(std::move(other.offset)), padding(other.padding), splitMap(std::move(other.splitMap)),
              neighbors(std::move(other.neighbors)) {}

        auto getDims() const { return this->mesh.getDims(); }
        auto getOffset() const { return this->offset; }
        void updatePadding() { this->derived().updatePaddingImpl_final(); }

        auto getLocalReadableRange() const {
            return DS::commonRange(this->localRange.getInnerRange(-padding), this->logicalRange);
        }
        auto getLocalWritableRange() const {
            return DS::commonRange(this->localRange, this->assignableRange);
        }
        auto getGlobalReadableRange() const { return this->logicalRange; }
        auto getGlobalWritableRange() const { return this->assignableRange; }

    protected:
        template <StructuredFieldExprType Other>
        void initPropsFromImpl_StructuredFieldExpr(const Other& other) const {
            this->initPropsFromImpl_MeshBasedFieldExpr(other);
            this->loc = other.loc;
            this->localRange = other.localRange;
            this->assignableRange = other.assignableRange;
            this->accessibleRange = other.accessibleRange;
            this->logicalRange = other.logicalRange;
            this->offset = other.offset;
            this->padding = other.padding;
            this->splitMap = other.splitMap;
            this->neighbors = other.neighbors;
        }

        bool couldEvalAtImpl_final(auto&& i) const {
            return DS::inRange(DS::commonRange(localRange.getInnerRange(-padding), logicalRange), i);
        }

    private:
        DEFINE_CRTP_HELPERS(Derived)
    };
}// namespace OpFlow
#endif//OPFLOW_STRUCTUREDFIELDEXPR_HPP
