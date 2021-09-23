// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2021 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#include <OpFlow>
#include <gmock/gmock.h>

using namespace OpFlow;
using namespace testing;

class EqnSolveHandlerTest : public Test {
protected:
    void SetUp() override {
        m = MeshBuilder<Mesh>().newMesh(65, 65).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();
        p = ExprBuilder<Field>()
                    .setMesh(m)
                    .setName("p")
                    .setBC(0, DimPos::start, BCType::Dirc, 0.)
                    .setBC(0, DimPos::end, BCType::Dirc, 0.)
                    .setBC(1, DimPos::start, BCType::Dirc, 0.)
                    .setBC(1, DimPos::end, BCType::Dirc, 0.)
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

TEST_F(EqnSolveHandlerTest, DefaultUnifiedSolve) {
    this->reset_case(0.5, 0.5);
    Solve(poisson_eqn(), p);
    ASSERT_TRUE(check_solution(1e-5));
}

TEST_F(EqnSolveHandlerTest, ManualUnifiedSolve) {
    this->reset_case(0.5, 0.5);
    StructSolverParams<OpFlow::StructSolverType::GMRES> params;
    params.tol = 1e-10;
    StructSolverParams<OpFlow::StructSolverType ::PFMG> p_params;
    p = 0.;
    Solve(poisson_eqn(), p, params, p_params);
    ASSERT_TRUE(check_solution(1e-10));
}

TEST_F(EqnSolveHandlerTest, DefaultHandlerSolve) {
    this->reset_case(0.5, 0.5);
    auto solver = PrecondStructSolver<StructSolverType::GMRES, StructSolverType::PFMG> {};
    auto handler = makeEqnSolveHandler(poisson_eqn(), p, solver);
    handler.solve();
    ASSERT_TRUE(check_solution(1e-5));
}

TEST_F(EqnSolveHandlerTest, ManualHandlerSolve) {
    this->reset_case(0.5, 0.5);
    StructSolverParams<OpFlow::StructSolverType::GMRES> params;
    params.tol = 1e-10;
    StructSolverParams<OpFlow::StructSolverType ::PFMG> p_params;
    auto solver = PrecondStructSolver<StructSolverType::GMRES, StructSolverType::PFMG> {params, p_params};
    auto handler = makeEqnSolveHandler(poisson_eqn(), p, solver);
    handler.solve();
    ASSERT_TRUE(check_solution(1e-10));
}

TEST_F(EqnSolveHandlerTest, DefaultUnifiedSolveTwice) {
    this->reset_case(0.5, 0.5);
    Solve(poisson_eqn(), p);
    ASSERT_TRUE(check_solution(1e-5));
    this->reset_case(0.3, 0.6);
    Solve(poisson_eqn(), p);
    ASSERT_TRUE(check_solution(1e-5));
}

TEST_F(EqnSolveHandlerTest, HandlerSolveTwice) {
    this->reset_case(0.5, 0.5);
    StructSolverParams<OpFlow::StructSolverType::GMRES> params;
    params.tol = 1e-10;
    params.printLevel = 2;
    params.maxIter = 10;
    StructSolverParams<OpFlow::StructSolverType ::PFMG> p_params;
    auto solver = PrecondStructSolver<StructSolverType::GMRES, StructSolverType::PFMG> {params, p_params};
    auto handler = makeEqnSolveHandler(poisson_eqn(), p, solver);
    handler.solve();
    ASSERT_TRUE(check_solution(1e-10));
    this->reset_case(0.3, 0.6);
    handler.solve();
    ASSERT_TRUE(check_solution(1e-10));
}

// used for memory leak detection
TEST_F(EqnSolveHandlerTest, HandlerSolveRepeat) {
    StructSolverParams<OpFlow::StructSolverType::GMRES> params;
    params.tol = 1e-10;
    params.printLevel = 2;
    params.maxIter = 10;
    StructSolverParams<OpFlow::StructSolverType ::PFMG> p_params;
    auto solver = PrecondStructSolver<StructSolverType::GMRES, StructSolverType::PFMG> {params, p_params};
    auto handler = makeEqnSolveHandler(poisson_eqn(), p, solver);
    for (auto i = 0; i < 10; ++i) {
        this->reset_case(0.3, 0.6);
        handler.solve();
        ASSERT_TRUE(check_solution(1e-10));
    }
}

// other types of solvers test
TEST_F(EqnSolveHandlerTest, BiCGSTABPFMG) {
    this->reset_case(0.5, 0.5);
    StructSolverParams<OpFlow::StructSolverType::BICGSTAB> params;
    params.tol = 1e-10;
    params.maxIter = 10;
    StructSolverParams<OpFlow::StructSolverType ::PFMG> p_params;
    p = 0.;
    Solve(poisson_eqn(), p, params, p_params);
    ASSERT_TRUE(check_solution(1e-10));
}

TEST_F(EqnSolveHandlerTest, PCGPFMG) {
    this->reset_case(0.5, 0.5);
    StructSolverParams<OpFlow::StructSolverType::PCG> params;
    params.tol = 1e-10;
    params.maxIter = 10;
    StructSolverParams<OpFlow::StructSolverType ::PFMG> p_params;
    p = 0.;
    Solve(poisson_eqn(), p, params, p_params);
    ASSERT_TRUE(check_solution(1e-10));
}

TEST_F(EqnSolveHandlerTest, PCG) {
    this->reset_case(0.5, 0.5);
    StructSolverParams<OpFlow::StructSolverType::PCG> params;
    params.tol = 1e-10;
    params.maxIter = 100;
    StructSolverParams<OpFlow::StructSolverType ::None> p_params;
    p = 0.;
    Solve(poisson_eqn(), p, params, p_params);
    ASSERT_TRUE(check_solution(1e-5));
}

TEST_F(EqnSolveHandlerTest, BiCGSTAB) {
    this->reset_case(0.5, 0.5);
    StructSolverParams<OpFlow::StructSolverType::BICGSTAB> params;
    params.tol = 1e-10;
    params.maxIter = 100;
    StructSolverParams<OpFlow::StructSolverType ::None> p_params;
    p = 0.;
    Solve(poisson_eqn(), p, params, p_params);
    ASSERT_TRUE(check_solution(1e-5));
}

TEST_F(EqnSolveHandlerTest, GMRES) {
    this->reset_case(0.5, 0.5);
    StructSolverParams<OpFlow::StructSolverType::GMRES> params;
    params.tol = 1e-10;
    params.maxIter = 100;
    params.kDim = 20;
    StructSolverParams<OpFlow::StructSolverType ::None> p_params;
    p = 0.;
    Solve(poisson_eqn(), p, params, p_params);
    ASSERT_TRUE(check_solution(1e-2));
}

TEST_F(EqnSolveHandlerTest, GMRESJACOBI) {
    this->reset_case(0.5, 0.5);
    StructSolverParams<OpFlow::StructSolverType::GMRES> params;
    params.tol = 1e-10;
    params.maxIter = 10;
    StructSolverParams<OpFlow::StructSolverType ::Jacobi> p_params;
    p = 0.;
    Solve(poisson_eqn(), p, params, p_params);
    ASSERT_TRUE(check_solution(1e-10));
}

TEST_F(EqnSolveHandlerTest, GMRESSMG) {
    this->reset_case(0.5, 0.5);
    StructSolverParams<OpFlow::StructSolverType::GMRES> params;
    params.tol = 1e-10;
    params.maxIter = 10;
    StructSolverParams<OpFlow::StructSolverType ::SMG> p_params;
    p = 0.;
    Solve(poisson_eqn(), p, params, p_params);
    ASSERT_TRUE(check_solution(1e-10));
}

TEST_F(EqnSolveHandlerTest, AMGCLUnifiedSolve) {
    this->reset_case(0.5, 0.5);
    using Solver = amgcl::make_solver<
            amgcl::amg<amgcl::backend::builtin<double>, amgcl::coarsening::smoothed_aggregation,
                       amgcl::relaxation::spai0>,
            amgcl::solver::bicgstab<amgcl::backend::builtin<double>>>;
    Solve<Solver>(poisson_eqn(), p, DS::MDRangeMapper<2> {p.assignableRange});
    ASSERT_TRUE(check_solution(1e-8));
}

TEST_F(EqnSolveHandlerTest, AMGCLUnifiedSolveMixedPercision) {
    this->reset_case(0.5, 0.5);
    using Solver = amgcl::make_solver<
            amgcl::amg<amgcl::backend::builtin<float>, amgcl::coarsening::smoothed_aggregation,
                       amgcl::relaxation::spai0>,
            amgcl::solver::bicgstab<amgcl::backend::builtin<double>>>;
    Solve<Solver>(poisson_eqn(), p, DS::MDRangeMapper<2> {p.assignableRange});
    ASSERT_TRUE(check_solution(5e-7));
}

TEST_F(EqnSolveHandlerTest, AMGCLUnifiedSolveCG) {
    this->reset_case(0.5, 0.5);
    using Solver = amgcl::make_solver<
            amgcl::amg<amgcl::backend::builtin<double>, amgcl::coarsening::smoothed_aggregation,
                       amgcl::relaxation::spai0>,
            amgcl::solver::cg<amgcl::backend::builtin<double>>>;
    Solve<Solver>(poisson_eqn(), p, DS::MDRangeMapper<2> {p.assignableRange});
    ASSERT_TRUE(check_solution(2e-8));
}

TEST_F(EqnSolveHandlerTest, AMGCLUnifiedSolveGMRES) {
    this->reset_case(0.5, 0.5);
    using Solver = amgcl::make_solver<
            amgcl::amg<amgcl::backend::builtin<double>, amgcl::coarsening::smoothed_aggregation,
                       amgcl::relaxation::spai0>,
            amgcl::solver::gmres<amgcl::backend::builtin<double>>>;
    Solve<Solver>(poisson_eqn(), p, DS::MDRangeMapper<2> {p.assignableRange});
    ASSERT_TRUE(check_solution(2e-8));
}