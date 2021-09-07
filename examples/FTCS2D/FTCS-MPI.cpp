// FTCS scheme for 2D heat transfer equation.
#include <OpFlow>

using namespace OpFlow;

int main(int argc, char** argv) {
    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;

    InitEnvironment(&argc, &argv);
    int rank, nproc;
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    constexpr auto n = 1025;
    auto mesh = MeshBuilder<Mesh>().newMesh(n, n).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();
    auto info = makeParallelInfo();
    setGlobalParallelInfo(info);
    setGlobalParallelPlan(makeParallelPlan(getGlobalParallelInfo(), ParallelIdentifier::DistributeMem));
    std::shared_ptr<AbstractSplitStrategy<Field>> strategy = std::make_shared<EvenSplitStrategy<Field>>();
    auto u = ExprBuilder<Field>()
                     .setName("u")
                     .setMesh(mesh)
                     .setBC(0, DimPos::start, BCType::Dirc, 1.)
                     .setBC(0, DimPos::end, BCType::Dirc, 1.)
                     .setBC(1, DimPos::start, BCType::Dirc, 1.)
                     .setBC(1, DimPos::end, BCType::Dirc, 1.)
                     .setLoc(std::array {LocOnMesh ::Center, LocOnMesh ::Center})
                     .setPadding(1)
                     .setSplitStrategy(strategy)
                     .build();

    const Real dt = 0.1 / Math::pow2(n - 1), alpha = 1.0;
    Utils::H5Stream uf("./sol.h5");
    uf.fixedMesh();
    uf << Utils::TimeStamp(0.) << u;
    auto t0 = std::chrono::system_clock::now();
    for (auto i = 1; i <= 50000; ++i) {
        if (i % 100 == 0) OP_MPI_MASTER_INFO("Current step {}", i);
        u = u + dt * alpha * (d2x<D2SecondOrderCentered>(u) + d2y<D2SecondOrderCentered>(u));
        if (i % 1000 == 0) uf << Utils::TimeStamp(i * dt) << u;
    }
    auto t1 = std::chrono::system_clock::now();
    OP_MPI_MASTER_INFO("Elapsed time: {}ms",
                       std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());
    if (u.localRange.start[0] <= n / 2 && n / 2 < u.localRange.end[0] && u.localRange.start[1] <= n / 2
        && n / 2 < u.localRange.end[1])
        OP_INFO("Center val: {}", u.evalAt(DS::MDIndex<2> {n / 2, n / 2}));
    uf.close();

    FinalizeEnvironment();
    return 0;
}