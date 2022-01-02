//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2021 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_SEMISTRUCTSOLVERFAC_HPP
#define OPFLOW_SEMISTRUCTSOLVERFAC_HPP

#include "SemiStructSolver.hpp"

namespace OpFlow {
    template <>
    struct SemiStructSolverParams<SemiStructSolverType::FAC> : SemiStructSolverParamsBase {
        std::optional<int> logging;
    };

    template <>
    struct SemiStructSolver<SemiStructSolverType::FAC> {
        using Param = SemiStructSolverParams<SemiStructSolverType::FAC>;
        constexpr auto static type = SemiStructSolverType::FAC;
        Param params;

        SemiStructSolver() { HYPRE_SStructFACCreate(params.comm, &solver); }
        SemiStructSolver(const Param& p) : params(p) { HYPRE_SStructFACCreate(params.comm, &solver); }
        ~SemiStructSolver() {
            //if (initialized) HYPRE_SStructFACDestroy2(solver);
        }
        SemiStructSolver(const SemiStructSolver& s) : params(s.params) {
            HYPRE_SStructFACCreate(params.comm, &solver);
        }
        SemiStructSolver(SemiStructSolver&& s) noexcept
            : params(std::move(s.params)), solver(std::move(s.solver)) {
            s.solver = nullptr;
        }

        void init() {
            if (params.tol) HYPRE_SStructFACSetTol(solver, params.tol.value());
            if (params.maxIter) HYPRE_SStructFACSetMaxIter(solver, params.maxIter.value());
            if (params.logging) HYPRE_SStructFACSetLogging(solver, params.logging.value());
        }

        void setPrecond(auto&&...) {}
        auto& getSolver() { return solver; }
        const auto& getSolver() const { return solver; }
        auto getSolveFunc() const { return HYPRE_SStructFACSolve3; }
        auto getSetUpFunc() const { return HYPRE_SStructFACSetup2; }

        auto solve(HYPRE_SStructMatrix& A, HYPRE_SStructVector& b, HYPRE_SStructVector& x) {
            return HYPRE_SStructFACSolve3(solver, A, b, x);
        }

        auto setup(HYPRE_SStructMatrix& A, HYPRE_SStructVector& b, HYPRE_SStructVector& x) {
            initialized = true;
            return HYPRE_SStructFACSetup2(solver, A, b, x);
        }

        void dump(HYPRE_SStructMatrix& A, HYPRE_SStructVector& b) {
            if (params.dumpPath) {
                HYPRE_SStructMatrixPrint((params.dumpPath.value() + "_A.mat").c_str(), A, 0);
                HYPRE_SStructVectorPrint((params.dumpPath.value() + "_b.mat").c_str(), b, 0);
            }
        }

        auto getIterNum() {
            int ret;
            HYPRE_SStructFACGetNumIterations(solver, &ret);
            return ret;
        }
        auto getFinalRes() const {
            Real ret;
            HYPRE_SStructFACGetFinalRelativeResidualNorm(solver, &ret);
            return ret;
        }

    private:
        HYPRE_SStructSolver solver;
        bool initialized = false;
    };
}// namespace OpFlow
#endif//OPFLOW_SEMISTRUCTSOLVERFAC_HPP
