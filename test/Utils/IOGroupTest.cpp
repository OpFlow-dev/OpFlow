//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2022 by the OpFlow developers
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

TEST(IOGroupTest, SingleField) {
    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<double, Mesh>;

    auto m = MeshBuilder<Mesh>().newMesh(10, 10).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();

    auto u = ExprBuilder<Field>()
                     .setName("u")
                     .setMesh(m)
                     .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                     .build();

    u = 2;

    auto group = Utils::makeIOGroup<Utils::H5Stream>("./", StreamIn | StreamOut, u);
    group.dump(Utils::TimeStamp(0.));
    u = 1;
    group.read(Utils::TimeStamp(0.));

    ASSERT_EQ((u[DS::MDIndex<2> {0, 0}]), 2);
}

TEST(IOGroupTest, DoubleField) {
    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<double, Mesh>;

    auto m = MeshBuilder<Mesh>().newMesh(10, 10).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();

    auto u = ExprBuilder<Field>()
                     .setName("u")
                     .setMesh(m)
                     .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                     .build();
    auto v = u;
    v.name = "v";

    u = 2;
    v = 1;

    auto group = Utils::makeIOGroup<Utils::H5Stream>("./a", StreamIn | StreamOut, u, v);
    group.dump(Utils::TimeStamp(1.));
    u = 1;
    v = 2;
    group.read(Utils::TimeStamp(1.));

    ASSERT_EQ((u[DS::MDIndex<2> {0, 0}]), 2);
    ASSERT_EQ((v[DS::MDIndex<2> {0, 0}]), 1);
}

TEST(IOGroupTest, DoubleFieldWithoutName) {
    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<double, Mesh>;

    auto m = MeshBuilder<Mesh>().newMesh(10, 10).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();

    auto u = ExprBuilder<Field>().setMesh(m).setLoc({LocOnMesh::Center, LocOnMesh::Center}).build();
    auto v = u;

    u = 2;
    v = 1;

    auto group = Utils::makeIOGroup<Utils::H5Stream>("./", StreamIn | StreamOut, u, v);
    group.dump(Utils::TimeStamp(0.));
    u = 1;
    v = 2;
    group.read(Utils::TimeStamp(0.));

    ASSERT_EQ((u[DS::MDIndex<2> {0, 0}]), 2);
    ASSERT_EQ((v[DS::MDIndex<2> {0, 0}]), 1);
}

TEST(IOGroupTest, WriteOnlyStream) {
    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<double, Mesh>;

    auto m = MeshBuilder<Mesh>().newMesh(10, 10).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();

    auto u = ExprBuilder<Field>()
                     .setName("u")
                     .setMesh(m)
                     .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                     .build();
    auto v = u;
    v.name = "v";

    u = 2;
    v = 1;

    auto group = Utils::makeIOGroup<Utils::TecplotASCIIStream>("./", u, v);
    group.dump(Utils::TimeStamp(0.));

    ASSERT_TRUE(true);
}