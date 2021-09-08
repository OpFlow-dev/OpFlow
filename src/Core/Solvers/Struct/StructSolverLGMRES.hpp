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

#ifndef OPFLOW_STRUCTSOLVERLGMRES_HPP
#define OPFLOW_STRUCTSOLVERLGMRES_HPP

#include "StructSolver.hpp"

namespace OpFlow {
    template <>
    struct StructSolverParams<StructSolverType::LGMRES> : StructSolverParamsBase {
        std::optional<Real> absTol;
        std::optional<int> kDim, augDim, logging, printLevel;
    };

    template <>
    struct StructSolver<StructSolverType::LGMRES> {
        using Param = StructSolverParams<StructSolverType::LGMRES>;
        constexpr auto static type = StructSolverType::LGMRES;
        Param params;

        StructSolver() { HYPRE_StructLGMRESCreate(params.comm, &solver); }
        StructSolver(const Param& p) : params(p) { HYPRE_StructLGMRESCreate(params.comm, &solver); }
        ~StructSolver() { HYPRE_StructLGMRESDestroy(solver); }
        StructSolver(const StructSolver& s) : params(s.params) {
            HYPRE_StructLGMRESCreate(params.comm, &solver);
        }
        StructSolver(StructSolver&& s) noexcept : params(std::move(s.params)), solver(std::move(s.solver)) {
            s.solver = nullptr;
        }

        void init() {
            if (params.tol) HYPRE_StructLGMRESSetTol(solver, params.tol.value());
            if (params.absTol) HYPRE_StructLGMRESSetAbsoluteTol(solver, params.absTol.value());
            if (params.maxIter) HYPRE_StructLGMRESSetMaxIter(solver, params.maxIter.value());
            if (params.kDim) HYPRE_StructLGMRESSetKDim(solver, params.kDim.value());
            if (params.logging) HYPRE_StructLGMRESSetLogging(solver, params.logging.value());
            if (params.printLevel) HYPRE_StructLGMRESSetPrintLevel(solver, params.printLevel.value());
        }

        void setPrecond(HYPRE_PtrToStructSolverFcn precond, HYPRE_PtrToStructSolverFcn precond_setup,
                        HYPRE_StructSolver& precond_solver) {
            HYPRE_StructLGMRESSetPrecond(solver, precond, precond_setup, precond_solver);
        }

        auto& getSolver() { return solver; }
        const auto& getSolver() const { return solver; }
        auto getSolveFunc() const { return HYPRE_StructLGMRESSolve; }
        auto getSetUpFunc() const { return HYPRE_StructLGMRESSetup; }

        auto solve(HYPRE_StructMatrix& A, HYPRE_StructVector& b, HYPRE_StructVector& x) {
            return HYPRE_StructLGMRESSolve(solver, A, b, x);
        }

        auto setup(HYPRE_StructMatrix& A, HYPRE_StructVector& b, HYPRE_StructVector& x) {
            return HYPRE_StructLGMRESSetup(solver, A, b, x);
        }

        void dump(HYPRE_StructMatrix& A, HYPRE_StructVector& b) {
            if (params.dumpPath) {
                HYPRE_StructMatrixPrint((params.dumpPath.value() + "_A.mat").c_str(), A, 0);
                HYPRE_StructVectorPrint((params.dumpPath.value() + "_b.vec").c_str(), b, 0);
            }
        }

        auto getIterNum() {
            int ret;
            HYPRE_StructLGMRESGetNumIterations(solver, &ret);
            return ret;
        }

        auto getFinalRes() const {
            Real ret;
            HYPRE_StructLGMRESGetFinalRelativeResidualNorm(solver, &ret);
            return ret;
        }

    private:
        HYPRE_StructSolver solver;
    };

}// namespace OpFlow
#endif//OPFLOW_STRUCTSOLVERLGMRES_HPP
