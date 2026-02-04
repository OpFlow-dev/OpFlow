//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2026 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_BASICDATATYPES_HPP
#define OPFLOW_BASICDATATYPES_HPP

#ifndef OPFLOW_INSIDE_MODULE
#include <complex>
#include <exception>
#endif

OPFLOW_MODULE_EXPORT namespace OpFlow {

    using Index = int;
    using Rank = long long;
    using Size = long long;

#ifdef OPFLOW_SINGLE_REAL
    using Real = float;
#else
    using Real = double;
#endif
    using Cplx = std::complex<Real>;

    // user defined literal suffix
    inline constexpr Real operator""_r(long double x) { return (Real) x; }

    inline constexpr Real operator""_r(unsigned long long int x) { return (Real) x; }

    // exception objects
    struct CouldNotSafeEval : std::exception {

        std::string reason;
        CouldNotSafeEval() = default;
        CouldNotSafeEval(const std::string& r) : reason(r) {}

        [[nodiscard]] const char* what() const noexcept override { return reason.c_str(); }
    };

}// namespace OpFlow
#endif//OPFLOW_BASICDATATYPES_HPP
