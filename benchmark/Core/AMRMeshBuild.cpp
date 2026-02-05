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

static void AMRMeshBuild2D(benchmark::State& state) {
    using namespace OpFlow;
    omp_set_num_threads(state.range(0));
    for (auto _ : state) {
        using Mesh = CartesianAMRMesh<Meta::int_<2>>;
        //using Field = CartAMRField<Real, Mesh>;
        constexpr int n = 65, maxlevel = 4, ratio = 2, buffWidth = 5;
        constexpr auto h = 1. / (n - 1);
        benchmark::DoNotOptimize(MeshBuilder<Mesh>()
                                         .setBaseMesh(MeshBuilder<CartesianMesh<Meta::int_<2>>>()
                                                              .newMesh(n, n)
                                                              .setMeshOfDim(0, 0., 1.)
                                                              .setMeshOfDim(1, 0., 1.)
                                                              .build())
                                         .setRefinementRatio(ratio)
                                         .setFillRateThreshold(0.8)
                                         .setSlimThreshold(buffWidth)
                                         .setBuffWidth(buffWidth)
                                         .setMaxLevel(maxlevel)
                                         .setMarkerFunction([&](auto&& i) {
                                             auto l = i.l;
                                             double cx = 0.5, cy = 0.75;
                                             double radius = 0.15;
                                             double ht = h / Math::int_pow(ratio, l);
                                             double eps = buffWidth * h / Math::int_pow(ratio, maxlevel - 1);
                                             double x[4], y[4];
                                             x[0] = x[2] = ht * i[0];
                                             x[1] = x[3] = ht * (i[0] + 1);
                                             y[0] = y[1] = ht * i[1];
                                             y[2] = y[3] = ht * (i[1] + 1);
                                             double r[4];
                                             r[0] = (x[0] - cx) * (x[0] - cx) + (y[0] - cy) * (y[0] - cy);
                                             r[1] = (x[1] - cx) * (x[1] - cx) + (y[1] - cy) * (y[1] - cy);
                                             r[2] = (x[2] - cx) * (x[2] - cx) + (y[2] - cy) * (y[2] - cy);
                                             r[3] = (x[3] - cx) * (x[3] - cx) + (y[3] - cy) * (y[3] - cy);
                                             bool allin = true, allout = true;
                                             auto r_min = std::max(0., radius - eps);
                                             for (double k : r) {
                                                 allin &= (k < r_min * r_min);
                                                 allout &= (k > (radius + eps) * (radius + eps));
                                             }
                                             if (!allin && !allout) return true;
                                             else
                                                 return false;
                                         })
                                         .build());
    }
}

BENCHMARK(AMRMeshBuild2D)->RangeMultiplier(2)->Range(1, omp_get_max_threads())->UseRealTime();

static void AMRMeshBuild3D(benchmark::State& state) {
    using namespace OpFlow;
    omp_set_num_threads(state.range(0));
    for (auto _ : state) {
        using Mesh = CartesianAMRMesh<Meta::int_<3>>;
        //using Field = CartAMRField<Real, Mesh>;
        constexpr int n = 17, maxlevel = 4, ratio = 4, buffWidth = 5;
        constexpr auto h = 1. / (n - 1);
        benchmark::DoNotOptimize(MeshBuilder<Mesh>()
                                         .setBaseMesh(MeshBuilder<CartesianMesh<Meta::int_<3>>>()
                                                              .newMesh(n, n, n)
                                                              .setMeshOfDim(0, 0., 1.)
                                                              .setMeshOfDim(1, 0., 1.)
                                                              .setMeshOfDim(2, 0., 1.)
                                                              .build())
                                         .setRefinementRatio(ratio)
                                         .setFillRateThreshold(0.8)
                                         .setSlimThreshold(buffWidth)
                                         .setBuffWidth(buffWidth)
                                         .setMaxLevel(maxlevel)
                                         .setMarkerFunction([&](auto&& i) {
                                             auto l = i.l;
                                             double cx = 0.35, cy = 0.35, cz = 0.35;
                                             double radius = 0.15;
                                             double ht = h / Math::int_pow(ratio, l);
                                             double eps = buffWidth * h / Math::int_pow(ratio, maxlevel - 1);
                                             double x[8], y[8], z[8];
                                             x[0] = x[2] = x[4] = x[6] = ht * i[0];
                                             x[1] = x[3] = x[5] = x[7] = ht * (i[0] + 1);
                                             y[0] = y[1] = y[5] = y[4] = ht * i[1];
                                             y[2] = y[3] = y[6] = y[7] = ht * (i[1] + 1);
                                             z[0] = z[1] = z[2] = z[3] = ht * i[2];
                                             z[4] = z[5] = z[6] = z[7] = ht * (i[2] + 1);
                                             double r[8];
                                             r[0] = (x[0] - cx) * (x[0] - cx) + (y[0] - cy) * (y[0] - cy)
                                                    + (z[0] - cz) * (z[0] - cz);
                                             r[1] = (x[1] - cx) * (x[1] - cx) + (y[1] - cy) * (y[1] - cy)
                                                    + (z[1] - cz) * (z[1] - cz);
                                             r[2] = (x[2] - cx) * (x[2] - cx) + (y[2] - cy) * (y[2] - cy)
                                                    + (z[2] - cz) * (z[2] - cz);
                                             r[3] = (x[3] - cx) * (x[3] - cx) + (y[3] - cy) * (y[3] - cy)
                                                    + (z[3] - cz) * (z[3] - cz);
                                             r[4] = (x[4] - cx) * (x[4] - cx) + (y[4] - cy) * (y[4] - cy)
                                                    + (z[4] - cz) * (z[4] - cz);
                                             r[5] = (x[5] - cx) * (x[5] - cx) + (y[5] - cy) * (y[5] - cy)
                                                    + (z[5] - cz) * (z[5] - cz);
                                             r[6] = (x[6] - cx) * (x[6] - cx) + (y[6] - cy) * (y[6] - cy)
                                                    + (z[6] - cz) * (z[6] - cz);
                                             r[7] = (x[7] - cx) * (x[7] - cx) + (y[7] - cy) * (y[7] - cy)
                                                    + (z[7] - cz) * (z[7] - cz);

                                             bool allin = true, allout = true;
                                             auto r_min = std::max(0., radius - eps);
                                             for (double k : r) {
                                                 allin &= (k < r_min * r_min);
                                                 allout &= (k > (radius + eps) * (radius + eps));
                                             }
                                             if (!allin && !allout) return true;
                                             else
                                                 return false;
                                         })
                                         .build());
    }
}

BENCHMARK(AMRMeshBuild3D)
        ->RangeMultiplier(2)
        ->Range(1, omp_get_max_threads())
        ->UseRealTime()
        ->Unit(benchmark::kSecond);

BENCHMARK_MAIN();