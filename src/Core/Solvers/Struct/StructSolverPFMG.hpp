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

#ifndef OPFLOW_STRUCTSOLVERPFMG_HPP
#define OPFLOW_STRUCTSOLVERPFMG_HPP

#include "StructSolver.hpp"

namespace OpFlow {
    template <>
    struct StructSolverParams<StructSolverType::PFMG> : StructSolverParamsBase {
        std::optional<int> maxLevels, relChange;
        std::optional<bool> useZeroGuess;
        std::optional<int> relaxType;
        std::optional<Real> jacobiWeight;
        std::optional<int> rapType;
        std::optional<int> numPreRelax, numPostRelax, skipRelax;
        std::optional<std::vector<Real>> dxyz;
        std::optional<int> logging, printLevel;
    };

    template <>
    struct StructSolver<StructSolverType::PFMG> {
        using Param = StructSolverParams<StructSolverType::PFMG>;
        constexpr auto static type = StructSolverType::PFMG;
        Param params;

        StructSolver() { HYPRE_StructPFMGCreate(params.comm, &solver); }
        StructSolver(const Param& p) : params(p) { HYPRE_StructPFMGCreate(params.comm, &solver); }
        ~StructSolver() { HYPRE_StructPFMGDestroy(solver); }
        StructSolver(const StructSolver& s) : params(s.params) {
            HYPRE_StructPFMGCreate(params.comm, &solver);
        }
        StructSolver(StructSolver&& s) noexcept : params(std::move(s.params)), solver(std::move(s.solver)) {
            s.solver = nullptr;
        }

        void init() {
            if (params.tol) HYPRE_StructPFMGSetTol(solver, params.tol.value());
            if (params.maxIter) HYPRE_StructPFMGSetMaxIter(solver, params.maxIter.value());
            if (params.maxLevels) HYPRE_StructPFMGSetMaxLevels(solver, params.maxLevels.value());
            if (params.relChange) HYPRE_StructPFMGSetRelChange(solver, params.relChange.value());
            if (params.useZeroGuess && params.useZeroGuess.value_or(false))
                HYPRE_StructPFMGSetZeroGuess(solver);
            if (params.useZeroGuess && !params.useZeroGuess.value_or(true))
                HYPRE_StructPFMGSetNonZeroGuess(solver);
            if (params.relaxType) HYPRE_StructPFMGSetRelaxType(solver, params.relaxType.value());
            if (params.relaxType.value_or(1) && params.jacobiWeight)
                HYPRE_StructPFMGSetJacobiWeight(solver, params.jacobiWeight.value());
            if (params.rapType) HYPRE_StructPFMGSetRAPType(solver, params.rapType.value());
            if (params.numPreRelax) HYPRE_StructPFMGSetNumPreRelax(solver, params.numPreRelax.value());
            if (params.numPostRelax) HYPRE_StructPFMGSetNumPostRelax(solver, params.numPostRelax.value());
            if (params.skipRelax) HYPRE_StructPFMGSetSkipRelax(solver, params.skipRelax.value());
            if (params.dxyz) HYPRE_StructPFMGSetDxyz(solver, params.dxyz.value().data());
            if (params.logging) HYPRE_StructPFMGSetLogging(solver, params.logging.value());
            if (params.printLevel) HYPRE_StructPFMGSetPrintLevel(solver, params.printLevel.value());
        }

        void reinit() {
            HYPRE_StructPFMGDestroy(solver);
            HYPRE_StructPFMGCreate(params.comm, &solver);
            init();
        }

        auto solve(HYPRE_StructMatrix& A, HYPRE_StructVector& b, HYPRE_StructVector& x) {
            return HYPRE_StructPFMGSolve(solver, A, b, x);
        }

        auto setup(HYPRE_StructMatrix& A, HYPRE_StructVector& b, HYPRE_StructVector& x) {
            return HYPRE_StructPFMGSetup(solver, A, b, x);
        }

        auto& getSolver() { return solver; }
        const auto& getSolver() const { return solver; }
        auto getSolveFunc() const { return HYPRE_StructPFMGSolve; }
        auto getSetUpFunc() const { return HYPRE_StructPFMGSetup; }

        auto getIterNum() const {
            int ret;
            HYPRE_StructPFMGGetNumIterations(solver, &ret);
            return ret;
        }

        auto getFinalRes() const {
            Real ret;
            HYPRE_StructPFMGGetFinalRelativeResidualNorm(solver, &ret);
            return ret;
        }

    private:
        HYPRE_StructSolver solver;
    };

}// namespace OpFlow
#endif//OPFLOW_STRUCTSOLVERPFMG_HPP
