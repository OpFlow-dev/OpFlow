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