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

#ifndef OPFLOW_STRUCTSOLVERNONE_HPP
#define OPFLOW_STRUCTSOLVERNONE_HPP

#include "StructSolver.hpp"

namespace OpFlow {

    // note: the None solver type is only created for unification of the upper level solver API
    // where None indicates un-preconditioned solver. Any use of these types otherwise is illegal.
    template <>
    struct StructSolverParams<StructSolverType::None> {};

    template <>
    struct StructSolver<StructSolverType::None> {
        using Param = StructSolverParams<StructSolverType::None>;
        constexpr auto static type = StructSolverType::None;
        Param params;

        StructSolver() = default;
        StructSolver(const Param& p) : params(p) {}

        void init() {}
        auto solve(auto&&...) {}
        auto setup(auto&&...) {}

        auto& getSolver() { return solver; }
        const auto& getSolver() const { return solver; }
        auto getSolveFunc() const { return HYPRE_StructDiagScale; }
        auto getSetUpFunc() const { return HYPRE_StructDiagScaleSetup; }

        auto getIterNum() const { return 0; }
        auto getFinalRes() const { return Real(0.); }

    private:
        HYPRE_StructSolver solver;
    };
}// namespace OpFlow
#endif//OPFLOW_STRUCTSOLVERNONE_HPP
