//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2023 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#include <gmock/gmock.h>
#include <print>
import opflow;

using namespace OpFlow;
using namespace testing;

class EqnSetTest : public Test {
protected:
    ParallelPlan ori_plan;
    void SetUp() override {
        m = MeshBuilder<Mesh>().newMesh(5, 5).setMeshOfDim(0, 0., 4.).setMeshOfDim(1, 0., 4.).build();
    }

    void reset_case(double xc, double yc) {
        r.initBy([&](auto&& x) {
            auto dist = Math::norm2(x[0] - xc, x[1] - yc);
            auto hevi = Math::smoothHeviside(r.getMesh().dx(0, 0) * 8, dist - 0.2);
            return 1. * hevi + (1. - hevi) * 1000;
        });
        b = 1.0;
        p = 0.;
    }

    auto poisson_eqn() {
        return [&](auto&& e) {
            return b
                   == dx<D1FirstOrderCentered>(dx<D1FirstOrderCentered>(e) / d1IntpCenterToCorner<0>((r)))
                              + dy<D1FirstOrderCentered>(dy<D1FirstOrderCentered>(e)
                                                         / d1IntpCenterToCorner<1>(r));
        };
    }

    auto simple_poisson() {
        return [&](auto&& e) { return 1.0 == d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e); };
    }

    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;
    Mesh m;
    Field p, r, b;
};

TEST_F(EqnSetTest, SimplePoisson_2Eqn) {
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
    // must give a valid initial value as AMGCLEqnSolveHandler relies on it
    p = 0;
    auto p2 = p;
    auto p_true = p;

    DS::ColoredMDRangeMapper<2> mapper {p.assignableRange, p.assignableRange};

    typedef amgcl::backend::builtin<double> SBackend;
    typedef amgcl::backend::builtin<float> PBackend;
    typedef amgcl::make_solver<
            amgcl::amg<PBackend, amgcl::coarsening::smoothed_aggregation, amgcl::relaxation::spai0>,
            amgcl::solver::bicgstab<SBackend>>
            PSolver;

    std::vector<IJSolverParams<PSolver>> params(2);

    SolveEqns<PSolver>(
            [&](auto&& e, auto&&) {
                return 1.0 == d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e);
            },
            [&](auto&&, auto&& e) {
                return 1.0 == d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e);
            },
            p, p2, mapper, params);

    params[0].dumpPath = "./A1";
    params[0].verbose = true;
    Solve<PSolver>(
            [&](auto&& e) { return 1.0 == d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e); },
            p_true, DS::MDRangeMapper<2> {p.assignableRange}, params[0]);

    rangeFor_s(p.assignableRange, [&](auto&& k) {
        if (std::isnan(p[k]) || std::isnan(p_true[k])) {
            std::print("p[{}] = {}, p2[{}] = {}, p_true[{}] = {}", k, p[k], k, p2[k], k, p_true[k]);
        }
        ASSERT_NEAR(p[k], p_true[k], std::abs(1e-10 * p_true[k]));
        ASSERT_NEAR(p2[k], p[k], std::abs(1e-10 * p[k]));
    });
}

TEST_F(EqnSetTest, SimplePoisson_Neum_2Eqn) {
    p = ExprBuilder<Field>()
                .setMesh(m)
                .setName("p")
                .setBC(0, DimPos::start, BCType::Neum, 0.)
                .setBC(0, DimPos::end, BCType::Neum, 0.)
                .setBC(1, DimPos::start, BCType::Neum, 0.)
                .setBC(1, DimPos::end, BCType::Neum, 0.)
                .setExt(0, DimPos::start, 1)
                .setExt(0, DimPos::end, 1)
                .setExt(1, DimPos::start, 1)
                .setExt(1, DimPos::end, 1)
                .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                .build();
    p = 0;
    auto p2 = p;
    auto p_true = p;

    DS::ColoredMDRangeMapper<2> mapper {p.assignableRange, p.assignableRange};

    typedef amgcl::backend::builtin<double> SBackend;
    typedef amgcl::backend::builtin<float> PBackend;
    typedef amgcl::make_solver<
            amgcl::amg<PBackend, amgcl::coarsening::smoothed_aggregation, amgcl::relaxation::spai0>,
            amgcl::solver::bicgstab<SBackend>>
            PSolver;

    std::vector<IJSolverParams<PSolver>> params(2);
    params[0].pinValue = true;
    params[1].pinValue = true;
    params[0].p.solver.tol = 1e-20;
    params[1].p.solver.tol = 1e-20;

    SolveEqns<PSolver>(
            [&](auto&& e, auto&&) {
                return 1.0 == d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e);
            },
            [&](auto&&, auto&& e) {
                return 1.0 == d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e);
            },
            p, p2, mapper, params);

    params[0].dumpPath = "./A2";
    Solve<PSolver>(
            [&](auto&& e) { return 1.0 == d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e); },
            p_true, DS::MDRangeMapper<2> {p.assignableRange}, params[0]);

    rangeFor_s(p.assignableRange, [&](auto&& k) {
        if (std::isnan(p[k]) || std::isnan(p_true[k])) {
            std::print("p[{}] = {}, p2[{}] = {}, p_true[{}] = {}", k, p[k], k, p2[k], k, p_true[k]);
        }

        ASSERT_NEAR(p[k], p_true[k], std::abs(1e-10 * p_true[k]));
        ASSERT_DOUBLE_EQ(p[k], p2[k]);
    });
}

TEST_F(EqnSetTest, SimplePoisson_10Eqn) {
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
    // must give a valid initial value as AMGCLEqnSolveHandler relies on it
    p = 0;
    auto p2 = p;
    auto p3 = p;
    auto p4 = p;
    auto p5 = p;
    auto p6 = p;
    auto p7 = p;
    auto p8 = p;
    auto p9 = p;
    auto p0 = p;
    auto p_true = p;

    DS::ColoredMDRangeMapper<2> mapper {
            p.assignableRange, p.assignableRange, p.assignableRange, p.assignableRange, p.assignableRange,
            p.assignableRange, p.assignableRange, p.assignableRange, p.assignableRange, p.assignableRange};

    typedef amgcl::backend::builtin<double> SBackend;
    typedef amgcl::backend::builtin<float> PBackend;
    typedef amgcl::make_solver<
            amgcl::amg<PBackend, amgcl::coarsening::smoothed_aggregation, amgcl::relaxation::spai0>,
            amgcl::solver::bicgstab<SBackend>>
            PSolver;

    std::vector<IJSolverParams<PSolver>> params(10);

    SolveEqns<PSolver>(
            [&](auto&& e, auto&&, auto&&, auto&&, auto&&, auto&&, auto&&, auto&&, auto&&, auto&&) {
                return 1.0 == d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e);
            },
            [&](auto&&, auto&& e, auto&&, auto&&, auto&&, auto&&, auto&&, auto&&, auto&&, auto&&) {
                return 1.0 == d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e);
            },
            [&](auto&&, auto&&, auto&& e, auto&&, auto&&, auto&&, auto&&, auto&&, auto&&, auto&&) {
                return 1.0 == d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e);
            },
            [&](auto&&, auto&&, auto&&, auto&& e, auto&&, auto&&, auto&&, auto&&, auto&&, auto&&) {
                return 1.0 == d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e);
            },
            [&](auto&&, auto&&, auto&&, auto&&, auto&& e, auto&&, auto&&, auto&&, auto&&, auto&&) {
                return 1.0 == d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e);
            },
            [&](auto&&, auto&&, auto&&, auto&&, auto&&, auto&& e, auto&&, auto&&, auto&&, auto&&) {
                return 1.0 == d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e);
            },
            [&](auto&&, auto&&, auto&&, auto&&, auto&&, auto&&, auto&& e, auto&&, auto&&, auto&&) {
                return 1.0 == d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e);
            },
            [&](auto&&, auto&&, auto&&, auto&&, auto&&, auto&&, auto&&, auto&& e, auto&&, auto&&) {
                return 1.0 == d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e);
            },
            [&](auto&&, auto&&, auto&&, auto&&, auto&&, auto&&, auto&&, auto&&, auto&& e, auto&&) {
                return 1.0 == d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e);
            },
            [&](auto&&, auto&&, auto&&, auto&&, auto&&, auto&&, auto&&, auto&&, auto&&, auto&& e) {
                return 1.0 == d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e);
            },
            p, p2, p3, p4, p5, p6, p7, p8, p9, p0, mapper, params);

    params[0].dumpPath = "./A1";
    params[0].verbose = true;
    Solve<PSolver>(
            [&](auto&& e) { return 1.0 == d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e); },
            p_true, DS::MDRangeMapper<2> {p.assignableRange}, params[0]);

    rangeFor_s(p.assignableRange, [&](auto&& k) {
        if (std::isnan(p[k]) || std::isnan(p_true[k])) {
            std::print("p[{}] = {}, p2[{}] = {}, p_true[{}] = {}", k, p[k], k, p2[k], k, p_true[k]);
        }
        ASSERT_NEAR(p[k], p_true[k], std::abs(1e-10 * p_true[k]));
        ASSERT_NEAR(p2[k], p[k], std::abs(1e-10 * p[k]));
        ASSERT_NEAR(p3[k], p[k], std::abs(1e-10 * p[k]));
        ASSERT_NEAR(p4[k], p[k], std::abs(1e-10 * p[k]));
        ASSERT_NEAR(p5[k], p[k], std::abs(1e-10 * p[k]));
        ASSERT_NEAR(p6[k], p[k], std::abs(1e-10 * p[k]));
        ASSERT_NEAR(p7[k], p[k], std::abs(1e-10 * p[k]));
        ASSERT_NEAR(p8[k], p[k], std::abs(1e-10 * p[k]));
        ASSERT_NEAR(p9[k], p[k], std::abs(1e-10 * p[k]));
        ASSERT_NEAR(p0[k], p[k], std::abs(1e-10 * p[k]));
    });
}
