//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2026 by the OpFlow developers
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

#include <OpFlow>
#include <gmock/gmock.h>

using namespace OpFlow;
using namespace testing;

TEST(AMGCLTest, Mat1) {
    for (int _t = 0; _t < 1; _t++) {
        std::vector<std::ptrdiff_t> row {0,  5,  10, 15, 20, 25, 30, 35, 40,
                                         45, 50, 55, 60, 65, 70, 75, 80, 85},
                col {1,  0, 4,  5,  6,  2,  1,  0,  5,  6,  3,  2,  1,      6, 7,      3,           2,
                     7,  8, 9,  5,  4,  8,  0,  9,  6,  5,  4,  9,  1,      7, 6,      5,           10,
                     2,  7, 6,  11, 3,  12, 9,  8,  12, 4,  13, 10, 9,      8, 13,     5,           11,
                     10, 9, 14, 6,  11, 10, 15, 7,  6,  13, 12, 8,  7,      6, 14,     13,          12,
                     9,  8, 15, 14, 13, 10, 9,  15, 14, 11, 10, 9,  -90000, 0, 800000, -12000000000};
        std::vector<double> val {-1, 6, -1, 0,  0, -1, 5, -1, -1, 0,  -1, 5, -1, -1, 0,  6, -1, -1, 0,  0,
                                 -1, 5, -1, -1, 0, -1, 4, -1, -1, -1, -1, 4, -1, -1, -1, 5, -1, -1, -1, 0,
                                 -1, 5, -1, -1, 0, -1, 4, -1, -1, -1, -1, 4, -1, -1, -1, 5, -1, -1, -1, 0,
                                 -1, 6, -1, 0,  0, -1, 5, -1, -1, 0,  -1, 5, -1, -1, 0,  6, -1, -1, 0,  0},
                rhs {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, x;
        x.resize(rhs.size());

        typedef amgcl::backend::builtin<double> SBackend;
        typedef amgcl::backend::builtin<float> PBackend;
        typedef amgcl::make_solver<
                amgcl::amg<PBackend, amgcl::coarsening::smoothed_aggregation, amgcl::relaxation::spai0>,
                amgcl::solver::bicgstab<SBackend>>
                PSolver;

        auto rc = x.size();
        auto A = amgcl::adapter::zero_copy(rc, row.data(), col.data(), val.data());

        IJSolverParams<PSolver> p;
        PSolver solver(*A, p.p, p.bp);
        solver(rhs, x);

        ASSERT_NEAR(x[0], -0.375, 1e-10);
    }
}