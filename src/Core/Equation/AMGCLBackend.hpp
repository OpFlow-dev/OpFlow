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
#include <tuple>

namespace OpFlow {
    template <typename Solver, typename D>
    struct AMGCLBackend {
        static void solve(const DS::CSRMatrix& mat, std::vector<D>& x, typename Solver::params p,
                          typename Solver::backend_params bp, bool verbose = false) {
            int rows = mat.row.size() - 1;
            auto A_tie = std::tie(rows, mat.row, mat.col, mat.val);
            Solver solver(A_tie, p, bp);
            int iters;
            double error;
            std::tie(iters, error) = solver(mat.rhs, x);
            if (verbose) { OP_INFO("AMGCL report: iter = {}, relerr = {}", iters, error); }
        }
    };
}// namespace OpFlow

#endif//OPFLOW_AMGCLBACKEND_HPP
