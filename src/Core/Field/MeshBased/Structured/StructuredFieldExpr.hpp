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

#ifndef OPFLOW_STRUCTUREDFIELDEXPR_HPP
#define OPFLOW_STRUCTUREDFIELDEXPR_HPP

#include "Core/BC/BCBase.hpp"
#include "Core/Field/MeshBased/MeshBasedFieldExpr.hpp"
#include "DataStructures/Pair.hpp"
#include "StructuredFieldExprTrait.hpp"
#include <array>
#include <memory>
#include <utility>
#include <vector>

namespace OpFlow {

    template <typename Derived>
    struct StructuredFieldExpr : MeshBasedFieldExpr<Derived> {
        std::array<LocOnMesh, internal::StructuredFieldExprTrait<Derived>::dim> loc;
        using RangeType = typename internal::StructuredFieldExprTrait<Derived>::range_type;
        using IndexType = typename internal::StructuredFieldExprTrait<Derived>::index_type;
        RangeType localRange;     ///< local accessible range
        RangeType assignableRange;///< global assignable range
        RangeType accessibleRange;///< global accessible range
        IndexType offset;         ///< index offset for distributed parallelization
        int padding = 0;          ///< padding width for distributed parallelization
        std::vector<RangeType> splitMap;///< Map of rank to range for distributed parallelization
        std::vector<std::pair<int, RangeType>> neighbors;///< Neighbor patches rank & range info

        std::array<
                DS::Pair<std::unique_ptr<BCBase<typename internal::StructuredFieldExprTrait<Derived>::type>>>,
                internal::FieldExprTrait<Derived>::dim>
                bc;

        StructuredFieldExpr() = default;
        StructuredFieldExpr(const StructuredFieldExpr& other)
            : MeshBasedFieldExpr<Derived>(other), localRange(other.localRange),
              assignableRange(other.assignableRange), accessibleRange(other.accessibleRange),
              offset(other.offset), padding(other.padding), loc(other.loc), splitMap(other.splitMap),
              neighbors(other.neighbors) {
            for (auto i = 0; i < bc.size(); ++i) {
                bc[i].start = other.bc[i].start ? other.bc[i].start->getCopy() : nullptr;
                bc[i].end = other.bc[i].end ? other.bc[i].end->getCopy() : nullptr;
            }
        }
        StructuredFieldExpr(StructuredFieldExpr&& other) noexcept
            : MeshBasedFieldExpr<Derived>(std::move(other)), localRange(std::move(other.localRange)),
              assignableRange(std::move(other.assignableRange)),
              accessibleRange(std::move(other.accessibleRange)), offset(std::move(other.offset)),
              padding(other.padding), bc(std::move(other.bc)), loc(std::move(other.loc)),
              splitMap(std::move(other.splitMap)), neighbors(std::move(other.neighbors)) {}

        auto getDims() const { return this->mesh.getDims(); }
        auto getOffset() const { return this->offset; }
        void updateBC() { this->derived().updateBC(); }
        void updatePadding() { this->derived().updatePadding(); }
        template <StructuredFieldExprType Other>
        void initPropsFrom(const Other& other) {
            static_cast<MeshBasedFieldExpr<Derived>*>(this)->template initPropsFrom(other);
            this->loc = other.loc;
            this->localRange = other.localRange;
            this->assignableRange = other.assignableRange;
            this->accessibleRange = other.accessibleRange;
            this->offset = other.offset;
            this->padding = other.padding;
            this->splitMap = other.splitMap;
            this->neighbors = other.neighbors;
            for (auto i = 0; i < internal::FieldExprTrait<Derived>::dim; ++i) {
                constexpr static bool convertible
                        = std::is_same_v<typename internal::StructuredFieldExprTrait<Other>::elem_type,
                                         typename internal::StructuredFieldExprTrait<Derived>::elem_type>;
                if constexpr (convertible) {
                    bc[i].start = other.bc[i].start ? other.bc[i].start->getCopy() : nullptr;
                    bc[i].end = other.bc[i].end ? other.bc[i].end->getCopy() : nullptr;
                }
            }
        }

    private:
        DEFINE_CRTP_HELPERS(Derived)
    };
}// namespace OpFlow
#endif//OPFLOW_STRUCTUREDFIELDEXPR_HPP
