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

int main(int argc, char* argv[]) {
    InitEnvironment(&argc, &argv);

    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;

    auto m = MeshBuilder<Mesh>().newMesh(11, 11).setMeshOfDim(0, 0., 10.).setMeshOfDim(1, 0., 10.).build();
    auto u = ExprBuilder<Field>()
                     .setMesh(m)
                     .setBC(0, OpFlow::DimPos::start, BCType::Dirc, 0.)
                     .setBC(0, OpFlow::DimPos::end, BCType::Dirc, 0.)
                     .setBC(1, OpFlow::DimPos::start, BCType::Dirc, 0.)
                     .setBC(1, OpFlow::DimPos::end, BCType::Dirc, 0.)
                     .build();

    typedef amgcl::backend::builtin<double> SBackend;
#ifdef MIXED_PRECISION
    typedef amgcl::backend::builtin<float> PBackend;
#else
    typedef amgcl::backend::builtin<double> PBackend;
#endif
    typedef amgcl::make_solver<
            amgcl::amg<PBackend, amgcl::coarsening::smoothed_aggregation, amgcl::relaxation::spai0>,
            amgcl::solver::bicgstab<SBackend>>
            Solver;
    SBackend ::params p;
    PBackend ::params bp;
    auto handler = makeEqnSolveHandler<Solver>(
            [&](auto&& e) { return d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e) == 1.0; }, u,
            DS::MDRangeMapper<2> {u.assignableRange});
    OP_INFO("Built solver handler.");
    handler.solve();
    OP_INFO("Solver finished.");

    Utils::TecplotASCIIStream uf("u.tec");
    uf << u;

    FinalizeEnvironment();
    return 0;
}
