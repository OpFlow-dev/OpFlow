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
#include <benchmark/benchmark.h>

static void Laplace_2d(benchmark::State& state) {
    using namespace OpFlow;

    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;

    auto n = state.range(0);

    auto m = MeshBuilder<Mesh>().newMesh(n, n).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();

    auto u = ExprBuilder<Field>()
                     .setMesh(m)
                     .setLoc(std::array {LocOnMesh::Center, LocOnMesh::Center})
                     .setBC(0, DimPos::start, BCType::Periodic)
                     .setBC(0, DimPos::end, BCType::Periodic)
                     .setBC(1, DimPos::start, BCType::Periodic)
                     .setBC(1, DimPos::end, BCType::Periodic)
                     .setExt(1)
                     .build();
    u = 1.0;
    auto v = u;

    omp_set_num_threads(state.range(1));
    for (auto _ : state) { u = d2x<D2SecondOrderCentered>(v) + d2y<D2SecondOrderCentered>(v); }
}

static void Laplace_2d_Params(benchmark::internal::Benchmark* b) {
    for (auto i = 8; i <= 8 << 10; i *= 2)
        for (auto j = 1; j <= omp_get_max_threads(); j *= 2) b->Args({i + 1, j});
}

BENCHMARK(Laplace_2d)->Apply(Laplace_2d_Params)->UseRealTime();

/*
static void Laplace_3d(benchmark::State& state) {
    using namespace OpFlow;

    using Mesh = CartesianMesh<Meta::int_<3>>;
    using Field = CartesianField<Real, Mesh>;

    auto n = state.range(0);

    auto m = MeshBuilder<Mesh>()
                     .newMesh(n, n, n)
                     .setMeshOfDim(0, 0., 1.)
                     .setMeshOfDim(1, 0., 1.)
                     .setMeshOfDim(2, 0., 1.)
                     .build();

    auto u = ExprBuilder<Field>()
                     .setMesh(m)
                     .setLoc(std::array {LocOnMesh::Center, LocOnMesh::Center, LocOnMesh::Center})
                     .setBC(0, DimPos::start, BCType::Periodic)
                     .setBC(0, DimPos::end, BCType::Periodic)
                     .setBC(1, DimPos::start, BCType::Periodic)
                     .setBC(1, DimPos::end, BCType::Periodic)
                     .setBC(2, DimPos::start, BCType::Periodic)
                     .setBC(2, DimPos::end, BCType::Periodic)
                     .setExt(1)
                     .build();
    u = 1.0;
    auto v = u;

    omp_set_num_threads(state.range(1));
    for (auto _ : state) {
        u = d2x<D2SecondOrderCentered>(v) + d2y<D2SecondOrderCentered>(v) + d2z<D2SecondOrderCentered>(v);
    }
}

static void Laplace_3d_Params(benchmark::internal::Benchmark* b) {
    for (auto i = 8; i <= 512; i *= 2)
        for (auto j = 1; j <= omp_get_max_threads(); j *= 2) b->Args({i + 1, j});
}

//BENCHMARK(Laplace_3d)->Apply(Laplace_3d_Params)->UseRealTime();
*/

BENCHMARK_MAIN();