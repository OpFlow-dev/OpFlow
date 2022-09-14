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

TEST(TecIOMPITest, BinaryTest) {
    using namespace OpFlow;
    using Mesh = CartesianMesh<Meta::int_<3>>;
    using Field = CartesianField<Real, Mesh>;

    constexpr auto n = 5;
    auto mesh = MeshBuilder<Mesh>()
                        .newMesh(n, n, n)
                        .setMeshOfDim(0, 0., 1.)
                        .setMeshOfDim(1, 0., 1.)
                        .setMeshOfDim(2, 0., 1.)
                        .build();
    std::shared_ptr<AbstractSplitStrategy<Field>> strategy = std::make_shared<EvenSplitStrategy<Field>>();
    auto u = ExprBuilder<Field>()
                     .setName("u")
                     .setMesh(mesh)
                     .setBC(0, DimPos::start, BCType::Dirc, 1.)
                     .setBC(0, DimPos::end, BCType::Dirc, 1.)
                     .setBC(1, DimPos::start, BCType::Dirc, 1.)
                     .setBC(1, DimPos::end, BCType::Dirc, 1.)
                     .setBC(2, DimPos::start, BCType::Dirc, 1.)
                     .setBC(2, DimPos::end, BCType::Dirc, 1.)
                     .setExt(1)
                     .setLoc(std::array {LocOnMesh ::Center, LocOnMesh ::Center, LocOnMesh::Center})
                     .setSplitStrategy(strategy)
                     .build();

    u = 0.;
    //if (DS::inRange(u.getLocalWritableRange(), DS::MDIndex<2>(2, 4))) u[DS::MDIndex<2>(2, 4)] = 2.;
    Utils::TecplotBinaryStream uf("./u.szplt");
    uf << u;
    ASSERT_TRUE(true);
}

TEST(TecIOMPITest, TwoBinaryTest) {
    using namespace OpFlow;
    using Mesh = CartesianMesh<Meta::int_<3>>;
    using Field = CartesianField<Real, Mesh>;

    constexpr auto n = 5;
    auto mesh = MeshBuilder<Mesh>()
                        .newMesh(n, n, n)
                        .setMeshOfDim(0, 0., 1.)
                        .setMeshOfDim(1, 0., 1.)
                        .setMeshOfDim(2, 0., 1.)
                        .build();
    std::shared_ptr<AbstractSplitStrategy<Field>> strategy = std::make_shared<EvenSplitStrategy<Field>>();
    auto u = ExprBuilder<Field>()
                     .setName("u")
                     .setMesh(mesh)
                     .setBC(0, DimPos::start, BCType::Dirc, 1.)
                     .setBC(0, DimPos::end, BCType::Dirc, 1.)
                     .setBC(1, DimPos::start, BCType::Dirc, 1.)
                     .setBC(1, DimPos::end, BCType::Dirc, 1.)
                     .setBC(2, DimPos::start, BCType::Dirc, 1.)
                     .setBC(2, DimPos::end, BCType::Dirc, 1.)
                     .setExt(1)
                     .setLoc(std::array {LocOnMesh ::Center, LocOnMesh ::Center, LocOnMesh::Center})
                     .setSplitStrategy(strategy)
                     .build();

    u = 0.;
    if (DS::inRange(u.getLocalWritableRange(), DS::MDIndex<3> {2, 4, 2})) u[DS::MDIndex<3>(2, 4, 2)] = 2.;
    Utils::TecplotBinaryStream uf("./u1.szplt"), uf2("./u2.szplt");
    uf << u;
    if (DS::inRange(u.getLocalWritableRange(), DS::MDIndex<3>(2, 4, 2))) u[DS::MDIndex<3>(2, 4, 2)] = 0.;
    uf2 << u;
    ASSERT_TRUE(true);
}