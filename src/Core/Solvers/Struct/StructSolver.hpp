//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2025 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_STRUCTSOLVER_HPP
#define OPFLOW_STRUCTSOLVER_HPP

#ifndef OPFLOW_INSIDE_MODULE
#include <HYPRE.h>
#include <optional>
#endif

OPFLOW_MODULE_EXPORT namespace OpFlow {

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
        std::optional<int> maxIter {};
#ifdef OPFLOW_WITH_MPI
        MPI_Comm comm = MPI_COMM_WORLD;
#else
        int comm = 0;
#endif
        bool staticMat = false;
        bool pinValue = false;
        std::optional<std::string> dumpPath {};
    };

    template <StructSolverType type>
    struct StructSolver;

}// namespace OpFlow
#endif//OPFLOW_STRUCTSOLVER_HPP
