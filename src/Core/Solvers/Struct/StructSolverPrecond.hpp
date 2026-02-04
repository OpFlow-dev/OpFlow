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

#ifndef OPFLOW_STRUCTSOLVERPRECOND_HPP
#define OPFLOW_STRUCTSOLVERPRECOND_HPP

#include "StructSolver.hpp"

OPFLOW_MODULE_EXPORT namespace OpFlow {
    template <StructSolverType Type, StructSolverType PrecondType>
    struct PrecondStructSolver {
        constexpr auto static type = Type;
        constexpr auto static precondType = PrecondType;
        using Param = StructSolverParams<type>;
        using PrecondParam = StructSolverParams<precondType>;
        Param params;
        PrecondParam precondParam;

        PrecondStructSolver() = default;
        PrecondStructSolver(const Param& p, const PrecondParam& pp)
            : solver(p), precond(pp), params(p), precondParam(pp) {}

        void init() {
            solver.init();
            precond.init();
            solver.setPrecond(precond.getSolveFunc(), precond.getSetUpFunc(), precond.getSolver());
        }

        void reinit() {
            solver.reinit();
            precond.reinit();
            solver.setPrecond(precond.getSolveFunc(), precond.getSetUpFunc(), precond.getSolver());
        }

        auto& getSolver() { return solver.getSolver(); }
        const auto& getSolver() const { return solver.getSolver(); }
        auto getSolveFunc() const { return solver.getSolveFunc(); }
        auto getSetUpFunc() const { return solver.getSetUpFunc(); }

        auto solve(HYPRE_StructMatrix& A, HYPRE_StructVector& b, HYPRE_StructVector& x) {
            return solver.solve(A, b, x);
        }

        auto setup(HYPRE_StructMatrix& A, HYPRE_StructVector& b, HYPRE_StructVector& x) {
            return solver.setup(A, b, x);
        }

        void dump(HYPRE_StructMatrix& A, HYPRE_StructVector& b) {
            if (params.dumpPath) {
                HYPRE_StructMatrixPrint((params.dumpPath.value() + "_A.mat").c_str(), A, 0);
                HYPRE_StructVectorPrint((params.dumpPath.value() + "_b.vec").c_str(), b, 0);
            }
        }

        auto getIterNum() { return solver.getIterNum(); }

        auto getFinalRes() const { return solver.getFinalRes(); }

    private:
        StructSolver<type> solver;
        StructSolver<precondType> precond;
    };

}// namespace OpFlow
#endif//OPFLOW_STRUCTSOLVERPRECOND_HPP
