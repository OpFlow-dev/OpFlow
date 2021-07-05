// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 - 2021 by the OpFlow developers
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
#include <fmt/include/fmt/format.h>
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
    } else {
        ASSERT_FLOAT_EQ(mesh.x(0, 9), 9. / 10);
        ASSERT_FLOAT_EQ(mesh.x(1, 9), 9. / 20);
        ASSERT_FLOAT_EQ(mesh.x(2, 9), 9. / 30);
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
    } else {
        ASSERT_FLOAT_EQ(mesh.x(0, 9), 9. / 10);
        ASSERT_FLOAT_EQ(mesh.x(1, 9), 9. / 20);
        ASSERT_FLOAT_EQ(mesh.x(2, 9), 9. / 30);
    }
}

// tests for derived meshes
/*
TEST(CartesianMeshTest, CenterDerived) {
    auto m_derived = m_3d_dynamic->getDerivedMesh<centered>();
    ASSERT_TRUE(m_derived->isDerived());
    ASSERT_EQ(m_derived->baseMesh, m_3d_dynamic);
}

TEST(CartesianMeshTest, xFaceCenterDerived) {
    auto m_derived = m_3d_dynamic->getDerivedMesh<xFaceCentered>();
    ASSERT_TRUE(m_derived->isDerived());
    ASSERT_EQ(m_derived->baseMesh, m_3d_dynamic);
}

TEST(CartesianMeshTest, yFaceCenterDerived) {
    auto m_derived = m_3d_dynamic->getDerivedMesh<yFaceCentered>();
    ASSERT_TRUE(m_derived->isDerived());
    ASSERT_EQ(m_derived->baseMesh, m_3d_dynamic);
}

TEST(CartesianMeshTest, zFaceCenterDerived) {
    auto m_derived = m_3d_dynamic->getDerivedMesh<zFaceCentered>();
    ASSERT_TRUE(m_derived->isDerived());
    ASSERT_EQ(m_derived->baseMesh, m_3d_dynamic);
}

MATCHER(TupleOfVecMatcher, "") {
    auto& vec1 = std::get<0>(arg);
    auto& vec2 = std::get<1>(arg);
    auto n = vec1.size();
    auto ret = true;
    //return ret;
    for (auto i = 0; i < n; ++i) {
        auto th = std::numeric_limits<OpFlow::Real>::epsilon() * std::abs(vec1[i]) * 100;
        if (th == 0) th = std::numeric_limits<OpFlow::Real>::epsilon();
        ret &= std::abs(vec1[i] - vec2[i]) < th;
        if (!ret) std::cout << fmt::format("{:20.15e} not within {}\n", vec1[i] - vec2[i], th);
    }
    return ret;
}

auto RealEq(OpFlow::Real x) {
    if constexpr (std::is_same_v<OpFlow::Real, float>) {
        return FloatEq(x);
    } else {
        return DoubleEq(x);
    }
}

template <typename M>
auto MeshMatcher(const M& mref) {
    return AllOf(Field(&M::dims, ContainerEq(mref.dims)),
                 Field(&M::x, Pointwise(TupleOfVecMatcher(), mref.x)),
                 Field(&M::dx, Pointwise(TupleOfVecMatcher(), mref.dx)),
                 Field(&M::dimTypes, ContainerEq(mref.dimTypes)), Field(&M::Dx, RealEq(mref.Dx)),
                 Field(&M::Dy, RealEq(mref.Dy)), Field(&M::Dz, RealEq(mref.Dz)),
                 Field(&M::xMin, RealEq(mref.xMin)), Field(&M::xMax, RealEq(mref.xMax)),
                 Field(&M::yMin, RealEq(mref.yMin)), Field(&M::yMax, RealEq(mref.yMax)),
                 Field(&M::zMin, RealEq(mref.zMin)), Field(&M::zMax, RealEq(mref.zMax)));
}

TEST(CartesianMeshTest, GetCenteredDynamicMesh) {
    m_3d_dynamic->setMeshOfDim(0, 0., 1.)->setMeshOfDim(1, 0., 1.)->setMeshOfDim(2, 0., 1.);
    auto m = m_3d_dynamic->getDerivedMesh<centered>();
    CartesianMesh<int_<3>> mref(9, 19, 29);
    mref.setMeshOfDim(0, 1. / 18., 17. / 18.)
            ->setMeshOfDim(1, 1. / 38., 37. / 38.)
            ->setMeshOfDim(2, 1. / 58., 57. / 58.);
    ASSERT_THAT(*m, MeshMatcher(mref));
}

TEST(CartesianMeshTest, GetxFaceCenteredDynamicMesh) {
    m_3d_dynamic->setMeshOfDim(0, 0., 1.)->setMeshOfDim(1, 0., 1.)->setMeshOfDim(2, 0., 1.);
    auto m = m_3d_dynamic->getDerivedMesh<xFaceCentered>();
    CartesianMesh<int_<3>> mref(10, 19, 29);
    mref.setMeshOfDim(0, 0., 1.)->setMeshOfDim(1, 1. / 38., 37. / 38.)->setMeshOfDim(2, 1. / 58., 57. / 58.);
    ASSERT_THAT(*m, MeshMatcher(mref));
}

TEST(CartesianMeshTest, GetyFaceCenteredDynamicMesh) {
    m_3d_dynamic->setMeshOfDim(0, 0., 1.)->setMeshOfDim(1, 0., 1.)->setMeshOfDim(2, 0., 1.);
    auto m = m_3d_dynamic->getDerivedMesh<yFaceCentered>();
    CartesianMesh<int_<3>> mref(9, 20, 29);
    mref.setMeshOfDim(0, 1. / 18., 17. / 18.)->setMeshOfDim(1, 0., 1.)->setMeshOfDim(2, 1. / 58., 57. / 58.);
    ASSERT_THAT(*m, MeshMatcher(mref));
}

TEST(CartesianMeshTest, GetzFaceCenteredDynamicMesh) {
    m_3d_dynamic->setMeshOfDim(0, 0., 1.)->setMeshOfDim(1, 0., 1.)->setMeshOfDim(2, 0., 1.);
    auto m = m_3d_dynamic->getDerivedMesh<zFaceCentered>();
    CartesianMesh<int_<3>> mref(9, 19, 30);
    mref.setMeshOfDim(0, 1. / 18., 17. / 18.)->setMeshOfDim(1, 1. / 38., 37. / 38.)->setMeshOfDim(2, 0., 1.);
    ASSERT_THAT(*m, MeshMatcher(mref));
}

TEST(CartesianMeshTest, GetCenteredMeshWithNonUniformMesh) {
    auto m_base = CartesianMesh<OpFlow::Meta::int_<1>>(10);
    m_base.setMeshOfDim(0, [](auto i) { return i * i * 1.0; });
    auto m_derived = m_base.getDerivedMesh<centered>();
    auto m_ref = CartesianMesh<OpFlow::Meta::int_<1>>(9);
    for (auto i = -0; i < 9 + 0; ++i) {
        // the padding distance of derived mesh should be determined by the base mesh
        m_ref.x[0][i] = (m_base.x[0][i] + m_base.x[0][i + 1]) / 2;
    }
    // Min & Max don't include padding
    m_ref.xMin = m_ref.x[0][0];
    m_ref.xMax = m_ref.x[0][8];
    m_ref.dimTypes[0] = NonUniform;
    for (auto i = -0; i < 9 + 0 - 1; ++i) { m_ref.dx[0][i] = m_ref.x[0][i + 1] - m_ref.x[0][i]; }
    ASSERT_THAT(*m_derived, MeshMatcher(m_ref));
}

TEST(CartesianMeshTest, GetxFaceCenteredMeshWithNonUniformMesh) {
    auto m_base = CartesianMesh<OpFlow::Meta::int_<1>>(10);
    m_base.setMeshOfDim(0, [](auto i) { return i * i * 1.0; });
    auto m_derived = m_base.getDerivedMesh<xFaceCentered>();
    auto m_ref = m_base;// In this case, the derived mesh is the same as the base mesh
    ASSERT_THAT(*m_derived, MeshMatcher(m_ref));
}

TEST(CartesianMeshTest, GetyFaceCenteredMeshWithNonUniformMesh) {
    auto m_base = CartesianMesh<OpFlow::Meta::int_<2>>(10, 20);
    m_base.setMeshOfDim(0, 0., 1.)->setMeshOfDim(1, [](auto i) { return i * i * 1.0; });
    auto m_derived = m_base.getDerivedMesh<yFaceCentered>();
    auto m_ref = CartesianMesh<OpFlow::Meta::int_<2>>(9, 20);
    m_ref.setMeshOfDim(0, 1. / 18, 17. / 18);
    for (auto i = -0; i < 20 + 0; ++i) { m_ref.x[1][i] = m_base.x[1][i]; }
    m_ref.yMin = m_ref.x[1][0];
    m_ref.yMax = m_ref.x[1][19];
    m_ref.dimTypes[1] = NonUniform;
    for (auto i = -0; i < 20 + 0 - 1; ++i) { m_ref.dx[1][i] = m_base.dx[1][i]; }
    ASSERT_THAT(*m_derived, MeshMatcher(m_ref));
}

TEST(CartesianMeshTest, GetzFaceCenteredMeshWithNonUniformMesh) {
    auto m_base = CartesianMesh<OpFlow::Meta::int_<3>>(10, 20, 30);
    auto f = [](auto i) { return i * i * 1.0; };
    m_base.setMeshOfDim(0, f)->setMeshOfDim(1, f)->setMeshOfDim(2, f);
    auto m_derived = m_base.getDerivedMesh<zFaceCentered>();
    auto m_ref = CartesianMesh<OpFlow::Meta::int_<3>>(9, 19, 30);
    for (auto d = 0; d < 2; ++d) {
        for (auto i = -0; i < m_ref.dims[d] + 0; ++i) {
            m_ref.x[d][i] = (m_base.x[d][i] + m_base.x[d][i + 1]) / 2;
        }
        if (d == 0) {
            m_ref.xMin = m_ref.x[d][0];
            m_ref.xMax = m_ref.x[d][m_ref.dims[d] - 1];
        } else {
            m_ref.yMin = m_ref.x[d][0];
            m_ref.yMax = m_ref.x[d][m_ref.dims[d] - 1];
        }
        m_ref.dimTypes[d] = NonUniform;
        for (auto i = -0; i < m_ref.dims[d] + 0 - 1; ++i) {
            m_ref.dx[d][i] = m_ref.x[d][i + 1] - m_ref.x[d][i];
        }
    }
    for (auto i = -0; i < m_ref.dims[2] + 0; ++i) { m_ref.x[2][i] = m_base.x[2][i]; }
    m_ref.zMin = m_ref.x[2][0];
    m_ref.zMax = m_ref.x[2][m_ref.dims[2] - 1];
    m_ref.dimTypes[2] = NonUniform;
    for (auto i = -0; i < m_ref.dims[2] + 0 - 1; ++i) { m_ref.dx[2][i] = m_base.dx[2][i]; }
    ASSERT_THAT(*m_derived, MeshMatcher(m_ref));
}

TEST(CartesianMeshTest, GetCenteredWithBDMesh) {
    m_3d_dynamic->setMeshOfDim(0, 0., 1.)->setMeshOfDim(1, 0., 1.)->setMeshOfDim(2, 0., 1.);
    auto m = m_3d_dynamic->getDerivedMesh<centeredWithBD>();
    CartesianMesh<int_<3>> mref(11, 21, 31);
    mref.setMeshOfDim(0,
                      [&](auto i) {
                          if (i == 0) return 0._r;
                          if (i == 10) return 1._r;
                          return (m_3d_dynamic->x[0][i - 1] + m_3d_dynamic->x[0][i]) / 2._r;
                      })
            ->setMeshOfDim(1,
                           [&](auto i) {
                               if (i == 0) return 0._r;
                               if (i == 20) return 1._r;
                               return (m_3d_dynamic->x[1][i - 1] + m_3d_dynamic->x[1][i]) / 2._r;
                           })
            ->setMeshOfDim(2, [&](auto i) {
                if (i == 0) return 0._r;
                if (i == 30) return 1._r;
                return (m_3d_dynamic->x[2][i - 1] + m_3d_dynamic->x[2][i]) / 2._r;
            });
    ASSERT_THAT(*m, MeshMatcher(mref));
}

TEST(CartesianMeshTest, GetxFaceCenteredWithBDDynamicMesh) {
    m_3d_dynamic->setMeshOfDim(0, 0., 1.)->setMeshOfDim(1, 0., 1.)->setMeshOfDim(2, 0., 1.);
    auto m = m_3d_dynamic->getDerivedMesh<xFaceCenteredWithBD>();
    CartesianMesh<int_<3>> mref(10, 21, 31);
    mref.setMeshOfDim(0, 0., 1.)
            ->setMeshOfDim(1, [](int k) { return (k == 0 ? 0. : (k == 20 ? 1. : (2. * k - 1.) / 38.)); })
            ->setMeshOfDim(2, [](int k) { return (k == 0 ? 0. : (k == 30 ? 1. : (2. * k - 1.) / 58.)); });
    ASSERT_THAT(*m, MeshMatcher(mref));
}

TEST(CartesianMeshTest, GetyFaceCenteredWithBDDynamicMesh) {
    m_3d_dynamic->setMeshOfDim(0, 0., 1.)->setMeshOfDim(1, 0., 1.)->setMeshOfDim(2, 0., 1.);
    auto m = m_3d_dynamic->getDerivedMesh<yFaceCenteredWithBD>();
    CartesianMesh<int_<3>> mref(11, 20, 31);
    mref.setMeshOfDim(0, [](int k) { return (k == 0 ? 0. : (k == 10 ? 1. : (2. * k - 1.) / 18.)); })
            ->setMeshOfDim(1, 0., 1.)
            ->setMeshOfDim(2, [](int k) { return (k == 0 ? 0. : (k == 30 ? 1. : (2. * k - 1.) / 58.)); });
    ASSERT_THAT(*m, MeshMatcher(mref));
}

TEST(CartesianMeshTest, GetzFaceCenteredWithBDDynamicMesh) {
    m_3d_dynamic->setMeshOfDim(0, 0., 1.)->setMeshOfDim(1, 0., 1.)->setMeshOfDim(2, 0., 1.);
    auto m = m_3d_dynamic->getDerivedMesh<zFaceCenteredWithBD>();
    CartesianMesh<int_<3>> mref(11, 21, 30);
    mref.setMeshOfDim(0, [](int k) {
            return (k == 0 ? 0. : (k == 10 ? 1. : (2. * k - 1.) / 18.));
        })->setMeshOfDim(1, [](int k) {
              return (k == 0 ? 0. : (k == 20 ? 1. : (2. * k - 1.) / 38.));
          })->setMeshOfDim(2, 0., 1.);
    ASSERT_THAT(*m, MeshMatcher(mref));
}*/

// todo: Add tests for getCenteredPeriodicMesh