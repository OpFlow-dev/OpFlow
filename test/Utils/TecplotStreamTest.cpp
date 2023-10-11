//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2023 by the OpFlow developers
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

TEST(TecIOTest, BinaryTest) {
    using namespace OpFlow;
    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;

    constexpr auto n = 5;
    auto mesh = MeshBuilder<Mesh>().newMesh(n, 2 * n).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();
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
    u[DS::MDIndex<2>(2, 4)] = 2.;
    Utils::TecplotBinaryStream uf("./u.plt");
    uf << u;
    ASSERT_TRUE(true);
}

TEST(TecIOTest, TwoBinaryTest) {
    using namespace OpFlow;
    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;

    constexpr auto n = 5;
    auto mesh = MeshBuilder<Mesh>().newMesh(n, 2 * n).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();
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
    u[DS::MDIndex<2>(2, 4)] = 2.;
    Utils::TecplotBinaryStream uf("./u.plt"), uf2("./u2.plt");
    uf << u;
    u[DS::MDIndex<2>(2, 4)] = 0.;
    uf2 << u;
    ASSERT_TRUE(true);
}