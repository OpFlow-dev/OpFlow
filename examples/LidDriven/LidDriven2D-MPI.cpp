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
int main(int argc, char* argv[]) {
    using namespace OpFlow;
    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;

    InitEnvironment(&argc, &argv);
    auto info = makeParallelInfo();
    setGlobalParallelInfo(info);
    setGlobalParallelPlan(makeParallelPlan(getGlobalParallelInfo(), ParallelIdentifier::DistributeMem));
    std::shared_ptr<AbstractSplitStrategy<Field>> strategy = std::make_shared<EvenSplitStrategy<Field>>();

    // meshes & fields
    auto n = 65;
    auto mesh = MeshBuilder<Mesh>().newMesh(n, n).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();
    auto builder = ExprBuilder<Field>()
                           .setMesh(mesh)
                           .setBC(0, DimPos::start, BCType::Dirc, 0.)
                           .setBC(0, DimPos::end, BCType::Dirc, 0.)
                           .setBC(1, DimPos::start, BCType::Dirc, 0.)
                           .setBC(1, DimPos::end, BCType::Dirc, 0.)
                           .setPadding(1)
                           .setSplitStrategy(strategy);
    auto u = builder.setName("u")
                     .setBC(1, DimPos::end, BCType::Dirc, 1.)
                     .setLoc({LocOnMesh ::Corner, LocOnMesh ::Center})
                     .build();
    auto du = builder.setName("du").setBC(1, DimPos::end, BCType::Dirc, 0.).build();
    auto v = builder.setName("v")
                     .setBC(1, DimPos::end, BCType::Dirc, 0.)
                     .setLoc({LocOnMesh ::Center, LocOnMesh ::Corner})
                     .build();
    auto dv = v;
    dv.name = "dv";
    auto p = builder.setName("p")
                     .setBC(0, DimPos::start, BCType::Neum, 0.)
                     .setBC(0, DimPos::end, BCType::Neum, 0.)
                     .setBC(1, DimPos::start, BCType::Neum, 0.)
                     .setBC(1, DimPos::end, BCType::Neum, 0.)
                     .setLoc({LocOnMesh ::Center, LocOnMesh ::Center})
                     .build();
    auto dp = p;
    dp.name = "dp";
    u = 0;
    du = 0;
    v = 0;
    dv = 0;
    p = 0;
    dp = 0;

    // composite operators
    auto conv_xx = [&](auto&& _1, auto&& _2) {
        return dx<D1FirstOrderCentered>(d1IntpCornerToCenter<0>(_1) * d1IntpCornerToCenter<0>(_2));
    };
    auto conv_xy = [&](auto&& _1, auto&& _2) {
        return dy<D1FirstOrderCentered>(d1IntpCenterToCorner<1>(_1) * d1IntpCenterToCorner<0>(_2));
    };
    auto conv_yx = [&](auto&& _1, auto&& _2) {
        return dx<D1FirstOrderCentered>(d1IntpCenterToCorner<1>(_1) * d1IntpCenterToCorner<0>(_2));
    };
    auto conv_yy = [&](auto&& _1, auto&& _2) {
        return dy<D1FirstOrderCentered>(d1IntpCornerToCenter<1>(_1) * d1IntpCornerToCenter<1>(_2));
    };

    // solvers
    const Real dt = 0.5e-2, nu = 1.0e-2;
    StructSolverParams<StructSolverType::GMRES> params;
    params.tol = 1e-10;
    params.maxIter = 100;
    StructSolverParams<StructSolverType::GMRES> poisson_params = params;
    StructSolverParams<StructSolverType::PFMG> p_params {.useZeroGuess = true,
                                                         .relaxType = 1,
                                                         .rapType = 0,
                                                         .numPreRelax = 1,
                                                         .numPostRelax = 1,
                                                         .skipRelax = 0};
    p_params.tol = 1e-10;
    auto solver = PrecondStructSolver<StructSolverType::GMRES, StructSolverType::PFMG>(params, p_params);
    auto u_handler = makeEqnSolveHandler(
            [&](auto&& e) {
                return e / dt + conv_xx(u, e) + 0.5 * conv_xy(e, v)
                       == nu * (d2x<D2SecondOrderCentered>(u) + d2y<D2SecondOrderCentered>(u))
                                  + 0.5 * nu * (d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e))
                                  - (conv_xx(u, u) + conv_xy(u, v)) - dx<D1FirstOrderCentered>(p);
            },
            du, solver);
    auto v_handler = makeEqnSolveHandler(
            [&](auto&& e) {
                return e / dt + conv_yy(v, e) + conv_yy(v, v) + conv_yx(u, v) + 0.5 * conv_yx(u, e)
                               + 0.5 * conv_yx(du, v)
                       == nu * (d2x<D2SecondOrderCentered>(v) + d2y<D2SecondOrderCentered>(v))
                                  + 0.5 * nu * (d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e))
                                  - dy<D1FirstOrderCentered>(p);
            },
            dv, solver);
    poisson_params.staticMat = true;
    poisson_params.pinValue = true;
    auto p_solver
            = PrecondStructSolver<StructSolverType::GMRES, StructSolverType::PFMG>(poisson_params, p_params);
    auto p_handler = makeEqnSolveHandler(
            [&](auto&& e) {
                return d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e)
                       == (dx<D1FirstOrderCentered>(du) + dy<D1FirstOrderCentered>(dv)) / dt;
            },
            dp, p_solver);

    // writers
    Utils::H5Stream stream("./solution.h5");

    // main algorithm
    auto t0 = std::chrono::system_clock::now();
    for (auto i = 0; i < 100; ++i) {
        u_handler->solve();
        v_handler->solve();
        du = du - 0.5 * dt * conv_xy(u, dv);
        u = u + du;
        v = v + dv;
        p_handler->solve();
        u = u - dt * dx<D1FirstOrderCentered>(dp);
        v = v - dt * dy<D1FirstOrderCentered>(dp);
        p = p + dp;
        stream << Utils::TimeStamp(i) << u << v << p;
        OP_MPI_MASTER_INFO("Current step {}", i);
    }
    auto t1 = std::chrono::system_clock::now();
    OP_MPI_MASTER_INFO("Elapsed time: {}ms",
                       std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());

    FinalizeEnvironment();
    return 0;
}