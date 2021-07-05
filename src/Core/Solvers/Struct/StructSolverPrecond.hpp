#ifndef OPFLOW_STRUCTSOLVERPRECOND_HPP
#define OPFLOW_STRUCTSOLVERPRECOND_HPP

#include "StructSolver.hpp"

namespace OpFlow {
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

        auto& getSolver() { return solver; }
        const auto& getSolver() const { return solver; }
        auto getSolveFunc() const { return solver.getSolveFunc(); }
        auto getSetUpFunc() const { return solver.getSetUpFunc(); }

        auto solve(HYPRE_StructMatrix& A, HYPRE_StructVector& b, HYPRE_StructVector& x) {
            return solver.solve(A, b, x);
        }

        auto setup(HYPRE_StructMatrix& A, HYPRE_StructVector& b, HYPRE_StructVector& x) {
            return solver.setup(A, b, x);
        }

        auto getIterNum() { return solver.getIterNum(); }

        auto getFinalRes() const { return solver.getFinalRes(); }

    private:
        StructSolver<type> solver;
        StructSolver<precondType> precond;
    };

}// namespace OpFlow
#endif//OPFLOW_STRUCTSOLVERPRECOND_HPP
