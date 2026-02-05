//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2022  by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#include <OpFlow>
#include <benchmark/benchmark.h>

static void AMGCLEqnSolve_2d_matgen(benchmark::State& state) {
    using namespace OpFlow;

    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;

    auto n = state.range(0);

    auto m = MeshBuilder<Mesh>().newMesh(n, n).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();

    auto u = ExprBuilder<Field>()
                     .setMesh(m)
                     .setLoc(std::array {LocOnMesh::Center, LocOnMesh::Center})
                     .setBC(0, DimPos::start, BCType::Dirc, 0.)
                     .setBC(0, DimPos::end, BCType::Dirc, 0.)
                     .setBC(1, DimPos::start, BCType::Dirc, 0.)
                     .setBC(1, DimPos::end, BCType::Dirc, 0.)
                     .setExt(1)
                     .build();

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
    IJSolverParams<Solver> params;
    params.p.solver.maxiter = 0;
    auto handler = makeEqnSolveHandler<Solver>(
            [&](auto&& e) { return d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e) == 1.0; }, u,
            DS::MDRangeMapper<2> {u.assignableRange}, params);

    for (auto _ : state) {
        state.PauseTiming();
        u = 0.;
        state.ResumeTiming();
        handler->generateAb();
    }
}

static void AMGCLEqnSolve_2d_solve(benchmark::State& state) {
    using namespace OpFlow;

    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;

    auto n = state.range(0);

    auto m = MeshBuilder<Mesh>().newMesh(n, n).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();

    auto u = ExprBuilder<Field>()
                     .setMesh(m)
                     .setLoc(std::array {LocOnMesh::Center, LocOnMesh::Center})
                     .setBC(0, DimPos::start, BCType::Dirc, 0.)
                     .setBC(0, DimPos::end, BCType::Dirc, 0.)
                     .setBC(1, DimPos::start, BCType::Dirc, 0.)
                     .setBC(1, DimPos::end, BCType::Dirc, 0.)
                     .setExt(1)
                     .build();

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
    IJSolverParams<Solver> params;
    params.p.solver.maxiter = 100;
    params.p.solver.tol = 1e-10;
    params.staticMat = true;
    auto handler = makeEqnSolveHandler<Solver>(
            [&](auto&& e) { return d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e) == 1.0; }, u,
            DS::MDRangeMapper<2> {u.assignableRange}, params);
    [[maybe_unused]] auto [iter, err, abserr] = handler->solve();

    for (auto _ : state) {
        state.PauseTiming();
        u = 0.;
        state.ResumeTiming();
        handler->solve();
    }
}

static void AMGCLEqnSolve_2d_solve_dymat(benchmark::State& state) {
    using namespace OpFlow;

    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;

    auto n = state.range(0);

    auto m = MeshBuilder<Mesh>().newMesh(n, n).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();

    auto u = ExprBuilder<Field>()
                     .setMesh(m)
                     .setLoc(std::array {LocOnMesh::Center, LocOnMesh::Center})
                     .setBC(0, DimPos::start, BCType::Dirc, 0.)
                     .setBC(0, DimPos::end, BCType::Dirc, 0.)
                     .setBC(1, DimPos::start, BCType::Dirc, 0.)
                     .setBC(1, DimPos::end, BCType::Dirc, 0.)
                     .setExt(1)
                     .build();

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
    IJSolverParams<Solver> params;
    params.p.solver.maxiter = 100;
    params.p.solver.tol = 1e-10;
    params.staticMat = false;
    auto handler = makeEqnSolveHandler<Solver>(
            [&](auto&& e) { return d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e) == 1.0; }, u,
            DS::MDRangeMapper<2> {u.assignableRange}, params);
    [[maybe_unused]] auto [iter, err, abserr] = handler->solve();

    for (auto _ : state) {
        state.PauseTiming();
        u = 0.;
        state.ResumeTiming();
        handler->solve();
    }
}

static void HYPREEqnSolve_2d_solve_dymat(benchmark::State& state) {
    using namespace OpFlow;

    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;

    auto n = state.range(0);

    auto m = MeshBuilder<Mesh>().newMesh(n, n).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();

    auto u = ExprBuilder<Field>()
                     .setMesh(m)
                     .setLoc(std::array {LocOnMesh::Center, LocOnMesh::Center})
                     .setBC(0, DimPos::start, BCType::Dirc, 0.)
                     .setBC(0, DimPos::end, BCType::Dirc, 0.)
                     .setBC(1, DimPos::start, BCType::Dirc, 0.)
                     .setBC(1, DimPos::end, BCType::Dirc, 0.)
                     .setExt(1)
                     .build();

    StructSolverParams<StructSolverType::PCG> params;
    params.tol = 1e-10;
    params.staticMat = false;
    StructSolverParams<StructSolverType::PFMG> p_params;
    auto solver = PrecondStructSolver<StructSolverType::PCG, StructSolverType::PFMG> {params, p_params};
    auto handler = makeEqnSolveHandler(
            [&](auto&& e) { return d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e) == 1.0; }, u,
            solver);
    [[maybe_unused]] auto [iter, err, abserr] = handler->solve();

    for (auto _ : state) {
        state.PauseTiming();
        u = 0.;
        state.ResumeTiming();
        handler->solve();
    }
}

static void HYPREEqnSolve_2d_solve(benchmark::State& state) {
    using namespace OpFlow;

    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;

    auto n = state.range(0);

    auto m = MeshBuilder<Mesh>().newMesh(n, n).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();

    auto u = ExprBuilder<Field>()
                     .setMesh(m)
                     .setLoc(std::array {LocOnMesh::Center, LocOnMesh::Center})
                     .setBC(0, DimPos::start, BCType::Dirc, 0.)
                     .setBC(0, DimPos::end, BCType::Dirc, 0.)
                     .setBC(1, DimPos::start, BCType::Dirc, 0.)
                     .setBC(1, DimPos::end, BCType::Dirc, 0.)
                     .setExt(1)
                     .build();

    StructSolverParams<StructSolverType::PCG> params;
    params.tol = 1e-10;
    params.staticMat = true;
    StructSolverParams<StructSolverType::PFMG> p_params;
    auto solver = PrecondStructSolver<StructSolverType::PCG, StructSolverType::PFMG> {params, p_params};
    auto handler = makeEqnSolveHandler(
            [&](auto&& e) { return d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e) == 1.0; }, u,
            solver);
    [[maybe_unused]] auto [iter, err, abserr] = handler->solve();

    for (auto _ : state) {
        state.PauseTiming();
        u = 0.;
        state.ResumeTiming();
        handler->solve();
    }
}

static void EqnSolve_2d_Params(benchmark::internal::Benchmark* b) {
    for (auto i = 8; i <= 1 << 10; i *= 2) b->Args({i + 1});
}

BENCHMARK(AMGCLEqnSolve_2d_matgen)->Apply(EqnSolve_2d_Params)->UseRealTime()->Unit(benchmark::kSecond);
BENCHMARK(AMGCLEqnSolve_2d_solve)->Apply(EqnSolve_2d_Params)->UseRealTime()->Unit(benchmark::kSecond);
BENCHMARK(AMGCLEqnSolve_2d_solve_dymat)->Apply(EqnSolve_2d_Params)->UseRealTime()->Unit(benchmark::kSecond);
BENCHMARK(HYPREEqnSolve_2d_solve)->Apply(EqnSolve_2d_Params)->UseRealTime()->Unit(benchmark::kSecond);
BENCHMARK(HYPREEqnSolve_2d_solve_dymat)->Apply(EqnSolve_2d_Params)->UseRealTime()->Unit(benchmark::kSecond);

int main(int argc, char** argv) {
    OpFlow::EnvironmentGardian _(&argc, &argv);
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    return 0;
}