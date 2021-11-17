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

#ifndef OPFLOW_FIELDASSIGNER_HPP
#define OPFLOW_FIELDASSIGNER_HPP

#include "Core/Field/FieldExprTrait.hpp"
#include "Core/Field/MeshBased/SemiStructured/CartAMRFieldExprTrait.hpp"
#include "Core/Field/MeshBased/SemiStructured/CartAMRFieldTrait.hpp"
#include "Core/Field/MeshBased/Structured/CartesianFieldExprTrait.hpp"
#include "Core/Field/MeshBased/Structured/CartesianFieldTrait.hpp"
#include "RangeFor.hpp"
#include "StructFor.hpp"

namespace OpFlow::internal {
    struct FieldAssigner {
        static auto& assign(auto&& src, auto&& dst) {
            if (src.contains(dst)) {
                auto temp = dst;
                assign_impl(src, temp);
                assign_impl(temp, dst);
            } else {
                assign_impl(src, dst);
            }
            return dst;
        }

    private:
        template <CartesianFieldType To, CartesianFieldExprType From>
        static auto& assign_impl(From& src, To& dst) {
            src.prepare();
            OP_EXPECT_MSG(dst.assignableRange == DS::commonRange(dst.assignableRange, src.logicalRange),
                          "Assign warning: dst's assignableRange not covered by src's accessibleRange.\ndst "
                          "= {}, range = {}\nsrc = {}, range = {}",
                          dst.getName(), dst.assignableRange.toString(), src.getName(),
                          src.logicalRange.toString());

            rangeFor(DS::commonRange(dst.assignableRange, dst.localRange),
                     [&](auto&& i) { dst[i] = src.evalAt(i); });
            dst.updatePadding();
            return dst;
        }
        template <CartAMRFieldType To, CartAMRFieldExprType From>
        static auto& assign_impl(From& src, To& dst) {
            src.prepare();
            auto levels = dst.getLevels();
#pragma omp parallel
            for (auto i = 0; i < levels; ++i) {
                auto parts = dst.accessibleRanges[i].size();
#pragma omp for nowait schedule(dynamic)
                for (auto j = 0; j < parts; ++j) {
                    OP_EXPECT_MSG(DS::inRange(dst.assignableRanges[i][j], src.logicalRanges[i][j]),
                                  "Assign warning: dst's assignableRange not covered by src's "
                                  "logicalRanges at level {} part {}.\ndst "
                                  "= {}, range = {}\nsrc = {}, range = {}",
                                  i, j, dst.getName(), dst.assignableRanges[i][j].toString(), src.getName(),
                                  src.logicalRanges[i][j].toString());
                    rangeFor_s(DS::commonRange(dst.assignableRanges[i][j], dst.logicalRanges[i][j]),
                               [&](auto&& k) { dst[k] = src.evalAt(k); });
                }
            }
            dst.updatePadding();
            return dst;
        }
    };
}// namespace OpFlow::internal
#endif//OPFLOW_FIELDASSIGNER_HPP
