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

#ifndef OPFLOW_AMGCLBACKEND_HPP
#define OPFLOW_AMGCLBACKEND_HPP

#include "DataStructures/Matrix/CSRMatrix.hpp"
#include <amgcl/adapter/zero_copy.hpp>
#ifdef OPFLOW_WITH_MPI
#include <amgcl/mpi/distributed_matrix.hpp>
#include <amgcl/mpi/make_solver.hpp>
#endif
#include "EqnSolveHandler.hpp"
#include <optional>
#include <tuple>

namespace OpFlow {
    template <typename Solver, typename D>
    struct AMGCLBackend {
        constexpr static bool _enable_mpi = !requires { typename Solver::col_type; };

        // the static solver which performs a fresh solve on each invoke
        static EqnSolveState solve(const DS::CSRMatrix& mat, std::vector<D>& x, typename Solver::params p,
                                   typename Solver::backend_params bp, bool verbose = false) {
            int rows = mat.row.size() - 1;
            std::unique_ptr<Solver> solver;
#if defined(OPFLOW_WITH_MPI)
            if constexpr (_enable_mpi) {
                amgcl::mpi::communicator world(MPI_COMM_WORLD);
                auto A = std::make_shared<amgcl::mpi::distributed_matrix<typename Solver::backend_type>>(
                        world,
                        *amgcl::adapter::zero_copy(rows, mat.row.begin(), mat.col.begin(), mat.val.begin()));
                solver = std::make_unique<Solver>(world, A, p, bp);
            } else {
                auto A = amgcl::adapter::zero_copy(rows, mat.row.begin(), mat.col.begin(), mat.val.begin());
                //auto A_tie = std::tie(rows, mat.row, mat.col, mat.val);
                solver = std::make_unique<Solver>(*A, p, bp);
            }
#else
            auto A = amgcl::adapter::zero_copy(rows, mat.row.begin(), mat.col.begin(), mat.val.begin());
            //auto A_tie = std::tie(rows, mat.row, mat.col, mat.val);
            solver = std::make_unique<Solver>(*A, p, bp);
#endif
            //Solver solver(A_tie, p, bp);
            int iters;
            double error;
            std::tie(iters, error) = (*solver)(mat.rhs, x);
            if (verbose) { OP_INFO("AMGCL report: iter = {}, relerr = {}", iters, error); }
            return EqnSolveState {iters, error};
        }

        // the dynamic solver which tries to reuse the built preconditioner before
        EqnSolveState solve_dy(const DS::CSRMatrix& mat, std::vector<D>& x, typename Solver::params p,
                               typename Solver::backend_params bp, bool verbose = false) {
            rebuild_solver(mat, p, bp);
            OP_ASSERT_MSG(solver, "AMGCLBackend: solver not initialized.");
            auto [iters, error] = (*solver)(mat.rhs, x);
            if (verbose) { OP_INFO("AMGCL report: iter = {}, relerr = {}", iters, error); }
            solve_counter++;
            return EqnSolveState {iters, error};
        }

    private:
        void rebuild_solver(const DS::CSRMatrix& mat, typename Solver::params& p,
                            typename Solver::backend_params& bp) {
            if (!solver || rebuilt_period.has_value() && solve_counter % rebuilt_period.value() == 0) {
                int rows = mat.row.size() - 1;
#if defined(OPFLOW_WITH_MPI)
                if constexpr (_enable_mpi) {
                    amgcl::mpi::communicator world(MPI_COMM_WORLD);
                    auto A = std::make_shared<amgcl::mpi::distributed_matrix<typename Solver::backend_type>>(
                            world, *amgcl::adapter::zero_copy(rows, mat.row.begin(), mat.col.begin(),
                                                              mat.val.begin()));
                    solver = std::make_unique<Solver>(world, A, p, bp);
                } else {
                    auto A = amgcl::adapter::zero_copy(rows, mat.row.begin(), mat.col.begin(),
                                                       mat.val.begin());
                    solver = std::make_unique<Solver>(*A, p, bp);
                }
#else
                auto A = amgcl::adapter::zero_copy(rows, mat.row.begin(), mat.col.begin(), mat.val.begin());
                solver = std::make_unique<Solver>(*A, p, bp);
#endif
            }
        }
        std::unique_ptr<Solver> solver;
        unsigned long long solve_counter = 0;
        std::optional<int> rebuilt_period {};
    };
}// namespace OpFlow

#endif//OPFLOW_AMGCLBACKEND_HPP
