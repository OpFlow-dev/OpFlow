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

    auto m = MeshBuilder<Mesh>().newMesh(11, 11).setMeshOfDim(0, 0., 10.).setMeshOfDim(1, 0., 10.).build();
    auto u = ExprBuilder<Field>()
                     .setMesh(m)
                     .setBC(0, OpFlow::DimPos::start, BCType::Dirc, 0.)
                     .setBC(0, OpFlow::DimPos::end, BCType::Dirc, 0.)
                     .setBC(1, OpFlow::DimPos::start, BCType::Dirc, 0.)
                     .setBC(1, OpFlow::DimPos::end, BCType::Dirc, 0.)
                     .setPadding(1)
                     .setSplitStrategy(strategy)
                     .build();

    StructSolverParams<StructSolverType::GMRES> params;
    params.dumpPath = "u";
    params.printLevel = 2;
    params.staticMat = true;
    StructSolverParams<StructSolverType::PFMG> p_params;
    p_params.maxIter = 1;
    p_params.tol = 0.0;
    p_params.useZeroGuess = true;
    p_params.rapType = 0;
    p_params.relaxType = 1;
    p_params.numPreRelax = 1;
    p_params.numPostRelax = 1;
    p_params.skipRelax = 0;

    auto solver = PrecondStructSolver<StructSolverType::GMRES, StructSolverType::PFMG>(params, p_params);
    auto handler = makeEqnSolveHandler(
            [&](auto&& e) { return d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e) == 1.0; }, u,
            solver);
    OP_MPI_MASTER_INFO("Built solver handler.");
    handler->solve();
    OP_MPI_MASTER_INFO("Solver finished.");

    FinalizeEnvironment();
    return 0;
}
