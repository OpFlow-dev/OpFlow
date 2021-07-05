#ifndef OPFLOW_UNIFIEDSOLVE_HPP
#define OPFLOW_UNIFIEDSOLVE_HPP

#include "Core/Equation/EqnSolveHandler.hpp"
#include "Core/Equation/Equation.hpp"
#include "Core/Solvers/SemiStruct/SemiStructSolver.hpp"
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
              StructSolverType pType = StructSolverType::None, EquationType E, StructuredFieldExprType T>
    void Solve(const E& equation, T&& target, StructSolverParams<type> params = StructSolverParams<type> {},
               StructSolverParams<pType> precParams = StructSolverParams<pType> {}) {
        auto solver = PrecondStructSolver<type, pType>(params, precParams);
        auto handler = EqnSolveHandler([&](auto&&) { return equation; }, target, solver);
        handler.solve();
    }

    template <StructSolverType type = StructSolverType::GMRES,
              StructSolverType pType = StructSolverType::None, EquationType E, StructuredFieldExprType T>
    void Solve(const std::vector<E>& equations, std::vector<T>& targets,
               StructSolverParams<type> params = StructSolverParams<type> {},
               StructSolverParams<pType> precParams = StructSolverParams<pType> {}) {
        OP_ASSERT(equations.size() == targets.size());
        auto solver = PrecondStructSolver<type, pType>(params, precParams);
        std::vector<std::function<E(T&)>> getters(targets.size());
        for (auto i = 0; i < targets.size(); ++i) {
            getters[i] = [&](auto&& e) { return equations[i]; };
        }
        auto handler = EqnSolveHandler(getters, targets, solver);
        handler.solve();
    }

    template <StructSolverType type = StructSolverType::GMRES,
              StructSolverType pType = StructSolverType::None, typename F, StructuredFieldExprType T>
    void Solve(const F& func, T&& target, StructSolverParams<type> params = StructSolverParams<type> {},
               StructSolverParams<pType> precParams = StructSolverParams<pType> {}) {
        auto solver = PrecondStructSolver<type, pType>(params, precParams);
        auto handler = EqnSolveHandler(func, target, solver);
        handler.solve();
    }

    template <StructSolverType type = StructSolverType::GMRES,
              StructSolverType pType = StructSolverType::None, typename F, StructuredFieldExprType T>
    void Solve(const std::vector<F>& func, std::vector<T>& targets,
               StructSolverParams<type> params = StructSolverParams<type> {},
               StructSolverParams<pType> precParams = StructSolverParams<pType> {}) {
        auto solver = PrecondStructSolver<type, pType>(params, precParams);
        auto handler = EqnSolveHandler(func, targets, solver);
        handler.solve();
    }

    template <StructSolverType type = StructSolverType::GMRES,
              StructSolverType pType = StructSolverType::None>
    void levelSolve(auto& level, auto&& eqn_generator, auto&& target_getter,
                    StructSolverParams<type> params = StructSolverParams<type> {},
                    StructSolverParams<pType> precParams = StructSolverParams<pType> {}) {
        auto solver = PrecondStructSolver<type, pType>(params, precParams);
        using ZoneType = Meta::RealType<decltype(level.zones[0])>;
        using TargetType = Meta::RealType<decltype(target_getter(std::declval<ZoneType&>()))>;
        using StencilFieldType = StencilField<TargetType>;
        using EqnType = Meta::RealType<decltype(
                eqn_generator(std::declval<ZoneType&>())(std::declval<StencilField<TargetType>&>()))>;
        constexpr static auto dim = internal::StructuredFieldExprTrait<TargetType>::dim;
        std::vector<std::function<EqnType(StencilFieldType&)>> getters;
        std::vector<TargetType> targets;
        getters.reserve(level.zones.size());
        targets.reserve(level.zones.size());

        for (auto i = 0; i < level.zones.size(); ++i) {
            getters.push_back(eqn_generator(level.zones[i]));
            targets.push_back(target_getter(level.zones[i]));
        }
        auto handler = EqnSolveHandler(getters, targets, solver);
        handler.solve();
    }
}// namespace OpFlow
#endif//OPFLOW_UNIFIEDSOLVE_HPP
