//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2021 by the OpFlow developers
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

using namespace OpFlow;
using namespace testing;

class ConvTest_CartesianField : public Test {
protected:
    void SetUp() override {
        m = MeshBuilder<Mesh>().newMesh(10, 10).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();
        u = ExprBuilder<Field>()
                    .setMesh(m)
                    .setBC(0, DimPos::start, BCType::Dirc, 0.)
                    .setBC(0, DimPos::end, BCType::Dirc, 0.)
                    .setBC(1, DimPos::start, BCType::Neum, 0.)
                    .setBC(1, DimPos::end, BCType::Neum, 0.)
                    .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                    .build();
        v = ExprBuilder<Field>()
                    .setMesh(m)
                    .setBC(0, DimPos::start, BCType::Neum, 0.)
                    .setBC(0, DimPos::end, BCType::Neum, 0.)
                    .setBC(1, DimPos::start, BCType::Dirc, 0.)
                    .setBC(1, DimPos::end, BCType::Dirc, 0.)
                    .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                    .build();
        u = 1;
        v = 2;
    }

    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;
    Mesh m;
    Field u, v;
};

TEST_F(ConvTest_CartesianField, RangeCheck) {
    auto kernel = DS::FixedSizeTensor<double, 3, 5>();
    auto t = conv(u, kernel);
    t.prepare();
    ASSERT_EQ(t.accessibleRange.start[0], u.accessibleRange.start[0] + 1);
    ASSERT_EQ(t.accessibleRange.start[1], u.accessibleRange.start[1] + 2);
    ASSERT_EQ(t.accessibleRange.end[0], u.accessibleRange.end[0] - 1);
    ASSERT_EQ(t.accessibleRange.end[1], u.accessibleRange.end[1] - 2);
}

TEST_F(ConvTest_CartesianField, ValueCheck_EqualSize) {
    rangeFor(u.assignableRange, [&](auto&& ii) { u[ii] = ii[1] * 10 + ii[0]; });
    for (auto j = 0; j < 3; ++j) {
        for (auto i = 0; i < 3; ++i) {
            auto kernel = DS::FixedSizeTensor<double, 3, 3> {0.};
            kernel[DS::MDIndex<2> {i, j}] = 1.;
            auto t = conv(u, kernel);
            t.prepare();
            ASSERT_EQ(t.evalAt(DS::MDIndex<2> {1, 1}), j * 10 + i);
        }
    }
}

TEST_F(ConvTest_CartesianField, ValueCheck_InequalSize) {
    rangeFor(u.assignableRange, [&](auto&& ii) { u[ii] = ii[1] * 10 + ii[0]; });
    for (auto j = 0; j < 3; ++j) {
        for (auto i = 0; i < 5; ++i) {
            auto kernel = DS::FixedSizeTensor<double, 5, 3> {0.};
            kernel[DS::MDIndex<2> {i, j}] = 1.;
            auto t = conv(u, kernel);
            t.prepare();
            ASSERT_EQ(t.evalAt(DS::MDIndex<2> {2, 1}), j * 10 + i);
        }
    }
}