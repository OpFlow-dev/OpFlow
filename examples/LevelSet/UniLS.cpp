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

    auto n = 100;
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
    auto p1 = p, p2 = p, p3 = p, p0 = p;

    u.initBy([](auto&& x) {
        return 2 * std::sin(PI * x[1]) * std::cos(PI * x[1]) * Math::pow2(std::sin(PI * x[0]));
    });
    v.initBy([](auto&& x) {
        return -2 * std::sin(PI * x[0]) * std::cos(PI * x[0]) * Math::pow2(std::sin(PI * x[1]));
    });

    Utils::TecplotASCIIStream uf("u.tec"), vf("v.tec"), pf("p.tec");
    uf << Utils::TimeStamp(0) << u;
    vf << Utils::TimeStamp(0) << v;

    int buffWidth = 5;
    auto h = 1. / (n - 1);
    auto _eps = 1e-6;
    auto _1 = [&](auto&& _p, auto&& _pp) {
        return _p / OpFlow::sqrt(_p * _p + _eps)
               * (OpFlow::sqrt(
                          OpFlow::pow(
                                  OpFlow::max(-OpFlow::min(0., dx<DU>(_pp)), OpFlow::max(dx<DD>(_pp), 0.)), 2)
                          + OpFlow::pow(
                                  OpFlow::max(-OpFlow::min(0., dy<DU>(_pp)), OpFlow::max(dy<DD>(_pp), 0.)),
                                  2))
                  - 1.);
    };
    auto _2 = [&](auto&& _p, auto&& _pp) {
        return _p / OpFlow::sqrt(_p * _p + _eps)
               * (OpFlow::sqrt(
                          OpFlow::pow(
                                  OpFlow::max(OpFlow::max(0., dx<DU>(_pp)), -OpFlow::min(dx<DD>(_pp), 0.)), 2)
                          + OpFlow::pow(
                                  OpFlow::max(OpFlow::max(0., dy<DU>(_pp)), -OpFlow::min(dy<DD>(_pp), 0.)),
                                  2))
                  - 1.);
    };

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
        p0 = p;
        // reinit
        for (auto _ = 0; _ < 4; ++_) {
            auto h1 = conditional(p > 0, _1(p, p), _2(p, p));
            p1 = p - dt * h1;
            auto h2 = conditional(p > 0, _1(p, p1), _2(p, p1));
            p2 = p1 - dt / 4. * (-3 * h1 + h2);
            auto h3 = conditional(p > 0, _1(p, p2), _2(p, p2));
            p3 = p2 - dt / 12. * (-h1 - h2 + 8 * h3);
            constexpr auto _c = 16. / 24., _o = 1. / 24.;
            constexpr DS::FixedSizeTensor<double, 2, 3, 3> conv_ker {_o, _o, _o, _o, _c, _o, _o, _o, _o};
            constexpr auto func = [](Real eps) { return Math::smoothDelta(eps, 0); };
            constexpr auto functor = Utils::NamedFunctor<func, Utils::makeCXprString("smoothDelta")>();

            auto lambda = -makeExpression<DecableOp<Convolution<conv_ker>, IdentityOp>>(
                                  makeExpression<UniOpAdaptor<functor>>(p) * (p3 - p0) / (_))
                          / makeExpression<DecableOp<Convolution<conv_ker>, IdentityOp>>(
                                  pow(makeExpression<UniOpAdaptor<functor>>(p0), 2));
            p = p3;
        }
        pf << Utils::TimeStamp(i) << p;
        OP_INFO("Current step: {}", i);
    }
}
