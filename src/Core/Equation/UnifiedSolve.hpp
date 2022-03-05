//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2022 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#ifndef OPFLOW_UNIFIEDSOLVE_HPP
#define OPFLOW_UNIFIEDSOLVE_HPP

#include "Core/Equation/AMGCLBackend.hpp"
#include "Core/Equation/Equation.hpp"
#include "Core/Equation/HYPREEqnSolveHandler.hpp"
#include "Core/Solvers/IJ/IJSolver.hpp"
#include "Core/Solvers/SemiStruct/SemiStructSolver.hpp"
#include "Core/Solvers/SemiStruct/SemiStructSolverFAC.hpp"
#include "Core/Solvers/Struct/StructSolver.hpp"
#include "Core/Solvers/Struct/StructSolverBiCGSTAB.hpp"
#include "Core/Solvers/Struct/StructSolverCycRed.hpp"
#include "Core/Solvers/Struct/StructSolverFGMRES.hpp"
#include "Core/Solvers/Struct/StructSolverGMRES.hpp"
#include "Core/Solvers/Struct/StructSolverJacobi.hpp"
#include "Core/Solvers/Struct/StructSolverLGMRES.hpp"
#include "Core/Solvers/Struct/StructSolverNone.hpp"
#include "Core/Solvers/Struct/StructSolverPCG.hpp"
#include "Core/Solvers/Struct/StructSolverPFMG.hpp"
#include "Core/Solvers/Struct/StructSolverPrecond.hpp"
#include "Core/Solvers/Struct/StructSolverSMG.hpp"

namespace OpFlow {
    template <StructSolverType type = StructSolverType::GMRES,
              StructSolverType pType = StructSolverType::None, typename F, StructuredFieldExprType T>
    void Solve(const F& func, T&& target, StructSolverParams<type> params = StructSolverParams<type> {},
               StructSolverParams<pType> precParams = StructSolverParams<pType> {}) {
        auto solver = PrecondStructSolver<type, pType>(params, precParams);
        auto handler = makeEqnSolveHandler(func, target, solver);
        handler->solve();
    }

    template <SemiStructSolverType type = SemiStructSolverType::FAC,
              SemiStructSolverType pType = SemiStructSolverType::None, typename F,
              SemiStructuredFieldExprType T>
    void Solve(const F& func, T&& target,
               SemiStructSolverParams<type> params = SemiStructSolverParams<type> {},
               SemiStructSolverParams<pType> precParams = SemiStructSolverParams<pType> {}) {
        if constexpr (pType != SemiStructSolverType::None) {
            auto solver = PrecondSemiStructSolver<type, pType>(params, precParams);
            auto handler = makeEqnSolveHandler(func, target, solver);
            handler->solve();
        } else {
            auto solver = SemiStructSolver<type>(params);
            auto handler = HYPREEqnSolveHandler<Meta::RealType<F>, Meta::RealType<T>, SemiStructSolver<type>>(
                    func, target, solver);
            handler.solve();
        }
    }

    template <typename S, typename F, FieldExprType T>
    void Solve(F&& func, T&& target, auto&& indexer, IJSolverParams<S> params = IJSolverParams<S> {}) {
        auto handler = makeEqnSolveHandler(func, target, indexer, params);
        handler->solve();
    }

    template <typename S>
    void SolveEqns(auto&&... fs) {
        auto t = std::forward_as_tuple(OP_PERFECT_FOWD(fs)...);
        auto&& [getters, rest1] = Meta::tuple_split<sizeof...(fs) / 2 - 1>(t);
        auto&& [targets, rest2] = Meta::tuple_split<sizeof...(fs) / 2 - 1>(rest1);
        auto& mapper = std::get<0>(rest2);
        auto& params = std::get<1>(rest2);
        auto eqn_holder = makeEqnHolder(getters, targets);
        auto st_holder = makeStencilHolder(eqn_holder);
        std::vector<bool> pin;
        for (const auto& p : params) pin.push_back(p.pinValue);
        auto mat = CSRMatrixGenerator::generate(st_holder, mapper, pin);
        std::vector<Real> x(mat.rhs.size());
        AMGCLBackend<S, Real>::solve(mat, x, params[0].p, params[0].bp, params[0].verbose);
        Meta::static_for<decltype(st_holder)::size>([&]<int i>(Meta::int_<i>) {
            auto target = eqn_holder.template getTargetPtr<i>();
            rangeFor(target->assignableRange, [&](auto&& k) {
                (*target)[k] = x[mapper(DS::ColoredIndex<Meta::RealType<decltype(k)>> {k, i})];
            });
        });
    }
}// namespace OpFlow
#endif