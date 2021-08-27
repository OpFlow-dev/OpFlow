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

using Mesh2 = CartesianMesh<Meta::int_<2>>;
using Field2 = CartesianField<double, Mesh2>;

TEST(CartesianFieldBuilderTest, ConstDircBC) {
    auto mesh = MeshBuilder<Mesh2>().newMesh(10, 10).build();
    auto field = ExprBuilder<Field2>().setMesh(mesh).setBC(0, DimPos::start, BCType::Dirc, 1.).build();

    for (auto i = 0; i < 10; ++i) { ASSERT_EQ(field.bc[0].start->evalAt(DS::MDIndex<2>(0, i)), 1.); }
}

TEST(CartesianFieldBuilderTest, ConstNeumBC) {
    auto mesh = MeshBuilder<Mesh2>().newMesh(10, 10).build();
    auto field = ExprBuilder<Field2>().setMesh(mesh).setBC(0, DimPos::start, BCType::Neum, 1.).build();

    for (auto i = 0; i < 10; ++i) { ASSERT_EQ(field.bc[0].start->evalAt(DS::MDIndex<2>(0, i)), 1.); }
}

TEST(CartesianFieldBuilderTest, FunctorDircBC) {
    auto mesh = MeshBuilder<Mesh2>().newMesh(10, 10).build();
    auto functor = [](auto&& i) { return i[0] * i[0] + i[1] * i[1]; };
    auto field = ExprBuilder<Field2>().setMesh(mesh).setBC(0, DimPos::start, BCType::Dirc, functor).build();

    for (auto i = 0; i < 10; ++i) {
        ASSERT_EQ(field.bc[0].start->evalAt(DS::MDIndex<2>(0, i)), functor(DS::MDIndex<2>(0, i)));
    }
}

TEST(CartesianFieldBuilderTest, FunctorNeumBC) {
    auto mesh = MeshBuilder<Mesh2>().newMesh(10, 10).build();
    auto functor = [](auto&& i) { return i[0] * i[0] + i[1] * i[1]; };
    auto field = ExprBuilder<Field2>().setMesh(mesh).setBC(0, DimPos::start, BCType::Neum, functor).build();

    for (auto i = 0; i < 10; ++i) {
        ASSERT_EQ(field.bc[0].start->evalAt(DS::MDIndex<2>(0, i)), functor(DS::MDIndex<2>(0, i)));
    }
}

TEST(CartesianFieldBuilderTest, SymmBC) {
    auto mesh = MeshBuilder<Mesh2>().newMesh(10, 10).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();
    auto field = ExprBuilder<Field2>()
                         .setMesh(mesh)
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

TEST(CartesianFieldBuilderTest, ASymmBC) {
    auto mesh = MeshBuilder<Mesh2>().newMesh(10, 10).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();
    auto field = ExprBuilder<Field2>()
                         .setMesh(mesh)
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
TEST(CartesianFieldBuilderDeathTest, ASymmBCDieOnNonZeroBC) {
    auto mesh = MeshBuilder<Mesh2>().newMesh(10, 10).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();
    auto field = ExprBuilder<Field2>()
                         .setMesh(mesh)
                         .setBC(0, DimPos::start, BCType::ASymm)
                         .setBC(0, DimPos::end, BCType::ASymm)
                         .build();
    field.initBy([](auto&& x) { return x[0]; });
    ASSERT_DEATH(field.bc[0].end->evalAt(DS::MDIndex<2>(9, 9)), "");
}
#endif

TEST(CartesianFieldBuilderTest, PeriodicBC) {
    auto mesh = MeshBuilder<Mesh2>().newMesh(10, 10).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();
    auto field = ExprBuilder<Field2>()
                         .setMesh(mesh)
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
TEST(CartesianFieldBuilderTest, PeriodicBC_TakeLeftSide) {
    auto mesh = MeshBuilder<Mesh2>().newMesh(10, 10).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();
    auto field = ExprBuilder<Field2>()
                         .setMesh(mesh)
                         .setBC(0, DimPos::start, BCType::Periodic)
                         .setBC(0, DimPos::end, BCType::Periodic)
                         .build();
    field.initBy([](auto&& x) { return x[0]; });
    ASSERT_TRUE(DS::inRange(field.assignableRange, DS::MDIndex<2>(0, 0)));
}

TEST(CartesianFieldBuilderTest, PeriodicBC_AbandonRightSide) {
    auto mesh = MeshBuilder<Mesh2>().newMesh(10, 10).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();
    auto field = ExprBuilder<Field2>()
                         .setMesh(mesh)
                         .setBC(0, DimPos::start, BCType::Periodic)
                         .setBC(0, DimPos::end, BCType::Periodic)
                         .build();
    field.initBy([](auto&& x) { return x[0]; });
    ASSERT_FALSE(DS::inRange(field.accessibleRange, DS::MDIndex<2>(9, 0)));
}

TEST(CartesianFieldBuilderTest, BuildAfterDeclear) {
    Mesh2 m;
    Field2 u, v;
    m = MeshBuilder<Mesh2>().newMesh(10, 10).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();
    u = ExprBuilder<Field2>().setMesh(m).build();
    v = ExprBuilder<Field2>().setMesh(m).build();
    u.initBy([](auto&& x) { return x[0]; });
    v.initBy([](auto&& x) { return x[1]; });
    rangeFor_s(u.accessibleRange, [&](auto&& i) { ASSERT_DOUBLE_EQ(u[i], m.x(0, i)); });
    rangeFor_s(v.accessibleRange, [&](auto&& i) { ASSERT_DOUBLE_EQ(v[i], m.x(1, i)); });
    v = u;
    rangeFor_s(v.assignableRange, [&](auto&& i) { ASSERT_DOUBLE_EQ(v[i], u[i]); });
}