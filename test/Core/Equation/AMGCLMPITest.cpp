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

#include <OpFlow>
#include <gmock/gmock.h>

using namespace OpFlow;

class AMGCLMPITest : public testing::Test {
protected:
    void SetUp() override {
        m = MeshBuilder<Mesh>().newMesh(65, 65).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();
        strategy = std::make_shared<EvenSplitStrategy<Field>>();

        p = ExprBuilder<Field>()
                    .setMesh(m)
                    .setName("p")
                    .setBC(0, DimPos::start, BCType::Dirc, 0.)
                    .setBC(0, DimPos::end, BCType::Dirc, 0.)
                    .setBC(1, DimPos::start, BCType::Dirc, 0.)
                    .setBC(1, DimPos::end, BCType::Dirc, 0.)
                    .setExt(1)
                    .setPadding(1)
                    .setSplitStrategy(strategy)
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
                    .setExt(2)
                    .setPadding(1)
                    .setSplitStrategy(strategy)
                    .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                    .build();
        b = p;
        b.name = "b";
        p_true.initBy([&](auto&& x) { return x[0] * (1. - x[0]) * x[1] * (1. - x[1]); });
    }

    void reset_case(double xc, double yc) {
        r.initBy([&](auto&& x) {
            auto dist = Math::norm2(x[0] - xc, x[1] - yc);
            auto hevi = Math::smoothHeviside(r.getMesh().dx(0, 0) * 8, dist - 0.2);
            return 1. * hevi + (1. - hevi) * 1000;
        });
        b = dx<D1FirstOrderCentered>(dx<D1FirstOrderCentered>(p_true) / d1IntpCenterToCorner<0>((r)))
            + dy<D1FirstOrderCentered>(dy<D1FirstOrderCentered>(p_true) / d1IntpCenterToCorner<1>(r));
        p = 0.;
    }

    bool check_solution(double rel = 1e-10) {
        auto res = p - p_true;
        res.prepare();
        bool ret = true;
        rangeFor_s(p.getLocalWritableRange(), [&](auto&& i) {
            auto c_res = res.evalAt(i);
            auto p_ref = p_true.evalAt(i);
            auto rel_res = std::abs(c_res) / std::abs(p_ref);
            if (std::isnan(c_res)) {
                OP_ERROR("Check fail: res = nan @ {}", i);
                ret = false;
            }
            if (rel_res > rel) {
                OP_ERROR("Check fail: res = {} / {} @ {}", c_res, rel_res, i);
                ret = false;
            }
        });
        return ret;
    }

    auto poisson_eqn() {
        return [&](auto&& e) {
            return b
                   == dx<D1FirstOrderCentered>(dx<D1FirstOrderCentered>(e) / d1IntpCenterToCorner<0>((r)))
                              + dy<D1FirstOrderCentered>(dy<D1FirstOrderCentered>(e)
                                                         / d1IntpCenterToCorner<1>(r));
        };
    }

    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;
    Mesh m;
    Field p, r, b, p_true;
    std::shared_ptr<AbstractSplitStrategy<Field>> strategy;
};

TEST_F(AMGCLMPITest, AMGCLUnifiedSolveGMRES) {
    this->reset_case(0.5, 0.5);
    using DBackend = amgcl::backend::builtin<double>;
    using Solver = amgcl::mpi::make_solver<
            amgcl::mpi::amg<DBackend, amgcl::mpi::coarsening::smoothed_aggregation<DBackend>,
                            amgcl::mpi::relaxation::spai0<DBackend>>,
            amgcl::mpi::solver::gmres<DBackend>>;
    IJSolverParams<Solver> params;
    params.verbose = true;
    OP_INFO("Solve poisson equation with AMGCL");
    Solve<Solver>(poisson_eqn(), p,
                  DS::BlockedMDRangeMapper<2> {
                          strategy->getSplitMap(p.getMesh().getRange(), getGlobalParallelPlan())},
                  params);
    ASSERT_TRUE(check_solution(2e-8));
}

TEST_F(AMGCLMPITest, AMGCLHandlerSolveGMRES) {
    this->reset_case(0.5, 0.5);
    using DBackend = amgcl::backend::builtin<double>;
    using Solver = amgcl::mpi::make_solver<
            amgcl::mpi::amg<DBackend, amgcl::mpi::coarsening::smoothed_aggregation<DBackend>,
                            amgcl::mpi::relaxation::spai0<DBackend>>,
            amgcl::mpi::solver::gmres<DBackend>>;
    IJSolverParams<Solver> params;
    params.verbose = true;
    OP_INFO("Solve poisson equation with AMGCL");
    auto handler = makeEqnSolveHandler<Solver>(poisson_eqn(), p,
                                               DS::BlockedMDRangeMapper<2> {strategy->getSplitMap(
                                                       p.getMesh().getRange(), getGlobalParallelPlan())},
                                               params);
    handler->solve();
    ASSERT_TRUE(check_solution(2e-8));
}
