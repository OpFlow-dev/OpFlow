#ifndef OPFLOW_STRUCTSOLVER_HPP
#define OPFLOW_STRUCTSOLVER_HPP

#include <HYPRE.h>
#include <optional>

namespace OpFlow {

    enum class StructSolverType {
        None, /* None only for un-preconditioned cases */
        Jacobi,
        SMG,
        PFMG,
        CYCRED,
        PCG,
        GMRES,
        FGMRES,
        LGMRES,
        BICGSTAB
    };

    template <StructSolverType type>
    struct StructSolverParams;

    struct StructSolverParamsBase {
        // common params
        std::optional<Real> tol {};
        std::optional<int> maxIter;
        MPI_Comm comm = MPI_COMM_WORLD;
        bool staticMat = false;
        bool pinValue = false;
        std::optional<std::string> dumpPath {};
    };

    template <StructSolverType type>
    struct StructSolver;

}// namespace OpFlow
#endif//OPFLOW_STRUCTSOLVER_HPP
