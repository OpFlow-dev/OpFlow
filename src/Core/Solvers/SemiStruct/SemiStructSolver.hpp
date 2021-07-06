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

#ifndef OPFLOW_SEMISTRUCTSOLVER_HPP
#define OPFLOW_SEMISTRUCTSOLVER_HPP

#include <mpi.h>
#include <optional>

namespace OpFlow {
    enum class SemiStructSolverType {
        None,// only for un-preconditioned cases
        Jacobi,
        SMG,
        PFMG,
        Split,
        SysPFMG,
        FAC,
        Maxwell,
        AMG,
        AMS,
        ADS,
        MLI,
        ParaSails,
        Euclid,
        PILUT,
        PCG,
        GMRES,
        FGMRES,
        LGMRES,
        BiCGSTAB,
        Hybird,
        LOBPCG
    };

    template <SemiStructSolverType type>
    struct SemiStructSolverParams;

    struct SemiStructSolverParamsBase {
        // common params
        std::optional<Real> tol {};
        std::optional<int> maxIter;
        MPI_Comm comm = MPI_COMM_WORLD;
        bool staticMat = false;
        bool pinValue = false;
        std::optional<std::string> dumpPath {};
    };

    template <SemiStructSolverType type>
    struct SemiStructSolver;
}// namespace OpFlow
#endif//OPFLOW_SEMISTRUCTSOLVER_HPP
