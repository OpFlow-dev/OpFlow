#!/usr/bin/env bash
set -euxo pipefail

cmake_args=(
  -S "${SRC_DIR}"
  -B build
  -G Ninja
  -DCMAKE_BUILD_TYPE=Release
  -DCMAKE_INSTALL_PREFIX="${PREFIX}"
  -DOPFLOW_ENABLE_MODULE=OFF
  -DOPFLOW_BUILD_EXAMPLES=OFF
  -DOPFLOW_BUILD_TESTS=OFF
  -DOPFLOW_BUILD_BENCHMARKS=OFF
  -DOPFLOW_BUILD_DOCS=OFF
  -DOPFLOW_INSTALL=ON
)

if [[ "${mpi}" == "nompi" ]]; then
  cmake_args+=(
    -DOPFLOW_WITH_MPI=OFF
  )
else
  # Prefer MPI from this conda-build environment.
  cmake_args+=(
    -DOPFLOW_WITH_MPI=ON
    "-DCMAKE_C_COMPILER=${PREFIX}/bin/mpicc"
    "-DCMAKE_CXX_COMPILER=${PREFIX}/bin/mpicxx"
    "-DMPI_C_COMPILER=${PREFIX}/bin/mpicc"
    "-DMPI_CXX_COMPILER=${PREFIX}/bin/mpicxx"
  )
fi

if [[ "${openmp}" == "on" ]]; then
  cmake_args+=(-DOPFLOW_WITH_OPENMP=ON)
else
  cmake_args+=(-DOPFLOW_WITH_OPENMP=OFF)
fi

cmake "${cmake_args[@]}" ${CMAKE_ARGS}
cmake --build build -- -j"${CPU_COUNT:-2}"
cmake --install build

