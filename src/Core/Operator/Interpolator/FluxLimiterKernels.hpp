// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2022 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_FLUXLIMITERKERNELS_HPP
#define OPFLOW_FLUXLIMITERKERNELS_HPP

#include <algorithm>
#include <cmath>
#include <ratio>

namespace OpFlow {
    enum class KappaScheme {
        CDS,  // Central difference, kappa = 1
        QUICK,// Quadratic-upwind interpolation, kappa = 1/2
        CUI,  // Cubic-upwind interpolation, kappa = 1/3
        Fromm,// Fromm's scheme, kappa = 0
        LUI   // Linear-upwind interpolation, kappa = -1
    };

    template <KappaScheme s>
    struct KappaKernel {
        constexpr static auto eval(auto r) { return eval(1., r); }

        constexpr static auto eval(auto slop_u, auto slop_f) {
            double kappa;
            if constexpr (s == KappaScheme::CDS) kappa = 1;
            else if constexpr (s == KappaScheme::QUICK)
                kappa = 0.5;
            else if constexpr (s == KappaScheme::CUI)
                kappa = 1. / 3.;
            else if constexpr (s == KappaScheme::Fromm)
                kappa = 0.;
            else
                kappa = -1.;
            return (1 + kappa) / 2. * slop_f + (1 - kappa) / 2. * slop_u;
        }
    };

    // Minmod limiter
    struct MinmodKernel {
        constexpr static auto eval(auto r) { return std::max(0., std::min(r, 1.)); }
    };

    // Superbee limiter
    struct SuperbeeKernel {
        constexpr static auto eval(auto r) { return std::max({0., std::min(2. * r, 1.), std::min(r, 2.)}); }
    };

    // MUSCL limiter
    struct MUSCLKernel {
        constexpr static auto eval(auto r) { return std::max(0., std::min({2 * r, (r + 1) / 2., 2.})); }
    };

    // Harmonic limiter
    struct HarmonicKernel {
        static auto eval(auto r) { return (r + std::fabs(r)) / (r + 1); }
    };

    // van Albada limiter
    struct vanAlbadaKernel {
        constexpr static auto eval(auto r) { return r * (r + 1) / (r * r + 1); }
    };

    // Symmetric piecewise-linear (SPL) schemes
    /// SPL-min limiter
    /// \tparam M M param
    /// \tparam Kappa kappa param
    template <Meta::StdRatio M, Meta::StdRatio Kappa>
    struct SPLMinKernel {
        static constexpr double m = M::num / M::den;
        static constexpr double kappa = Kappa::num / Kappa::den;

        constexpr static auto eval(auto r) {
            return std::max({0., std::min({m * r, 0.5 * (1 + kappa) * r + 0.5 * (1 - kappa),
                                           0.5 * (1 - kappa) * r + 0.5 * (1 + kappa), m})});
        }
    };

    /// SPL-max limiter
    /// \tparam M M param
    /// \tparam Kappa kappa param
    template <Meta::StdRatio M, Meta::StdRatio Kappa>
    struct SPLMaxKernel {
        static constexpr double m = M::num / M::den;
        static constexpr double kappa = Kappa::num / Kappa::den;

        constexpr static auto eval(auto r) {
            return std::max({0., std::min({m * r,
                                           std::max({0.5 * (1 + kappa) * r + 0.5 * (1 - kappa),
                                                     0.5 * (1 - kappa) * r + 0.5 * (1 + kappa)}),
                                           m})});
        }
    };

    // UMIST limiter
    using UMISTKernel = SPLMinKernel<std::ratio<2, 1>, std::ratio<1, 2>>;

    // SPL-1/3 limiter
    using SPL13Kernel = SPLMinKernel<std::ratio<2, 1>, std::ratio<1, 3>>;

    // SPL-max-1/2 limiter
    using SPLMax12Kernel = SPLMaxKernel<std::ratio<2, 1>, std::ratio<1, 2>>;

    // SPL-max-1/3 limiter
    using SPLMax13Kernel = SPLMaxKernel<std::ratio<2, 1>, std::ratio<1, 3>>;

    // Generalized piecewise-linear (GPL) schemes
    template <Meta::StdRatio M, Meta::StdRatio Alpha, Meta::StdRatio Kappa>
    struct GPLKappaKernel {
        static constexpr double m = M::num / M::den;
        static constexpr double alpha = Alpha::num / Alpha::den;
        static constexpr double kappa = Kappa::num / Kappa::den;

        constexpr static auto eval(auto r) {
            return std::max({0., std::min({(2 + alpha) * r, 0.5 * (1 + kappa) * r + 0.5 * (1 - kappa), m})});
        }
    };

    // Koren's limited CUI scheme
    using KorenLimitedCUIKernel = GPLKappaKernel<std::ratio<2, 1>, std::ratio<0, 1>, std::ratio<1, 3>>;

    // SMART scheme
    using SMARTKernel = GPLKappaKernel<std::ratio<4, 1>, std::ratio<0, 1>, std::ratio<1, 2>>;

    // Chakravarthy-Osher limiters
    // BSOU scheme
    using BSOUKernel = GPLKappaKernel<std::ratio<1, 1>, std::ratio<0, 1>, std::ratio<-1, 1>>;
}// namespace OpFlow

#endif//OPFLOW_FLUXLIMITERKERNELS_HPP
