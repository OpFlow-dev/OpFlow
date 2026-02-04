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
#include <print>
import opflow;

using namespace OpFlow;

class CartesianFieldTest : public testing::Test {
protected:
    void SetUp() override {
        m = MeshBuilder<Mesh>().newMesh(11).setMeshOfDim(0, 0., 1.).build();
        m2 = MeshBuilder<Mesh2>().newMesh(11, 11).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();
    }

    using Mesh = CartesianMesh<Meta::int_<1>>;
    using Field = CartesianField<double, Mesh>;
    using Mesh2 = CartesianMesh<Meta::int_<2>>;
    using Field2 = CartesianField<double, Mesh2>;
    Mesh m;
    Mesh2 m2;
};

TEST_F(CartesianFieldTest, CenterDircRangeCheck) {
    auto u = ExprBuilder<Field>()
                     .setMesh(m)
                     .setBC(0, DimPos::start, BCType::Dirc, 0.)
                     .setBC(0, DimPos::end, BCType::Dirc, 0.)
                     .setLoc(LocOnMesh::Center)
                     .build();
    auto mr = m.getRange();
    ASSERT_EQ(u.accessibleRange.start[0], mr.start[0]);
    ASSERT_EQ(u.accessibleRange.end[0], mr.end[0] - 1);
    ASSERT_EQ(u.localRange.start[0], mr.start[0]);
    ASSERT_EQ(u.localRange.end[0], mr.end[0] - 1);
    ASSERT_EQ(u.logicalRange.start[0], mr.start[0]);
    ASSERT_EQ(u.logicalRange.end[0], mr.end[0] - 1);
    ASSERT_EQ(u.assignableRange.start[0], mr.start[0]);
    ASSERT_EQ(u.assignableRange.end[0], mr.end[0] - 1);
}

TEST_F(CartesianFieldTest, CenterDircWithExtRangeCheck) {
    auto u = ExprBuilder<Field>()
                     .setMesh(m)
                     .setBC(0, DimPos::start, BCType::Dirc, 0.)
                     .setBC(0, DimPos::end, BCType::Dirc, 0.)
                     .setExt(0, DimPos::start, 1)
                     .setExt(0, DimPos::end, 1)
                     .setLoc(LocOnMesh::Center)
                     .build();
    auto mr = m.getRange();
    // extension only affects the logical range
    ASSERT_EQ(u.accessibleRange.start[0], mr.start[0]);
    ASSERT_EQ(u.accessibleRange.end[0], mr.end[0] - 1);
    ASSERT_EQ(u.localRange.start[0], mr.start[0]);
    ASSERT_EQ(u.localRange.end[0], mr.end[0] - 1);
    // extent by 1 cell
    ASSERT_EQ(u.logicalRange.start[0], mr.start[0] - 1);
    ASSERT_EQ(u.logicalRange.end[0], mr.end[0]);
    ASSERT_EQ(u.assignableRange.start[0], mr.start[0]);
    ASSERT_EQ(u.assignableRange.end[0], mr.end[0] - 1);
}

TEST_F(CartesianFieldTest, CornerDircRangeCheck) {
    auto u = ExprBuilder<Field>()
                     .setMesh(m)
                     .setBC(0, DimPos::start, BCType::Dirc, 0.)
                     .setBC(0, DimPos::end, BCType::Dirc, 0.)
                     .setLoc(LocOnMesh::Corner)
                     .build();
    auto mr = m.getRange();
    ASSERT_EQ(u.accessibleRange.start[0], mr.start[0]);
    ASSERT_EQ(u.accessibleRange.end[0], mr.end[0]);
    ASSERT_EQ(u.localRange.start[0], mr.start[0]);
    ASSERT_EQ(u.localRange.end[0], mr.end[0]);
    ASSERT_EQ(u.logicalRange.start[0], mr.start[0]);
    ASSERT_EQ(u.logicalRange.end[0], mr.end[0]);
    ASSERT_EQ(u.assignableRange.start[0], mr.start[0] + 1);
    ASSERT_EQ(u.assignableRange.end[0], mr.end[0] - 1);
}

TEST_F(CartesianFieldTest, CornerDircWithExtRangeCheck) {
    auto u = ExprBuilder<Field>()
                     .setMesh(m)
                     .setBC(0, DimPos::start, BCType::Dirc, 0.)
                     .setBC(0, DimPos::end, BCType::Dirc, 0.)
                     .setExt(0, DimPos::start, 1)
                     .setExt(0, DimPos::end, 1)
                     .setLoc(LocOnMesh::Corner)
                     .build();
    auto mr = m.getRange();
    // extension only affects the logical range
    ASSERT_EQ(u.accessibleRange.start[0], mr.start[0]);
    ASSERT_EQ(u.accessibleRange.end[0], mr.end[0]);
    ASSERT_EQ(u.localRange.start[0], mr.start[0]);
    ASSERT_EQ(u.localRange.end[0], mr.end[0]);
    // extent by 1 cell
    ASSERT_EQ(u.logicalRange.start[0], mr.start[0] - 1);
    ASSERT_EQ(u.logicalRange.end[0], mr.end[0] + 1);
    ASSERT_EQ(u.assignableRange.start[0], mr.start[0] + 1);
    ASSERT_EQ(u.assignableRange.end[0], mr.end[0] - 1);
}

TEST_F(CartesianFieldTest, CornerPeriodicRangeCheck) {
    auto u = ExprBuilder<Field>()
                     .setMesh(m)
                     .setBC(0, DimPos::start, BCType::Periodic)
                     .setBC(0, DimPos::end, BCType::Periodic)
                     .setLoc(LocOnMesh::Corner)
                     .build();
    auto mr = m.getRange();
    ASSERT_EQ(u.accessibleRange.start[0], mr.start[0]);
    ASSERT_EQ(u.accessibleRange.end[0], mr.end[0] - 1);
    ASSERT_EQ(u.assignableRange.start[0], mr.start[0]);
    ASSERT_EQ(u.assignableRange.end[0], mr.end[0] - 1);
    ASSERT_EQ(u.localRange.start[0], mr.start[0]);
    ASSERT_EQ(u.localRange.end[0], mr.end[0] - 1);
    ASSERT_EQ(u.logicalRange.start[0], u.accessibleRange.start[0]);
    ASSERT_EQ(u.logicalRange.end[0], u.accessibleRange.end[0]);
}

TEST_F(CartesianFieldTest, CenterPeriodicRangeCheck) {
    auto u = ExprBuilder<Field>()
                     .setMesh(m)
                     .setBC(0, DimPos::start, BCType::Periodic)
                     .setBC(0, DimPos::end, BCType::Periodic)
                     .setLoc(LocOnMesh::Center)
                     .build();
    auto mr = m.getRange();
    ASSERT_EQ(u.accessibleRange.start[0], mr.start[0]);
    ASSERT_EQ(u.accessibleRange.end[0], mr.end[0] - 1);
    ASSERT_EQ(u.assignableRange.start[0], mr.start[0]);
    ASSERT_EQ(u.assignableRange.end[0], mr.end[0] - 1);
    ASSERT_EQ(u.localRange.start[0], mr.start[0]);
    ASSERT_EQ(u.localRange.end[0], mr.end[0] - 1);
    ASSERT_EQ(u.logicalRange.start[0], u.accessibleRange.start[0]);
    ASSERT_EQ(u.logicalRange.end[0], u.accessibleRange.end[0]);
}

TEST_F(CartesianFieldTest, BuildAfterDeclear) {
    auto u = ExprBuilder<Field2>().setMesh(m2).build();
    auto v = ExprBuilder<Field2>().setMesh(m2).build();
    u.initBy([](auto&& x) { return x[0]; });
    v.initBy([](auto&& x) { return x[1]; });
    rangeFor_s(u.accessibleRange, [&](auto&& i) { ASSERT_DOUBLE_EQ(u[i], m2.x(0, i)); });
    rangeFor_s(v.accessibleRange, [&](auto&& i) { ASSERT_DOUBLE_EQ(v[i], m2.x(1, i)); });
    v = u;
    rangeFor_s(v.assignableRange, [&](auto&& i) { ASSERT_DOUBLE_EQ(v[i], u[i]); });
}

TEST_F(CartesianFieldTest, CenterPeriodicValueCheck) {
    auto u = ExprBuilder<Field2>()
                     .setMesh(m2)
                     .setBC(0, OpFlow::DimPos::start, OpFlow::BCType::Periodic)
                     .setBC(0, OpFlow::DimPos::end, OpFlow::BCType::Periodic)
                     .setBC(1, OpFlow::DimPos::start, OpFlow::BCType::Periodic)
                     .setBC(1, OpFlow::DimPos::end, OpFlow::BCType::Periodic)
                     .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                     .setExt(1)
                     .build();
    rangeFor_s(u.getLocalWritableRange(), [&](auto&& i) { u[i] = i[1] * 10 + i[0]; });
    u.updatePadding();
    rangeFor_s(u.getLocalReadableRange(), [&](auto&& i) {
        if (u[i] != (i[1] + 10) % 10 * 10 + (i[0] + 10) % 10) std::print(std::cerr, "Not equal at {}", i);
        ASSERT_DOUBLE_EQ(u[i], (i[1] + 10) % 10 * 10 + (i[0] + 10) % 10);
    });
}