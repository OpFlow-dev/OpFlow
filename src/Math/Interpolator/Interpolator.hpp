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

#ifndef OPFLOW_INTERPOLATOR_HPP
#define OPFLOW_INTERPOLATOR_HPP

#include "Core/Macros.hpp"

namespace OpFlow::Math {

    struct Interpolator1D {
        OPFLOW_STRONG_INLINE static auto intp(double x1, auto y1, double x2, auto y2) {
            return (x1 * y2 - x2 * y1) / (x1 - x2);
        }

        OPFLOW_STRONG_INLINE static auto intp(double x1, auto y1, double x2, auto y2, double x3, auto y3) {
            return (x1 * x2 * (x1 - x2) * y3 + x1 * x3 * (x3 - x1) * y2 + x2 * x3 * (x2 - x3) * y1)
                   / ((x1 - x2) * (x1 - x3) * (x2 - x3));
        }

        OPFLOW_STRONG_INLINE static auto intp(double x1, auto y1, double x2, auto y2, double x3, auto y3,
                                              double x4, auto y4) {
            return y1 * (-x2 * x3 * x4) / ((x1 - x2) * (x1 - x3) * (x1 - x4))
                   + y2 * (-x1 * x3 * x4) / ((x2 - x1) * (x2 - x3) * (x2 - x4))
                   + y3 * (-x1 * x2 * x4) / ((x3 - x1) * (x3 - x2) * (x3 - x4))
                   + y4 * (-x1 * x2 * x3) / ((x4 - x1) * (x4 - x2) * (x4 - x3));
        }

        OPFLOW_STRONG_INLINE static auto intp(double x1, auto y1, double x2, auto y2, double x) {
            return intp(x1 - x, y1, x2 - x, y2);
        }

        OPFLOW_STRONG_INLINE static auto intp(double x1, auto y1, double x2, auto y2, double x3, auto y3,
                                              double x) {
            return intp(x1 - x, y1, x2 - x, y2, x3 - x, y3);
        }

        OPFLOW_STRONG_INLINE static auto intp(double x1, auto y1, double x2, auto y2, double x3, auto y3,
                                              double x4, auto y4, double x) {
            return intp(x1 - x, y1, x2 - x, y2, x3 - x, y3, x4 - x, y4);
        }
    };

    struct Interpolator2D {
        OPFLOW_STRONG_INLINE static auto biLinearIntp(auto f00, auto f10, auto f01, auto f11, double x,
                                                      double y) {
            return f00 * (1 - x) * (1 - y) + f10 * x * (1 - y) + f01 * (1 - x) * y + f11 * x * y;
        }

        OPFLOW_STRONG_INLINE static auto biLinearIntp(double x1, double x2, double y1, double y2, auto f00,
                                                      auto f10, auto f01, auto f11, double x, double y) {
            return biLinearIntp(f00, f10, f01, f11, (x - x1) / (x2 - x1), (y - y1) / (y2 - y1));
        }
    };

    struct Interpolator3D {
        OPFLOW_STRONG_INLINE static auto triLinearIntp(auto f000, auto f001, auto f010, auto f011, auto f100,
                                                       auto f101, auto f110, auto f111, double x, double y,
                                                       double z) {
            return f000 * (1 - x) * (1 - y) * (1 - z) + f001 * (1 - x) * (1 - y) * z
                   + f010 * (1 - x) * y * (1 - z) + f011 * (1 - x) * y * z + f100 * x * (1 - y) * (1 - z)
                   + f101 * x * (1 - y) * z + f110 * x * y * (1 - z) + f111 * x * y * z;
        }

        OPFLOW_STRONG_INLINE static auto triLinearIntp(double x1, double x2, double y1, double y2, double z1,
                                                       double z2, auto f000, auto f001, auto f010, auto f011,
                                                       auto f100, auto f101, auto f110, auto f111, double x,
                                                       double y, double z) {
            return triLinearIntp(f000, f001, f010, f011, f100, f101, f110, f111, (x - x1) / (x2 - x1),
                                 (y - y1) / (y2 - y1), (z - z1) / (z2 - z1));
        }
    };

    OPFLOW_STRONG_INLINE auto mid(const auto& x1, const auto& x2) { return (x1 + x2) * 0.5; }
}// namespace OpFlow::Math
#endif//OPFLOW_INTERPOLATOR_HPP
