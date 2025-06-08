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

#ifndef OPFLOW_NUMERIC_HPP
#define OPFLOW_NUMERIC_HPP

#include "Core/BasicDataTypes.hpp"
#include "Core/Constants.hpp"
#include "Core/Meta.hpp"
#ifndef OPFLOW_INSIDE_MODULE
#include <cmath>
#include <type_traits>
#endif

OPFLOW_MODULE_EXPORT namespace OpFlow::Math {

    template <typename T>
    inline constexpr int signum(T x, std::false_type is_signed) {
        return T(0) < x;
    }

    template <typename T>
    inline constexpr int signum(T x, std::true_type is_signed) {
        return (T(0) < x) - (x < T(0));
    }

    template <typename T>
    inline constexpr int signum(T x) {
        return signum(x, std::is_signed<T>());
    }

    template <typename T>
    inline constexpr auto norm2(T x, T y) -> T {
        return std::sqrt(x * x + y * y);
    }

    template <typename T>
    inline constexpr auto norm2(T x, T y, T z) -> T {
        return std::sqrt(x * x + y * y + z * z);
    }

    template <typename Width>
    struct DiscreteDelta {
        constexpr static auto width = Width::value;

        constexpr static double eval(double d);
    };

    template <>
    struct DiscreteDelta<Meta::int_<2>> {
        constexpr static double eval(double d) {
            d = std::max(d, -d);
            return d >= 1. ? 0. : 1. - d;
        }
    };

    template <>
    struct DiscreteDelta<Meta::int_<3>> {
        constexpr static double eval(double d) {
            d = std::max(d, -d);
            return d >= 1.5 ? 0.
                            : (d < 0.5 ? (1 + std::sqrt(1. - 3. * d * d)) / 3.
                                       : (5. - 3. * d - std::sqrt(1. - 3. * (1. - d) * (1. - d))) / 6.);
        }
    };

    template <>
    struct DiscreteDelta<Meta::int_<4>> {
        constexpr static double eval(double d) {
            d = std::max(d, -d);
            return d >= 2. ? 0.
                           : (d <= 1. ? (3. - 2. * d + std::sqrt(1. + 4. * d - 4. * d * d)) / 8.
                                      : (5. - 2. * d - std::sqrt(-7. + 12. * d - 4. * d * d)) / 8.);
        }
    };

    inline auto smoothHeviside(Real eps, Real d) {
        if (d < -eps) return 0.;
        else if (d > eps)
            return 1.;
        else
            return 0.5 * (1 + d / eps + 1 / PI * std::sin(d * PI / eps));
    }

    inline auto smoothDelta(Real eps, Real d) {
        return std::max(d, -d) > eps ? 0. : 1. / 2. / eps * (1 + std::cos(d * PI / eps));
    }

    inline constexpr auto int_pow(int a, unsigned int n) {
        auto ret = 1;
        for (unsigned int i = 0; i < n; ++i) ret *= a;
        return ret;
    }

    inline constexpr auto pow2(double a) { return a * a; }

    inline constexpr auto pow3(double a) { return a * a * a; }
}// namespace OpFlow::Math
#endif//OPFLOW_NUMERIC_HPP
