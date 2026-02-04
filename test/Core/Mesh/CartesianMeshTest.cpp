// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2026 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#include <gmock/gmock.h>
#include <iostream>

#include <OpFlow>

using namespace OpFlow;
using namespace OpFlow::Meta;

using Mesh1 = CartesianMesh<int_<1>>;
using Mesh3 = CartesianMesh<int_<3>>;

TEST(CartesianMeshTest, SetMeshPointsByFunctor) {
    auto mesh = MeshBuilder<Mesh3>()
                        .newMesh(10, 20, 30)
                        .setMeshOfDim(0, [](int i) { return i / 10.; })
                        .setMeshOfDim(1, [](int j) { return j / 20.; })
                        .setMeshOfDim(2, [](int k) { return k / 30.; })
                        .build();
    if constexpr (std::is_same_v<OpFlow::Real, double>) {
        ASSERT_DOUBLE_EQ(mesh.x(0, 9), 9. / 10);
        ASSERT_DOUBLE_EQ(mesh.x(1, 9), 9. / 20);
        ASSERT_DOUBLE_EQ(mesh.x(2, 9), 9. / 30);
        // default padding width 5
        ASSERT_DOUBLE_EQ(mesh.x(0, -5), -5. / 10);
        ASSERT_DOUBLE_EQ(mesh.x(0, 14), 14. / 10);
        ASSERT_DOUBLE_EQ(mesh.x(1, -5), -5. / 20);
        ASSERT_DOUBLE_EQ(mesh.x(1, 24), 24. / 20);
        ASSERT_DOUBLE_EQ(mesh.x(2, -5), -5. / 30);
        ASSERT_DOUBLE_EQ(mesh.x(2, 34), 34. / 30);
        // default symm padding
        for (int i = -5; i < 14; ++i) {
            ASSERT_NEAR(mesh.dx(0, i), 1. / 10, 1E-10);
            ASSERT_NEAR(mesh.idx(0, i), 10., 1E-10);
        }
    } else {
        ASSERT_FLOAT_EQ(mesh.x(0, 9), 9. / 10);
        ASSERT_FLOAT_EQ(mesh.x(1, 9), 9. / 20);
        ASSERT_FLOAT_EQ(mesh.x(2, 9), 9. / 30);
        // default padding width 5
        ASSERT_FLOAT_EQ(mesh.x(0, -5), -5. / 10);
        ASSERT_FLOAT_EQ(mesh.x(0, 14), 14. / 10);
        ASSERT_FLOAT_EQ(mesh.x(1, -5), -5. / 20);
        ASSERT_FLOAT_EQ(mesh.x(1, 24), 24. / 20);
        ASSERT_FLOAT_EQ(mesh.x(2, -5), -5. / 30);
        ASSERT_FLOAT_EQ(mesh.x(2, 34), 34. / 30);
        // default symm padding
        for (int i = -5; i < 14; ++i) {
            ASSERT_FLOAT_EQ(mesh.dx(0, i), 1. / 10);
            ASSERT_FLOAT_EQ(mesh.idx(0, i), 10.);
        }
    }
}

TEST(CartesianMeshTest, SetMeshPointsByNumber) {
    auto mesh = MeshBuilder<Mesh3>()
                        .newMesh(11, 21, 31)
                        .setMeshOfDim(0, 0., 1.)
                        .setMeshOfDim(1, 0., 1.)
                        .setMeshOfDim(2, 0., 1.)
                        .build();
    if constexpr (std::is_same_v<OpFlow::Real, double>) {
        ASSERT_DOUBLE_EQ(mesh.x(0, 9), 9. / 10);
        ASSERT_DOUBLE_EQ(mesh.x(1, 9), 9. / 20);
        ASSERT_DOUBLE_EQ(mesh.x(2, 9), 9. / 30);
        // default padding width 5
        ASSERT_DOUBLE_EQ(mesh.x(0, -5), -5. / 10);
        ASSERT_DOUBLE_EQ(mesh.x(0, 14), 14. / 10);
        ASSERT_DOUBLE_EQ(mesh.x(1, -5), -5. / 20);
        ASSERT_DOUBLE_EQ(mesh.x(1, 24), 24. / 20);
        ASSERT_DOUBLE_EQ(mesh.x(2, -5), -5. / 30);
        ASSERT_DOUBLE_EQ(mesh.x(2, 34), 34. / 30);
        // default symm padding
        for (int i = -5; i < 14; ++i) {
            ASSERT_NEAR(mesh.dx(0, i), 1. / 10, 1e-10);
            ASSERT_NEAR(mesh.idx(0, i), 10., 1e-10);
        }
    } else {
        ASSERT_FLOAT_EQ(mesh.x(0, 9), 9. / 10);
        ASSERT_FLOAT_EQ(mesh.x(1, 9), 9. / 20);
        ASSERT_FLOAT_EQ(mesh.x(2, 9), 9. / 30);
        // default padding width 5
        ASSERT_FLOAT_EQ(mesh.x(0, -5), -5. / 10);
        ASSERT_FLOAT_EQ(mesh.x(0, 14), 14. / 10);
        ASSERT_FLOAT_EQ(mesh.x(1, -5), -5. / 20);
        ASSERT_FLOAT_EQ(mesh.x(1, 24), 24. / 20);
        ASSERT_FLOAT_EQ(mesh.x(2, -5), -5. / 30);
        ASSERT_FLOAT_EQ(mesh.x(2, 34), 34. / 30);
        // default symm padding
        for (int i = -5; i < 14; ++i) {
            ASSERT_FLOAT_EQ(mesh.dx(0, i), 1. / 10);
            ASSERT_FLOAT_EQ(mesh.idx(0, i), 10.);
        }
    }
}

TEST(CartesianMeshTest, SymmPadding) {
    auto mesh = MeshBuilder<Mesh1>()
                        .newMesh(5)
                        .setMeshOfDim(0, [](int i) { return i * i; })
                        .setPadWidth(3)
                        .build();
    if constexpr (std::is_same_v<OpFlow::Real, double>) {
        // default symm padding
        ASSERT_DOUBLE_EQ(mesh.x(0, -1), -1);
        ASSERT_DOUBLE_EQ(mesh.x(0, -2), -4);
        ASSERT_DOUBLE_EQ(mesh.x(0, -3), -9);
        ASSERT_DOUBLE_EQ(mesh.x(0, 5), 23);
        ASSERT_DOUBLE_EQ(mesh.x(0, 6), 28);
        ASSERT_DOUBLE_EQ(mesh.x(0, 7), 31);
    } else {
        // default symm padding
        ASSERT_FLOAT_EQ(mesh.x(0, -1), -1);
        ASSERT_FLOAT_EQ(mesh.x(0, -2), -4);
        ASSERT_FLOAT_EQ(mesh.x(0, -3), -9);
        ASSERT_FLOAT_EQ(mesh.x(0, 5), 23);
        ASSERT_FLOAT_EQ(mesh.x(0, 6), 28);
        ASSERT_FLOAT_EQ(mesh.x(0, 7), 31);
    }
}