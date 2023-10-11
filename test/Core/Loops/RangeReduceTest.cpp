// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2023 by the OpFlow developers
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
#include <vector>

using namespace OpFlow;
using namespace testing;

class RangeReduceTest : public testing::Test {
protected:
    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;

    Field u;

    void SetUp() override {
        auto m = MeshBuilder<Mesh>()
                         .newMesh(200, 200)
                         .setMeshOfDim(0, 0., 1.)
                         .setMeshOfDim(1, 0., 1.)
                         .build();
        u = ExprBuilder<Field>().setMesh(m).setLoc({LocOnMesh::Center, LocOnMesh::Center}).build();
    }
};

TEST_F(RangeReduceTest, D1Reduce_s) {
    std::vector<int> v(10, 1);
    for (int i = 1; i < 10; ++i) { v[i] = v[i - 1] * 10; }
    auto val = rangeReduce_s(
            DS::Range<1> {std::array {10}}, [](auto&& a, auto&& b) { return a + b; },
            [&](auto&& i) { return v[i[0]]; });
    ASSERT_EQ(val, 1111111111);
}

TEST_F(RangeReduceTest, D1Reduce) {
    std::vector<int> v(10, 1);
    for (int i = 1; i < 10; ++i) { v[i] = v[i - 1] * 10; }
    auto val = rangeReduce(
            DS::Range<1> {std::array {10}}, [](auto&& a, auto&& b) { return a + b; },
            [&](auto&& i) { return v[i[0]]; });
    ASSERT_EQ(val, 1111111111);
}

TEST_F(RangeReduceTest, D2Reduce_s) {
    u = 1;
    auto val = rangeReduce_s(
            u.accessibleRange, [](auto&& a, auto&& b) { return a + b; }, [&](auto&& i) { return u[i]; });
    ASSERT_EQ(val, u.accessibleRange.count());
}

TEST_F(RangeReduceTest, D2Reduce) {
    u.initBy([&](auto&& x) { return std::cos(x[0]) * std::sin(x[1]) * std::sin(x[0] * x[1]); });
    auto val2 = rangeReduce_s(
            u.assignableRange, [](auto&& a, auto&& b) { return a + b; }, [&](auto&& i) { return u[i]; });
    auto val = rangeReduce(
            u.assignableRange, [](auto&& a, auto&& b) { return a + b; }, [&](auto&& i) { return u[i]; });
    ASSERT_NEAR(double(val) / double(val2), 1.0, 1e-14);
}
