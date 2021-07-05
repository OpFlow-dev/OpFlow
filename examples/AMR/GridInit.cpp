#include "pch.hpp"
using namespace OpFlow;

void mesh_gen2() {
    using Mesh = CartesianAMRMesh<Meta::int_<2>>;
    using Field = CartAMRField<double, Mesh>;

    int n = 9, maxlevel = 5, ratio = 2;
    double h = 8. / (n - 1);
    auto m = MeshBuilder<Mesh>()
                     .setBaseMesh(MeshBuilder<CartesianMesh<Meta::int_<2>>>()
                                          .newMesh(n, n)
                                          .setMeshOfDim(0, 0., 8.)
                                          .setMeshOfDim(1, 0., 8.)
                                          .build())
                     .setRefinementRatio(ratio)
                     .setFillRateThreshold(0.8)
                     .setSlimThreshold(3)
                     .setMaxLevel(maxlevel)
                     .setBuffWidth(1)
                     .setMarkerFunction([&](auto&& i) {
                         auto l = i.l;
                         double c = 4;
                         double radius = 2.0;
                         double ht = h / Math::int_pow(ratio, l);
                         double eps = h / Math::int_pow(ratio, maxlevel);
                         double x[4], y[4];
                         x[0] = x[2] = ht * i[0];
                         x[1] = x[3] = ht * (i[0] + 1);
                         y[0] = y[1] = ht * i[1];
                         y[2] = y[3] = ht * (i[1] + 1);
                         double r[4];
                         r[0] = (x[0] - c) * (x[0] - c) + (y[0] - c) * (y[0] - c);
                         r[1] = (x[1] - c) * (x[1] - c) + (y[1] - c) * (y[1] - c);
                         r[2] = (x[2] - c) * (x[2] - c) + (y[2] - c) * (y[2] - c);
                         r[3] = (x[3] - c) * (x[3] - c) + (y[3] - c) * (y[3] - c);
                         bool allin = true, allout = true;
                         for (double k : r) {
                             allin &= (k < (radius - eps) * (radius - eps));
                             allout &= (k > (radius + eps) * (radius + eps));
                         }
                         if (!allin && !allout) return true;
                         else
                             return false;
                     })
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

    Utils::VTKAMRStream uf("u");
    uf << Utils::TimeStamp(0) << u;
}

void mesh_gen3() {
    int n = 9, maxlevel = 5, ratio = 2;
    double h = 8. / (n - 1);

    auto m3 = MeshBuilder<CartesianAMRMesh<Meta::int_<3>>>()
                      .setBaseMesh(MeshBuilder<CartesianMesh<Meta::int_<3>>>()
                                           .newMesh(n, n)
                                           .setMeshOfDim(0, 0., 8.)
                                           .setMeshOfDim(1, 0., 8.)
                                           .setMeshOfDim(2, 0., 8.)
                                           .build())
                      .setRefinementRatio(ratio)
                      .setFillRateThreshold(0.8)
                      .setSlimThreshold(3)
                      .setMaxLevel(maxlevel)
                      .setBuffWidth(1)
                      .setMarkerFunction([&](auto&& i) {
                          int l = i.l;
                          double c = 4;
                          double radius = 2.0;
                          double ht = h / Math::int_pow(ratio, l);
                          double eps = h / Math::int_pow(ratio, maxlevel);
                          double x[8], y[8], z[8];
                          x[0] = x[3] = x[4] = x[7] = ht * i[0];
                          x[1] = x[2] = x[5] = x[6] = ht * (i[0] + 1);
                          y[0] = y[1] = y[4] = y[5] = ht * i[1];
                          y[2] = y[3] = y[6] = y[7] = ht * (i[1] + 1);
                          z[0] = z[1] = z[2] = z[3] = ht * i[2];
                          z[4] = z[5] = z[6] = z[7] = ht * (i[2] + 1);
                          double r[8];
                          r[0] = (x[0] - c) * (x[0] - c) + (y[0] - c) * (y[0] - c) + (z[0] - c) * (z[0] - c);
                          r[1] = (x[1] - c) * (x[1] - c) + (y[1] - c) * (y[1] - c) + (z[1] - c) * (z[1] - c);
                          r[2] = (x[2] - c) * (x[2] - c) + (y[2] - c) * (y[2] - c) + (z[2] - c) * (z[2] - c);
                          r[3] = (x[3] - c) * (x[3] - c) + (y[3] - c) * (y[3] - c) + (z[3] - c) * (z[3] - c);
                          r[4] = (x[4] - c) * (x[4] - c) + (y[4] - c) * (y[4] - c) + (z[4] - c) * (z[4] - c);
                          r[5] = (x[5] - c) * (x[5] - c) + (y[5] - c) * (y[5] - c) + (z[5] - c) * (z[5] - c);
                          r[6] = (x[6] - c) * (x[6] - c) + (y[6] - c) * (y[6] - c) + (z[6] - c) * (z[6] - c);
                          r[7] = (x[7] - c) * (x[7] - c) + (y[7] - c) * (y[7] - c) + (z[7] - c) * (z[7] - c);
                          bool allin = true, allout = true;
                          for (double k : r) {
                              allin &= (k < (radius - eps) * (radius - eps));
                              allout &= (k > (radius + eps) * (radius + eps));
                          }
                          if (!allin && !allout) return true;
                          else
                              return false;
                      })
                      .build();

    auto u3 = ExprBuilder<CartAMRField<double, CartesianAMRMesh<Meta::int_<3>>>>()
                      .setMesh(m3)
                      .setName("u")
                      .setLoc({LocOnMesh ::Center, LocOnMesh ::Center, LocOnMesh::Center})
                      .setBC(0, DimPos::start, BCType::Dirc, 0.)
                      .setBC(0, DimPos::end, BCType::Dirc, 0.)
                      .setBC(1, DimPos::start, BCType::Dirc, 0.)
                      .setBC(1, DimPos::end, BCType::Dirc, 0.)
                      .setBC(2, DimPos::start, BCType::Dirc, 0.)
                      .setBC(2, DimPos::end, BCType::Dirc, 0.)
                      .build();

    Utils::VTKAMRStream uf3("u");
    uf3 << Utils::TimeStamp(0) << u3;
}