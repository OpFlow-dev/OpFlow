// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2026 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#ifndef OPFLOW_D1FLUXLIMITERBASEDINTPOP_HPP
#define OPFLOW_D1FLUXLIMITERBASEDINTPOP_HPP

#include "Core/Operator/Interpolator/D1FluxLimiter.hpp"
#include "Core/Operator/Interpolator/FluxLimiterKernels.hpp"

OPFLOW_MODULE_EXPORT namespace OpFlow {
    // This file contains handy alias for commonly used flux limiter based interpolators.

    // QUICK scheme
    template <std::size_t d, IntpDirection dir>
    using D1QUICK = D1FluxLimiterGen<KappaKernel<KappaScheme::QUICK>>::Op<d, dir>;

    // Central difference
    template <std::size_t d, IntpDirection dir>
    using D1Central = D1FluxLimiterGen<KappaKernel<KappaScheme::CDS>>::Op<d, dir>;

    // Cubic-upwind interpolation
    template <std::size_t d, IntpDirection dir>
    using D1CUI = D1FluxLimiterGen<KappaKernel<KappaScheme::CUI>>::Op<d, dir>;

    // Fromm's scheme
    template <std::size_t d, IntpDirection dir>
    using D1Fromm = D1FluxLimiterGen<KappaKernel<KappaScheme::Fromm>>::Op<d, dir>;

    // Linear upwind interpolation
    template <std::size_t d, IntpDirection dir>
    using D1LinearUpwind = D1FluxLimiterGen<KappaKernel<KappaScheme::LUI>>::Op<d, dir>;

    // Minmod
    template <std::size_t d, IntpDirection dir>
    using D1Minmod = D1FluxLimiterGen<MinmodKernel>::Op<d, dir>;

    // Superbee
    template <std::size_t d, IntpDirection dir>
    using D1Superbee = D1FluxLimiterGen<SuperbeeKernel>::Op<d, dir>;

    // MUSCL
    template <std::size_t d, IntpDirection dir>
    using D1MUSCL = D1FluxLimiterGen<MUSCLKernel>::Op<d, dir>;

    // Harmonic
    template <std::size_t d, IntpDirection dir>
    using D1Harmonic = D1FluxLimiterGen<HarmonicKernel>::Op<d, dir>;

    // van Albada
    template <std::size_t d, IntpDirection dir>
    using D1Albada = D1FluxLimiterGen<vanAlbadaKernel>::Op<d, dir>;
}// namespace OpFlow

#endif//OPFLOW_D1FLUXLIMITERBASEDINTPOP_HPP
