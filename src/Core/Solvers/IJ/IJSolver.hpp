//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2021  by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------
//
//

#ifndef OPFLOW_IJSOLVER_HPP
#define OPFLOW_IJSOLVER_HPP

#include <mpi.h>
#include <optional>

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

#include <amgcl/adapter/crs_tuple.hpp>
#include <amgcl/backend/builtin.hpp>

#include <amgcl/mpi/amg.hpp>
#include <amgcl/mpi/coarsening/smoothed_aggregation.hpp>
#include <amgcl/mpi/distributed_matrix.hpp>
#include <amgcl/mpi/make_solver.hpp>
#include <amgcl/mpi/relaxation/spai0.hpp>
#include <amgcl/mpi/solver/bicgstab.hpp>

#include <amgcl/amg.hpp>
#include <amgcl/coarsening/smoothed_aggregation.hpp>
#include <amgcl/make_solver.hpp>
#include <amgcl/relaxation/spai0.hpp>
#include <amgcl/solver/bicgstab.hpp>

#include <amgcl/io/binary.hpp>
#include <amgcl/profiler.hpp>

#if defined(AMGCL_HAVE_PARMETIS)
#include <amgcl/mpi/partition/parmetis.hpp>
#elif defined(AMGCL_HAVE_SCOTCH)
#include <amgcl/mpi/partition/ptscotch.hpp>
#endif

namespace OpFlow {
    template <typename Solver>
    struct IJSolver {
        IJSolver() = default;
        ~IJSolver() { ; }
        IJSolver(const typename Solver::params& p, const typename Solver::backend_params& bp)
            : params(p), bparams(bp) {}
        IJSolver(const IJSolver& other) : bparams(other.bparams), params(other.params) {
            if (other.solver) solver = std::make_unique<Solver>(*other.solver);
            if (other.A_shared)
                A_shared = std::make_shared<amgcl::mpi::distributed_matrix<typename Solver::backend_type>>(
                        *other.A_shared);
        }

        template <typename T>
        void init(std::ptrdiff_t rows, std::vector<ptrdiff_t>& ptr, std::vector<std::ptrdiff_t>& col,
                  std::vector<T>& val) {
#if defined(OPFLOW_WITH_MPI) && defined(OPFLOW_DISTRIBUTE_MODEL_MPI)
            amgcl::mpi::communicator world(MPI_COMM_WORLD);
            A_shared = std::make_shared<amgcl::mpi::distributed_matrix<typename Solver::backend_type>>(
                    world, std::tie(rows, ptr, col, val));
            OP_INFO("Rank {} Built mat", getWorkerId());
            solver = std::make_unique<Solver>(world, A_shared, params, bparams);
#else
            auto A = std::tie(rows, ptr, col, val);
            solver = std::make_unique<Solver>(A, params, bparams);
#endif
        }

        template <typename T>
        void solve(std::vector<T>& rhs, std::vector<T>& x) {
#if defined(OPFLOW_WITH_MPI) && defined(OPFLOW_DISTRIBUTE_MODEL_MPI)
            std::tie(iters, error) = (*solver)(*A_shared, rhs, x);
#else
            std::tie(iters, error) = (*solver)(rhs, x);
#endif
        }

        [[nodiscard]] std::string logInfo() const {
            std::string ret = fmt::format("Iter = {}, Res = {}\n", iters, error);
            return ret;
        }

    private:
        typename Solver::backend_params bparams;
        typename Solver::params params;
        std::unique_ptr<Solver> solver = nullptr;
        std::shared_ptr<amgcl::mpi::distributed_matrix<typename Solver::backend_type>> A_shared = nullptr;
        int iters {};
        double error {};
    };
}// namespace OpFlow

#endif//OPFLOW_IJSOLVER_HPP
