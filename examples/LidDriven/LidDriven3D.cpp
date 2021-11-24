#include <OpFlow>
int main(int argc, char* argv[]) {
    using namespace OpFlow;
    using Mesh = CartesianMesh<Meta::int_<3>>;
    using Field = CartesianField<Real, Mesh>;

    InitEnvironment(&argc, &argv);

    // meshes & fields
    auto n = 65;
    auto mesh = MeshBuilder<Mesh>()
                        .newMesh(n, n, n)
                        .setMeshOfDim(0, 0., 1.)
                        .setMeshOfDim(1, 0., 1.)
                        .setMeshOfDim(2, 0., 1.)
                        .build();
    auto builder = ExprBuilder<Field>()
                           .setMesh(mesh)
                           .setBC(0, DimPos::start, BCType::Dirc, 0.)
                           .setBC(0, DimPos::end, BCType::Dirc, 0.)
                           .setBC(1, DimPos::start, BCType::Dirc, 0.)
                           .setBC(1, DimPos::end, BCType::Dirc, 0.)
                           .setBC(2, DimPos::start, BCType::Dirc, 0.)
                           .setBC(2, DimPos::end, BCType::Dirc, 0.);
    auto u = builder.setName("u")
                     .setBC(1, DimPos::end, BCType::Dirc, 1.)
                     .setLoc({LocOnMesh ::Corner, LocOnMesh ::Center, LocOnMesh::Center})
                     .build();
    auto du = builder.setName("du").setBC(1, DimPos::end, BCType::Dirc, 0.).build();
    auto v = builder.setName("v").setLoc({LocOnMesh ::Center, LocOnMesh ::Corner, LocOnMesh::Center}).build();
    auto dv = v;
    dv.name = "dv";
    auto w = builder.setName("w")
                     .setLoc({LocOnMesh ::Center, LocOnMesh ::Center, LocOnMesh ::Corner})
                     .build();
    auto dw = w;
    dw.name = "dw";
    auto p = builder.setName("p")
                     .setBC(0, DimPos::start, BCType::Neum, 0.)
                     .setBC(0, DimPos::end, BCType::Neum, 0.)
                     .setBC(1, DimPos::start, BCType::Neum, 0.)
                     .setBC(1, DimPos::end, BCType::Neum, 0.)
                     .setBC(2, DimPos::start, BCType::Neum, 0.)
                     .setBC(2, DimPos::end, BCType::Neum, 0.)
                     .setLoc({LocOnMesh ::Center, LocOnMesh ::Center, LocOnMesh ::Center})
                     .build();
    auto dp = p;
    dp.name = "dp";
    u = 0;
    du = 0;
    v = 0;
    dv = 0;
    w = 0;
    dw = 0;
    p = 0;
    dp = 0;

    // composite operators
    auto conv_xx = [&](auto&& _1, auto&& _2) {
        return dx<D1FirstOrderCenteredDownwind>(d1IntpCornerToCenter<0>(_1) * d1IntpCornerToCenter<0>(_2));
    };
    auto conv_xy = [&](auto&& _1, auto&& _2) {
        return dy<D1FirstOrderCenteredUpwind>(d1IntpCenterToCorner<1>(_1) * d1IntpCenterToCorner<0>(_2));
    };
    auto conv_xz = [&](auto&& _1, auto&& _2) {
        return dz<D1FirstOrderCenteredUpwind>(d1IntpCenterToCorner<2>(_1) * d1IntpCenterToCorner<0>(_2));
    };
    auto conv_yx = [&](auto&& _1, auto&& _2) {
        return dx<D1FirstOrderCenteredUpwind>(d1IntpCenterToCorner<1>(_1) * d1IntpCenterToCorner<0>(_2));
    };
    auto conv_yy = [&](auto&& _1, auto&& _2) {
        return dy<D1FirstOrderCenteredDownwind>(d1IntpCornerToCenter<1>(_1) * d1IntpCornerToCenter<1>(_2));
    };
    auto conv_yz = [&](auto&& _1, auto&& _2) {
        return dz<D1FirstOrderCenteredUpwind>(d1IntpCenterToCorner<2>(_1) * d1IntpCenterToCorner<1>(_2));
    };
    auto conv_zx = [&](auto&& _1, auto&& _2) {
        return dx<D1FirstOrderCenteredUpwind>(d1IntpCenterToCorner<2>(_1) * d1IntpCenterToCorner<0>(_2));
    };
    auto conv_zy = [&](auto&& _1, auto&& _2) {
        return dy<D1FirstOrderCenteredUpwind>(d1IntpCenterToCorner<2>(_1) * d1IntpCenterToCorner<1>(_2));
    };
    auto conv_zz = [&](auto&& _1, auto&& _2) {
        return dz<D1FirstOrderCenteredDownwind>(d1IntpCornerToCenter<2>(_1) * d1IntpCornerToCenter<2>(_2));
    };
    auto laplace = [&](auto&& _1) {
        return d2x<D2SecondOrderCentered>(_1) + d2y<D2SecondOrderCentered>(_1)
               + d2z<D2SecondOrderCentered>(_1);
    };

    // solvers
    const Real dt = 0.5e-2, nu = 1.0e-2;
    StructSolverParams<StructSolverType::GMRES> params;
    params.tol = 1e-6;
    params.maxIter = 100;
    StructSolverParams<StructSolverType::PCG> poisson_params;
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
                return e / dt + conv_xx(u, e) + 0.5 * conv_xy(e, v) + 0.5 * conv_xz(e, w)
                       == nu * laplace(u) + 0.5 * nu * laplace(e)
                                  - (conv_xx(u, u) + conv_xy(u, v) + conv_xz(u, w))
                                  - dx<D1FirstOrderCenteredDownwind>(p);
            },
            du, solver);
    auto v_handler = makeEqnSolveHandler(
            [&](auto&& e) {
                return e / dt + conv_yy(v, e) + conv_yy(v, v) + conv_yx(u, v) + conv_yz(v, w)
                               + 0.5 * conv_yx(u, e) + 0.5 * conv_yx(du, v) + 0.5 * conv_yz(e, w)
                       == nu * laplace(v) + 0.5 * nu * laplace(e) - dy<D1FirstOrderCenteredDownwind>(p);
            },
            dv, solver);
    auto w_handler = makeEqnSolveHandler(
            [&](auto&& e) {
                return e / dt + 0.5 * conv_zx(u, e) + 0.5 * conv_zy(v, e) + conv_zz(w, e)
                       == nu * laplace(w) + 0.5 * nu * laplace(e) - conv_zx(u, w) - conv_zy(v, w)
                                  - conv_zz(w, w) - 0.5 * conv_zx(du, w) - 0.5 * conv_zy(dv, w)
                                  - dz<D1FirstOrderCenteredDownwind>(p);
            },
            dw, solver);
    poisson_params.staticMat = true;
    poisson_params.pinValue = true;
    auto p_solver
            = PrecondStructSolver<StructSolverType::PCG, StructSolverType::PFMG>(poisson_params, p_params);
    auto p_handler = makeEqnSolveHandler(
            [&](auto&& e) {
                return laplace(e) * -1.0
                       == (dx<D1FirstOrderCenteredUpwind>(du) + dy<D1FirstOrderCenteredUpwind>(dv)
                           + dz<D1FirstOrderCenteredUpwind>(dw))
                                  / -dt;
            },
            dp, p_solver);

    // writers
    Utils::TecplotASCIIStream uf("u.tec"), vf("v.tec"), wf("w.tec"), pf("p.tec");

    // main algorithm
    auto t0 = std::chrono::system_clock::now();
    for (auto i = 0; i < 100; ++i) {
        u_handler.solve();
        v_handler.solve();
        w_handler.solve();
        dv = dv - 0.5 * dt * conv_yz(v, dw);
        du = du - 0.5 * dt * conv_xy(u, dv) - 0.5 * dt * conv_xz(u, dw);
        u = u + du;
        v = v + dv;
        w = w + dw;
        p_handler.solve();
        u = u - dt * dx<D1FirstOrderCenteredDownwind>(dp);
        v = v - dt * dy<D1FirstOrderCenteredDownwind>(dp);
        w = w - dt * dz<D1FirstOrderCenteredDownwind>(dp);
        p = p + dp;
        uf << Utils::TimeStamp(i) << u;
        vf << Utils::TimeStamp(i) << v;
        wf << Utils::TimeStamp(i) << w;
        pf << Utils::TimeStamp(i) << p;
        OP_INFO("Current step {}", i);
    }
    auto t1 = std::chrono::system_clock::now();
    OP_INFO("Elapsed time: {}ms", std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());

    FinalizeEnvironment();
    return 0;
}