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

#ifndef OPFLOW_STRUCTSOLVERBICGSTAB_HPP
#define OPFLOW_STRUCTSOLVERBICGSTAB_HPP

#include "StructSolver.hpp"

OPFLOW_MODULE_EXPORT namespace OpFlow {
    template <>
    struct StructSolverParams<StructSolverType::BICGSTAB> : StructSolverParamsBase {
        std::optional<Real> absTol;
        std::optional<int> logging, printLevel;
    };

    template <>
    struct StructSolver<StructSolverType::BICGSTAB> {
        using Param = StructSolverParams<StructSolverType::BICGSTAB>;
        constexpr auto static type = StructSolverType::BICGSTAB;
        Param params;

        StructSolver() { HYPRE_StructBiCGSTABCreate(params.comm, &solver); }
        StructSolver(const Param& p) : params(p) { HYPRE_StructBiCGSTABCreate(params.comm, &solver); }
        ~StructSolver() { HYPRE_StructBiCGSTABDestroy(solver); }
        StructSolver(const StructSolver& s) : params(s.params) {
            HYPRE_StructBiCGSTABCreate(params.comm, &solver);
        }
        StructSolver(StructSolver&& s) noexcept : params(std::move(s.params)), solver(std::move(s.solver)) {
            s.solver = nullptr;
        }

        void init() {
            if (params.tol) HYPRE_StructBiCGSTABSetTol(solver, params.tol.value());
            if (params.absTol) HYPRE_StructBiCGSTABSetAbsoluteTol(solver, params.absTol.value());
            if (params.maxIter) HYPRE_StructBiCGSTABSetMaxIter(solver, params.maxIter.value());
            if (params.logging) HYPRE_StructBiCGSTABSetLogging(solver, params.logging.value());
            if (params.printLevel) HYPRE_StructBiCGSTABSetPrintLevel(solver, params.printLevel.value());
        }

        void reinit() {
            HYPRE_StructBiCGSTABDestroy(solver);
            HYPRE_StructBiCGSTABCreate(params.comm, &solver);
            init();
        }

        void setPrecond(HYPRE_PtrToStructSolverFcn precond, HYPRE_PtrToStructSolverFcn precond_setup,
                        HYPRE_StructSolver& precond_solver) {
            HYPRE_StructBiCGSTABSetPrecond(solver, precond, precond_setup, precond_solver);
        }

        auto& getSolver() { return solver; }
        const auto& getSolver() const { return solver; }
        auto getSolveFunc() const { return HYPRE_StructBiCGSTABSolve; }
        auto getSetUpFunc() const { return HYPRE_StructBiCGSTABSetup; }

        auto solve(HYPRE_StructMatrix& A, HYPRE_StructVector& b, HYPRE_StructVector& x) {
            return HYPRE_StructBiCGSTABSolve(solver, A, b, x);
        }

        auto setup(HYPRE_StructMatrix& A, HYPRE_StructVector& b, HYPRE_StructVector& x) {
            return HYPRE_StructBiCGSTABSetup(solver, A, b, x);
        }

        void dump(HYPRE_StructMatrix& A, HYPRE_StructVector& b) {
            if (params.dumpPath) {
                HYPRE_StructMatrixPrint((params.dumpPath.value() + "_A.mat").c_str(), A, 0);
                HYPRE_StructVectorPrint((params.dumpPath.value() + "_b.vec").c_str(), b, 0);
            }
        }

        auto getIterNum() {
            int ret;
            HYPRE_StructBiCGSTABGetNumIterations(solver, &ret);
            return ret;
        }

        auto getFinalRes() const {
            Real ret;
            HYPRE_StructBiCGSTABGetFinalRelativeResidualNorm(solver, &ret);
            return ret;
        }

    private:
        HYPRE_StructSolver solver;
    };

}// namespace OpFlow
#endif//OPFLOW_STRUCTSOLVERBICGSTAB_HPP
