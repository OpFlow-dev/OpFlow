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
//
//

#include <OpFlow>
#include <gmock/gmock.h>

using namespace OpFlow;
using namespace testing;

class EvenSplitStrategyTest : public Test {
protected:
    void SetUp() override {}
    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<double, Mesh>;
};

TEST_F(EvenSplitStrategyTest, DivisibleDim) {
    auto range = DS::Range<2> {std::array<int, 2> {33, 33}};
    auto plan = ParallelPlan {};
    plan.distributed_workers_count = 4;
    setGlobalParallelPlan(plan);
    auto strategy = EvenSplitStrategy<Field> {};
    // input nodal range, return centered range
    auto map = strategy.getSplitMap(range, plan);
    ASSERT_TRUE((map[0] == DS::Range<2> {std::array {16, 16}}));
    ASSERT_TRUE((map[1] == DS::Range<2> {std::array {16, 0}, std::array {32, 16}}));
    ASSERT_TRUE((map[2] == DS::Range<2> {std::array {0, 16}, std::array {16, 32}}));
    ASSERT_TRUE((map[3] == DS::Range<2> {std::array {16, 16}, std::array {32, 32}}));
}

TEST_F(EvenSplitStrategyTest, DivisibleDimInEqual) {
    auto range = DS::Range<2> {std::array<int, 2> {49, 33}};
    auto plan = ParallelPlan {};
    plan.distributed_workers_count = 6;
    setGlobalParallelPlan(plan);
    auto strategy = EvenSplitStrategy<Field> {};
    // input nodal range, return centered range
    auto map = strategy.getSplitMap(range, plan);
    ASSERT_TRUE((map[0] == DS::Range<2> {std::array {16, 16}}));
    ASSERT_TRUE((map[1] == DS::Range<2> {std::array {16, 0}, std::array {32, 16}}));
    ASSERT_TRUE((map[2] == DS::Range<2> {std::array {32, 0}, std::array {48, 16}}));
    ASSERT_TRUE((map[3] == DS::Range<2> {std::array {0, 16}, std::array {16, 32}}));
    ASSERT_TRUE((map[4] == DS::Range<2> {std::array {16, 16}, std::array {32, 32}}));
    ASSERT_TRUE((map[5] == DS::Range<2> {std::array {32, 16}, std::array {48, 32}}));
}

TEST_F(EvenSplitStrategyTest, UnDivisibleDim) {
    auto range = DS::Range<2> {std::array<int, 2> {34, 34}};
    auto plan = ParallelPlan {};
    plan.distributed_workers_count = 4;
    setGlobalParallelPlan(plan);
    auto strategy = EvenSplitStrategy<Field> {};
    // input nodal range, return centered range
    auto map = strategy.getSplitMap(range, plan);
    ASSERT_TRUE((map[0] == DS::Range<2> {std::array {16, 16}}));
    ASSERT_TRUE((map[1] == DS::Range<2> {std::array {16, 0}, std::array {33, 16}}));
    ASSERT_TRUE((map[2] == DS::Range<2> {std::array {0, 16}, std::array {16, 33}}));
    ASSERT_TRUE((map[3] == DS::Range<2> {std::array {16, 16}, std::array {33, 33}}));
}

TEST_F(EvenSplitStrategyTest, UnDivisibleDimInEqual) {
    auto range = DS::Range<2> {std::array<int, 2> {50, 34}};
    auto plan = ParallelPlan {};
    plan.distributed_workers_count = 6;
    setGlobalParallelPlan(plan);
    auto strategy = EvenSplitStrategy<Field> {};
    // input nodal range, return centered range
    auto map = strategy.getSplitMap(range, plan);
    ASSERT_TRUE((map[0] == DS::Range<2> {std::array {16, 16}}));
    ASSERT_TRUE((map[1] == DS::Range<2> {std::array {16, 0}, std::array {32, 16}}));
    ASSERT_TRUE((map[2] == DS::Range<2> {std::array {32, 0}, std::array {49, 16}}));
    ASSERT_TRUE((map[3] == DS::Range<2> {std::array {0, 16}, std::array {16, 33}}));
    ASSERT_TRUE((map[4] == DS::Range<2> {std::array {16, 16}, std::array {32, 33}}));
    ASSERT_TRUE((map[5] == DS::Range<2> {std::array {32, 16}, std::array {49, 33}}));
}

TEST_F(EvenSplitStrategyTest, SerialTest) {
    auto range = DS::Range<2> {std::array<int, 2> {10, 10}};
    auto plan = ParallelPlan {};
    plan.distributed_workers_count = 1;
    setGlobalParallelPlan(plan);
    auto strategy = EvenSplitStrategy<Field> {};
    // input nodal range, return centered range
    auto map = strategy.getSplitMap(range, plan);
    ASSERT_TRUE((map[0] == DS::Range<2> {std::array<int, 2> {9, 9}}));
}

TEST_F(EvenSplitStrategyTest, WithOffset) {
    auto range = DS::Range<2> {std::array<int, 2> {3, 3}, std::array<int, 2> {36, 36}};
    auto plan = ParallelPlan {};
    plan.distributed_workers_count = 4;
    setGlobalParallelPlan(plan);
    auto strategy = EvenSplitStrategy<Field> {};
    // input nodal range, return centered range
    auto map = strategy.getSplitMap(range, plan);
    ASSERT_TRUE((map[0] == DS::Range<2> {std::array {3, 3}, std::array {19, 19}}));
    ASSERT_TRUE((map[1] == DS::Range<2> {std::array {19, 3}, std::array {35, 19}}));
    ASSERT_TRUE((map[2] == DS::Range<2> {std::array {3, 19}, std::array {19, 35}}));
    ASSERT_TRUE((map[3] == DS::Range<2> {std::array {19, 19}, std::array {35, 35}}));
}

TEST_F(EvenSplitStrategyTest, 1DTest) {
    auto range = DS::Range<1> {std::array<int, 1> {17}};
    auto plan = ParallelPlan {};
    plan.distributed_workers_count = 4;
    setGlobalParallelPlan(plan);
    auto strategy = EvenSplitStrategy<CartesianField<double, CartesianMesh<Meta::int_<1>>>> {};
    auto map = strategy.getSplitMap(range, plan);
    ASSERT_TRUE((map[0] == DS::Range<1> {std::array {4}}));
    ASSERT_TRUE((map[1] == DS::Range<1> {std::array {4}, std::array {8}}));
    ASSERT_TRUE((map[2] == DS::Range<1> {std::array {8}, std::array {12}}));
    ASSERT_TRUE((map[3] == DS::Range<1> {std::array {12}, std::array {16}}));
}

TEST_F(EvenSplitStrategyTest, 3DTest) {
    auto range = DS::Range<3> {std::array<int, 3> {5, 6, 7}};
    auto plan = ParallelPlan {};
    plan.distributed_workers_count = 6;
    setGlobalParallelPlan(plan);
    auto strategy = EvenSplitStrategy<CartesianField<double, CartesianMesh<Meta::int_<3>>>> {};
    auto map = strategy.getSplitMap(range, plan);
    ASSERT_EQ(map[0], (DS::Range<3> {std::array {4, 2, 2}}));
    ASSERT_EQ(map[5], (DS::Range<3> {std::array {0, 2, 4}, std::array {4, 5, 6}}));
}