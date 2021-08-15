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

#ifndef OPFLOW_UNIFIEDSOLVE_HPP
#define OPFLOW_UNIFIEDSOLVE_HPP

#include "Core/Equation/EqnSolveHandler.hpp"
#include "Core/Equation/Equation.hpp"
#include "Core/Solvers/SemiStruct/SemiStructSolver.hpp"
#include "Core/Solvers/Struct/StructSolver.hpp"
#include "Core/Solvers/Struct/StructSolverBiCGSTAB.hpp"
#include "Core/Solvers/Struct/StructSolverCycRed.hpp"
#include "Core/Solvers/Struct/StructSolverFGMRES.hpp"
#include "Core/Solvers/Struct/StructSolverGMRES.hpp"
#include "Core/Solvers/Struct/StructSolverJacobi.hpp"
#include "Core/Solvers/Struct/StructSolverLGMRES.hpp"
#include "Core/Solvers/Struct/StructSolverNone.hpp"
#include "Core/Solvers/Struct/StructSolverPCG.hpp"
#include "Core/Solvers/Struct/StructSolverPFMG.hpp"
#include "Core/Solvers/Struct/StructSolverPrecond.hpp"
#include "Core/Solvers/Struct/StructSolverSMG.hpp"

namespace OpFlow {
    template <StructSolverType type = StructSolverType::GMRES,
              StructSolverType pType = StructSolverType::None, typename F, StructuredFieldExprType T>
    void Solve(const F& func, T&& target, StructSolverParams<type> params = StructSolverParams<type> {},
               StructSolverParams<pType> precParams = StructSolverParams<pType> {}) {
        auto solver = PrecondStructSolver<type, pType>(params, precParams);
        auto handler = EqnSolveHandler(func, target, solver);
        handler.solve();
    }
}// namespace OpFlow
#endif//OPFLOW_UNIFIEDSOLVE_HPP
