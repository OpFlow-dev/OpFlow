// FTCS scheme for 2D heat transfer equation.
#include <OpFlow>

using namespace OpFlow;

int main(int argc, char** argv) {
    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;

    InitEnvironment(argc, argv);
    int rank, nproc;
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    OP_INFO("Rank {} of {}", rank, nproc);
/*
    constexpr auto n = 65;
    auto mesh = MeshBuilder<Mesh>().newMesh(n, n)
            .setMeshOfDim(0, 0., 1.)
            .setMeshOfDim(1, 0., 1.)
            .build();
    auto u = ExprBuilder<Field>()
            .setName("u")
            .setMesh(mesh)
            .setBC(0, DimPos::start, BCType::Dirc, 1.)
            .setBC(0, DimPos::end, BCType::Dirc, 1.)
            .setBC(1, DimPos::start, BCType::Dirc, 1.)
            .setBC(1, DimPos::end, BCType::Dirc, 1.)
            .build();
    u = 0;
    const Real dt = 0.1 / Math::pow2(n - 1), alpha = 1.0;
    Utils::TecplotASCIIStream uf("u.tec");
    uf << Utils::TimeStamp(0.) << u;
    auto t0 = std::chrono::system_clock::now();
    for (auto i = 1; i <= 5000; ++i) {
        OP_INFO("Current step {}", i);
        u = u + dt * alpha * (d2x<D2SecondOrderCentered>(u) + d2y<D2SecondOrderCentered>(u));
        if (i % 100 == 0) uf << Utils::TimeStamp(i * dt) << u;
    }
    auto t1 = std::chrono::system_clock::now();
    OP_INFO("Elapsed time: {}ms", std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());
*/
    FinalizeEnvironment();
    return 0;
}