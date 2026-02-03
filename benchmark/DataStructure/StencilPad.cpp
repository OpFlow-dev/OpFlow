// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2021 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#include <OpFlow>
#include <benchmark/benchmark.h>

using namespace OpFlow;

class StencilPadBench : public benchmark::Fixture
{
public:
    void SetUp(const ::benchmark::State&) override
    {
        auto m = MeshBuilder<Mesh>().newMesh(n, n).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();

        u = ExprBuilder<Field>()
            .setMesh(m)
            .setLoc(std::array{LocOnMesh::Center, LocOnMesh::Center})
            .setBC(0, DimPos::start, BCType::Dirc, 0.)
            .setBC(0, DimPos::end, BCType::Dirc, 0.)
            .setBC(1, DimPos::start, BCType::Dirc, 0.)
            .setBC(1, DimPos::end, BCType::Dirc, 0.)
            .setExt(3)
            .build();

        auto m3 = MeshBuilder<Mesh3>()
                  .newMesh(n / 2, n / 2, n / 2)
                  .setMeshOfDim(0, 0., 1.)
                  .setMeshOfDim(1, 0., 1.)
                  .setMeshOfDim(2, 0., 1.)
                  .build();

        u3 = ExprBuilder<Field3>()
             .setMesh(m3)
             .setLoc(std::array{LocOnMesh::Center, LocOnMesh::Center, LocOnMesh::Center})
             .setBC(0, DimPos::start, BCType::Dirc, 0.)
             .setBC(0, DimPos::end, BCType::Dirc, 0.)
             .setBC(1, DimPos::start, BCType::Dirc, 0.)
             .setBC(1, DimPos::end, BCType::Dirc, 0.)
             .setBC(2, DimPos::start, BCType::Dirc, 0.)
             .setBC(2, DimPos::end, BCType::Dirc, 0.)
             .setExt(3)
             .build();

        omp_set_num_threads(1);
    }

    void TearDown(const ::benchmark::State&) override
    {
    }

    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;
    using Mesh3 = CartesianMesh<Meta::int_<3>>;
    using Field3 = CartesianField<Real, Mesh3>;
    using idx = DS::MDIndex<2>;
    using idx3 = DS::MDIndex<3>;

    int n = 64;
    Field u;
    Field3 u3;
};

BENCHMARK_DEFINE_F(StencilPadBench, FakeMap_5P)(benchmark::State&st)
{
    auto su = u.template getStencilField<DS::fake_map_default>();

    for (auto _ : st)
    {
        rangeFor_s(u.assignableRange, [&](auto&& i)
        {
            benchmark::DoNotOptimize(su[i] * 4 - su[i.template prev<0>()] - su[i.template next<0>()]
                - su[i.template prev<1>()] - su[i.template next<1>()]);
        });
    }
}

BENCHMARK_DEFINE_F(StencilPadBench, FakeMap_9P)(benchmark::State&st)
{
    auto su = u.template getStencilField<DS::fake_map_default>();

    for (auto _ : st)
    {
        rangeFor_s(u.assignableRange, [&](auto&& i)
        {
            benchmark::DoNotOptimize(su[i] * 8 - su[i.template prev<0>()] - su[i.template next<0>()]
                - su[i.template prev<1>()] - su[i.template next<1>()]
                - su[i.template prev<0>().template prev<1>()]
                - su[i.template prev<0>().template next<1>()]
                - su[i.template next<0>().template prev<1>()]
                - su[i.template next<0>().template next<1>()]);
        });
    }
}

BENCHMARK_DEFINE_F(StencilPadBench, FakeMap_7P)(benchmark::State&st)
{
    auto su = u3.template getStencilField<DS::fake_map_default>();

    for (auto _ : st)
    {
        rangeFor_s(u3.assignableRange, [&](auto&& i)
        {
            benchmark::DoNotOptimize(su[i] * 6 - su[i.template prev<0>()] - su[i.template next<0>()]
                - su[i.template prev<1>()] - su[i.template next<1>()]
                - su[i.template prev<2>()] - su[i.template next<2>()]);
        });
    }
}

BENCHMARK_DEFINE_F(StencilPadBench, FakeMap_27P)(benchmark::State&st)
{
    auto su = u3.template getStencilField<DS::fake_map_default>();

    for (auto _ : st)
    {
        rangeFor_s(u3.assignableRange, [&](auto&& i)
        {
            benchmark::DoNotOptimize(
                su[i] * 26 - su[i.template prev<0>()] - su[i.template next<0>()]
                - su[i.template prev<1>()] - su[i.template next<1>()] - su[i.template prev<2>()]
                - su[i.template next<2>()] - su[i + idx3{-1, -1, -1}] - su[i + idx3{-1, -1, 0}]
                - su[i + idx3{-1, -1, 1}] - su[i + idx3{-1, 0, -1}] - su[i + idx3{-1, 0, 1}]
                - su[i + idx3{-1, 1, -1}] - su[i + idx3{-1, 1, 0}] - su[i + idx3{-1, 1, 1}]
                - su[i + idx3{0, -1, -1}] - su[i + idx3{0, -1, 1}] - su[i + idx3{0, 1, -1}]
                - su[i + idx3{0, 1, 1}] - su[i + idx3{1, -1, -1}] - su[i + idx3{1, -1, 0}]
                - su[i + idx3{1, -1, 1}] - su[i + idx3{1, 0, -1}] - su[i + idx3{1, 0, 1}]
                - su[i + idx3{1, 1, -1}] - su[i + idx3{1, 1, 0}] - su[i + idx3{1, 1, 1}]);
        });
    }
}

BENCHMARK_DEFINE_F(StencilPadBench, STDMap_5P)(benchmark::State&st)
{
    auto su = u.template getStencilField<std::unordered_map>();

    for (auto _ : st)
    {
        rangeFor_s(u.assignableRange, [&](auto&& i)
        {
            benchmark::DoNotOptimize(su[i] * 4 - su[i.template prev<0>()] - su[i.template next<0>()]
                - su[i.template prev<1>()] - su[i.template next<1>()]);
        });
    }
}

BENCHMARK_DEFINE_F(StencilPadBench, STDMap_9P)(benchmark::State&st)
{
    auto su = u.template getStencilField<std::unordered_map>();

    for (auto _ : st)
    {
        rangeFor_s(u.assignableRange, [&](auto&& i)
        {
            benchmark::DoNotOptimize(su[i] * 8 - su[i.template prev<0>()] - su[i.template next<0>()]
                - su[i.template prev<1>()] - su[i.template next<1>()]
                - su[i.template prev<0>().template prev<1>()]
                - su[i.template prev<0>().template next<1>()]
                - su[i.template next<0>().template prev<1>()]
                - su[i.template next<0>().template next<1>()]);
        });
    }
}

BENCHMARK_DEFINE_F(StencilPadBench, STDMap_7P)(benchmark::State&st)
{
    auto su = u3.template getStencilField<std::unordered_map>();

    for (auto _ : st)
    {
        rangeFor_s(u3.assignableRange, [&](auto&& i)
        {
            benchmark::DoNotOptimize(su[i] * 6 - su[i.template prev<0>()] - su[i.template next<0>()]
                - su[i.template prev<1>()] - su[i.template next<1>()]
                - su[i.template prev<2>()] - su[i.template next<2>()]);
        });
    }
}

BENCHMARK_DEFINE_F(StencilPadBench, STDMap_27P)(benchmark::State&st)
{
    auto su = u3.template getStencilField<std::unordered_map>();

    for (auto _ : st)
    {
        rangeFor_s(u3.assignableRange, [&](auto&& i)
        {
            benchmark::DoNotOptimize(
                su[i] * 26 - su[i.template prev<0>()] - su[i.template next<0>()]
                - su[i.template prev<1>()] - su[i.template next<1>()] - su[i.template prev<2>()]
                - su[i.template next<2>()] - su[i + idx3{-1, -1, -1}] - su[i + idx3{-1, -1, 0}]
                - su[i + idx3{-1, -1, 1}] - su[i + idx3{-1, 0, -1}] - su[i + idx3{-1, 0, 1}]
                - su[i + idx3{-1, 1, -1}] - su[i + idx3{-1, 1, 0}] - su[i + idx3{-1, 1, 1}]
                - su[i + idx3{0, -1, -1}] - su[i + idx3{0, -1, 1}] - su[i + idx3{0, 1, -1}]
                - su[i + idx3{0, 1, 1}] - su[i + idx3{1, -1, -1}] - su[i + idx3{1, -1, 0}]
                - su[i + idx3{1, -1, 1}] - su[i + idx3{1, 0, -1}] - su[i + idx3{1, 0, 1}]
                - su[i + idx3{1, 1, -1}] - su[i + idx3{1, 1, 0}] - su[i + idx3{1, 1, 1}]);
        });
    }
}

BENCHMARK_REGISTER_F(StencilPadBench, FakeMap_5P) -> Unit(
benchmark::kMicrosecond
);
BENCHMARK_REGISTER_F(StencilPadBench, FakeMap_9P) -> Unit(
benchmark::kMicrosecond
);
BENCHMARK_REGISTER_F(StencilPadBench, FakeMap_7P) -> Unit(
benchmark::kMicrosecond
);
BENCHMARK_REGISTER_F(StencilPadBench, FakeMap_27P) -> Unit(
benchmark::kMicrosecond
);
BENCHMARK_REGISTER_F(StencilPadBench, STDMap_5P) -> Unit(
benchmark::kMicrosecond
);
BENCHMARK_REGISTER_F(StencilPadBench, STDMap_9P) -> Unit(
benchmark::kMicrosecond
);
BENCHMARK_REGISTER_F(StencilPadBench, STDMap_7P) -> Unit(
benchmark::kMicrosecond
);
BENCHMARK_REGISTER_F(StencilPadBench, STDMap_27P) -> Unit(
benchmark::kMicrosecond
);

BENCHMARK_MAIN();