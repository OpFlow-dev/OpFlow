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
using DU = D1WENO53Upwind<d>;
template <std::size_t d>
using DD = D1WENO53Downwind<d>;

void ls() {
    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;

    constexpr auto n = 65;
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
    constexpr auto h = 1. / (n - 1);
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

    auto dt = 1. / (n - 1);
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
            auto h1 = conditional(p0 > 0, _1(p0, p), _2(p0, p));
            p1 = p - dt * h1;
            auto h2 = conditional(p0 > 0, _1(p0, p1), _2(p0, p1));
            p2 = p1 - dt / 4. * (-3 * h1 + h2);
            auto h3 = conditional(p0 > 0, _1(p0, p2), _2(p0, p2));
            p3 = p2 - dt / 12. * (-h1 - h2 + 8 * h3);
            constexpr auto _c = 16. / 24., _o = 1. / 24.;
            constexpr DS::FixedSizeTensor<double, 3, 3> conv_ker {_o, _o, _o, _o, _c, _o, _o, _o, _o};
            constexpr auto func = [=](Real d) { return Math::smoothDelta(h, d); };
            constexpr auto functor = Utils::NamedFunctor<func, Utils::makeCXprString("smoothDelta")>();
            constexpr auto delta_op
                    = [=](auto&& e) { return makeExpression<UniOpAdaptor<functor>>(OP_PERFECT_FOWD(e)); };
            constexpr auto int_op = [=](auto&& e) { return h * h * conv(OP_PERFECT_FOWD(e), conv_ker); };

            auto lambda = -int_op(delta_op(p0) * (p3 - p0) / (_ + 1)) / int_op(pow(delta_op(p0), 2) + 1e-14);
            p = p3 + lambda * (_ + 1) * delta_op(p0);
        }
        pf << Utils::TimeStamp(i) << p;
        OP_INFO("Current step: {}", i);
    }
}

void ls_3d() {
    using Mesh = CartesianMesh<Meta::int_<3>>;
    using Field = CartesianField<Real, Mesh>;

    constexpr int n = 129;
    constexpr auto h = 1. / (n - 1);
    auto m = MeshBuilder<Mesh>()
                     .newMesh(n, n, n)
                     .setMeshOfDim(0, 0., 1.)
                     .setMeshOfDim(1, 0., 1.)
                     .setMeshOfDim(2, 0., 1.)
                     .build();
    auto u = ExprBuilder<Field>()
                     .setMesh(m)
                     .setName("u")
                     .setLoc({LocOnMesh::Center, LocOnMesh ::Center, LocOnMesh ::Center})
                     .setBC(0, DimPos::start, BCType::Dirc, 0.)
                     .setBC(0, DimPos::end, BCType::Dirc, 0.)
                     .setBC(1, DimPos::start, BCType::Dirc, 0.)
                     .setBC(1, DimPos::end, BCType::Dirc, 0.)
                     .setBC(2, DimPos::start, BCType::Dirc, 0.)
                     .setBC(2, DimPos::end, BCType::Dirc, 0.)
                     .build();
    auto v = u;
    v.name = "v";
    auto w = u;
    w.name = "w";
    auto p = ExprBuilder<Field>()
                     .setName("p")
                     .setMesh(m)
                     .setBC(0, DimPos::start, BCType::Neum, 0.)
                     .setBC(0, DimPos::end, BCType::Neum, 0.)
                     .setBC(1, DimPos::start, BCType::Neum, 0.)
                     .setBC(1, DimPos::end, BCType::Neum, 0.)
                     .setBC(2, DimPos::start, BCType::Neum, 0.)
                     .setBC(2, DimPos::end, BCType::Neum, 0.)
                     .setLoc({LocOnMesh::Center, LocOnMesh::Center, LocOnMesh ::Center})
                     .build();
    p.initBy([](auto&& x) {
        return std::sqrt(Math::pow2(x[0] - 0.35) + Math::pow2(x[1] - 0.35) + Math::pow2(x[2] - 0.35)) - 0.15;
    });
    auto p1 = p, p2 = p, p3 = p, p0 = p;

    u.initBy([](auto&& x) {
        return 2 * Math::pow2(std::sin(PI * x[0])) * std::sin(2 * PI * x[1]) * std::sin(2 * PI * x[2]);
    });
    v.initBy([](auto&& x) {
        return -std::sin(2 * PI * x[0]) * Math::pow2(std::sin(PI * x[1])) * std::sin(2 * PI * x[2]);
    });
    w.initBy([](auto&& x) {
        return -std::sin(2 * PI * x[0]) * std::sin(2 * PI * x[1]) * Math::pow2(std::sin(PI * x[2]));
    });

    auto root = std::format("Result_{:%m-%d_%H-%M-%S}/",
                            std::chrono::current_zone()->to_local(std::chrono::system_clock::now()));
    Utils::TecplotASCIIStream uf("u.tec"), vf("v.tec"), wf("w.tec"), pf("p.tec");
    uf << Utils::TimeStamp(0) << u;
    vf << Utils::TimeStamp(0) << v;
    wf << Utils::TimeStamp(0) << w;
    pf << Utils::TimeStamp(0) << p;

    auto dt = 1. / (n - 1) / 3.;
    auto _eps = 1e-6;
    auto _1 = [&](auto&& _p, auto&& _pp) {
        return _p / OpFlow::sqrt(_p * _p + _eps)
               * (OpFlow::sqrt(
                          OpFlow::pow(
                                  OpFlow::max(-OpFlow::min(0., dx<DU>(_pp)), OpFlow::max(dx<DD>(_pp), 0.)), 2)
                          + OpFlow::pow(
                                  OpFlow::max(-OpFlow::min(0., dy<DU>(_pp)), OpFlow::max(dy<DD>(_pp), 0.)), 2)
                          + OpFlow::pow(
                                  OpFlow::max(-OpFlow::min(0., dz<DU>(_pp)), OpFlow::max(dz<DD>(_pp), 0.)),
                                  2))
                  - 1.);
    };
    auto _2 = [&](auto&& _p, auto&& _pp) {
        return _p / OpFlow::sqrt(_p * _p + _eps)
               * (OpFlow::sqrt(
                          OpFlow::pow(
                                  OpFlow::max(OpFlow::max(0., dx<DU>(_pp)), -OpFlow::min(dx<DD>(_pp), 0.)), 2)
                          + OpFlow::pow(
                                  OpFlow::max(OpFlow::max(0., dy<DU>(_pp)), -OpFlow::min(dy<DD>(_pp), 0.)), 2)
                          + OpFlow::pow(
                                  OpFlow::max(OpFlow::max(0., dz<DU>(_pp)), -OpFlow::min(dz<DD>(_pp), 0.)),
                                  2))
                  - 1.);
    };

    for (auto i = 1; i < 4. / dt; ++i) {
        p1 = p
             - dt
                       * (u * conditional(u > 0., dx<DD>(p), dx<DU>(p))
                          + v * conditional(v > 0., dy<DD>(p), dy<DU>(p))
                          + w * conditional(w > 0., dz<DD>(p), dz<DU>(p)));
        p2 = (p1
              - dt
                        * (u * conditional(u > 0., dx<DD>(p1), dx<DU>(p1))
                           + v * conditional(v > 0., dy<DD>(p1), dy<DU>(p1))
                           + w * conditional(w > 0., dz<DD>(p1), dz<DU>(p1))))
                     / 4.
             + 0.75 * p;
        p3 = (p2
              - dt
                        * (u * conditional(u > 0., dx<DD>(p2), dx<DU>(p2))
                           + v * conditional(v > 0., dy<DD>(p2), dy<DU>(p2))
                           + w * conditional(w > 0., dz<DD>(p2), dz<DU>(p2))))
                     * 2. / 3.
             + p / 3.;
        p = p3;
        p0 = p;
        // reinit
        for (auto _ = 0; _ < 2; ++_) {
            constexpr auto h_min = h;
            auto h1 = conditional(p0 > 0, _1(p0, p), _2(p0, p));
            p1 = p - dt * h1;
            auto h2 = conditional(p0 > 0, _1(p0, p1), _2(p0, p1));
            p2 = p1 - dt / 4. * (-3 * h1 + h2);
            auto h3 = conditional(p0 > 0, _1(p0, p2), _2(p0, p2));
            p3 = p2 - dt / 12. * (-h1 - h2 + 8 * h3);
            constexpr auto _c = 64. / 90., _o = 1. / 90.;
            constexpr DS::FixedSizeTensor<double, 3, 3, 3> conv_ker {_o, _o, _o, _o, _o, _o, _o, _o, _o,
                                                                     _o, _o, _o, _o, _c, _o, _o, _o, _o,
                                                                     _o, _o, _o, _o, _o, _o, _o, _o, _o};
            constexpr auto func = [=](Real d) { return Math::smoothDelta(1.5 * h_min, d); };
            constexpr auto functor = Utils::NamedFunctor<func, Utils::makeCXprString("smoothDelta")>();
            constexpr auto delta_op
                    = [=](auto&& e) { return makeExpression<UniOpAdaptor<functor>>(OP_PERFECT_FOWD(e)); };
            constexpr auto int_op = [=](auto&& e) { return h * h * conv(OP_PERFECT_FOWD(e), conv_ker); };

            auto lambda = -int_op(delta_op(p0) * (p3 - p0) / (_ + 1)) / int_op(pow(delta_op(p0), 2) + 1e-14);
            p = p3 + lambda * (_ + 1) * delta_op(p0);
        }
        if (i % 3 == 0) pf << Utils::TimeStamp(i) << p;

        OP_INFO("Current step: {}", i);
    }
}
