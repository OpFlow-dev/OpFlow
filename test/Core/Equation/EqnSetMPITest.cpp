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

#include <amgcl/backend/builtin.hpp>
#include <amgcl/mpi/amg.hpp>
#include <amgcl/mpi/coarsening/smoothed_aggregation.hpp>
#include <amgcl/mpi/make_solver.hpp>
#include <amgcl/mpi/relaxation/spai0.hpp>
#include <amgcl/mpi/solver/bicgstab.hpp>
#include <cmath>
#include <format>
#include <gmock/gmock.h>
#include <iostream>
#ifdef OPFLOW_USE_MODULE
import opflow;
#else
#include <OpFlow>
#endif

using namespace OpFlow;
using namespace testing;

class EqnSetMPITest : public Test {
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

TEST_F(EqnSetMPITest, SimplePoisson_2Eqn) {
    auto strategy = std::make_shared<EvenSplitStrategy<Field>>();
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
    // must give a valid initial value as AMGCLEqnSolveHandler relies on it
    p = 0;
    auto p2 = p;
    auto p_true = p;

    DS::ColoredBlockedMDRangeMapper<2> mapper {p.getLocalWritableRange(), p.getLocalWritableRange()};

    typedef amgcl::backend::builtin<double> SBackend;
    typedef amgcl::mpi::make_solver<
            amgcl::mpi::amg<SBackend, amgcl::mpi::coarsening::smoothed_aggregation<SBackend>,
                            amgcl::mpi::relaxation::spai0<SBackend>>,
            amgcl::mpi::solver::bicgstab<SBackend>>
            PSolver;

    std::vector<IJSolverParams<PSolver>> params(2);

    params[0].dumpPath = "./EqnSetMPITest_A1";
    params[0].verbose = true;
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
            p_true, DS::BlockedMDRangeMapper<2> {p.getLocalWritableRange()}, params[0]);

    rangeFor_s(p.getLocalWritableRange(), [&](auto&& k) {
        if (std::isnan(p[k]) || std::isnan(p_true[k])) {
            std::cout << std::format("p[{}] = {}, p2[{}] = {}, p_true[{}] = {}", k, p[k], k, p2[k], k,
                                     p_true[k]);
        }
        ASSERT_NEAR(p[k], p_true[k], std::abs(1e-10 * p_true[k]));
        ASSERT_NEAR(p2[k], p[k], std::abs(1e-10 * p[k]));
    });
}

TEST_F(EqnSetMPITest, SimplePoisson_Neum_2Eqn) {
    auto strategy = std::make_shared<EvenSplitStrategy<Field>>();
    p = ExprBuilder<Field>()
                .setMesh(m)
                .setName("p")
                .setBC(0, DimPos::start, BCType::Neum, 0.)
                .setBC(0, DimPos::end, BCType::Neum, 0.)
                .setBC(1, DimPos::start, BCType::Neum, 0.)
                .setBC(1, DimPos::end, BCType::Neum, 0.)
                .setExt(1)
                .setPadding(1)
                .setSplitStrategy(strategy)
                .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                .build();
    p = 0;
    auto p2 = p;
    auto p_true = p;

    DS::ColoredBlockedMDRangeMapper<2> mapper {p.getLocalWritableRange(), p.getLocalWritableRange()};

    typedef amgcl::backend::builtin<double> SBackend;
    typedef amgcl::mpi::make_solver<
            amgcl::mpi::amg<SBackend, amgcl::mpi::coarsening::smoothed_aggregation<SBackend>,
                            amgcl::mpi::relaxation::spai0<SBackend>>,
            amgcl::mpi::solver::bicgstab<SBackend>>
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

    params[0].dumpPath = "./EqnSetMPITest_A2";
    Solve<PSolver>(
            [&](auto&& e) { return 1.0 == d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e); },
            p_true, DS::BlockedMDRangeMapper<2> {p.getLocalWritableRange()}, params[0]);

    rangeFor_s(p.getLocalWritableRange(), [&](auto&& k) {
        if (std::isnan(p[k]) || std::isnan(p_true[k])) {
            std::cout << std::format("p[{}] = {}, p2[{}] = {}, p_true[{}] = {}", k, p[k], k, p2[k], k,
                                     p_true[k]);
        }

        ASSERT_NEAR(p[k], p_true[k], std::abs(1e-10 * p_true[k]));
        ASSERT_NEAR(p[k], p2[k], std::abs(1e-10 * p_true[k]));
    });
}

TEST_F(EqnSetMPITest, SimplePoisson_10Eqn) {
    auto strategy = std::make_shared<EvenSplitStrategy<Field>>();
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

    DS::ColoredBlockedMDRangeMapper<2> mapper {p.getLocalWritableRange(), p.getLocalWritableRange(),
                                               p.getLocalWritableRange(), p.getLocalWritableRange(),
                                               p.getLocalWritableRange(), p.getLocalWritableRange(),
                                               p.getLocalWritableRange(), p.getLocalWritableRange(),
                                               p.getLocalWritableRange(), p.getLocalWritableRange()};

    typedef amgcl::backend::builtin<double> SBackend;
    typedef amgcl::mpi::make_solver<
            amgcl::mpi::amg<SBackend, amgcl::mpi::coarsening::smoothed_aggregation<SBackend>,
                            amgcl::mpi::relaxation::spai0<SBackend>>,
            amgcl::mpi::solver::bicgstab<SBackend>>
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

    params[0].dumpPath = "./EqnSetMPITest_A10";
    params[0].verbose = true;
    Solve<PSolver>(
            [&](auto&& e) { return 1.0 == d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e); },
            p_true, DS::BlockedMDRangeMapper<2> {p.getLocalWritableRange()}, params[0]);

    rangeFor_s(p.getLocalWritableRange(), [&](auto&& k) {
        if (std::isnan(p[k]) || std::isnan(p_true[k])) {
            std::cout << std::format("p[{}] = {}, p2[{}] = {}, p_true[{}] = {}", k, p[k], k, p2[k], k,
                                     p_true[k]);
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
