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

using namespace OpFlow;
using namespace testing;

class ConditionalTest : public Test {
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

TEST_F(ConditionalTest, TwoField) {
    auto t = conditional(u > 0, u, v);
    t.prepare();
    ASSERT_EQ(t.accessibleRange, DS::commonRange(u.accessibleRange, v.accessibleRange));
}

TEST_F(ConditionalTest, TwoField_Value) {
    auto t = conditional(u > 0, u, v);
    t.prepare();
    ASSERT_EQ(t.evalAt(DS::MDIndex<2> {0, 0}), (1));
}

TEST_F(ConditionalTest, ScalarCond) {
    auto t = conditional(true, u, v);
    t.prepare();
    ASSERT_EQ(t.accessibleRange, DS::commonRange(u.accessibleRange, v.accessibleRange));
}

TEST_F(ConditionalTest, ScalarCond_Value) {
    auto t = conditional(false, u, v);
    t.prepare();
    ASSERT_EQ(t.evalAt(DS::MDIndex<2> {0, 0}), (2));
}

TEST_F(ConditionalTest, ScalarField) {
    auto t = conditional(true, u, 0);
    t.prepare();
    ASSERT_EQ(t.accessibleRange, u.accessibleRange);
}

TEST_F(ConditionalTest, ScalarField_Value) {
    auto t = conditional(false, u, 100);
    t.prepare();
    ASSERT_EQ(t.evalAt(DS::MDIndex<2> {0, 0}), (100));
}