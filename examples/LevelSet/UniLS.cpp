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
using namespace OpFlow;

template <std::size_t d>
using DU = DecableOp<D1WENO53Upwind<d>, D1FirstOrderBiasedUpwind<d>>;
template <std::size_t d>
using DD = DecableOp<D1WENO53Downwind<d>, D1FirstOrderBiasedDownwind<d>>;

void ls() {
    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;

    auto n = 10;
    auto m = MeshBuilder<Mesh>().newMesh(n, n).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();
    auto u = ExprBuilder<Field>()
                     .setName("u")
                     .setMesh(m)
                     .setBC(0, DimPos::start, BCType::Dirc, 0.)
                     .setBC(0, DimPos::end, BCType::Dirc, 0.)
                     .setBC(1, DimPos::start, BCType::Dirc, 0.)
                     .setBC(1, DimPos::end, BCType::Dirc, 0.)
                     .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                     .build();
    auto v = u;
    v.name = "v";
    auto p = ExprBuilder<Field>()
                     .setName("p")
                     .setMesh(m)
                     .setBC(0, DimPos::start, BCType::Neum, 0.)
                     .setBC(0, DimPos::end, BCType::Neum, 0.)
                     .setBC(1, DimPos::start, BCType::Neum, 0.)
                     .setBC(1, DimPos::end, BCType::Neum, 0.)
                     .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                     .build();

    p.initBy([](auto&& x) { return std::sqrt(Math::pow2(x[0] - 0.5) + Math::pow2(x[1] - 0.75)) - 0.15; });
    auto p1 = p, p2 = p, p3 = p;

    u.initBy([](auto&& x) {
        return 2 * std::sin(PI * x[1]) * std::cos(PI * x[1]) * Math::pow2(std::sin(PI * x[0]));
    });
    v.initBy([](auto&& x) {
        return -2 * std::sin(PI * x[0]) * std::cos(PI * x[0]) * Math::pow2(std::sin(PI * x[1]));
    });

    Utils::TecplotASCIIStream uf("u.tec"), vf("v.tec"), pf("p.tec");
    uf << Utils::TimeStamp(0) << u;
    vf << Utils::TimeStamp(0) << v;

    auto dt = 1. / n;
    for (auto i = 0; i < 2. / dt; ++i) {
        p1 = p
             - dt
                       * (u * conditional(u > 0., dx<DD>(p), dx<DU>(p))
                          + v * conditional(v > 0., dy<DD>(p), dy<DU>(p)));
        p2 = (p1
              - dt
                        * (u * conditional(u > 0., dx<DD>(p1), dx<DU>(p1))
                           + v * conditional(v > 0., dy<DD>(p), dy<DU>(p))))
                     / 4.
             + 0.75 * p;
        p3 = (p2
              - dt
                        * (u * conditional(u > 0., dx<DD>(p2), dx<DU>(p2))
                           + v * conditional(v > 0., dy<DD>(p2), dy<DU>(p2))))
                     * 2. / 3.
             + p / 3.;
        p = p3;
        pf << Utils::TimeStamp(i) << p;
    }
}
