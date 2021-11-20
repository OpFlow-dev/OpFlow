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
using namespace testing;

class DecayExprTest : public Test {
protected:
    void SetUp() override {
        m = MeshBuilder<Mesh>().newMesh(10, 10).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();
        u = ExprBuilder<Field>()
                    .setMesh(m)
                    .setBC(0, DimPos::start, BCType::Dirc, 0.)
                    .setBC(0, DimPos::end, BCType::Dirc, 0.)
                    .setBC(1, DimPos::start, BCType::Neum, 0.)
                    .setBC(1, DimPos::end, BCType::Neum, 0.)
                    .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                    .build();
        v = ExprBuilder<Field>()
                    .setMesh(m)
                    .setBC(0, DimPos::start, BCType::Neum, 0.)
                    .setBC(0, DimPos::end, BCType::Neum, 0.)
                    .setBC(1, DimPos::start, BCType::Dirc, 0.)
                    .setBC(1, DimPos::end, BCType::Dirc, 0.)
                    .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                    .build();
        u = 1;
        v = 2;
    }

    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;
    Mesh m;
    Field u, v;
};

TEST_F(DecayExprTest, RangeCheck) {
    auto kernel = DS::FixedSizeTensor<double, 3, 3> {};
    auto t = decay(conv(u, kernel), u);
    t.prepare();
    ASSERT_TRUE(t.accessibleRange == u.accessibleRange);
}

TEST_F(DecayExprTest, ValueCheck) {
    auto kernel = DS::FixedSizeTensor<double, 3, 3> {0.};
    kernel[DS::MDIndex<2> {1, 1}] = 1.;
    auto t = decay(conv(v, kernel), u);
    t.prepare();
    auto idx = DS::MDIndex<2>(0, 0);
    auto val = t.evalAt(idx);
    ASSERT_DOUBLE_EQ(t.evalAt(DS::MDIndex<2>(0, 0)), u[DS::MDIndex<2>(0, 0)]);
    ASSERT_DOUBLE_EQ(t.evalAt(DS::MDIndex<2>(1, 1)), v[DS::MDIndex<2>(1, 1)]);
}

#ifndef NDEBUG
using DecayDeathTest = DecayExprTest;

TEST_F(DecayDeathTest, FailWithDifferentMesh) {
    auto mm = MeshBuilder<Mesh>().newMesh(11, 11).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();

    auto vv = ExprBuilder<Field>()
                      .setMesh(mm)
                      .setBC(0, DimPos::start, BCType::Neum, 0.)
                      .setBC(0, DimPos::end, BCType::Neum, 0.)
                      .setBC(1, DimPos::start, BCType::Dirc, 0.)
                      .setBC(1, DimPos::end, BCType::Dirc, 0.)
                      .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                      .build();
    auto t = decay(u, vv);
    ASSERT_DEATH(t.prepare(), "");
}

TEST_F(DecayDeathTest, FailWithDifferentLoc) {
    auto mm = MeshBuilder<Mesh>().newMesh(10, 10).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();

    auto vv = ExprBuilder<Field>()
                      .setMesh(mm)
                      .setBC(0, DimPos::start, BCType::Neum, 0.)
                      .setBC(0, DimPos::end, BCType::Neum, 0.)
                      .setBC(1, DimPos::start, BCType::Dirc, 0.)
                      .setBC(1, DimPos::end, BCType::Dirc, 0.)
                      .setLoc({LocOnMesh::Corner, LocOnMesh::Corner})
                      .build();
    auto t = decay(u, vv);
    ASSERT_DEATH(t.prepare(), "");
}
#endif