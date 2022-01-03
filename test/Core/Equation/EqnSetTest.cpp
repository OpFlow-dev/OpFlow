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
                   == dx<D1FirstOrderCenteredUpwind>(dx<D1FirstOrderCenteredDownwind>(e)
                                                     / d1IntpCenterToCorner<0>((r)))
                              + dy<D1FirstOrderCenteredUpwind>(dy<D1FirstOrderCenteredDownwind>(e)
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
    auto p2 = p;
    auto p_true = p;

    DS::ColoredMDRangeMapper<2> mapper {p.assignableRange, p.assignableRange};

    typedef amgcl::backend::builtin<double> SBackend;
    typedef amgcl::backend::builtin<float> PBackend;
    typedef amgcl::make_solver<
            amgcl::amg<PBackend, amgcl::coarsening::smoothed_aggregation, amgcl::relaxation::spai0>,
            amgcl::solver::cg<SBackend>>
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

    Solve<PSolver>(
            [&](auto&& e) { return 1.0 == d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e); },
            p_true, DS::MDRangeMapper<2> {p.assignableRange});

    rangeFor_s(p.assignableRange, [&](auto&& k) {
        ASSERT_NEAR(p[k] / p_true[k], 1.0, 1e-10);
        ASSERT_NEAR(p2[k] / p[k], 1.0, 1e-10);
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
    auto p2 = p;
    auto p_true = p;

    DS::ColoredMDRangeMapper<2> mapper {p.assignableRange, p.assignableRange};

    typedef amgcl::backend::builtin<double> SBackend;
    typedef amgcl::backend::builtin<float> PBackend;
    typedef amgcl::make_solver<
            amgcl::amg<PBackend, amgcl::coarsening::smoothed_aggregation, amgcl::relaxation::spai0>,
            amgcl::solver::cg<SBackend>>
            PSolver;

    std::vector<IJSolverParams<PSolver>> params(2);
    params[0].pinValue = true;
    params[1].pinValue = true;

    SolveEqns<PSolver>(
            [&](auto&& e, auto&&) {
                return 1.0 == d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e);
            },
            [&](auto&&, auto&& e) {
                return 1.0 == d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e);
            },
            p, p2, mapper, params);

    Solve<PSolver>(
            [&](auto&& e) { return 1.0 == d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e); },
            p_true, DS::MDRangeMapper<2> {p.assignableRange}, params[0]);

    rangeFor_s(p.assignableRange, [&](auto&& k) {
        ASSERT_LE(std::abs(p[k] - p_true[k]), 1e-10);
        ASSERT_DOUBLE_EQ(p[k], p2[k]);
    });
}