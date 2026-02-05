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
        p = std::make_shared<Field>(ExprBuilder<Field>()
                                            .setMesh(m)
                                            .setLoc(std::array {LocOnMesh::Center, LocOnMesh::Center})
                                            .setBC(0, DimPos::start, BCType::Dirc, 0.)
                                            .setBC(0, DimPos::end, BCType::Dirc, 0.)
                                            .setBC(1, DimPos::start, BCType::Dirc, 0.)
                                            .setBC(1, DimPos::end, BCType::Dirc, 0.)
                                            .setExt(1)
                                            .setPadding(1)
                                            .setSplitStrategy(s)
                                            .build());
        u = std::make_shared<Field>(ExprBuilder<Field>()
                                            .setMesh(m)
                                            .setLoc(std::array {LocOnMesh::Corner, LocOnMesh::Center})
                                            .setBC(0, DimPos::start, BCType::Dirc, 0.)
                                            .setBC(0, DimPos::end, BCType::Dirc, 0.)
                                            .setBC(1, DimPos::start, BCType::Dirc, 0.)
                                            .setBC(1, DimPos::end, BCType::Dirc, 0.)
                                            .setExt(1)
                                            .setPadding(1)
                                            .setSplitStrategy(s)
                                            .build());
        v = std::make_shared<Field>(ExprBuilder<Field>()
                                            .setMesh(m)
                                            .setLoc(std::array {LocOnMesh::Center, LocOnMesh::Corner})
                                            .setBC(0, DimPos::start, BCType::Dirc, 0.)
                                            .setBC(0, DimPos::end, BCType::Dirc, 0.)
                                            .setBC(1, DimPos::start, BCType::Dirc, 0.)
                                            .setBC(1, DimPos::end, BCType::Dirc, 0.)
                                            .setExt(1)
                                            .setPadding(1)
                                            .setSplitStrategy(s)
                                            .build());
        r = std::make_shared<Field>(ExprBuilder<Field>()
                                            .setMesh(m)
                                            .setLoc(std::array {LocOnMesh::Center, LocOnMesh::Center})
                                            .setBC(0, DimPos::start, BCType::Dirc, 1.)
                                            .setBC(0, DimPos::end, BCType::Dirc, 1.)
                                            .setBC(1, DimPos::start, BCType::Dirc, 1.)
                                            .setBC(1, DimPos::end, BCType::Dirc, 1.)
                                            .setExt(1)
                                            .setPadding(2)
                                            .setSplitStrategy(s)
                                            .build());
        rx = std::make_shared<Field>(ExprBuilder<Field>()
                                             .setMesh(m)
                                             .setLoc(std::array {LocOnMesh::Corner, LocOnMesh::Center})
                                             .setBC(0, DimPos::start, BCType::Dirc, 1.)
                                             .setBC(0, DimPos::end, BCType::Dirc, 1.)
                                             .setBC(1, DimPos::start, BCType::Dirc, 1.)
                                             .setBC(1, DimPos::end, BCType::Dirc, 1.)
                                             .setExt(1)
                                             .setPadding(2)
                                             .setSplitStrategy(s)
                                             .build());
        ry = std::make_shared<Field>(ExprBuilder<Field>()
                                             .setMesh(m)
                                             .setLoc(std::array {LocOnMesh::Center, LocOnMesh::Corner})
                                             .setBC(0, DimPos::start, BCType::Dirc, 1.)
                                             .setBC(0, DimPos::end, BCType::Dirc, 1.)
                                             .setBC(1, DimPos::start, BCType::Dirc, 1.)
                                             .setBC(1, DimPos::end, BCType::Dirc, 1.)
                                             .setExt(1)
                                             .setPadding(2)
                                             .setSplitStrategy(s)
                                             .build());
        params.p.solver.tol = 1e-16;
        params.p.solver.maxiter = 100;
        handler = makeEqnSolveHandler<Solver>(
                [&](auto&& e) {
                    return d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e) == 1.0;
                },
                *p, DS::BlockedMDRangeMapper<2> {p->getLocalWritableRange()}, params);
    }

    auto gatherTime(const auto& start, const auto& end) {
        double t;
        auto const duration = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
        auto elapsed_seconds = duration.count();
        MPI_Allreduce(&elapsed_seconds, &t, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
        return t;
    }

    void TearDown(const benchmark::State&) override {
        // explicit destruct is needed here to avoid MPI_Comm_free double-called
        handler = nullptr;
    }

    std::shared_ptr<Field> p, u, v, r, rx, ry;
    typedef amgcl::backend::builtin<double> SBackend;
#ifdef MIXED_PRECISION
    typedef amgcl::backend::builtin<float> PBackend;
#else
    typedef amgcl::backend::builtin<double> PBackend;
#endif
    typedef amgcl::mpi::make_solver<
            amgcl::mpi::amg<PBackend, amgcl::mpi::coarsening::smoothed_aggregation<PBackend>,
                            amgcl::mpi::relaxation::spai0<PBackend>>,
            amgcl::mpi::solver::bicgstab<SBackend>>
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

BENCHMARK_DEFINE_F(AMGCLEqnSolveBench, matgen_sppoisson)(benchmark::State& state) {
    double dt = 0.1;
    u->initBy([&](auto&& x) { return std::sin(x[0] * 2 * PI) * std::sin(x[1] * 2 * PI); });
    v->initBy([&](auto&& x) { return std::cos(x[0] * 2 * PI) * std::cos(x[1] * 2 * PI); });
    *p = 0;
    handler = makeEqnSolveHandler<Solver>(
            [&](auto&& e) {
                return d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e)
                       == (dx<D1FirstOrderCentered>(*u) + dy<D1FirstOrderCentered>(*v)) / dt;
            },
            *p, DS::BlockedMDRangeMapper<2> {p->getLocalWritableRange()}, params);

    while (state.KeepRunning()) {
        auto start = std::chrono::high_resolution_clock::now();
        handler->generateAb();
        auto end = std::chrono::high_resolution_clock::now();
        state.SetIterationTime(gatherTime(start, end));
    }
}

BENCHMARK_DEFINE_F(AMGCLEqnSolveBench, matgen_tppoisson)(benchmark::State& state) {
    double dt = 0.1;
    u->initBy([&](auto&& x) { return std::sin(x[0] * 2 * PI) * std::sin(x[1] * 2 * PI); });
    v->initBy([&](auto&& x) { return std::cos(x[0] * 2 * PI) * std::cos(x[1] * 2 * PI); });
    *p = 0;
    *r = 1;
    *rx = 1;
    *ry = 1;
    handler = makeEqnSolveHandler<Solver>(
            [&](auto&& e) {
                return dx<D1FirstOrderCentered>(dx<D1FirstOrderCentered>(e) / d1IntpCenterToCorner<0>(*r))
                               + dy<D1FirstOrderCentered>(dy<D1FirstOrderCentered>(e)
                                                          / d1IntpCenterToCorner<1>(*r))
                       == (dx<D1FirstOrderCentered>(*u) + dy<D1FirstOrderCentered>(*v)) / dt;
            },
            *p, DS::BlockedMDRangeMapper<2> {p->getLocalWritableRange()}, params);

    while (state.KeepRunning()) {
        auto start = std::chrono::high_resolution_clock::now();
        handler->generateAb();
        auto end = std::chrono::high_resolution_clock::now();
        state.SetIterationTime(gatherTime(start, end));
    }
}

BENCHMARK_DEFINE_F(AMGCLEqnSolveBench, matgen_tpcpoisson)(benchmark::State& state) {
    double dt = 0.1;
    u->initBy([&](auto&& x) { return std::sin(x[0] * 2 * PI) * std::sin(x[1] * 2 * PI); });
    v->initBy([&](auto&& x) { return std::cos(x[0] * 2 * PI) * std::cos(x[1] * 2 * PI); });
    *p = 0;
    *r = 1;
    *rx = 1;
    *ry = 1;
    handler = makeEqnSolveHandler<Solver>(
            [&](auto&& e) {
                return dx<D1FirstOrderCentered>(dx<D1FirstOrderCentered>(e) / *rx)
                               + dy<D1FirstOrderCentered>(dy<D1FirstOrderCentered>(e) / *ry)
                       == (dx<D1FirstOrderCentered>(*u) + dy<D1FirstOrderCentered>(*v)) / dt;
            },
            *p, DS::BlockedMDRangeMapper<2> {p->getLocalWritableRange()}, params);

    while (state.KeepRunning()) {
        auto start = std::chrono::high_resolution_clock::now();
        handler->generateAb();
        auto end = std::chrono::high_resolution_clock::now();
        state.SetIterationTime(gatherTime(start, end));
    }
}

BENCHMARK_DEFINE_F(AMGCLEqnSolveBench, solve)(benchmark::State& state) {
    params.p.solver.maxiter = 10;
    params.p.solver.tol = 1e-50;
    params.staticMat = false;
    handler = makeEqnSolveHandler<Solver>(
            [&](auto&& e) { return d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e) == 1.0; },
            *p, DS::BlockedMDRangeMapper<2> {p->getLocalWritableRange()}, params);
    [[maybe_unused]] auto [iter, err, abserr] = handler->solve();

    while (state.KeepRunning()) {
        *p = 0.;
        auto start = std::chrono::high_resolution_clock::now();
        handler->solve();
        auto end = std::chrono::high_resolution_clock::now();
        benchmark::DoNotOptimize(handler);
        state.SetIterationTime(gatherTime(start, end));
    }
}

BENCHMARK_DEFINE_F(AMGCLEqnSolveBench, dy_solve)(benchmark::State& state) {
    params.p.solver.maxiter = 10;
    params.p.solver.tol = 1e-50;
    params.staticMat = true;
    *p = 0;
    handler = makeEqnSolveHandler<Solver>(
            [&](auto&& e) { return d2x<D2SecondOrderCentered>(e) + d2y<D2SecondOrderCentered>(e) == 1.0; },
            *p, DS::BlockedMDRangeMapper<2> {p->getLocalWritableRange()}, params);
    [[maybe_unused]] auto [iter, err, abserr] = handler->solve();

    while (state.KeepRunning()) {
        *p = 0.;
        auto start = std::chrono::high_resolution_clock::now();
        handler->solve();
        auto end = std::chrono::high_resolution_clock::now();
        state.SetIterationTime(gatherTime(start, end));
    }
}

static void EqnSolve_2d_Params(benchmark::internal::Benchmark* b) {
    for (auto i = 32; i <= 1 << 12; i *= 2) b->Args({i + 1});
}

BENCHMARK_REGISTER_F(AMGCLEqnSolveBench, matgen)
        ->Apply(EqnSolve_2d_Params)
        ->UseManualTime()
        ->Unit(benchmark::kMillisecond);
BENCHMARK_REGISTER_F(AMGCLEqnSolveBench, matgen_sppoisson)
        ->Apply(EqnSolve_2d_Params)
        ->UseManualTime()
        ->Unit(benchmark::kMillisecond);
BENCHMARK_REGISTER_F(AMGCLEqnSolveBench, matgen_tppoisson)
        ->Apply(EqnSolve_2d_Params)
        ->UseManualTime()
        ->Unit(benchmark::kMillisecond);
BENCHMARK_REGISTER_F(AMGCLEqnSolveBench, matgen_tpcpoisson)
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
    auto info = makeParallelInfo();
    int max_thread = 0;
    if (argc > 1) {
        max_thread = std::stoi(argv[1]);
        if (max_thread > 0) info.threadInfo.thread_count = max_thread;
    }
    setGlobalParallelPlan(
            makeParallelPlan(info, ParallelIdentifier::DistributeMem | ParallelIdentifier::SharedMem));
    OP_MPI_MASTER_INFO("Run with {} procs each with {} threads",
                       getGlobalParallelPlan().distributed_workers_count,
                       getGlobalParallelPlan().shared_memory_workers_count);
    ::benchmark::Initialize(&argc, argv);
    //if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
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