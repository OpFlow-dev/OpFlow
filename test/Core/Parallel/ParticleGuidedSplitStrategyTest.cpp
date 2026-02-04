//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2026 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#include <OpFlow>
#include <gmock/gmock.h>
#include <print>

using namespace OpFlow;
using namespace testing;

class ParticleGuidedSplitStrategyTest : public Test {
protected:
    void SetUp() override {}
    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<double, Mesh>;
};

TEST_F(ParticleGuidedSplitStrategyTest, DivisibleDim) {
    auto range = DS::Range<2> {std::array<int, 2> {33, 33}};
    auto plan = ParallelPlan {};
    plan.distributed_workers_count = 4;
    setGlobalParallelPlan(plan);
    auto strategy = ParticleGuidedSplitStrategy<Field> {};
    auto mesh = MeshBuilder<Mesh>()
                        .newMesh(33, 33)
                        .setMeshOfDim(0, 0., 1.)
                        .setMeshOfDim(1, 0., 1.)
                        .setPadWidth(5)
                        .build();
    strategy.setMaxLevel(2);
    strategy.setRefMesh(mesh);
    // input nodal range, return centered range
    auto map = strategy.getSplitMap(range, plan);
    ASSERT_TRUE((map[0] == DS::Range<2> {std::array {16, 16}}));
    ASSERT_TRUE((map[2] == DS::Range<2> {std::array {16, 0}, std::array {32, 16}}));
    ASSERT_TRUE((map[1] == DS::Range<2> {std::array {0, 16}, std::array {16, 32}}));
    ASSERT_TRUE((map[3] == DS::Range<2> {std::array {16, 16}, std::array {32, 32}}));
}

TEST_F(ParticleGuidedSplitStrategyTest, DivisibleDimWithParticle) {
    auto range = DS::Range<2> {std::array<int, 2> {33, 33}};
    auto plan = ParallelPlan {};
    plan.distributed_workers_count = 4;
    setGlobalParallelPlan(plan);
    auto strategy = ParticleGuidedSplitStrategy<Field> {};
    auto mesh = MeshBuilder<Mesh>()
                        .newMesh(33, 33)
                        .setMeshOfDim(0, 0., 1.)
                        .setMeshOfDim(1, 0., 1.)
                        .setPadWidth(5)
                        .build();
    strategy.setMaxLevel(2);
    strategy.setRefMesh(mesh);
    std::vector<Particle<2>> parts;
    parts.push_back(Particle<2> {std::array<double, 2> {0.1, 0.1}});
    strategy.setParticles(parts);
    strategy.setParticleLoad(100);
    // input nodal range, return centered range
    auto map = strategy.getSplitMap(range, plan);
    for (auto& m : map) std::print("{}", m.toString());
    ASSERT_TRUE((map[0] == DS::Range<2> {std::array {15, 13}}));
    ASSERT_TRUE((map[2] == DS::Range<2> {std::array {15, 0}, std::array {32, 16}}));
    ASSERT_TRUE((map[1] == DS::Range<2> {std::array {0, 13}, std::array {15, 32}}));
    ASSERT_TRUE((map[3] == DS::Range<2> {std::array {15, 16}, std::array {32, 32}}));
}

TEST_F(ParticleGuidedSplitStrategyTest, DivisibleDimInEqual) {
    GTEST_SKIP() << "Non-power-of-2 procs currently not supported.";
    auto range = DS::Range<2> {std::array<int, 2> {49, 33}};
    auto plan = ParallelPlan {};
    plan.distributed_workers_count = 6;
    setGlobalParallelPlan(plan);
    auto strategy = ParticleGuidedSplitStrategy<Field> {};
    // input nodal range, return centered range
    auto map = strategy.getSplitMap(range, plan);
    ASSERT_TRUE((map[0] == DS::Range<2> {std::array {16, 16}}));
    ASSERT_TRUE((map[1] == DS::Range<2> {std::array {16, 0}, std::array {32, 16}}));
    ASSERT_TRUE((map[2] == DS::Range<2> {std::array {32, 0}, std::array {48, 16}}));
    ASSERT_TRUE((map[3] == DS::Range<2> {std::array {0, 16}, std::array {16, 32}}));
    ASSERT_TRUE((map[4] == DS::Range<2> {std::array {16, 16}, std::array {32, 32}}));
    ASSERT_TRUE((map[5] == DS::Range<2> {std::array {32, 16}, std::array {48, 32}}));
}

TEST_F(ParticleGuidedSplitStrategyTest, UnDivisibleDim) {
    auto range = DS::Range<2> {std::array<int, 2> {34, 34}};
    auto plan = ParallelPlan {};
    plan.distributed_workers_count = 4;
    setGlobalParallelPlan(plan);
    auto strategy = ParticleGuidedSplitStrategy<Field> {};
    auto mesh = MeshBuilder<Mesh>()
                        .newMesh(34, 34)
                        .setMeshOfDim(0, 0., 1.)
                        .setMeshOfDim(1, 0., 1.)
                        .setPadWidth(5)
                        .build();
    strategy.setMaxLevel(2);
    strategy.setRefMesh(mesh);
    // input nodal range, return centered range
    auto map = strategy.getSplitMap(range, plan);
    for (auto& m : map) std::print("{}", m.toString());
    ASSERT_TRUE((map[0] == DS::Range<2> {std::array {17, 17}}));
    ASSERT_TRUE((map[2] == DS::Range<2> {std::array {17, 0}, std::array {33, 17}}));
    ASSERT_TRUE((map[1] == DS::Range<2> {std::array {0, 17}, std::array {17, 33}}));
    ASSERT_TRUE((map[3] == DS::Range<2> {std::array {17, 17}, std::array {33, 33}}));
}

TEST_F(ParticleGuidedSplitStrategyTest, UnDivisibleDimInEqual) {
    GTEST_SKIP() << "Non-power-of-2 procs currently not supported.";
    auto range = DS::Range<2> {std::array<int, 2> {50, 34}};
    auto plan = ParallelPlan {};
    plan.distributed_workers_count = 6;
    setGlobalParallelPlan(plan);
    auto strategy = ParticleGuidedSplitStrategy<Field> {};
    // input nodal range, return centered range
    auto map = strategy.getSplitMap(range, plan);
    ASSERT_TRUE((map[0] == DS::Range<2> {std::array {16, 16}}));
    ASSERT_TRUE((map[1] == DS::Range<2> {std::array {16, 0}, std::array {32, 16}}));
    ASSERT_TRUE((map[2] == DS::Range<2> {std::array {32, 0}, std::array {49, 16}}));
    ASSERT_TRUE((map[3] == DS::Range<2> {std::array {0, 16}, std::array {16, 33}}));
    ASSERT_TRUE((map[4] == DS::Range<2> {std::array {16, 16}, std::array {32, 33}}));
    ASSERT_TRUE((map[5] == DS::Range<2> {std::array {32, 16}, std::array {49, 33}}));
}

TEST_F(ParticleGuidedSplitStrategyTest, SerialTest) {
    auto range = DS::Range<2> {std::array<int, 2> {10, 10}};
    auto plan = ParallelPlan {};
    plan.distributed_workers_count = 1;
    setGlobalParallelPlan(plan);
    auto strategy = ParticleGuidedSplitStrategy<Field> {};
    auto mesh = MeshBuilder<Mesh>()
                        .newMesh(10, 10)
                        .setMeshOfDim(0, 0., 1.)
                        .setMeshOfDim(1, 0., 1.)
                        .setPadWidth(5)
                        .build();
    strategy.setMaxLevel(0);
    strategy.setRefMesh(mesh);
    // input nodal range, return centered range
    auto map = strategy.getSplitMap(range, plan);
    ASSERT_TRUE((map[0] == DS::Range<2> {std::array<int, 2> {9, 9}}));
}

TEST_F(ParticleGuidedSplitStrategyTest, WithOffset) {
    auto range = DS::Range<2> {std::array<int, 2> {3, 3}, std::array<int, 2> {36, 36}};
    auto plan = ParallelPlan {};
    plan.distributed_workers_count = 4;
    setGlobalParallelPlan(plan);
    auto strategy = ParticleGuidedSplitStrategy<Field> {};
    auto mesh = MeshBuilder<Mesh>()
                        .newMesh(33, 33)
                        .setMeshOfDim(0, 0., 1.)
                        .setMeshOfDim(1, 0., 1.)
                        .setPadWidth(5)
                        .setStart(std::array<int, 2> {3, 3})
                        .build();
    strategy.setMaxLevel(2);
    strategy.setRefMesh(mesh);
    // input nodal range, return centered range
    auto map = strategy.getSplitMap(range, plan);
    for (auto& m : map) std::print("{}", m.toString());
    ASSERT_TRUE((map[0] == DS::Range<2> {std::array {3, 3}, std::array {19, 19}}));
    ASSERT_TRUE((map[2] == DS::Range<2> {std::array {19, 3}, std::array {35, 19}}));
    ASSERT_TRUE((map[1] == DS::Range<2> {std::array {3, 19}, std::array {19, 35}}));
    ASSERT_TRUE((map[3] == DS::Range<2> {std::array {19, 19}, std::array {35, 35}}));
}

TEST_F(ParticleGuidedSplitStrategyTest, 1DTest) {
    auto range = DS::Range<1> {std::array<int, 1> {17}};
    auto plan = ParallelPlan {};
    plan.distributed_workers_count = 4;
    setGlobalParallelPlan(plan);
    auto strategy = ParticleGuidedSplitStrategy<CartesianField<double, CartesianMesh<Meta::int_<1>>>> {};
    auto mesh = MeshBuilder<CartesianMesh<Meta::int_<1>>>()
                        .newMesh(17)
                        .setMeshOfDim(0, 0., 1.)
                        .setPadWidth(5)
                        .build();
    strategy.setMaxLevel(2);
    strategy.setRefMesh(mesh);
    auto map = strategy.getSplitMap(range, plan);
    ASSERT_TRUE((map[0] == DS::Range<1> {std::array {4}}));
    ASSERT_TRUE((map[1] == DS::Range<1> {std::array {4}, std::array {8}}));
    ASSERT_TRUE((map[2] == DS::Range<1> {std::array {8}, std::array {12}}));
    ASSERT_TRUE((map[3] == DS::Range<1> {std::array {12}, std::array {16}}));
}

TEST_F(ParticleGuidedSplitStrategyTest, 3DTest) {
    GTEST_SKIP() << "Non-power-of-2 procs currently not supported.";
    auto range = DS::Range<3> {std::array<int, 3> {5, 6, 7}};
    auto plan = ParallelPlan {};
    plan.distributed_workers_count = 6;
    setGlobalParallelPlan(plan);
    auto strategy = ParticleGuidedSplitStrategy<CartesianField<double, CartesianMesh<Meta::int_<3>>>> {};
    auto map = strategy.getSplitMap(range, plan);
    ASSERT_EQ(map[0], (DS::Range<3> {std::array {4, 2, 2}}));
    ASSERT_EQ(map[5], (DS::Range<3> {std::array {0, 2, 4}, std::array {4, 5, 6}}));
}