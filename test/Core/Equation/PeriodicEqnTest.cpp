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
using namespace testing;

class PeriodicEqnTest : public Test {
protected:
    ParallelPlan ori_plan;
    void SetUp() override {
        ori_plan = getGlobalParallelPlan();
        auto info = makeParallelInfo();
        info.threadInfo.thread_count = std::min(info.threadInfo.thread_count, 4);
        setGlobalParallelInfo(info);
        setGlobalParallelPlan(makeParallelPlan(getGlobalParallelInfo(), ParallelIdentifier::SharedMem));

        m = MeshBuilder<Mesh>()
                    .newMesh(33, 33)
                    .setMeshOfDim(0, 0., 2 * PI)
                    .setMeshOfDim(1, 0., 2 * PI)
                    .build();
        p = ExprBuilder<Field>()
                    .setMesh(m)
                    .setName("p")
                    .setBC(0, DimPos::start, BCType::Periodic)
                    .setBC(0, DimPos::end, BCType::Periodic)
                    .setBC(1, DimPos::start, BCType::Periodic)
                    .setBC(1, DimPos::end, BCType::Periodic)
                    .setExt(1)
                    .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                    .build();
        p_true = p;
        p_true.name = "ptrue";
        r = ExprBuilder<Field>()
                    .setMesh(m)
                    .setName("r")
                    .setBC(0, DimPos::start, BCType::Periodic)
                    .setBC(0, DimPos::end, BCType::Periodic)
                    .setBC(1, DimPos::start, BCType::Periodic)
                    .setBC(1, DimPos::end, BCType::Periodic)
                    .setExt(1)
                    .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                    .build();
        b = p;
        b.name = "b";
        p_true.initBy([&](auto&& x) { return std::sin(x[0]) * std::cos(x[1]); });
    }

    void TearDown() override { setGlobalParallelPlan(ori_plan); }

    void reset_case(double xc, double yc) {
        r.initBy([&](auto&& x) {
            auto dist = Math::norm2(x[0] - xc, x[1] - yc);
            auto hevi = Math::smoothHeviside(r.getMesh().dx(0, 0) * 8, dist - 0.2);
            return 1. * hevi + (1. - hevi) * 1;
        });
        b = dx<D1FirstOrderCentered>(dx<D1FirstOrderCentered>(p_true) / d1IntpCenterToCorner<0>((r)))
            + dy<D1FirstOrderCentered>(dy<D1FirstOrderCentered>(p_true) / d1IntpCenterToCorner<1>(r));
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
};

TEST_F(PeriodicEqnTest, ManualUnifiedSolve) {
    this->reset_case(0.5, 0.5);
    StructSolverParams<OpFlow::StructSolverType::GMRES> params;
    params.tol = 1e-10;
    StructSolverParams<OpFlow::StructSolverType ::PFMG> p_params;
    p = 0.;
    params.pinValue = true;
    Solve(poisson_eqn(), p, params, p_params);
    auto ave_p = rangeReduce(
            p.assignableRange, [](auto&& a, auto&& b) { return a + b; }, [&](auto&& idx) { return p[idx]; });
    p -= ave_p / p.assignableRange.count();
    ASSERT_TRUE(check_solution(1e-9));
}

TEST_F(PeriodicEqnTest, ManualHandlerSolve) {
    this->reset_case(0.5, 0.5);
    StructSolverParams<OpFlow::StructSolverType::GMRES> params;
    params.tol = 1e-10;
    params.pinValue = true;
    StructSolverParams<OpFlow::StructSolverType ::PFMG> p_params;
    auto solver = PrecondStructSolver<StructSolverType::GMRES, StructSolverType::PFMG> {params, p_params};
    auto handler = makeEqnSolveHandler(poisson_eqn(), p, solver);
    handler->solve();
    auto ave_p = rangeReduce(
            p.assignableRange, [](auto&& a, auto&& b) { return a + b; }, [&](auto&& idx) { return p[idx]; });
    p -= ave_p / p.assignableRange.count();
    ASSERT_TRUE(check_solution(1e-9));
}

TEST_F(PeriodicEqnTest, HandlerSolveTwice) {
    this->reset_case(0.5, 0.5);
    StructSolverParams<OpFlow::StructSolverType::GMRES> params;
    params.tol = 1e-10;
    params.printLevel = 2;
    params.maxIter = 10;
    params.pinValue = true;
    StructSolverParams<OpFlow::StructSolverType ::PFMG> p_params;
    auto solver = PrecondStructSolver<StructSolverType::GMRES, StructSolverType::PFMG> {params, p_params};
    auto handler = makeEqnSolveHandler(poisson_eqn(), p, solver);
    handler->solve();
    auto ave_p = rangeReduce(
            p.assignableRange, [](auto&& a, auto&& b) { return a + b; }, [&](auto&& idx) { return p[idx]; });
    p -= ave_p / p.assignableRange.count();
    ASSERT_TRUE(check_solution(1e-8));
    this->reset_case(0.3, 0.6);
    handler->solve();
    ave_p = rangeReduce(
            p.assignableRange, [](auto&& a, auto&& b) { return a + b; }, [&](auto&& idx) { return p[idx]; });
    p -= ave_p / p.assignableRange.count();
    ASSERT_TRUE(check_solution(1e-8));
}

// used for memory leak detection
TEST_F(PeriodicEqnTest, HandlerSolveRepeat) {
    StructSolverParams<OpFlow::StructSolverType::GMRES> params;
    params.tol = 1e-10;
    params.printLevel = 2;
    params.maxIter = 10;
    params.pinValue = true;
    StructSolverParams<OpFlow::StructSolverType ::PFMG> p_params;
    auto solver = PrecondStructSolver<StructSolverType::GMRES, StructSolverType::PFMG> {params, p_params};
    auto handler = makeEqnSolveHandler(poisson_eqn(), p, solver);
    for (auto i = 0; i < 10; ++i) {
        this->reset_case(0.3, 0.6);
        handler->solve();
        auto ave_p = rangeReduce(
                p.assignableRange, [](auto&& a, auto&& b) { return a + b; },
                [&](auto&& idx) { return p[idx]; });
        p -= ave_p / p.assignableRange.count();

        ASSERT_TRUE(check_solution(1e-8));
    }
}

// other types of solvers test
TEST_F(PeriodicEqnTest, BiCGSTABPFMG) {
    this->reset_case(0.5, 0.5);
    StructSolverParams<OpFlow::StructSolverType::BICGSTAB> params;
    params.tol = 1e-10;
    params.maxIter = 10;
    StructSolverParams<OpFlow::StructSolverType ::PFMG> p_params;
    p = 0.;
    params.pinValue = true;
    Solve(poisson_eqn(), p, params, p_params);
    auto ave_p = rangeReduce(
            p.assignableRange, [](auto&& a, auto&& b) { return a + b; }, [&](auto&& idx) { return p[idx]; });
    p -= ave_p / p.assignableRange.count();
    ASSERT_TRUE(check_solution(2e-7));
}

TEST_F(PeriodicEqnTest, PCGPFMG) {
    this->reset_case(0.5, 0.5);
    StructSolverParams<OpFlow::StructSolverType::PCG> params;
    params.tol = 1e-10;
    params.maxIter = 10;
    StructSolverParams<OpFlow::StructSolverType ::PFMG> p_params;
    p = 0.;
    params.pinValue = true;
    Solve(poisson_eqn(), p, params, p_params);
    auto ave_p = rangeReduce(
            p.assignableRange, [](auto&& a, auto&& b) { return a + b; }, [&](auto&& idx) { return p[idx]; });
    p -= ave_p / p.assignableRange.count();
    ASSERT_TRUE(check_solution(1e-10));
}

TEST_F(PeriodicEqnTest, PCG) {
    this->reset_case(0.5, 0.5);
    StructSolverParams<OpFlow::StructSolverType::PCG> params;
    params.tol = 1e-10;
    params.maxIter = 100;
    StructSolverParams<OpFlow::StructSolverType ::None> p_params;
    p = 0.;
    params.pinValue = true;
    Solve(poisson_eqn(), p, params, p_params);
    auto ave_p = rangeReduce(
            p.assignableRange, [](auto&& a, auto&& b) { return a + b; }, [&](auto&& idx) { return p[idx]; });
    p -= ave_p / p.assignableRange.count();
    ASSERT_TRUE(check_solution(1e-5));
}

TEST_F(PeriodicEqnTest, BiCGSTAB) {
    this->reset_case(0.5, 0.5);
    StructSolverParams<OpFlow::StructSolverType::BICGSTAB> params;
    params.tol = 1e-10;
    params.maxIter = 100;
    StructSolverParams<OpFlow::StructSolverType ::None> p_params;
    p = 0.;
    params.pinValue = true;
    Solve(poisson_eqn(), p, params, p_params);
    auto ave_p = rangeReduce(
            p.assignableRange, [](auto&& a, auto&& b) { return a + b; }, [&](auto&& idx) { return p[idx]; });
    p -= ave_p / p.assignableRange.count();
    ASSERT_TRUE(check_solution(1e-5));
}

TEST_F(PeriodicEqnTest, GMRES) {
    this->reset_case(0.5, 0.5);
    StructSolverParams<OpFlow::StructSolverType::GMRES> params;
    params.tol = 1e-10;
    params.maxIter = 100;
    params.kDim = 20;
    StructSolverParams<OpFlow::StructSolverType ::None> p_params;
    p = 0.;
    params.pinValue = true;
    params.printLevel = 2;
    Solve(poisson_eqn(), p, params, p_params);
    auto ave_p = rangeReduce(
            p.assignableRange, [](auto&& a, auto&& b) { return a + b; }, [&](auto&& idx) { return p[idx]; });
    p -= ave_p / p.assignableRange.count();
    // unpreconditioned version converges really bad
    ASSERT_TRUE(check_solution(1));
}

TEST_F(PeriodicEqnTest, GMRESJACOBI) {
    this->reset_case(0.5, 0.5);
    StructSolverParams<OpFlow::StructSolverType::GMRES> params;
    params.tol = 1e-10;
    params.maxIter = 10;
    StructSolverParams<OpFlow::StructSolverType ::Jacobi> p_params;
    p = 0.;
    params.pinValue = true;
    Solve(poisson_eqn(), p, params, p_params);
    auto ave_p = rangeReduce(
            p.assignableRange, [](auto&& a, auto&& b) { return a + b; }, [&](auto&& idx) { return p[idx]; });
    p -= ave_p / p.assignableRange.count();
    ASSERT_TRUE(check_solution(1e-9));
}

TEST_F(PeriodicEqnTest, GMRESSMG) {
    this->reset_case(0.5, 0.5);
    StructSolverParams<OpFlow::StructSolverType::GMRES> params;
    params.tol = 1e-10;
    params.maxIter = 10;
    StructSolverParams<OpFlow::StructSolverType ::SMG> p_params;
    p = 0.;
    params.pinValue = true;
    Solve(poisson_eqn(), p, params, p_params);
    auto ave_p = rangeReduce(
            p.assignableRange, [](auto&& a, auto&& b) { return a + b; }, [&](auto&& idx) { return p[idx]; });
    p -= ave_p / p.assignableRange.count();
    ASSERT_TRUE(check_solution(2e-10));
}

TEST_F(PeriodicEqnTest, AMGCLUnifiedSolve) {
    this->reset_case(0.5, 0.5);
    using Solver = amgcl::make_solver<
            amgcl::amg<amgcl::backend::builtin<double>, amgcl::coarsening::smoothed_aggregation,
                       amgcl::relaxation::spai0>,
            amgcl::solver::bicgstab<amgcl::backend::builtin<double>>>;
    IJSolverParams<Solver> param;
    param.pinValue = true;
    param.p.solver.tol = 1e-11;

    Solve<Solver>(poisson_eqn(), p, DS::MDRangeMapper<2> {p.assignableRange}, param);
    auto ave_p = rangeReduce(
            p.assignableRange, [](auto&& a, auto&& b) { return a + b; }, [&](auto&& idx) { return p[idx]; });
    p -= ave_p / p.assignableRange.count();
    ASSERT_TRUE(check_solution(1e-10));
}

TEST_F(PeriodicEqnTest, AMGCLUnifiedSolveMixedPercision) {
    this->reset_case(0.5, 0.5);
    using Solver = amgcl::make_solver<
            amgcl::amg<amgcl::backend::builtin<float>, amgcl::coarsening::smoothed_aggregation,
                       amgcl::relaxation::spai0>,
            amgcl::solver::bicgstab<amgcl::backend::builtin<double>>>;
    IJSolverParams<Solver> param;
    param.pinValue = true;
    param.p.solver.tol = 1e-11;
    Solve<Solver>(poisson_eqn(), p, DS::MDRangeMapper<2> {p.assignableRange}, param);
    auto ave_p = rangeReduce(
            p.assignableRange, [](auto&& a, auto&& b) { return a + b; }, [&](auto&& idx) { return p[idx]; });
    p -= ave_p / p.assignableRange.count();
    ASSERT_TRUE(check_solution(3e-4));
}

TEST_F(PeriodicEqnTest, AMGCLUnifiedSolveCG) {
    this->reset_case(0.5, 0.5);
    using Solver = amgcl::make_solver<
            amgcl::amg<amgcl::backend::builtin<double>, amgcl::coarsening::smoothed_aggregation,
                       amgcl::relaxation::spai0>,
            amgcl::solver::cg<amgcl::backend::builtin<double>>>;
    IJSolverParams<Solver> param;
    param.pinValue = true;
    param.p.solver.tol = 1e-11;
    Solve<Solver>(poisson_eqn(), p, DS::MDRangeMapper<2> {p.assignableRange}, param);
    auto ave_p = rangeReduce(
            p.assignableRange, [](auto&& a, auto&& b) { return a + b; }, [&](auto&& idx) { return p[idx]; });
    p -= ave_p / p.assignableRange.count();
    ASSERT_TRUE(check_solution(1e-10));
}

TEST_F(PeriodicEqnTest, AMGCLUnifiedSolveGMRES) {
    this->reset_case(0.5, 0.5);
    using Solver = amgcl::make_solver<
            amgcl::amg<amgcl::backend::builtin<double>, amgcl::coarsening::smoothed_aggregation,
                       amgcl::relaxation::spai0>,
            amgcl::solver::gmres<amgcl::backend::builtin<double>>>;
    IJSolverParams<Solver> param;
    param.pinValue = true;
    param.p.solver.tol = 1e-11;

    Solve<Solver>(poisson_eqn(), p, DS::MDRangeMapper<2> {p.assignableRange}, param);
    auto ave_p = rangeReduce(
            p.assignableRange, [](auto&& a, auto&& b) { return a + b; }, [&](auto&& idx) { return p[idx]; });
    p -= ave_p / p.assignableRange.count();
    ASSERT_TRUE(check_solution(1e-10));
}