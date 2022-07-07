//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2022  by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#include <OpFlow>
#include <benchmark/benchmark.h>

using namespace OpFlow;

class AMGCLEqnSolveBench : public benchmark::Fixture {
public:
    using Mesh = CartesianMesh<Meta::int_<2>>;
    using Field = CartesianField<Real, Mesh>;

    void SetUp(const benchmark::State& state) override {
        auto n = state.range(0);
        auto m = MeshBuilder<Mesh>().newMesh(n, n).setMeshOfDim(0, 0., 1.).setMeshOfDim(1, 0., 1.).build();
        std::shared_ptr<AbstractSplitStrategy<Field>> s = std::make_shared<EvenSplitStrategy<Field>>();
        u = ExprBuilder<Field>()
                    .setMesh(m)
                    .setLoc(std::array {LocOnMesh::Center, LocOnMesh::Center})
                    .setBC(0, DimPos::start, BCType::Dirc, 0.)
                    .setBC(0, DimPos::end, BCType::Dirc, 0.)
                    .setBC(1, DimPos::start, BCType::Dirc, 0.)
                    .setBC(1, DimPos::end, BCType::Dirc, 0.)
                    .setExt(1)
                    .setPadding(1)
                    .setSplitStrategy(s)
                    .build();
        handler = makeEqnSolveHandler<Solver>(
                [&](auto&& e) {
                    return d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e) == 1.0;
                },
                u, DS::BlockedMDRangeMapper<2> {u.getLocalWritableRange()}, params);
    }

    auto gatherTime(const auto& start, const auto& end) {
        double t;
        auto const duration = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
        auto elapsed_seconds = duration.count();
        MPI_Allreduce(&elapsed_seconds, &t, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
        return t;
    }

    Field u;
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
    IJSolverParams<Solver> params;
    std::unique_ptr<EqnSolveHandler> handler;
};

BENCHMARK_DEFINE_F(AMGCLEqnSolveBench, matgen)(benchmark::State& state) {
    while (state.KeepRunning()) {
        auto start = std::chrono::high_resolution_clock::now();
        handler->generateAb();
        auto end = std::chrono::high_resolution_clock::now();
        state.SetIterationTime(gatherTime(start, end));
    }
}

BENCHMARK_DEFINE_F(AMGCLEqnSolveBench, solve)(benchmark::State& state) {
    params.p.solver.maxiter = 100;
    params.p.solver.tol = 1e-10;
    params.staticMat = true;
    handler = makeEqnSolveHandler<Solver>(
            [&](auto&& e) { return d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e) == 1.0; }, u,
            DS::BlockedMDRangeMapper<2> {u.getLocalWritableRange()}, params);
    auto [iter, err, abserr] = handler->solve();

    while (state.KeepRunning()) {
        u = 0.;
        auto start = std::chrono::high_resolution_clock::now();
        handler->solve();
        auto end = std::chrono::high_resolution_clock::now();
        state.SetIterationTime(gatherTime(start, end));
    }
}

BENCHMARK_DEFINE_F(AMGCLEqnSolveBench, dy_solve)(benchmark::State& state) {
    params.p.solver.maxiter = 100;
    params.p.solver.tol = 1e-10;
    params.staticMat = false;
    handler = makeEqnSolveHandler<Solver>(
            [&](auto&& e) { return d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e) == 1.0; }, u,
            DS::BlockedMDRangeMapper<2> {u.getLocalWritableRange()}, params);
    auto [iter, err, abserr] = handler->solve();

    while (state.KeepRunning()) {
        u = 0.;
        auto start = std::chrono::high_resolution_clock::now();
        handler->solve();
        auto end = std::chrono::high_resolution_clock::now();
        state.SetIterationTime(gatherTime(start, end));
    }
}

static void EqnSolve_2d_Params(benchmark::internal::Benchmark* b) {
    for (auto i = 8; i <= 1 << 10; i *= 2) b->Args({i + 1});
}

BENCHMARK_REGISTER_F(AMGCLEqnSolveBench, matgen)
        ->Apply(EqnSolve_2d_Params)
        ->UseManualTime()
        ->Unit(benchmark::kMillisecond);
BENCHMARK_REGISTER_F(AMGCLEqnSolveBench, solve)
        ->Apply(EqnSolve_2d_Params)
        ->UseManualTime()
        ->Unit(benchmark::kMillisecond);
BENCHMARK_REGISTER_F(AMGCLEqnSolveBench, dy_solve)
        ->Apply(EqnSolve_2d_Params)
        ->UseManualTime()
        ->Unit(benchmark::kMillisecond);

// This reporter does nothing.
// We can use it to disable output from all but the root process
class NullReporter : public ::benchmark::BenchmarkReporter {
public:
    NullReporter() = default;
    bool ReportContext(const Context&) override { return true; }
    void ReportRuns(const std::vector<Run>&) override {}
    void Finalize() override {}
};

int main(int argc, char** argv) {
    OpFlow::EnvironmentGardian _(&argc, &argv);
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    if (getWorkerId() == 0)
        // root process will use a reporter from the usual set provided by
        // ::benchmark
        ::benchmark::RunSpecifiedBenchmarks();
    else {
        // reporting from other processes is disabled by passing a custom reporter
        NullReporter null;
        ::benchmark::RunSpecifiedBenchmarks(&null);
    }
    ::benchmark::Shutdown();
    return 0;
}
