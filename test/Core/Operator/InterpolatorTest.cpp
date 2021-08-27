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

#include <OpFlow>
#include <gmock/gmock.h>

using namespace OpFlow;
using namespace testing;

class Intp1DCenterToCornerTest : public Test {
protected:
    void SetUp() override {
        m = MeshBuilder<Mesh>().newMesh(10, 10).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();
        u = ExprBuilder<Field>()
                    .setMesh(m)
                    .setBC(0, DimPos::start, BCType::Dirc, 0.)
                    .setBC(0, DimPos::end, BCType::Dirc, 0.)
                    .setBC(1, DimPos::start, BCType::Neum, 0.)
                    .setBC(1, DimPos::end, BCType::Neum, 0.)
                    .setLoc({LocOnMesh::Corner, LocOnMesh::Center})
                    .build();
        v = ExprBuilder<Field>()
                    .setMesh(m)
                    .setBC(0, DimPos::start, BCType::Neum, 0.)
                    .setBC(0, DimPos::end, BCType::Neum, 0.)
                    .setBC(1, DimPos::start, BCType::Dirc, 0.)
                    .setBC(1, DimPos::end, BCType::Dirc, 0.)
                    .setLoc({LocOnMesh::Center, LocOnMesh::Corner})
                    .build();
        u = 1;
        v = 2;
    }

    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;
    Mesh m;
    Field u, v;
};

TEST_F(Intp1DCenterToCornerTest, NeumRangeCheck) {
    auto t = d1IntpCenterToCorner<1>(u);
    t.prepare();
    ASSERT_EQ(t.accessibleRange.start[1], m.getRange().start[1]);
    ASSERT_EQ(t.accessibleRange.end[1], m.getRange().end[1]);
}

TEST_F(Intp1DCenterToCornerTest, NeumBCValueCheck) {
    auto t = d1IntpCenterToCorner<1>(u);
    t.prepare();
    ASSERT_DOUBLE_EQ(t.evalSafeAt(DS::MDIndex<2> {0, 0}), 0.);
    ASSERT_DOUBLE_EQ(t.evalSafeAt(DS::MDIndex<2> {1, 0}), 1.);
}

TEST_F(Intp1DCenterToCornerTest, ProductRangeCheck) {
    auto t = d1IntpCenterToCorner<1>(u) * d1IntpCenterToCorner<0>(v);
    t.prepare();
    ASSERT_EQ(t.accessibleRange, m.getRange());
}

TEST_F(Intp1DCenterToCornerTest, ProductValueCheck) {
    auto t = d1IntpCenterToCorner<1>(u) * d1IntpCenterToCorner<0>(v);
    t.prepare();
    ASSERT_DOUBLE_EQ(t.evalSafeAt(DS::MDIndex<2> {0, 0}), 0.);
    ASSERT_DOUBLE_EQ(t.evalSafeAt(DS::MDIndex<2> {1, 0}), 0.);
}