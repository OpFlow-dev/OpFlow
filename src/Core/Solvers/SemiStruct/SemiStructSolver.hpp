#ifndef OPFLOW_SEMISTRUCTSOLVER_HPP
#define OPFLOW_SEMISTRUCTSOLVER_HPP

#include <optional>
#include <mpi.h>

namespace OpFlow {
    enum class SemiStructSolverType {
        None,// only for un-preconditioned cases
        Jacobi,
        SMG,
        PFMG,
        Split,
        SysPFMG,
        FAC,
        Maxwell,
        AMG,
        AMS,
        ADS,
        MLI,
        ParaSails,
        Euclid,
        PILUT,
        PCG,
        GMRES,
        FGMRES,
        LGMRES,
        BiCGSTAB,
        Hybird,
        LOBPCG
    };

    template <SemiStructSolverType type>
    struct SemiStructSolverParams;

    struct SemiStructSolverParamsBase {
        // common params
        std::optional<Real> tol {};
        std::optional<int> maxIter;
        MPI_Comm comm = MPI_COMM_WORLD;
        bool staticMat = false;
        bool pinValue = false;
        std::optional<std::string> dumpPath {};
    };

    template <SemiStructSolverType type>
    struct SemiStructSolver;
}// namespace OpFlow
#endif//OPFLOW_SEMISTRUCTSOLVER_HPP
