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

#include <OpFlow>

using namespace OpFlow;

int main(int argc, char* argv[]) {
    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;

    InitEnvironment(&argc, &argv);// clang-format off
    auto info = makeParallelInfo();
    setGlobalParallelInfo(info);
    setGlobalParallelPlan(makeParallelPlan(getGlobalParallelInfo(), ParallelIdentifier::SharedMem));

    const Real dt = 1e-3, nu = 1.0e-2;

    // meshes & fields
    auto n = 65;
    auto mesh = MeshBuilder<Mesh>().newMesh(n, n).setMeshOfDim(0, 0., 2 * PI).setMeshOfDim(1, 0., 2 * PI).build();
    auto builder = ExprBuilder<Field>().setMesh(mesh)
            .setBC(0, DimPos::start, BCType::Periodic).setBC(0, DimPos::end, BCType::Periodic)
            .setBC(1, DimPos::start, BCType::Periodic).setBC(1, DimPos::end, BCType::Periodic)
            .setExt(1);
    auto u = builder.setName("u")
            .setLoc({LocOnMesh ::Corner, LocOnMesh ::Center}).build();
    auto du = builder.setName("du").build();
    auto v = builder.setName("v")
            .setLoc({LocOnMesh ::Center, LocOnMesh ::Corner}).build();
    auto dv = v; dv.name = "dv";
    auto p = builder.setName("p")
            .setLoc({LocOnMesh ::Center, LocOnMesh ::Center}).build();
    auto dp = p; dp.name = "dp";
    u = 0; du = 0; v = 0; dv = 0; p = 0; dp = 0;
    u.initBy([&](auto&& x) { return std::sin(x[0]) * std::cos(x[1]); });
    v.initBy([&](auto&& x) { return -std::cos(x[0]) * std::sin(x[1]); });
    p.initBy([&](auto&& x) { return 0.25 * (std::cos(2 * x[0]) + std::cos(2 * x[1])) * std::exp(2 * nu * dt); });

    // composite operators
    auto conv_xx = [&](auto&& _1, auto&& _2) {
      return dx<D1FirstOrderCenteredDownwind>(d1IntpCornerToCenter<0>(_1) * d1IntpCornerToCenter<0>(_2));
    };
    auto conv_xy = [&](auto&& _1, auto&& _2) {
      return dy<D1FirstOrderCenteredUpwind>(d1IntpCenterToCorner<1>(_1) * d1IntpCenterToCorner<0>(_2));
    };
    auto conv_yx = [&](auto&& _1, auto&& _2) {
      return dx<D1FirstOrderCenteredUpwind>(d1IntpCenterToCorner<1>(_1) * d1IntpCenterToCorner<0>(_2));
    };
    auto conv_yy = [&](auto&& _1, auto&& _2) {
      return dy<D1FirstOrderCenteredDownwind>(d1IntpCornerToCenter<1>(_1) * d1IntpCornerToCenter<1>(_2));
    };

    // solvers
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

    auto u_handler = makeEqnSolveHandler<Solver>(
            [&](auto&& e) {
              return e / dt + conv_xx(u, e) + 0.5 * conv_xy(e, v)
                     == nu * (d2x<D2SecondOrderCentered>(u) + d2y<D2SecondOrderCentered>(u))
                        + 0.5 * nu * (d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e))
                        - (conv_xx(u, u) + conv_xy(u, v)) - dx<D1FirstOrderCenteredDownwind>(p);
            },
            du, DS::MDRangeMapper<2>{u.assignableRange});
    auto v_handler = makeEqnSolveHandler<Solver>(
            [&](auto&& e) {
              return e / dt + conv_yy(v, e) + conv_yy(v, v) + conv_yx(u, v) + 0.5 * conv_yx(u, e)
                     + 0.5 * conv_yx(du, v)
                     == nu * (d2x<D2SecondOrderCentered>(v) + d2y<D2SecondOrderCentered>(v))
                        + 0.5 * nu * (d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e))
                        - dy<D1FirstOrderCenteredDownwind>(p);
            },
            dv, DS::MDRangeMapper<2>{v.assignableRange});

    IJSolverParams<Solver> p_param; p_param.pinValue = true;
    auto p_handler = makeEqnSolveHandler<Solver>(
            [&](auto&& e) {
              return d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e)
                     == (dx<D1FirstOrderCenteredUpwind>(du) + dy<D1FirstOrderCenteredUpwind>(dv)) / dt;
            },
            dp, DS::MDRangeMapper<2>{p.assignableRange}, p_param);

    // writers
    Utils::TecplotASCIIStream uf("u.tec"), vf("v.tec"), pf("p.tec");

    // main algorithm
    auto t0 = std::chrono::system_clock::now();
    for (auto i = 0; i < 0.1 / dt; ++i) {
        u_handler.solve();
        v_handler.solve();
        du = du - dt * conv_xy(u, dv);
        u = u + du;
        v = v + dv;
        p_handler.solve();
        u = u - dt * dx<D1FirstOrderCenteredDownwind>(dp);
        v = v - dt * dy<D1FirstOrderCenteredDownwind>(dp);
        p = p + dp;
        auto ave_p = rangeReduce(p.assignableRange, [](auto&& a, auto&& b) { return a + b; }, [&](auto&& idx) { return p[idx]; }) / p.assignableRange.count();
        p = p - ave_p;
        uf << Utils::TimeStamp(i) << u; vf << Utils::TimeStamp(i) << v; pf << Utils::TimeStamp(i) << p;
        OP_INFO("Current step {}", i);
    }
    auto t1 = std::chrono::system_clock::now();
    OP_INFO("Elapsed time: {}ms", std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());

    auto ref_u = u; auto ref_v = v; auto ref_p = p;
    ref_u.initBy([&](auto&& x) { return std::sin(x[0]) * std::cos(x[1]) * std::exp(-2 * nu * 0.1); });
    ref_v.initBy([&](auto&& x) { return -std::cos(x[0]) * std::sin(x[1]) * std::exp(-2 * nu * 0.1); });
    ref_p.initBy([&](auto&& x) { return 0.25 * (std::cos(2 * x[0]) + std::cos(2 * x[1])) * std::exp(-4 * nu * 0.1); });

    auto err_u = (ref_u - u) * (ref_u - u);
    auto err_v = (ref_v - v) * (ref_v - v);
    auto err_p = (ref_p - p) * (ref_p - p);

    auto err2_u = rangeReduce(u.assignableRange, [](auto&& a, auto&& b) { return a + b; }, [&](auto&& i) { return err_u[i]; });
    auto err2_v = rangeReduce(v.assignableRange, [](auto&& a, auto&& b) { return a + b; }, [&](auto&& i) { return err_v[i]; });
    auto err2_p = rangeReduce(p.assignableRange, [](auto&& a, auto&& b) { return a + b; }, [&](auto&& i) { return err_p[i]; });

    OP_INFO("err_u = {}", err2_u / u.assignableRange.count());
    OP_INFO("err_v = {}", err2_v / v.assignableRange.count());
    OP_INFO("err_p = {}", err2_p / p.assignableRange.count());
    FinalizeEnvironment();
    return 0;
}