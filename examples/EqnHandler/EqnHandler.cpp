#include <OpFlow>
using namespace OpFlow;

int main(int argc, char* argv[]) {
    EnvironmentGardian _(&argc, &argv);

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
    OP_INFO("Built solver handler.");
    handler->solve();
    OP_INFO("Solver finished.");

    return 0;
}
