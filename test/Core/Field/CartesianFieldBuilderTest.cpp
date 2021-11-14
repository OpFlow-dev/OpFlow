//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2021  by the OpFlow developers
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

class CartesianFieldTest : public testing::Test {
protected:
    void SetUp() override {
        m = MeshBuilder<Mesh>().newMesh(10).setMeshOfDim(0, 0., 1.).build();
        m2 = MeshBuilder<Mesh2>().newMesh(10, 10).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();
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
    ASSERT_EQ(u.accessibleRange.start[0], m.getRange().start[0]);
    ASSERT_EQ(u.accessibleRange.end[0], m.getRange().end[0] - 1);
    ASSERT_EQ(u.logicalRange.start[0], m.getRange().start[0]);
    ASSERT_EQ(u.logicalRange.end[0], m.getRange().end[0] - 1);
    ASSERT_EQ(u.assignableRange.start[0], m.getRange().start[0]);
    ASSERT_EQ(u.assignableRange.end[0], m.getRange().end[0] - 1);
}

TEST_F(CartesianFieldTest, ConstDircBC) {
    auto field = ExprBuilder<Field2>().setMesh(m2).setBC(0, DimPos::start, BCType::Dirc, 1.).build();

    for (auto i = 0; i < 10; ++i) { ASSERT_EQ(field.bc[0].start->evalAt(DS::MDIndex<2>(0, i)), 1.); }
}

TEST_F(CartesianFieldTest, ConstNeumBC) {
    auto field = ExprBuilder<Field2>().setMesh(m2).setBC(0, DimPos::start, BCType::Neum, 1.).build();

    for (auto i = 0; i < 10; ++i) { ASSERT_EQ(field.bc[0].start->evalAt(DS::MDIndex<2>(0, i)), 1.); }
}

TEST_F(CartesianFieldTest, FunctorDircBC) {
    auto functor = [](auto&& i) { return i[0] * i[0] + i[1] * i[1]; };
    auto field = ExprBuilder<Field2>().setMesh(m2).setBC(0, DimPos::start, BCType::Dirc, functor).build();

    for (auto i = 0; i < 10; ++i) {
        ASSERT_EQ(field.bc[0].start->evalAt(DS::MDIndex<2>(0, i)), functor(DS::MDIndex<2>(0, i)));
    }
}

TEST_F(CartesianFieldTest, FunctorNeumBC) {
    auto functor = [](auto&& i) { return i[0] * i[0] + i[1] * i[1]; };
    auto field = ExprBuilder<Field2>().setMesh(m2).setBC(0, DimPos::start, BCType::Neum, functor).build();

    for (auto i = 0; i < 10; ++i) {
        ASSERT_EQ(field.bc[0].start->evalAt(DS::MDIndex<2>(0, i)), functor(DS::MDIndex<2>(0, i)));
    }
}

TEST_F(CartesianFieldTest, SymmBC) {
    auto field = ExprBuilder<Field2>()
                         .setMesh(m2)
                         .setBC(0, DimPos::start, BCType::Symm)
                         .setBC(0, DimPos::end, BCType::Symm)
                         .build();
    field.initBy([](auto&& x) { return x[0]; });
    for (auto i = 0; i < 10; ++i) {
        ASSERT_EQ(field.bc[0].start->evalAt(DS::MDIndex<2>(-i, i)), field.evalAt(DS::MDIndex<2>(i, i)));
    }
    for (auto i = 9; i >= 0; --i) {
        ASSERT_EQ(field.bc[0].end->evalAt(DS::MDIndex<2>(18 - i, i)), field.evalAt(DS::MDIndex<2>(i, i)));
    }
}

TEST_F(CartesianFieldTest, ASymmBC) {
    auto field = ExprBuilder<Field2>()
                         .setMesh(m2)
                         .setBC(0, DimPos::start, BCType::ASymm)
                         .setBC(0, DimPos::end, BCType::ASymm)
                         .build();
    field.initBy([](auto&& x) { return x[0]; });
    for (auto i = 0; i < 10; ++i) {
        ASSERT_EQ(-field.bc[0].start->evalAt(DS::MDIndex<2>(-i, i)), field.evalAt(DS::MDIndex<2>(i, i)));
    }
    for (auto i = 8; i >= 0; --i) {
        ASSERT_EQ(-field.bc[0].end->evalAt(DS::MDIndex<2>(18 - i, i)), field.evalAt(DS::MDIndex<2>(i, i)));
    }
}

#ifndef NDEBUG
using CartesianFieldDeathTest = CartesianFieldTest;
TEST_F(CartesianFieldDeathTest, ASymmBCDieOnNonZeroBC) {
    auto field = ExprBuilder<Field2>()
                         .setMesh(m2)
                         .setBC(0, DimPos::start, BCType::ASymm)
                         .setBC(0, DimPos::end, BCType::ASymm)
                         .build();
    field.initBy([](auto&& x) { return x[0]; });
    ASSERT_DEATH(field.bc[0].end->evalAt(DS::MDIndex<2>(9, 9)), "");
}
#endif

TEST_F(CartesianFieldTest, PeriodicBC) {
    auto field = ExprBuilder<Field2>()
                         .setMesh(m2)
                         .setBC(0, DimPos::start, BCType::Periodic)
                         .setBC(0, DimPos::end, BCType::Periodic)
                         .build();
    field.initBy([](auto&& x) { return x[0]; });
    for (auto i = 0; i < 9; ++i) {
        ASSERT_EQ(field.bc[0].start->evalAt(DS::MDIndex<2>(-i, i)), field.evalAt(DS::MDIndex<2>(9 - i, i)));
    }
    for (auto i = 9; i < 18; ++i) {
        ASSERT_EQ(field.bc[0].end->evalAt(DS::MDIndex<2>(i, i - 9)),
                  field.evalAt(DS::MDIndex<2>(i - 9, i - 9)));
    }
}
TEST_F(CartesianFieldTest, PeriodicBC_TakeLeftSide) {
    auto field = ExprBuilder<Field2>()
                         .setMesh(m2)
                         .setBC(0, DimPos::start, BCType::Periodic)
                         .setBC(0, DimPos::end, BCType::Periodic)
                         .build();
    field.initBy([](auto&& x) { return x[0]; });
    ASSERT_TRUE(DS::inRange(field.assignableRange, DS::MDIndex<2>(0, 0)));
}

TEST_F(CartesianFieldTest, PeriodicBC_AbandonRightSide) {
    auto field = ExprBuilder<Field2>()
                         .setMesh(m2)
                         .setBC(0, DimPos::start, BCType::Periodic)
                         .setBC(0, DimPos::end, BCType::Periodic)
                         .build();
    field.initBy([](auto&& x) { return x[0]; });
    ASSERT_FALSE(DS::inRange(field.accessibleRange, DS::MDIndex<2>(9, 0)));
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