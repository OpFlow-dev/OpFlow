//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2026 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#include <gmock/gmock.h>

#include <OpFlow>
using namespace OpFlow;

class DircBCTest : public testing::Test {
protected:
    void SetUp() override { m = MeshBuilder<Mesh>().newMesh(11).setMeshOfDim(0, 0., 1.).build(); }

    using Mesh = CartesianMesh<Meta::int_<1>>;
    using Field = CartesianField<double, Mesh>;
    Mesh m;
};

TEST_F(DircBCTest, CornerDircWithExtValCheck) {
    auto u = ExprBuilder<Field>()
                     .setMesh(m)
                     .setBC(0, DimPos::start, BCType::Dirc, 0.)
                     .setBC(0, DimPos::end, BCType::Dirc, 1.)
                     .setExt(0, DimPos::start, 1)
                     .setExt(0, DimPos::end, 1)
                     .setLoc(LocOnMesh::Corner)
                     .build();
    u.initBy([](auto&& x) { return x[0]; });
    auto er = u.logicalRange;
    for (int i = er.start[0]; i < er.end[0]; ++i) {
        ASSERT_DOUBLE_EQ(u.evalAt(DS::MDIndex<1> {i}), m.x(0, i));
    }
}

TEST_F(DircBCTest, CenterDircWithExtValCheck) {
    auto u = ExprBuilder<Field>()
                     .setMesh(m)
                     .setBC(0, DimPos::start, BCType::Dirc, 0.)
                     .setBC(0, DimPos::end, BCType::Dirc, 1.)
                     .setExt(0, DimPos::start, 1)
                     .setExt(0, DimPos::end, 1)
                     .setLoc(LocOnMesh::Center)
                     .build();
    u.initBy([](auto&& x) { return x[0]; });
    auto er = u.logicalRange;
    for (int i = er.start[0]; i < er.end[0]; ++i) {
        ASSERT_DOUBLE_EQ(u.evalAt(DS::MDIndex<1> {i}), m.x(0, i) + m.dx(0, i) / 2.);
    }
}
