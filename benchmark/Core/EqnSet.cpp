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
#include <benchmark/benchmark.h>

using namespace OpFlow;

static void EqnSolve_1eqn(benchmark::State& state) {
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

    auto eqn_holder = makeEqnHolder(
            [&](auto&& e) { return d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e) == 1.0; },
            u);
    auto st_holder = makeStencilHolder(eqn_holder);
    auto mapper = DS::ColoredMDRangeMapper<2>{u.assignableRange};
    auto pin = std::vector{false};

    for (auto _ : state)
        auto mat = CSRMatrixGenerator::generate(st_holder, mapper, pin);
}

static void EqnSolve_1eqn_Params(benchmark::internal::Benchmark* b) {
    for (auto i = 1024; i <= 2 << 10; i *= 2) b->Args({i + 1});
}

BENCHMARK(EqnSolve_1eqn)->Apply(EqnSolve_1eqn_Params)->UseRealTime()->Unit(benchmark::kSecond);

BENCHMARK_MAIN();