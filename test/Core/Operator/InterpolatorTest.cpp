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
    void SetUp() override { m = MeshBuilder<Mesh>().newMesh(10).setMeshOfDim(0, 0., 1.).build(); }

    using Mesh = CartesianMesh<Meta::int_<1>>;
    using Field = CartesianField<Real, Mesh>;
    Mesh m;
};

TEST_F(Intp1DCenterToCornerTest, DircCheck) {
    auto u = ExprBuilder<Field>()
                     .setMesh(m)
                     .setBC(0, DimPos::start, BCType::Dirc, 0.)
                     .setBC(0, DimPos::end, BCType::Dirc, 0.)
                     .setExt(0, DimPos::start, 1)
                     .setExt(0, DimPos::end, 1)
                     .setLoc(LocOnMesh::Center)
                     .build();
    auto t = d1IntpCenterToCorner<0>(u);
    t.prepare();
    ASSERT_EQ(t.accessibleRange.start[0], u.accessibleRange.start[0] + 1);
    ASSERT_EQ(t.accessibleRange.end[0], u.accessibleRange.end[0]);
    ASSERT_EQ(t.logicalRange.start[0], u.logicalRange.start[0] + 1);
    ASSERT_EQ(t.logicalRange.end[0], u.logicalRange.end[0]);
    ASSERT_EQ(t.assignableRange, DS::Range<1>::EmptyRange());
}

class Intp1DCornerToCenterTest : public Test {
protected:
    void SetUp() override { m = MeshBuilder<Mesh>().newMesh(10).setMeshOfDim(0, 0., 1.).build(); }

    using Mesh = CartesianMesh<Meta::int_<1>>;
    using Field = CartesianField<Real, Mesh>;
    Mesh m;
};

TEST_F(Intp1DCornerToCenterTest, DircCheck) {
    auto u = ExprBuilder<Field>()
                     .setMesh(m)
                     .setBC(0, DimPos::start, BCType::Dirc, 0.)
                     .setBC(0, DimPos::end, BCType::Dirc, 0.)
                     .setExt(0, DimPos::start, 1)
                     .setExt(0, DimPos::end, 1)
                     .setLoc(LocOnMesh::Corner)
                     .build();
    auto t = d1IntpCornerToCenter<0>(u);
    t.prepare();
    ASSERT_EQ(t.accessibleRange.start[0], u.accessibleRange.start[0]);
    ASSERT_EQ(t.accessibleRange.end[0], u.accessibleRange.end[0] - 1);
    ASSERT_EQ(t.logicalRange.start[0], u.logicalRange.start[0]);
    ASSERT_EQ(t.logicalRange.end[0], u.logicalRange.end[0] - 1);
    ASSERT_EQ(t.assignableRange, DS::Range<1>::EmptyRange());
}

class Intp1DQUICKCenterToCornerTest : public Test {
protected:
    void SetUp() override { m = MeshBuilder<Mesh>().newMesh(10).setMeshOfDim(0, 0., 1.).build(); }

    using Mesh = CartesianMesh<Meta::int_<1>>;
    using Field = CartesianField<Real, Mesh>;
    Mesh m;
};

TEST_F(Intp1DQUICKCenterToCornerTest, DircCheck) {
    auto u = ExprBuilder<Field>()
                     .setMesh(m)
                     .setBC(0, DimPos::start, BCType::Dirc, 0.)
                     .setBC(0, DimPos::end, BCType::Dirc, 0.)
                     .setExt(2)
                     .setLoc(LocOnMesh::Center)
                     .build();
    auto v = ExprBuilder<Field>()
                     .setMesh(m)
                     .setBC(0, DimPos::start, BCType::Dirc, 0.)
                     .setBC(0, DimPos::end, BCType::Dirc, 0.)
                     .setExt(2)
                     .setLoc(LocOnMesh::Corner)
                     .build();
    v = 1.;
    auto t = d1QUICKIntpCenterToCorner<0>(v, u);
    t.prepare();
    ASSERT_EQ(t.accessibleRange.start[0], u.accessibleRange.start[0] + 2);
    ASSERT_EQ(t.accessibleRange.end[0], u.accessibleRange.end[0] - 2);
    ASSERT_EQ(t.logicalRange.start[0], u.logicalRange.start[0] + 2);
    ASSERT_EQ(t.logicalRange.end[0], u.logicalRange.end[0] - 2);
    ASSERT_EQ(t.localRange.start[0], u.localRange.start[0] + 2);
    ASSERT_EQ(t.localRange.end[0], u.localRange.end[0] - 2);
    ASSERT_EQ(t.assignableRange, DS::Range<1>::EmptyRange());
}
