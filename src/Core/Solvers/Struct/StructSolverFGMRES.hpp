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

#ifndef OPFLOW_STRUCTSOLVERFGMRES_HPP
#define OPFLOW_STRUCTSOLVERFGMRES_HPP

#include "StructSolver.hpp"

OPFLOW_MODULE_EXPORT namespace OpFlow {
    template <>
    struct StructSolverParams<StructSolverType::FGMRES> : StructSolverParamsBase {
        std::optional<Real> absTol;
        std::optional<int> kDim, logging, printLevel;
    };

    template <>
    struct StructSolver<StructSolverType::FGMRES> {
        using Param = StructSolverParams<StructSolverType::FGMRES>;
        constexpr auto static type = StructSolverType::FGMRES;
        Param params;

        StructSolver() { HYPRE_StructFlexGMRESCreate(params.comm, &solver); }
        StructSolver(const Param& p) : params(p) { HYPRE_StructFlexGMRESCreate(params.comm, &solver); }
        ~StructSolver() { HYPRE_StructFlexGMRESDestroy(solver); }
        StructSolver(const StructSolver& s) : params(s.params) {
            HYPRE_StructFlexGMRESCreate(params.comm, &solver);
        }
        StructSolver(StructSolver&& s) noexcept : params(std::move(s.params)), solver(std::move(s.solver)) {
            s.solver = nullptr;
        }

        void init() {
            if (params.tol) HYPRE_StructFlexGMRESSetTol(solver, params.tol.value());
            if (params.absTol) HYPRE_StructFlexGMRESSetAbsoluteTol(solver, params.absTol.value());
            if (params.maxIter) HYPRE_StructFlexGMRESSetMaxIter(solver, params.maxIter.value());
            if (params.kDim) HYPRE_StructFlexGMRESSetKDim(solver, params.kDim.value());
            if (params.logging) HYPRE_StructFlexGMRESSetLogging(solver, params.logging.value());
            if (params.printLevel) HYPRE_StructFlexGMRESSetPrintLevel(solver, params.printLevel.value());
        }

        void setPrecond(HYPRE_PtrToStructSolverFcn precond, HYPRE_PtrToStructSolverFcn precond_setup,
                        HYPRE_StructSolver& precond_solver) {
            HYPRE_StructFlexGMRESSetPrecond(solver, precond, precond_setup, precond_solver);
        }

        auto& getSolver() { return solver; }
        const auto& getSolver() const { return solver; }
        auto getSolveFunc() const { return HYPRE_StructFlexGMRESSolve; }
        auto getSetUpFunc() const { return HYPRE_StructFlexGMRESSetup; }

        auto solve(HYPRE_StructMatrix& A, HYPRE_StructVector& b, HYPRE_StructVector& x) {
            return HYPRE_StructFlexGMRESSolve(solver, A, b, x);
        }

        auto setup(HYPRE_StructMatrix& A, HYPRE_StructVector& b, HYPRE_StructVector& x) {
            return HYPRE_StructFlexGMRESSetup(solver, A, b, x);
        }

        void dump(HYPRE_StructMatrix& A, HYPRE_StructVector& b) {
            if (params.dumpPath) {
                HYPRE_StructMatrixPrint((params.dumpPath.value() + "_A.mat").c_str(), A, 0);
                HYPRE_StructVectorPrint((params.dumpPath.value() + "_b.vec").c_str(), b, 0);
            }
        }

        auto getIterNum() {
            int ret;
            HYPRE_StructFlexGMRESGetNumIterations(solver, &ret);
            return ret;
        }

        auto getFinalRes() const {
            Real ret;
            HYPRE_StructFlexGMRESGetFinalRelativeResidualNorm(solver, &ret);
            return ret;
        }

    private:
        HYPRE_StructSolver solver;
    };

}// namespace OpFlow
#endif//OPFLOW_STRUCTSOLVERFGMRES_HPP
