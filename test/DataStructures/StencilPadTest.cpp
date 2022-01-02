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
using namespace testing;

class StencilPadTest : public testing::Test {
protected:
    using st = DS::StencilPad<DS::MDIndex<2>>;
    using cst = DS::StencilPad<DS::ColoredIndex<DS::MDIndex<2>>>;
    using st_m = DS::StencilPad<DS::MDIndex<2>, std::unordered_map>;
    using cst_m = DS::StencilPad<DS::ColoredIndex<DS::MDIndex<2>>, std::unordered_map>;
    using idx = DS::MDIndex<2>;
    using cidx = DS::ColoredIndex<DS::MDIndex<2>>;
};

TEST_F(StencilPadTest, Create_ST) {
    st s, ss(1.0);
    ASSERT_TRUE(s.pad.size() == 0);
    ASSERT_TRUE(ss.pad.size() == 0);
    ASSERT_DOUBLE_EQ(s.bias, 0.);
    ASSERT_DOUBLE_EQ(ss.bias, 1.);
}

TEST_F(StencilPadTest, Equality_ST) {
    st a, b, c(1.0);
    ASSERT_EQ(a, b);
    ASSERT_NE(a, c);
    a.pad[idx {0, 0}] = 1.0;
    b.pad[idx {0, 0}] = 1.0;
    ASSERT_EQ(a, b);
    a.reset();
    c.reset();
    ASSERT_EQ(a, c);
    a.pad[idx {0, 0}] = 1.0;
    a.pad[idx {1, 0}] = -1.0;
    c.pad[idx {1, 0}] = -1.0;
    c.pad[idx {0, 0}] = 1.0;
    ASSERT_EQ(a, c);
}

TEST_F(StencilPadTest, Add_ST) {
    st a, b, c;
    a.pad[idx {0, 0}] = 1.0;
    b.pad[idx {0, 0}] = 2.0;
    c.pad[idx {0, 0}] = 3.0;
    ASSERT_EQ(a + b, c);
    a.reset();
    b.reset();
    c.reset();
    a.pad[idx {0, 0}] = 1.0;
    b.pad[idx {1, 0}] = 1.0;
    c.pad[idx {0, 0}] = 1.0;
    c.pad[idx {1, 0}] = 1.0;
    ASSERT_EQ(a + b, c);
    a.reset();
    c.reset();
    c.bias = 1.0;
    ASSERT_EQ(a + 1.0, c);
}

TEST_F(StencilPadTest, Mul_ST) {
    st a, b, c;
    a.pad[idx {0, 0}] = 1.0;
    a.pad[idx {1, 0}] = 2.0;
    a.bias = 1.0;
    b = a * 2.0;
    c.pad[idx {0, 0}] = 2.0;
    c.pad[idx {1, 0}] = 4.0;
    c.bias = 2.0;
    ASSERT_EQ(b, c);
}

TEST_F(StencilPadTest, Div_ST) {
    st a, b, c;
    a.pad[idx {0, 0}] = 1.0;
    a.pad[idx {1, 0}] = 2.0;
    b = a / 2.0;
    c.pad[idx {0, 0}] = 0.5;
    c.pad[idx {1, 0}] = 1.0;
    ASSERT_EQ(b, c);
}

TEST_F(StencilPadTest, Create_ST_M) {
    st_m s, ss(1.0);
    ASSERT_TRUE(s.pad.size() == 0);
    ASSERT_TRUE(ss.pad.size() == 0);
    ASSERT_DOUBLE_EQ(s.bias, 0.);
    ASSERT_DOUBLE_EQ(ss.bias, 1.);
}

TEST_F(StencilPadTest, Equality_ST_M) {
    st_m a, b, c(1.0);
    ASSERT_EQ(a, b);
    ASSERT_NE(a, c);
    a.pad[idx {0, 0}] = 1.0;
    b.pad[idx {0, 0}] = 1.0;
    ASSERT_EQ(a, b);
    a.reset();
    c.reset();
    ASSERT_EQ(a, c);
    a.pad[idx {0, 0}] = 1.0;
    a.pad[idx {1, 0}] = -1.0;
    c.pad[idx {1, 0}] = -1.0;
    c.pad[idx {0, 0}] = 1.0;
    ASSERT_EQ(a, c);
}

TEST_F(StencilPadTest, Add_ST_M) {
    st_m a, b, c;
    a.pad[idx {0, 0}] = 1.0;
    b.pad[idx {0, 0}] = 2.0;
    c.pad[idx {0, 0}] = 3.0;
    ASSERT_EQ(a + b, c);
    a.reset();
    b.reset();
    c.reset();
    a.pad[idx {0, 0}] = 1.0;
    b.pad[idx {1, 0}] = 1.0;
    c.pad[idx {0, 0}] = 1.0;
    c.pad[idx {1, 0}] = 1.0;
    ASSERT_EQ(a + b, c);
    a.reset();
    c.reset();
    c.bias = 1.0;
    ASSERT_EQ(a + 1.0, c);
}

TEST_F(StencilPadTest, Mul_ST_M) {
    st_m a, b, c;
    a.pad[idx {0, 0}] = 1.0;
    a.pad[idx {1, 0}] = 2.0;
    b = a * 2.0;
    c.pad[idx {0, 0}] = 2.0;
    c.pad[idx {1, 0}] = 4.0;
    ASSERT_EQ(b, c);
}

TEST_F(StencilPadTest, Div_ST_M) {
    st_m a, b, c;
    a.pad[idx {0, 0}] = 1.0;
    a.pad[idx {1, 0}] = 2.0;
    b = a / 2.0;
    c.pad[idx {0, 0}] = 0.5;
    c.pad[idx {1, 0}] = 1.0;
    ASSERT_EQ(b, c);
}

TEST_F(StencilPadTest, Create_CST) {
    cst s, ss(1.0);
    ASSERT_TRUE(s.pad.size() == 0);
    ASSERT_TRUE(ss.pad.size() == 0);
    ASSERT_DOUBLE_EQ(s.bias, 0.);
    ASSERT_DOUBLE_EQ(ss.bias, 1.);
}

TEST_F(StencilPadTest, Equality_CST) {
    cst a, b, c(1.0);
    ASSERT_EQ(a, b);
    ASSERT_NE(a, c);
    a.pad[cidx {idx {0, 0}, 0}] = 1.0;
    b.pad[cidx {idx {0, 0}, 0}] = 1.0;
    ASSERT_EQ(a, b);
    a.reset();
    c.reset();
    ASSERT_EQ(a, c);
    a.pad[cidx {idx {0, 0}, 0}] = 1.0;
    a.pad[cidx {idx {1, 0}, 0}] = -1.0;
    c.pad[cidx {idx {1, 0}, 0}] = -1.0;
    c.pad[cidx {idx {0, 0}, 0}] = 1.0;
    ASSERT_EQ(a, c);
    a.reset();
    c.reset();
    a.pad[cidx {idx {0, 0}, 0}] = 1.0;
    c.pad[cidx {idx {0, 0}, 1}] = 1.0;
    ASSERT_NE(a, c);
}

TEST_F(StencilPadTest, Add_CST) {
    cst a, b, c;
    a.pad[cidx {idx {0, 0}, 0}] = 1.0;
    b.pad[cidx {idx {0, 0}, 0}] = 2.0;
    c.pad[cidx {idx {0, 0}, 0}] = 3.0;
    ASSERT_EQ(a + b, c);
    a.reset();
    b.reset();
    c.reset();
    a.pad[cidx {idx {0, 0}, 0}] = 1.0;
    b.pad[cidx {idx {1, 0}, 0}] = 1.0;
    c.pad[cidx {idx {0, 0}, 0}] = 1.0;
    c.pad[cidx {idx {1, 0}, 0}] = 1.0;
    ASSERT_EQ(a + b, c);
    a.reset();
    c.reset();
    c.bias = 1.0;
    ASSERT_EQ(a + 1.0, c);
    a.reset();
    b.reset();
    c.reset();
    a.pad[cidx {idx {0, 0}, 0}] = 1.0;
    b.pad[cidx {idx {0, 0}, 1}] = 1.0;
    c.pad[cidx {idx {0, 0}, 0}] = 1.0;
    c.pad[cidx {idx {0, 0}, 1}] = 1.0;
    ASSERT_EQ(a + b, c);
}

TEST_F(StencilPadTest, Mul_CST) {
    cst a, b, c;
    a.pad[cidx {idx {0, 0}, 0}] = 1.0;
    a.pad[cidx {idx {1, 0}, 0}] = 2.0;
    b = a * 2.0;
    c.pad[cidx {idx {0, 0}, 0}] = 2.0;
    c.pad[cidx {idx {1, 0}, 0}] = 4.0;
    ASSERT_EQ(b, c);
    a.reset();
    b.reset();
    c.reset();
    a.pad[cidx {idx {0, 0}, 0}] = 1.0;
    a.pad[cidx {idx {0, 0}, 1}] = 2.0;
    b = a * 2.0;
    c.pad[cidx {idx {0, 0}, 0}] = 2.0;
    c.pad[cidx {idx {0, 0}, 1}] = 4.0;
    ASSERT_EQ(b, c);
}

TEST_F(StencilPadTest, Div_CST) {
    cst a, b, c;
    a.pad[cidx {idx {0, 0}, 0}] = 1.0;
    a.pad[cidx {idx {1, 0}, 0}] = 2.0;
    b = a / 2.0;
    c.pad[cidx {idx {0, 0}, 0}] = 0.5;
    c.pad[cidx {idx {1, 0}, 0}] = 1.0;
    ASSERT_EQ(b, c);
    a.reset();
    b.reset();
    c.reset();
    a.pad[cidx {idx {0, 0}, 0}] = 1.0;
    a.pad[cidx {idx {0, 0}, 1}] = 2.0;
    b = a / 2.0;
    c.pad[cidx {idx {0, 0}, 0}] = 0.5;
    c.pad[cidx {idx {0, 0}, 1}] = 1.0;
    ASSERT_EQ(b, c);
}

TEST_F(StencilPadTest, Create_CST_M) {
    cst_m s, ss(1.0);
    ASSERT_TRUE(s.pad.size() == 0);
    ASSERT_TRUE(ss.pad.size() == 0);
    ASSERT_DOUBLE_EQ(s.bias, 0.);
    ASSERT_DOUBLE_EQ(ss.bias, 1.);
}

TEST_F(StencilPadTest, Equality_CST_M) {
    cst_m a, b, c(1.0);
    ASSERT_EQ(a, b);
    ASSERT_NE(a, c);
    a.pad[cidx {idx {0, 0}, 0}] = 1.0;
    b.pad[cidx {idx {0, 0}, 0}] = 1.0;
    ASSERT_EQ(a, b);
    a.reset();
    c.reset();
    ASSERT_EQ(a, c);
    a.pad[cidx {idx {0, 0}, 0}] = 1.0;
    a.pad[cidx {idx {1, 0}, 0}] = -1.0;
    c.pad[cidx {idx {1, 0}, 0}] = -1.0;
    c.pad[cidx {idx {0, 0}, 0}] = 1.0;
    ASSERT_EQ(a, c);
    a.reset();
    c.reset();
    a.pad[cidx {idx {0, 0}, 0}] = 1.0;
    c.pad[cidx {idx {0, 0}, 1}] = 1.0;
    ASSERT_NE(a, c);
}

TEST_F(StencilPadTest, Add_CST_M) {
    cst_m a, b, c;
    a.pad[cidx {idx {0, 0}, 0}] = 1.0;
    b.pad[cidx {idx {0, 0}, 0}] = 2.0;
    c.pad[cidx {idx {0, 0}, 0}] = 3.0;
    ASSERT_EQ(a + b, c);
    a.reset();
    b.reset();
    c.reset();
    a.pad[cidx {idx {0, 0}, 0}] = 1.0;
    b.pad[cidx {idx {1, 0}, 0}] = 1.0;
    c.pad[cidx {idx {0, 0}, 0}] = 1.0;
    c.pad[cidx {idx {1, 0}, 0}] = 1.0;
    ASSERT_EQ(a + b, c);
    a.reset();
    c.reset();
    c.bias = 1.0;
    ASSERT_EQ(a + 1.0, c);
    a.reset();
    b.reset();
    c.reset();
    a.pad[cidx {idx {0, 0}, 0}] = 1.0;
    b.pad[cidx {idx {0, 0}, 1}] = 1.0;
    c.pad[cidx {idx {0, 0}, 0}] = 1.0;
    c.pad[cidx {idx {0, 0}, 1}] = 1.0;
    ASSERT_EQ(a + b, c);
}

TEST_F(StencilPadTest, Mul_CST_M) {
    cst_m a, b, c;
    a.pad[cidx {idx {0, 0}, 0}] = 1.0;
    a.pad[cidx {idx {1, 0}, 0}] = 2.0;
    b = a * 2.0;
    c.pad[cidx {idx {0, 0}, 0}] = 2.0;
    c.pad[cidx {idx {1, 0}, 0}] = 4.0;
    ASSERT_EQ(b, c);
    a.reset();
    b.reset();
    c.reset();
    a.pad[cidx {idx {0, 0}, 0}] = 1.0;
    a.pad[cidx {idx {0, 0}, 1}] = 2.0;
    b = a * 2.0;
    c.pad[cidx {idx {0, 0}, 0}] = 2.0;
    c.pad[cidx {idx {0, 0}, 1}] = 4.0;
    ASSERT_EQ(b, c);
}

TEST_F(StencilPadTest, Div_CST_M) {
    cst_m a, b, c;
    a.pad[cidx {idx {0, 0}, 0}] = 1.0;
    a.pad[cidx {idx {1, 0}, 0}] = 2.0;
    b = a / 2.0;
    c.pad[cidx {idx {0, 0}, 0}] = 0.5;
    c.pad[cidx {idx {1, 0}, 0}] = 1.0;
    ASSERT_EQ(b, c);
    a.reset();
    b.reset();
    c.reset();
    a.pad[cidx {idx {0, 0}, 0}] = 1.0;
    a.pad[cidx {idx {0, 0}, 1}] = 2.0;
    b = a / 2.0;
    c.pad[cidx {idx {0, 0}, 0}] = 0.5;
    c.pad[cidx {idx {0, 0}, 1}] = 1.0;
    ASSERT_EQ(b, c);
}