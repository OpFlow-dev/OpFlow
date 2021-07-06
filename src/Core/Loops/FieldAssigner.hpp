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
            constexpr auto width = CartesianFieldExprTrait<From>::bc_width;
            auto bc_ranges = src.accessibleRange.getBCRanges(width + 1);
            for (const auto& r : bc_ranges) {
                rangeFor(DS::commonRange(dst.assignableRange, r),
                         [&](auto&& i) { dst[i] = src.evalSafeAt(i); });
            }
            auto inner_range = src.accessibleRange.getInnerRange(width + 1);
            rangeFor(DS::commonRange(dst.assignableRange, inner_range),
                     [&](auto&& i) { dst[i] = src.evalAt(i); });
            dst.updateBC();
            return dst;
        }
    };
}// namespace OpFlow::internal
#endif//OPFLOW_FIELDASSIGNER_HPP
