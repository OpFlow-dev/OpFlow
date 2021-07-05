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

#ifndef OPFLOW_STRUCTSOLVER_HPP
#define OPFLOW_STRUCTSOLVER_HPP

#include <HYPRE.h>
#include <optional>

namespace OpFlow {

    enum class StructSolverType {
        None, /* None only for un-preconditioned cases */
        Jacobi,
        SMG,
        PFMG,
        CYCRED,
        PCG,
        GMRES,
        FGMRES,
        LGMRES,
        BICGSTAB
    };

    template <StructSolverType type>
    struct StructSolverParams;

    struct StructSolverParamsBase {
        // common params
        std::optional<Real> tol {};
        std::optional<int> maxIter;
        MPI_Comm comm = MPI_COMM_WORLD;
        bool staticMat = false;
        bool pinValue = false;
        std::optional<std::string> dumpPath {};
    };

    template <StructSolverType type>
    struct StructSolver;

}// namespace OpFlow
#endif//OPFLOW_STRUCTSOLVER_HPP
