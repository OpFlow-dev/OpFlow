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

#ifndef OPFLOW_STRUCTSOLVERCYCRED_HPP
#define OPFLOW_STRUCTSOLVERCYCRED_HPP

#include "StructSolver.hpp"

OPFLOW_MODULE_EXPORT namespace OpFlow {
    template <>
    struct StructSolverParams<StructSolverType::CYCRED> {
        std::optional<int> tDim;
        std::optional<std::vector<int>> baseStride;
        MPI_Comm comm = MPI_COMM_WORLD;
        bool staticMat = false;
        bool pinValue = false;
        std::optional<std::string> dumpPath;
    };

    template <>
    struct StructSolver<StructSolverType::CYCRED> {
        using Param = StructSolverParams<StructSolverType::CYCRED>;
        constexpr auto static type = StructSolverType::CYCRED;
        Param params;

        StructSolver() { HYPRE_StructCycRedCreate(params.comm, &solver); }
        StructSolver(const Param& p) : params(p) { HYPRE_StructCycRedCreate(params.comm, &solver); }
        ~StructSolver() { HYPRE_StructCycRedDestroy(solver); }
        StructSolver(const StructSolver& s) : params(s.params) {
            HYPRE_StructCycRedCreate(params.comm, &solver);
        }
        StructSolver(StructSolver&& s) noexcept : params(std::move(s.params)), solver(std::move(s.solver)) {
            s.solver = nullptr;
        }

        void init() {
            if (params.tDim) HYPRE_StructCycRedSetTDim(solver, params.tDim.value());
            //if (params.baseStride) HYPRE_StructCycRedSetBase(solver, params.baseStride)
        }

        auto& getSolver() { return solver; }
        const auto& getSolver() const { return solver; }
        auto getSolveFunc() const { return HYPRE_StructCycRedSolve; }
        auto getSetUpFunc() const { return HYPRE_StructCycRedSetup; }

        auto solve(HYPRE_StructMatrix& A, HYPRE_StructVector& b, HYPRE_StructVector& x) {
            return HYPRE_StructCycRedSolve(solver, A, b, x);
        }

        auto setup(HYPRE_StructMatrix& A, HYPRE_StructVector& b, HYPRE_StructVector& x) {
            return HYPRE_StructCycRedSetup(solver, A, b, x);
        }

    private:
        HYPRE_StructSolver solver;
    };

}// namespace OpFlow
#endif//OPFLOW_STRUCTSOLVERCYCRED_HPP
