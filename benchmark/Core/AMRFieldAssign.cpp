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

using namespace OpFlow;

class AMRFixture2D : public benchmark::Fixture {
public:
    using Mesh = CartesianAMRMesh<Meta::int_<2>>;
    using Field = CartAMRField<Real, Mesh>;

    void SetUp(const ::benchmark::State& state) {
        constexpr int n = 65, maxlevel = 4, ratio = 2, buffWidth = 5;
        constexpr auto h = 1. / (n - 1);
        auto m = MeshBuilder<Mesh>()
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
                         .build();
        u = std::make_shared<Field>(ExprBuilder<Field>()
                                            .setMesh(m)
                                            .setName("u")
                                            .setLoc({LocOnMesh::Center, LocOnMesh ::Center})
                                            .setBC(0, DimPos::start, BCType::Dirc, 0.)
                                            .setBC(0, DimPos::end, BCType::Dirc, 0.)
                                            .setBC(1, DimPos::start, BCType::Dirc, 0.)
                                            .setBC(1, DimPos::end, BCType::Dirc, 0.)
                                            .build());
        v = std::make_shared<Field>(*u);
        v->name = "v";
    }

    void TearDown(const ::benchmark::State& state) {}

    std::shared_ptr<Field> u, v;
};

BENCHMARK_DEFINE_F(AMRFixture2D, ExprPrepare)(benchmark::State& state) {
    omp_set_num_threads(state.range(0));
    for (auto _ : state) { v->prepare(); }
}

BENCHMARK_DEFINE_F(AMRFixture2D, ExprUpdateBC)(benchmark::State& state) {
    omp_set_num_threads(state.range(0));
    for (auto _ : state) { u->updateBC(); }
}

BENCHMARK_DEFINE_F(AMRFixture2D, Assign)(benchmark::State& state) {
    omp_set_num_threads(state.range(0));
    for (auto _ : state) { internal::FieldAssigner::assign(*u, *v); }
}

BENCHMARK_REGISTER_F(AMRFixture2D, ExprPrepare)
        ->RangeMultiplier(2)
        ->Range(1, omp_get_max_threads())
        ->UseRealTime();
BENCHMARK_REGISTER_F(AMRFixture2D, ExprUpdateBC)
        ->RangeMultiplier(2)
        ->Range(1, omp_get_max_threads())
        ->UseRealTime();
BENCHMARK_REGISTER_F(AMRFixture2D, Assign)->RangeMultiplier(2)->Range(1, omp_get_max_threads())->UseRealTime();

BENCHMARK_DEFINE_F(AMRFixture2D, UpdatePadding)(benchmark::State& state) {
    omp_set_num_threads(state.range(0));
    for (auto _ : state) { u->updatePadding(); }
}
BENCHMARK_REGISTER_F(AMRFixture2D, UpdatePadding)
        ->RangeMultiplier(2)
        ->Range(1, omp_get_max_threads())
        ->UseRealTime();

BENCHMARK_DEFINE_F(AMRFixture2D, UpdateCovering)(benchmark::State& state) {
    omp_set_num_threads(state.range(0));
    for (auto _ : state) { u->updateCovering(); }
}
BENCHMARK_REGISTER_F(AMRFixture2D, UpdateCovering)
        ->RangeMultiplier(2)
        ->Range(1, omp_get_max_threads())
        ->UseRealTime();

BENCHMARK_MAIN();