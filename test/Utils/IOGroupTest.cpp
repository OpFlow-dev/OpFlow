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

TEST(IOGroupTest, SeparateFile) {
    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<double, Mesh>;

    auto m = MeshBuilder<Mesh>().newMesh(10, 10).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();

    auto u = ExprBuilder<Field>()
                     .setName("u")
                     .setMesh(m)
                     .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                     .build();

    u = 2;

    auto group = Utils::makeIOGroup<Utils::H5Stream>("./", StreamOut, u);
    group.dumpToSeparateFile();
    group.dump(Utils::TimeStamp(0.));
    u = 1;
    group.dump(Utils::TimeStamp(1.));
    u = 0;
    Utils::H5Stream readstream(std::format("./u_{:.6f}.h5", 0.), StreamIn);
    readstream >> u;

    ASSERT_EQ((u[DS::MDIndex<2> {0, 0}]), 2);
}

#ifndef NDEBUG
TEST(IOGroupTest, DEATH_NumberByStep) {
    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<double, Mesh>;

    auto m = MeshBuilder<Mesh>().newMesh(10, 10).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();

    auto u = ExprBuilder<Field>()
                     .setName("u")
                     .setMesh(m)
                     .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                     .build();

    u = 2;

    auto group = Utils::makeIOGroup<Utils::H5Stream>("./", StreamOut, u);
    group.dumpToSeparateFile();
    group.setNumberingType(OpFlow::Utils::NumberingType::ByStep);

    ASSERT_DEATH(group.dump(Utils::TimeStamp(0.)), "");
}
#endif

TEST(IOGroupTest, NumberByStep) {
    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<double, Mesh>;

    auto m = MeshBuilder<Mesh>().newMesh(10, 10).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();

    auto u = ExprBuilder<Field>()
                     .setName("u")
                     .setMesh(m)
                     .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                     .build();

    u = 2;

    auto group = Utils::makeIOGroup<Utils::H5Stream>("./", StreamOut, u);
    group.dumpToSeparateFile();
    group.setNumberingType(OpFlow::Utils::NumberingType::ByStep);
    group.dump(Utils::TimeStamp(0., 0));
    {
        Utils::H5Stream readstream("u_0.h5", StreamIn);
        readstream >> u;

        ASSERT_EQ((u[DS::MDIndex<2> {0, 0}]), 2);
    }
    u = 1.5;
    group.dump(Utils::TimeStamp(1., 1));
    {
        Utils::H5Stream readstream("u_1.h5", StreamIn);
        readstream.moveToTime(Utils::TimeStamp(1.));
        readstream >> u;

        ASSERT_EQ((u[DS::MDIndex<2> {0, 0}]), 1.5);
    }
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
    static_assert(
            requires(Utils::H5Stream s) { s.fixedMesh(); }, "H5Stream must have fixedMesh()");
    group.fixedMesh();
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

TEST(IOGroupTest, AllInOne) {
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
    group.setAllInOne(true);
    group.dump(Utils::TimeStamp(0.));

    ASSERT_TRUE(true);
}

TEST(IOGroupTest, Expression) {
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

    auto group = Utils::makeIOGroup<Utils::TecplotBinaryStream>("./", u - v);
    group.setAllInOne(true);
    group.dump(Utils::TimeStamp(0.));

    ASSERT_TRUE(true);
}