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

#include "Core/Arrays/Arrays.hpp"
#include "DataStructures/Index/RangedIndex.hpp"
#include <gmock/gmock.h>

using namespace testing;
using namespace OpFlow;

class RangeIndexTest : public ::testing::Test {

protected:
    virtual void SetUp() {
        r1 = RangedIndex<1>(IndexArray<1> {0}, IndexArray<1> {10});
        r2 = RangedIndex<2>(IndexArray<2> {0, 0}, IndexArray<2> {10, 20});
        r3 = RangedIndex<3>(IndexArray<3> {-1, -2, -3}, IndexArray<3> {1, 2, 3}, IndexArray<3> {1, 2, 3});
    }

    RangeIndex<1> r1;
    RangeIndex<2> r2;
    RangeIndex<3> r3;
};

TEST_F(RangeIndexTest, InitPosTest) {
    ASSERT_EQ(r1.pos(), (IndexArray<1> {0}));
    ASSERT_EQ(r2.pos(), (IndexArray<2> {0, 0}));
    ASSERT_EQ(r3.pos(), (IndexArray<3> {-1, -2, -3}));
}

TEST_F(RangeIndexTest, ToLinearTest) {
    ASSERT_EQ(r1.toLinear(), 0);
    ASSERT_EQ(r2.toLinear(), 0);
    ASSERT_EQ(r3.toLinear(), 0);
    ++r1;
    ++r2;
    ++r3;
    ASSERT_EQ(r1.toLinear(), 1);
    ASSERT_EQ(r2.toLinear(), 1);
    ASSERT_EQ(r3.toLinear(), 1);

    ASSERT_EQ(r1.toLinear(IndexArray<1> {8}), 8);
    ASSERT_EQ(r2.toLinear(IndexArray<2> {1, 2}), 21);
    ASSERT_EQ(r3.toLinear(IndexArray<3> {0, 0, 0}), 7);
}

TEST_F(RangeIndexTest, ToIndexTest) {
    for (auto i = 0; i < 10; ++i) { ASSERT_EQ(r1.toLinear(r1.toIndex(i)), i); }
    for (auto i = 0; i < 200; ++i) { ASSERT_EQ(r2.toLinear(r2.toIndex(i)), i); }
    for (auto i = 0; i < 8; ++i) { ASSERT_EQ(r3.toLinear(r3.toIndex(i)), i); }
}

TEST_F(RangeIndexTest, SizeTest) {
    ASSERT_EQ(r1.instSize(), 10);
    ASSERT_EQ(r2.instSize(), 200);
    ASSERT_EQ(r3.instSize(), 8);
}