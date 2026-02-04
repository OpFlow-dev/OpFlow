// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2026 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#include <OpFlow>
#include <gmock/gmock.h>

using namespace OpFlow;

template struct DS::MDIndex<3>;

class MDIndexTest : public testing::Test {
protected:
    void SetUp() override {}

    DS::MDIndex<3> i3;
};

TEST_F(MDIndexTest, InitZeroTest) {
    ASSERT_EQ(i3[0], 0);
    ASSERT_EQ(i3[1], 0);
    ASSERT_EQ(i3[2], 0);
}

TEST_F(MDIndexTest, InitFromIntList) {
    i3 = DS::MDIndex<3>(1, 2, 3);
    ASSERT_EQ(i3[0], 1);
    ASSERT_EQ(i3[1], 2);
    ASSERT_EQ(i3[2], 3);
}