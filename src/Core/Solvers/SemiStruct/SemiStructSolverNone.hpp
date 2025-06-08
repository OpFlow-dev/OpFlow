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

#ifndef OPFLOW_SEMISTRUCTSOLVERNONE_HPP
#define OPFLOW_SEMISTRUCTSOLVERNONE_HPP

#include "SemiStructSolver.hpp"

OPFLOW_MODULE_EXPORT namespace OpFlow {

    // note: the None solver type is only created for unification of the upper level solver API
    // where None indicates un-preconditioned solver. Any use of these types otherwise is illegal.
    template <>
    struct SemiStructSolverParams<SemiStructSolverType::None> {};

    template <>
    struct SemiStructSolver<SemiStructSolverType::None> {
        using Param = SemiStructSolverParams<SemiStructSolverType::None>;
        constexpr auto static type = SemiStructSolverType::None;
        Param params;

        SemiStructSolver() = default;
        SemiStructSolver(const Param& p) : params(p) {}

        void init() {}
        auto solve(auto&&...) {}
        auto setup(auto&&...) {}

        auto& getSolver() { return solver; }
        const auto& getSolver() const { return solver; }
        auto getSolveFunc() const { return HYPRE_SStructDiagScale; }
        auto getSetUpFunc() const { return HYPRE_SStructDiagScaleSetup; }

        auto getIterNum() const { return 0; }
        auto getFinalRes() const { return Real(0.); }

    private:
        HYPRE_SStructSolver solver;
    };
}// namespace OpFlow
#endif//OPFLOW_SEMISTRUCTSOLVERNONE_HPP
