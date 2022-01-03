//  ----------------------------------------------------------------------------
//
//  Copyright (c) 2019 - 2022 by the OpFlow developers
//
//  This file is part of OpFlow.
//
//  OpFlow is free software and is distributed under the MPL v2.0 license.
//  The full text of the license can be found in the file LICENSE at the top
//  level directory of OpFlow.
//
//  ----------------------------------------------------------------------------

#include <benchmark/benchmark.h>
#include <cstring>
#include <oneapi/tbb.h>
#include <valarray>
#include <vector>

static void std_vector(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<double> v(256 * 256 * 256);
        v.resize(v.size() * 8);
        benchmark::DoNotOptimize(v);
    }
}

static void std_valarray(benchmark::State& state) {
    for (auto _ : state) {
        std::valarray<double> v(256 * 256 * 256);
        v.resize(v.size() * 8);
        benchmark::DoNotOptimize(v);
    }
}

static void raw_array(benchmark::State& state) {
    for (auto _ : state) {
        double* v = new double[256 * 256 * 256];
        double* vn = new double[512 * 512 * 512];
        for (int i = 0; i < 256 * 256 * 256; ++i) vn[i] = v[i];
        std::swap(v, vn);
        delete[] vn;
        delete[] v;
    }
}

static void raw_array_memcpy(benchmark::State& state) {
    for (auto _ : state) {
        double* v = new double[256 * 256 * 256];
        double* vn = new double[512 * 512 * 512];
        memcpy(vn, v, sizeof(double) * 256 * 256 * 256);
        std::swap(v, vn);
        delete[] vn;
        delete[] v;
    }
}

static void raw_array_parallel(benchmark::State& state) {
    for (auto _ : state) {
        double* v = new double[256 * 256 * 256];
        double* vn = new double[512 * 512 * 512];
        oneapi::tbb::parallel_for(oneapi::tbb::blocked_range<int>(0, 256 * 256 * 256, 1024),
                                  [&](const oneapi::tbb::blocked_range<int>& r) {
                                      for (int i = r.begin(); i != r.end(); ++i) vn[i] = v[i];
                                  });
        std::swap(v, vn);
        delete[] vn;
        delete[] v;
    }
}

BENCHMARK(std_vector)->UseRealTime();
BENCHMARK(std_valarray)->UseRealTime();
BENCHMARK(raw_array)->UseRealTime();
BENCHMARK(raw_array_memcpy)->UseRealTime();
BENCHMARK(raw_array_parallel)->UseRealTime();

BENCHMARK_MAIN();