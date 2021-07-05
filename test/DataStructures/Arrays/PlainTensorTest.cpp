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

#include "DataStructures/Arrays/Tensor/PlainTensor.hpp"
#include "DataStructures/Index/MDIndex.hpp"
#include <gmock/gmock.h>

using namespace OpFlow;
using namespace OpFlow::DS;

template struct DS::PlainTensor<float, 4>;

class PlainTensorTest : public testing::Test {
protected:
    void SetUp() override { x.reShape(1, 2, 3, 4); }

    PlainTensor<float, 4> x;
};

TEST_F(PlainTensorTest, ConstTest) {
    x.setConstant(1);
    for (auto& i : x) { ASSERT_EQ(i, 1); }
}

TEST_F(PlainTensorTest, ParenthesesGetterTest) {
    x.setConstant(-1.);
    x(0, 1, 1, 0) = 1;
    ASSERT_EQ(x.get(1 * 1 + 1 * 1 * 2), 1);
    ASSERT_EQ(x(std::array<int, 4> {0, 1, 1, 0}), 1);
    MDIndex<4> idx;
    idx.set({0, 1, 1, 0});
    ASSERT_EQ(x(idx), 1);
}

TEST_F(PlainTensorTest, BracketsGetterTest) {
    x.setConstant(-1.);
    x(0, 1, 1, 0) = 1;
    auto idx = std::array<int, 4> {0, 1, 1, 0};
    ASSERT_EQ(x[idx], 1);
    MDIndex<4> idx2;
    idx2.set({0, 1, 1, 0});
    ASSERT_EQ(x[idx2], 1);
}