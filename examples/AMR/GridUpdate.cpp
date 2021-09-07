#include "GridUpdate.hpp"
#include "pch.hpp"
using namespace OpFlow;

void grid_update_2d() {
    spdlog::set_level(spdlog::level::debug);

    using Mesh = CartesianAMRMesh<Meta::int_<2>>;
    using Field = CartAMRField<double, Mesh>;

    int n = 9, maxlevel = 4, ratio = 2;
    double h = 1. / (n - 1);
    auto f = [&](auto cx, auto cy) {
        return [=](auto&& i) {
            auto l = i.l;
            double radius = .25;
            double ht = h / Math::int_pow(ratio, l);
            double eps = h / Math::int_pow(ratio, maxlevel);
            double x[4], y[4];
            x[0] = x[2] = ht * i[0];
            x[1] = x[3] = ht * (i[0] + 1);
            y[0] = y[1] = ht * i[1];
            y[2] = y[3] = ht * (i[1] + 1);
            double r[4];
            r[0] = (x[0] - cx) * (x[0] - cx) + (y[0] - cy) * (y[0] - cy);
            r[1] = (x[1] - cx) * (x[1] - cx) + (y[1] - cy) * (y[1] - cy);
            r[2] = (x[2] - cx) * (x[2] - cx) + (y[2] - cy) * (y[2] - cy);
            r[3] = (x[3] - cx) * (x[3] - cx) + (y[3] - cy) * (y[3] - cy);
            bool allin = true, allout = true;
            for (double k : r) {
                allin &= (k < (radius - eps) * (radius - eps));
                allout &= (k > (radius + eps) * (radius + eps));
            }
            if (!allin && !allout) return true;
            else
                return false;
        };
    };
    auto m = MeshBuilder<Mesh>()
                     .setBaseMesh(MeshBuilder<CartesianMesh<Meta::int_<2>>>()
                                          .newMesh(n, n)
                                          .setMeshOfDim(0, 0., 1.)
                                          .setMeshOfDim(1, 0., 1.)
                                          .build())
                     .setRefinementRatio(ratio)
                     .setFillRateThreshold(0.8)
                     .setSlimThreshold(3)
                     .setMaxLevel(maxlevel)
                     .setBuffWidth(1)
                     .setMarkerFunction(f(0.5, 0.5))
                     .build();

    auto u = ExprBuilder<Field>()
                     .setMesh(m)
                     .setName("u")
                     .setLoc({LocOnMesh ::Center, LocOnMesh ::Center})
                     .setBC(0, DimPos::start, BCType::Dirc, 0.)
                     .setBC(0, DimPos::end, BCType::Dirc, 0.)
                     .setBC(1, DimPos::start, BCType::Dirc, 0.)
                     .setBC(1, DimPos::end, BCType::Dirc, 0.)
                     .build();
    auto p = u;

    u.initBy([](auto&& x) { return std::sin(2 * PI * x[0]) * std::sin(2 * PI * x[1]); });
    p.initBy([&](auto&& x) {
        auto r = std::sqrt((x[0] - 0.5) * (x[0] - 0.5) + (x[1] - 0.5) * (x[1] - 0.5));
        return 1.0 - Math::smoothHeviside(h / Math::int_pow(ratio, maxlevel), r - 0.25);
    });

    Utils::VTKAMRStream uf("u"), pf("pp");
    pf << p;
    for (auto i = 0; i < 100; ++i) {
        auto cx = 0.5 + 0.5 * std::cos(i / 100. * 2 * PI);
        auto cy = 0.5 + 0.5 * std::sin(i / 100. * 2 * PI);
        auto m2 = MeshBuilder<Mesh>()
                          .setBaseMesh(MeshBuilder<CartesianMesh<Meta::int_<2>>>()
                                               .newMesh(n, n)
                                               .setMeshOfDim(0, 0., 1.)
                                               .setMeshOfDim(1, 0., 1.)
                                               .build())
                          .setRefinementRatio(ratio)
                          .setFillRateThreshold(0.8)
                          .setSlimThreshold(3)
                          .setMaxLevel(maxlevel)
                          .setBuffWidth(1)
                          .setMarkerFunction(f(cx, cy))
                          .build();

        u.replaceMeshBy(m2);
        uf << Utils::TimeStamp(i) << u;
    }
}