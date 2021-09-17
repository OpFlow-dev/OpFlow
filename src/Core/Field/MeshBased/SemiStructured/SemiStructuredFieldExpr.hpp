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

#ifndef OPFLOW_SEMISTRUCTUREDFIELDEXPR_HPP
#define OPFLOW_SEMISTRUCTUREDFIELDEXPR_HPP

#include "Core/Field/MeshBased/MeshBasedFieldExpr.hpp"
#include "DataStructures/Pair.hpp"
#include "SemiStructuredFieldExprTrait.hpp"
#include <array>

namespace OpFlow {

    template <typename Derived>
    struct SemiStructuredFieldExpr : MeshBasedFieldExpr<Derived> {
        std::vector<std::vector<typename internal::SemiStructuredFieldExprTrait<Derived>::range_type>>
                localRanges, assignableRanges, accessibleRanges;
        std::array<LocOnMesh, internal::SemiStructuredFieldExprTrait<Derived>::dim> loc;
        std::array<DS::Pair<std::unique_ptr<
                           BCBase<typename internal::SemiStructuredFieldExprTrait<Derived>::type>>>,
                   internal::FieldExprTrait<Derived>::dim>
                bc;

        SemiStructuredFieldExpr() = default;
        SemiStructuredFieldExpr(const SemiStructuredFieldExpr& other)
            : MeshBasedFieldExpr<Derived>(other), localRanges(other.localRanges),
              assignableRanges(other.assignableRanges), accessibleRanges(other.accessibleRanges),
              loc(other.loc) {
            for (auto i = 0; i < bc.size(); ++i) {
                bc[i].start = other.bc[i].start ? other.bc[i].start->getCopy() : nullptr;
                bc[i].end = other.bc[i].end ? other.bc[i].end->getCopy() : nullptr;
            }
        }

        SemiStructuredFieldExpr(SemiStructuredFieldExpr&& other) noexcept
            : MeshBasedFieldExpr<Derived>(std::move(other)), bc(std::move(other.bc)),
              loc(std::move(other.loc)), localRanges(std::move(other.localRanges)),
              assignableRanges(std::move(other.assignableRanges)),
              accessibleRanges(std::move(other.accessibleRanges)) {}

        auto getLevels() const {
            auto total = 0;
            for (auto& r : accessibleRanges) {
                if (!r.empty()) total++;
            }
            return total;
        }
        auto getPartsOnLevel(int i) const { return accessibleRanges[i].size(); }

        auto updateBC() { this->derived().updateBCImpl_final(); }

    protected:
        template <SemiStructuredFieldExprType Other>
        void initPropsFromImpl_SemiStructuredFieldExpr(const Other& other) {
            this->initPropsFromImpl_MeshBasedFieldExpr(other);
            this->loc = other.loc;
            this->localRanges = other.localRanges;
            this->assignableRanges = other.assignableRanges;
            this->accessibleRanges = other.accessibleRanges;
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

        bool couldEvalAtImpl_final(auto&& i) const {
            return accessibleRanges.size() > i.l && accessibleRanges[i.l].size() > i.p
                   && DS::inRange(accessibleRanges[i.l][i.p], i);
        }

    private:
        DEFINE_CRTP_HELPERS(Derived)
    };
}// namespace OpFlow
#endif//OPFLOW_SEMISTRUCTUREDFIELDEXPR_HPP
