#include <OpFlow>

using namespace OpFlow;

int main() {
    using Mesh = CartesianMesh<Meta::int_<1>>;
    using Field = CartesianField<Real, Mesh>;

    auto mesh = MeshBuilder<Mesh>().newMesh(101).setMeshOfDim(0, 0., 1.).build();
    auto u = ExprBuilder<Field>()
                     .setMesh(mesh)
                     .setName("u")
                     .setBC(0, DimPos::start, BCType::Dirc, 0.)
                     .setBC(0, DimPos::end, BCType::Dirc, 0.)
                     .setLoc(LocOnMesh::Corner)
                     .build();

    u.initBy([](auto&& i) { return 0.2 <= i[0] && i[0] <= 0.4 ? 1.0 : 0.0; });

    const Real dt = 0.5e-2;
    const Real c = 1.0;

    Utils::TecplotASCIIStream uf("u.tec");

    auto t0 = std::chrono::system_clock::now();
    for (auto i = 0; i < 100; ++i) {
        OP_INFO("Current step {}", i);
        // Scheme 1: 1st order upwind scheme
        u = u - dt * c * dx<D1FirstOrderBiasedDownwind>(u);
        // Scheme 2: 5th order WENO (3rd order at jump) scheme, use 1st order upwind at boundary
        // u = u - dt * c * d1<DecableOp<D1WENO53Downwind<0>, D1FirstOrderBiasedDownwind<0>>>(u);
        uf << Utils::TimeStamp(i * dt) << u;
    }
    auto t1 = std::chrono::system_clock::now();
    OP_INFO("Elapsed time: {}ms", std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());

    return 0;
}