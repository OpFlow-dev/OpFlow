#ifndef OPFLOW_BASICDATATYPES_HPP
#define OPFLOW_BASICDATATYPES_HPP

#include <complex>

namespace OpFlow {

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

}// namespace OpFlow
#endif//OPFLOW_BASICDATATYPES_HPP
