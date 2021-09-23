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

TEST(H5RWTest, ReadAfterWrite) {
    using namespace OpFlow;
    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;

    constexpr auto n = 5;
    auto mesh = MeshBuilder<Mesh>().newMesh(n, n).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();
    auto u = ExprBuilder<Field>()
                     .setName("u")
                     .setMesh(mesh)
                     .setBC(0, DimPos::start, BCType::Dirc, 1.)
                     .setBC(0, DimPos::end, BCType::Dirc, 1.)
                     .setBC(1, DimPos::start, BCType::Dirc, 1.)
                     .setBC(1, DimPos::end, BCType::Dirc, 1.)
                     .setLoc(std::array {LocOnMesh ::Center, LocOnMesh ::Center})
                     .build();

    u = 0.;
    u[DS::MDIndex<2>(2, 2)] = 2.;
    Utils::H5Stream stream("./u.h5");
    stream << u;

    auto v = u;
    v = 0.;
    Utils::H5Stream istream("./u.h5", StreamIn);
    istream >> v;
    ASSERT_EQ(v.evalAt(DS::MDIndex<2>(2, 2)), u.evalAt(DS::MDIndex<2>(2, 2)));
}

TEST(H5RWTest, ReadAfterWriteInEqualDim) {
    using namespace OpFlow;
    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;

    auto mesh = MeshBuilder<Mesh>().newMesh(5, 7).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();
    auto u = ExprBuilder<Field>()
                     .setName("u")
                     .setMesh(mesh)
                     .setBC(0, DimPos::start, BCType::Dirc, 1.)
                     .setBC(0, DimPos::end, BCType::Dirc, 1.)
                     .setBC(1, DimPos::start, BCType::Dirc, 1.)
                     .setBC(1, DimPos::end, BCType::Dirc, 1.)
                     .setLoc(std::array {LocOnMesh ::Center, LocOnMesh ::Center})
                     .build();

    u = 0.;
    auto map = DS::MDRangeMapper<2> {u.accessibleRange};
    rangeFor(u.assignableRange, [&](auto&& i) { u[i] = map(i); });
    Utils::H5Stream stream("./u_ieq.h5");
    stream << u;
    stream.close();

    auto v = u;
    v = 0.;
    Utils::H5Stream istream("./u_ieq.h5", StreamIn);
    istream >> v;
    istream.close();
    ASSERT_EQ(v.evalAt(DS::MDIndex<2>(2, 2)), u.evalAt(DS::MDIndex<2>(2, 2)));
}
