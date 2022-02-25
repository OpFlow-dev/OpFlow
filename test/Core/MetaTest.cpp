// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2022 by the OpFlow developers
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

TEST(MetaTest, StaticFor) {
    std::vector<int> a(10, 0);
    Meta::static_for<5>([&]<int i>(Meta::int_<i>) {
        a[i] = i;
    });
    for (int i = 0; i < 5; ++i) {
        ASSERT_EQ(a[i], i);
    }
    for (int i = 5; i < 10; ++i) {
        ASSERT_EQ(a[i], 0);
    }
}

TEST(MetaTest, StaticForReverse) {
    std::vector<int> a(10, 0);
    Meta::static_for<4, -1, -1>([&]<int i>(Meta::int_<i>) {
        a[i] = i;
    });
    for (int i = 0; i < 5; ++i) {
        ASSERT_EQ(a[i], i);
    }
    for (int i = 5; i < 10; ++i) {
        ASSERT_EQ(a[i], 0);
    }
}

TEST(MetaTest, StaticForStep2) {
    std::vector<int> a(10, 0);
    Meta::static_for<0, 10, 2>([&]<int i>(Meta::int_<i>) {
        a[i] = i;
    });

    for (int i = 0; i < 10; i += 2) {
        ASSERT_EQ(a[i], i);
    }
    for (int i = 1; i < 10; i += 2) {
        ASSERT_EQ(a[i], 0);
    }
}