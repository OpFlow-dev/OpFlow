//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2023 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_STRUCTSOLVERSMG_HPP
#define OPFLOW_STRUCTSOLVERSMG_HPP

#include "StructSolver.hpp"

namespace OpFlow {
    template <>
    struct StructSolverParams<StructSolverType::SMG> : StructSolverParamsBase {
        std::optional<int> memUse;
        std::optional<int> relChange;
        std::optional<bool> useZeroGuess;
        std::optional<int> numPreRelax, numPostRelax;
        std::optional<int> logging, printLevel;
    };

    template <>
    struct StructSolver<StructSolverType::SMG> {
        using Param = StructSolverParams<StructSolverType::SMG>;
        constexpr auto static type = StructSolverType::SMG;
        Param params;

        StructSolver() { HYPRE_StructSMGCreate(params.comm, &solver); }
        StructSolver(const Param& p) : params(p) { HYPRE_StructSMGCreate(params.comm, &solver); }
        ~StructSolver() { HYPRE_StructSMGDestroy(solver); }
        StructSolver(const StructSolver& s) : params(s.params) {
            HYPRE_StructSMGCreate(params.comm, &solver);
        }
        StructSolver(StructSolver&& s) noexcept : params(std::move(s.params)), solver(std::move(s.solver)) {
            s.solver = nullptr;
        }

        void init() {
            if (params.memUse) HYPRE_StructSMGSetMemoryUse(solver, params.memUse.value());
            if (params.tol) HYPRE_StructSMGSetTol(solver, params.tol.value());
            if (params.maxIter) HYPRE_StructSMGSetMaxIter(solver, params.maxIter.value());
            if (params.relChange) HYPRE_StructSMGSetRelChange(solver, params.relChange.value());
            if (params.useZeroGuess && params.useZeroGuess.value_or(false))
                HYPRE_StructSMGSetZeroGuess(solver);
            if (params.useZeroGuess && !params.useZeroGuess.value_or(true))
                HYPRE_StructSMGSetNonZeroGuess(solver);
            if (params.numPreRelax) HYPRE_StructSMGSetNumPreRelax(solver, params.numPreRelax.value());
            if (params.numPostRelax) HYPRE_StructSMGSetNumPostRelax(solver, params.numPostRelax.value());
            if (params.logging) HYPRE_StructSMGSetLogging(solver, params.logging.value());
            if (params.printLevel) HYPRE_StructSMGSetPrintLevel(solver, params.printLevel.value());
        }

        void reinit() {
            HYPRE_StructSMGDestroy(solver);
            HYPRE_StructSMGCreate(params.comm, &solver);
            init();
        }

        auto& getSolver() { return solver; }
        const auto& getSolver() const { return solver; }
        auto getSolveFunc() const { return HYPRE_StructSMGSolve; }
        auto getSetUpFunc() const { return HYPRE_StructSMGSetup; }

        auto solve(HYPRE_StructMatrix& A, HYPRE_StructVector& b, HYPRE_StructVector& x) {
            return HYPRE_StructSMGSolve(solver, A, b, x);
        }

        auto setup(HYPRE_StructMatrix& A, HYPRE_StructVector& b, HYPRE_StructVector& x) {
            return HYPRE_StructSMGSetup(solver, A, b, x);
        }

        void dump(HYPRE_StructMatrix& A, HYPRE_StructVector& b) {
            if (params.dumpPath) {
                HYPRE_StructMatrixPrint((params.dumpPath.value() + "_A.mat").c_str(), A, 0);
                HYPRE_StructVectorPrint((params.dumpPath.value() + "_b.vec").c_str(), b, 0);
            }
        }

        auto getIterNum() const {
            int ret;
            HYPRE_StructSMGGetNumIterations(solver, &ret);
            return ret;
        }

        auto getFinalRes() const {
            Real ret;
            HYPRE_StructSMGGetFinalRelativeResidualNorm(solver, &ret);
            return ret;
        }

    private:
        HYPRE_StructSolver solver;
    };

}// namespace OpFlow
#endif//OPFLOW_STRUCTSOLVERSMG_HPP
