#include "Poisson.hpp"
#define FMT_HEADER_ONLY
#include "fmt/chrono.h"
#include "fmt/compile.h"
#include "pch.hpp"
#include <filesystem>

using namespace OpFlow;

void Poisson() {
    using Mesh = CartesianAMRMesh<Meta::int_<2>>;
    using Field = CartAMRField<Real, Mesh>;

    int n = 9, maxlevel = 2, ratio = 2, buffWidth = 1;
    auto h = 2. / (n - 1);
    /*
    auto m = MeshBuilder<Mesh>()
                     .setBaseMesh(MeshBuilder<CartesianMesh<Meta::int_<2>>>()
                                          .newMesh(n, n)
                                          .setMeshOfDim(0, -1., 1.)
                                          .setMeshOfDim(1, -1., 1.)
                                          .build())
                     .setRefinementRatio(ratio)
                     .setFillRateThreshold(0.8)
                     .setSlimThreshold(buffWidth)
                     .setBuffWidth(buffWidth)
                     .setMaxLevel(maxlevel)
                     .setMarkerFunction([&](auto&& i) {
                         auto l = i.l;
                         double cx = 0., cy = 0.;
                         double radius = 0.15;
                         double ht = h / Math::int_pow(ratio, l);
                         double eps = buffWidth * h / Math::int_pow(ratio, maxlevel - 1);
                         double x[4], y[4];
                         x[0] = x[2] = -1 + ht * i[0];
                         x[1] = x[3] = -1 + ht * (i[0] + 1);
                         y[0] = y[1] = -1 + ht * i[1];
                         y[2] = y[3] = -1 + ht * (i[1] + 1);
                         double r[4];
                         r[0] = (x[0] - cx) * (x[0] - cx) + (y[0] - cy) * (y[0] - cy);
                         r[1] = (x[1] - cx) * (x[1] - cx) + (y[1] - cy) * (y[1] - cy);
                         r[2] = (x[2] - cx) * (x[2] - cx) + (y[2] - cy) * (y[2] - cy);
                         r[3] = (x[3] - cx) * (x[3] - cx) + (y[3] - cy) * (y[3] - cy);
                         bool allin = true, allout = true;
                         auto r_min = std::max(0., radius - eps);
                         for (double k : r) {
                             allin &= (k < r_min * r_min);
                             allout &= (k > (radius + eps) * (radius + eps));
                         }
                         if (!allin && !allout) return true;
                         else
                             return false;
                     })
                     .build(); */
    auto m = CartesianAMRMeshDirectComposer<Meta::int_<2>>()
                     .setBaseMesh(MeshBuilder<CartesianMesh<Meta::int_<2>>>()
                                          .newMesh(6, 6)
                                          .setMeshOfDim(0, 0., 5.)
                                          .setMeshOfDim(1, 0., 5.)
                                          .build())
                     .setBuffWidth(1)
                     .setRefinementRatio(2)
                     .addPatch(DS::LevelRange<2>(1, 0, DS::Range<2>(std::array {2, 2}, std::array {5, 5})))
                     .addPatch(DS::LevelRange<2>(1, 1, DS::Range<2>(std::array {4, 4}, std::array {7, 7})))
                     //.addPatch(DS::LevelRange<2>(1, 2, DS::Range<2>(std::array{10, 6}, std::array{13, 11})))
                     .build();
    auto u = ExprBuilder<Field>()
                     .setMesh(m)
                     .setName("u")
                     .setLoc({LocOnMesh::Center, LocOnMesh ::Center})
                     .setBC(0, DimPos::start, BCType::Dirc, 0.)
                     .setBC(0, DimPos::end, BCType::Dirc, 0.)
                     .setBC(1, DimPos::start, BCType::Dirc, 0.)
                     .setBC(1, DimPos::end, BCType::Dirc, 0.)
                     .build();
    auto v = u;
    v.name = "v";
    auto p = ExprBuilder<Field>()
                     .setName("p")
                     .setMesh(m)
                     .setBC(0, DimPos::start, BCType::Dirc, 0.)
                     .setBC(0, DimPos::end, BCType::Dirc, 0.)
                     .setBC(1, DimPos::start, BCType::Dirc, 0.)
                     .setBC(1, DimPos::end, BCType::Dirc, 0.)
                     .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                     .build();
    //p.initBy([](auto&& x) { return std::sqrt(Math::pow2(x[0] - 0.5) + Math::pow2(x[1] - 0.75)) - 0.15; });
    p = 0;
    auto _fmt = FMT_COMPILE("Result_{:%m-%d_%H-%M-%S}");
    auto root = fmt::format(_fmt, fmt::localtime(std::time(nullptr)));
    std::filesystem::create_directory(root);

    SemiStructSolverParams<SemiStructSolverType::FAC> params;
    params.dumpPath = root + "/p";
    params.maxIter = 10;
    Solve([&](auto&& e) { return d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e) == 1.; }, p,
          params);

    Utils::VTKAMRStream pf(root + "/p");
    pf << p;
}