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

    auto info = makeParallelInfo();
    setGlobalParallelInfo(info);
    setGlobalParallelPlan(makeParallelPlan(getGlobalParallelInfo(), ParallelIdentifier::DistributeMem));
    std::shared_ptr<AbstractSplitStrategy<Field>> strategy = std::make_shared<EvenSplitStrategy<Field>>();

    auto m = MeshBuilder<Mesh>().newMesh(5, 5).setMeshOfDim(0, 0., 10.).setMeshOfDim(1, 0., 10.).build();
    auto u = ExprBuilder<Field>()
                     .setMesh(m)
                     .setBC(0, OpFlow::DimPos::start, BCType::Neum, 0.)
                     .setBC(0, OpFlow::DimPos::end, BCType::Neum, 0.)
                     .setBC(1, OpFlow::DimPos::start, BCType::Neum, 0.)
                     .setBC(1, OpFlow::DimPos::end, BCType::Neum, 0.)
                     .setLoc({LocOnMesh::Center, LocOnMesh::Center})
                     .setPadding(1)
                     .setSplitStrategy(strategy)
                     .build();

    typedef amgcl::backend::builtin<double> DBackend;
    typedef amgcl::backend::builtin<float> FBackend;
    typedef amgcl::mpi::make_solver<
            amgcl::mpi::amg<FBackend, amgcl::mpi::coarsening::smoothed_aggregation<FBackend>,
                            amgcl::mpi::relaxation::spai0<FBackend>>,
            amgcl::mpi::solver::bicgstab<DBackend>>
            Solver;
    {
        auto ass_splitMap = u.splitMap;
        for (auto& r : ass_splitMap) { r = DS::commonRange(r, u.assignableRange); }

        IJSolverParams<Solver> p;
        p.staticMat = true;
        p.pinValue = true;
        p.dumpPath = "./";
        auto handler = makeEqnSolveHandler<Solver>(
                [&](auto&& e) {
                    return d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e) == 1.0;
                },
                u, DS::BlockedMDRangeMapper<2> {ass_splitMap}, p);
        OP_MPI_MASTER_INFO("Built solver handler.");
        handler->solve();
        OP_MPI_MASTER_INFO("Solver finished.");
    }

    FinalizeEnvironment();
    return 0;
}
