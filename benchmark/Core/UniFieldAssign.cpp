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

static void UniFieldAssign_2d(benchmark::State& state) {
    using namespace OpFlow;

    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;

    auto n = state.range(0);

    auto m = MeshBuilder<Mesh>().newMesh(n, n).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();

    auto u = ExprBuilder<Field>()
                     .setMesh(m)
                     .setLoc(std::array {LocOnMesh::Center, LocOnMesh::Center})
                     .build();
    auto v = u;

    omp_set_num_threads(state.range(1));
    for (auto _ : state) { u = v; }
}

static void UniFieldAssignRaw_2d(benchmark::State& state) {
    using namespace OpFlow;

    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;

    auto n = state.range(0);

    auto m = MeshBuilder<Mesh>().newMesh(n, n).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();

    auto u = ExprBuilder<Field>()
                     .setMesh(m)
                     .setLoc(std::array {LocOnMesh::Center, LocOnMesh::Center})
                     .build();
    auto v = u;

    omp_set_num_threads(state.range(1));
    for (auto _ : state) {
#pragma omp parallel for
#ifndef __clang__// clang current doesn't support sizes clause ( clang-13 )
#pragma omp tile sizes(8, 8)
#endif
        for (auto j = 0; j < n - 1; ++j)
            for (auto i = 0; i < n - 1; ++i) u(DS::MDIndex<2> {i, j}) = v(DS::MDIndex<2> {i, j});
    }
}

static void UniFieldAssign_2d_Params(benchmark::internal::Benchmark* b) {
    for (auto i = 8; i <= 8 << 10; i *= 2)
        for (auto j = 1; j <= omp_get_max_threads(); j *= 2) b->Args({i + 1, j});
}

BENCHMARK(UniFieldAssignRaw_2d)->Apply(UniFieldAssign_2d_Params)->UseRealTime();

BENCHMARK(UniFieldAssign_2d)->Apply(UniFieldAssign_2d_Params)->UseRealTime();

/*
static void UniFieldAssign_3d(benchmark::State& state) {
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
                     .build();
    auto v = u;

    omp_set_num_threads(state.range(1));
    for (auto _ : state) { u = v; }
}

static void UniFieldAssign_3d_Params(benchmark::internal::Benchmark* b) {
    for (auto i = 8; i <= 512; i *= 2)
        for (auto j = 1; j <= omp_get_max_threads(); j *= 2) b->Args({i + 1, j});
}

//BENCHMARK(UniFieldAssign_3d)->Apply(UniFieldAssign_3d_Params)->UseRealTime();
*/

BENCHMARK_MAIN();