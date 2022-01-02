// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2022 by the OpFlow developers
//
// This file is part of OpFlow.
//
// OpFlow is free software and is distributed under the MPL v2.0 license.
// The full text of the license can be found in the file LICENSE at the top
// level directory of OpFlow.
//
// ----------------------------------------------------------------------------

#include "Core/Mesh/Structured/CartesianMesh.hpp"
#include "Core/BasicDataTypes.hpp"
#include "Core/Mesh/Structured/CartesianMeshTrait.hpp"
#include "Core/Mesh/Structured/CartesianMeshView.hpp"
#include "Core/Mesh/Structured/CartesianMeshViewTrait.hpp"
#include <fmt/format.h>
#include <gmock/gmock.h>
#include <iostream>

using namespace OpFlow;
using namespace OpFlow::Meta;

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