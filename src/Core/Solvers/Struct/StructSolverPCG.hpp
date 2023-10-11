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

#ifndef OPFLOW_STRUCTSOLVERPCG_HPP
#define OPFLOW_STRUCTSOLVERPCG_HPP

#include "StructSolver.hpp"

namespace OpFlow {
    template <>
    struct StructSolverParams<StructSolverType::PCG> : StructSolverParamsBase {
        std::optional<Real> absTol;
        std::optional<int> twoNorm, relChange, logging, printLevel;
    };

    template <>
    struct StructSolver<StructSolverType::PCG> {
        using Param = StructSolverParams<StructSolverType::PCG>;
        constexpr auto static type = StructSolverType::PCG;
        Param params;

        StructSolver() { HYPRE_StructPCGCreate(params.comm, &solver); }
        StructSolver(const Param& p) : params(p) { HYPRE_StructPCGCreate(params.comm, &solver); }
        ~StructSolver() { HYPRE_StructPCGDestroy(solver); }
        StructSolver(const StructSolver& s) : params(s.params) {
            HYPRE_StructPCGCreate(params.comm, &solver);
        }
        StructSolver(StructSolver&& s) noexcept : params(std::move(s.params)), solver(std::move(s.solver)) {
            s.solver = nullptr;
        }

        void init() {
            if (params.tol) HYPRE_StructPCGSetTol(solver, params.tol.value());
            if (params.absTol) HYPRE_StructPCGSetAbsoluteTol(solver, params.absTol.value());
            if (params.maxIter) HYPRE_StructPCGSetMaxIter(solver, params.maxIter.value());
            if (params.twoNorm) HYPRE_StructPCGSetTwoNorm(solver, params.twoNorm.value());
            if (params.relChange) HYPRE_StructPCGSetRelChange(solver, params.relChange.value());
            if (params.logging) HYPRE_StructPCGSetLogging(solver, params.logging.value());
            if (params.printLevel) HYPRE_StructPCGSetPrintLevel(solver, params.printLevel.value());
        }

        void reinit() {
            HYPRE_StructPCGDestroy(solver);
            HYPRE_StructPCGCreate(params.comm, &solver);
            init();
        }

        void setPrecond(HYPRE_PtrToStructSolverFcn precond, HYPRE_PtrToStructSolverFcn precond_setup,
                        HYPRE_StructSolver& precond_solver) {
            HYPRE_StructPCGSetPrecond(solver, precond, precond_setup, precond_solver);
        }

        auto& getSolver() { return solver; }
        const auto& getSolver() const { return solver; }
        auto getSolveFunc() const { return HYPRE_StructPCGSolve; }
        auto getSetUpFunc() const { return HYPRE_StructPCGSetup; }

        auto solve(HYPRE_StructMatrix& A, HYPRE_StructVector& b, HYPRE_StructVector& x) {
            return HYPRE_StructPCGSolve(solver, A, b, x);
        }

        auto setup(HYPRE_StructMatrix& A, HYPRE_StructVector& b, HYPRE_StructVector& x) {
            return HYPRE_StructPCGSetup(solver, A, b, x);
        }

        auto dump(HYPRE_StructMatrix& A, HYPRE_StructVector& b) {
            if (params.dumpPath) {
                HYPRE_StructMatrixPrint((params.dumpPath.value() + "_A.mat").c_str(), A, 0);
                HYPRE_StructVectorPrint((params.dumpPath.value() + "_b.vec").c_str(), b, 0);
            }
        }

        auto getIterNum() {
            int ret;
            HYPRE_StructPCGGetNumIterations(solver, &ret);
            return ret;
        }

        auto getFinalRes() const {
            Real ret;
            HYPRE_StructPCGGetFinalRelativeResidualNorm(solver, &ret);
            return ret;
        }

    private:
        HYPRE_StructSolver solver;
    };

}// namespace OpFlow
#endif//OPFLOW_STRUCTSOLVERPCG_HPP
