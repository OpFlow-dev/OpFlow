// FTCS scheme for 1D heat transfer equation.
#include <OpFlow>
using namespace OpFlow;
int main() {// clang-format off
    using Mesh = CartesianMesh<Meta::int_<1>>;
    using Field = CartesianField<Real, Mesh>;
    auto mesh = MeshBuilder<Mesh>().newMesh(100001).setMeshOfDim(0, 0., 100.).build();
    auto u = ExprBuilder<Field>().setName("u").setMesh(mesh)
            .setBC(0, DimPos::start, BCType::Dirc, 0.).setBC(0, DimPos::end, BCType::Dirc, 1.).build();
    const Real dt = 1e-16, alpha = 1.0;
    auto t0 = std::chrono::system_clock::now();
    for (auto i = 0; i < 300000; ++i) {
        OP_INFO("Current step {}", i);
        u = u + dt * alpha * d2x<D2SecondOrderCentered>(u);
    }
    auto t1 = std::chrono::system_clock::now();
    OP_INFO("Elapsed time: {}ms", std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());

    return 0;
}