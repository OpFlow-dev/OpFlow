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

#include <OpFlow>
#include <gmock/gmock.h>

using namespace OpFlow;
using namespace testing;

class AMGCLTest : public Test {
protected:
    ParallelPlan ori_plan;
    void SetUp() override {
        ori_plan = getGlobalParallelPlan();
        auto info = makeParallelInfo();
        info.threadInfo.thread_count = std::min(info.threadInfo.thread_count, 4);
        setGlobalParallelInfo(info);
        setGlobalParallelPlan(makeParallelPlan(getGlobalParallelInfo(), ParallelIdentifier::SharedMem));

        m = MeshBuilder<Mesh>().newMesh(65, 65).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();
        p = ExprBuilder<Field>()
                    .setMesh(m)
                    .setName("p")
                    .setBC(0, DimPos::start, BCType::Dirc, 0.)
                    .setBC(0, DimPos::end, BCType::Dirc, 0.)
                    .setBC(1, DimPos::start, BCType::Dirc, 0.)
                    .setBC(1, DimPos::end, BCType::Dirc, 0.)
                    .setExt(0, DimPos::start, 1)
                    .setExt(0, DimPos::end, 1)
                    .setExt(1, DimPos::start, 1)
                    .setExt(1, DimPos::end, 1)
                    .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                    .build();
        p_true = p;
        p_true.name = "ptrue";
        r = ExprBuilder<Field>()
                    .setMesh(m)
                    .setName("r")
                    .setBC(0, DimPos::start, BCType::Dirc, 1.)
                    .setBC(0, DimPos::end, BCType::Dirc, 1.)
                    .setBC(1, DimPos::start, BCType::Dirc, 1.)
                    .setBC(1, DimPos::end, BCType::Dirc, 1.)
                    .setExt(0, DimPos::start, 1)
                    .setExt(0, DimPos::end, 1)
                    .setExt(1, DimPos::start, 1)
                    .setExt(1, DimPos::end, 1)
                    .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                    .build();
        b = p;
        b.name = "b";
        p_true.initBy([&](auto&& x) { return x[0] * (1. - x[0]) * x[1] * (1. - x[1]); });
    }

    void TearDown() override { setGlobalParallelPlan(ori_plan); }

    void reset_case(double xc, double yc) {
        r.initBy([&](auto&& x) {
            auto dist = Math::norm2(x[0] - xc, x[1] - yc);
            auto hevi = Math::smoothHeviside(r.getMesh().dx(0, 0) * 8, dist - 0.2);
            return 1. * hevi + (1. - hevi) * 1000;
        });
        b = dx<D1FirstOrderCenteredUpwind>(dx<D1FirstOrderCenteredDownwind>(p_true)
                                           / d1IntpCenterToCorner<0>((r)))
            + dy<D1FirstOrderCenteredUpwind>(dy<D1FirstOrderCenteredDownwind>(p_true)
                                             / d1IntpCenterToCorner<1>(r));
        p = 0.;
    }

    bool check_solution(double rel = 1e-10) {
        auto res = p - p_true;
        res.prepare();
        bool ret = true;
        rangeFor_s(p.assignableRange, [&](auto&& i) {
            auto c_res = res.evalAt(i);
            auto p_ref = p_true.evalAt(i);
            auto rel_res = std::abs(c_res) / std::abs(p_ref);
            if (std::isnan(c_res)) {
                OP_ERROR("Check fail: res = nan @ {}", i.toString());
                ret = false;
            }
            if (rel_res > rel) {
                OP_ERROR("Check fail: res = {} / {} @ {}", c_res, rel_res, i.toString());
                ret = false;
            }
        });
        return ret;
    }

    auto poisson_eqn() {
        return [&](auto&& e) {
            return b
                   == dx<D1FirstOrderCenteredUpwind>(dx<D1FirstOrderCenteredDownwind>(e)
                                                     / d1IntpCenterToCorner<0>((r)))
                              + dy<D1FirstOrderCenteredUpwind>(dy<D1FirstOrderCenteredDownwind>(e)
                                                               / d1IntpCenterToCorner<1>(r));
        };
    }

    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;
    Mesh m;
    Field p, r, b, p_true;
};

TEST_F(AMGCLTest, GeneralCSR) {
    typedef amgcl::backend::builtin<double> SBackend;
#ifdef MIXED_PRECISION
    typedef amgcl::backend::builtin<float> PBackend;
#else
    typedef amgcl::backend::builtin<double> PBackend;
#endif
    typedef amgcl::make_solver<
            amgcl::amg<PBackend, amgcl::coarsening::smoothed_aggregation, amgcl::relaxation::spai0>,
            amgcl::solver::bicgstab<SBackend>>
            Solver;
    SBackend ::params p;
    PBackend ::params bp;

    std::vector<int> row, col;
    std::vector<double> val, x, rhs;
    // 3x3 poisson equation
    row.resize(10);
    col.resize(5 * 9);
    val.resize(5 * 9);
    x.resize(9);
    rhs.resize(9);
    val.assign(45, 0.);
    auto trim = [](int i) { return std::min(std::max(0, i), 8); };

    for (int j = 0; j < 3; ++j) {
        for (int i = 0; i < 3; ++i) {
            int rank = i + 3 * j;
            OP_INFO("{} {}", i, j);
            row[rank] = 5 * rank;
            col[5 * rank] = rank;
            col[5 * rank + 1] = rank - 1;
            col[5 * rank + 2] = rank + 1;
            col[5 * rank + 3] = rank - 3;
            col[5 * rank + 4] = rank + 3;
            val[5 * rank] = 4.;
            if (i != 0) val[5 * rank + 1] = -1.;
            if (i != 2) val[5 * rank + 2] = -1.;
            if (j != 0) val[5 * rank + 3] = -1.;
            if (j != 2) val[5 * rank + 4] = -1.;
        }
    }
    row.back() = 46;

    rhs.assign(9, 1.);
    x.assign(9, 0.);

    int n = 9;
    int iters;
    double err;
    OP_INFO("1");
    auto A = std::tie(n, row, col, val);
    OP_INFO("2");
    Solver solve(A);
    std::tie(iters, err) = solve(A, rhs, x);
    OP_INFO("Iters = {}, error = {}", iters, err);
    ASSERT_TRUE(true);
}