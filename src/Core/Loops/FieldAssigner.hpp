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
        template <BasicArithOp Op = BasicArithOp::Eq>
        static auto& assign(auto&& src, auto&& dst) {
            if (src.contains(dst)) {
                auto temp = dst;
                assign_impl<BasicArithOp::Eq>(src, temp);
                assign_impl<Op>(temp, dst);
            } else {
                assign_impl<Op>(src, dst);
            }
            return dst;
        }

    private:
        template <BasicArithOp Op = BasicArithOp::Eq, CartesianFieldType To, CartesianFieldExprType From>
        static auto& assign_impl(From& src, To& dst) {
            src.prepare();
            OP_EXPECT_MSG(dst.assignableRange == DS::commonRange(dst.assignableRange, src.logicalRange),
                          "Assign warning: dst's assignableRange not covered by src's accessibleRange.\ndst "
                          "= {}, range = {}\nsrc = {}, range = {}",
                          dst.getName(), dst.assignableRange.toString(), src.getName(),
                          src.logicalRange.toString());

            if constexpr (Op == BasicArithOp::Eq)
                rangeFor(DS::commonRange(dst.assignableRange, dst.localRange),
                         [&](auto&& i) { dst[i] = src.evalAt(i); });
            else if constexpr (Op == BasicArithOp::Add)
                rangeFor(DS::commonRange(dst.assignableRange, dst.localRange),
                         [&](auto&& i) { dst[i] += src.evalAt(i); });
            else if constexpr (Op == BasicArithOp::Minus)
                rangeFor(DS::commonRange(dst.assignableRange, dst.localRange),
                         [&](auto&& i) { dst[i] -= src.evalAt(i); });
            else if constexpr (Op == BasicArithOp::Mul)
                rangeFor(DS::commonRange(dst.assignableRange, dst.localRange),
                         [&](auto&& i) { dst[i] *= src.evalAt(i); });
            else if constexpr (Op == BasicArithOp::Div)
                rangeFor(DS::commonRange(dst.assignableRange, dst.localRange),
                         [&](auto&& i) { dst[i] /= src.evalAt(i); });
            else if constexpr (Op == BasicArithOp::Mod)
                rangeFor(DS::commonRange(dst.assignableRange, dst.localRange),
                         [&](auto&& i) { dst[i] %= src.evalAt(i); });
            else if constexpr (Op == BasicArithOp::And)
                rangeFor(DS::commonRange(dst.assignableRange, dst.localRange),
                         [&](auto&& i) { dst[i] &= src.evalAt(i); });
            else if constexpr (Op == BasicArithOp::Or)
                rangeFor(DS::commonRange(dst.assignableRange, dst.localRange),
                         [&](auto&& i) { dst[i] |= src.evalAt(i); });
            else if constexpr (Op == BasicArithOp::Xor)
                rangeFor(DS::commonRange(dst.assignableRange, dst.localRange),
                         [&](auto&& i) { dst[i] ^= src.evalAt(i); });
            else if constexpr (Op == BasicArithOp::LShift)
                rangeFor(DS::commonRange(dst.assignableRange, dst.localRange),
                         [&](auto&& i) { dst[i] <<= src.evalAt(i); });
            else if constexpr (Op == BasicArithOp::RShift)
                rangeFor(DS::commonRange(dst.assignableRange, dst.localRange),
                         [&](auto&& i) { dst[i] >>= src.evalAt(i); });
            else
                OP_NOT_IMPLEMENTED;

            dst.updatePadding();
            return dst;
        }
        template <BasicArithOp Op = BasicArithOp::Eq, CartAMRFieldType To, CartAMRFieldExprType From>
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
                    if constexpr (Op == BasicArithOp::Eq)
                        rangeFor_s(DS::commonRange(dst.assignableRanges[i][j], dst.logicalRanges[i][j]),
                                   [&](auto&& k) { dst[k] = src.evalAt(k); });
                    else if constexpr (Op == BasicArithOp::Add)
                        rangeFor_s(DS::commonRange(dst.assignableRanges[i][j], dst.logicalRanges[i][j]),
                                   [&](auto&& k) { dst[k] += src.evalAt(k); });
                    else if constexpr (Op == BasicArithOp::Minus)
                        rangeFor_s(DS::commonRange(dst.assignableRanges[i][j], dst.logicalRanges[i][j]),
                                   [&](auto&& k) { dst[k] -= src.evalAt(k); });
                    else if constexpr (Op == BasicArithOp::Mul)
                        rangeFor_s(DS::commonRange(dst.assignableRanges[i][j], dst.logicalRanges[i][j]),
                                   [&](auto&& k) { dst[k] *= src.evalAt(k); });
                    else if constexpr (Op == BasicArithOp::Div)
                        rangeFor_s(DS::commonRange(dst.assignableRanges[i][j], dst.logicalRanges[i][j]),
                                   [&](auto&& k) { dst[k] /= src.evalAt(k); });
                    else if constexpr (Op == BasicArithOp::Mod)
                        rangeFor_s(DS::commonRange(dst.assignableRanges[i][j], dst.logicalRanges[i][j]),
                                   [&](auto&& k) { dst[k] %= src.evalAt(k); });
                    else if constexpr (Op == BasicArithOp::And)
                        rangeFor_s(DS::commonRange(dst.assignableRanges[i][j], dst.logicalRanges[i][j]),
                                   [&](auto&& k) { dst[k] &= src.evalAt(k); });
                    else if constexpr (Op == BasicArithOp::Or)
                        rangeFor_s(DS::commonRange(dst.assignableRanges[i][j], dst.logicalRanges[i][j]),
                                   [&](auto&& k) { dst[k] |= src.evalAt(k); });
                    else if constexpr (Op == BasicArithOp::Xor)
                        rangeFor_s(DS::commonRange(dst.assignableRanges[i][j], dst.logicalRanges[i][j]),
                                   [&](auto&& k) { dst[k] ^= src.evalAt(k); });
                    else if constexpr (Op == BasicArithOp::LShift)
                        rangeFor_s(DS::commonRange(dst.assignableRanges[i][j], dst.logicalRanges[i][j]),
                                   [&](auto&& k) { dst[k] <<= src.evalAt(k); });
                    else if constexpr (Op == BasicArithOp::RShift)
                        rangeFor_s(DS::commonRange(dst.assignableRanges[i][j], dst.logicalRanges[i][j]),
                                   [&](auto&& k) { dst[k] >>= src.evalAt(k); });
                    else
                        OP_NOT_IMPLEMENTED;
                }
            }
            dst.updatePadding();
            return dst;
        }
    };
}// namespace OpFlow::internal
#endif//OPFLOW_FIELDASSIGNER_HPP
