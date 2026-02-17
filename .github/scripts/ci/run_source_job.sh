#!/usr/bin/env bash
set -euo pipefail

mpi=""
openmp=""
build_type=""
owner="opflow-dev"
platform="osx-arm64"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --mpi)
      mpi="$2"
      shift 2
      ;;
    --openmp)
      openmp="$2"
      shift 2
      ;;
    --build-type)
      build_type="$2"
      shift 2
      ;;
    --owner)
      owner="$2"
      shift 2
      ;;
    --platform)
      platform="$2"
      shift 2
      ;;
    *)
      echo "Unknown argument: $1" >&2
      exit 2
      ;;
  esac
done

if [[ -z "$mpi" || -z "$openmp" || -z "$build_type" ]]; then
  echo "Usage: run_source_job.sh --mpi <nompi|openmpi> --openmp <on|off> --build-type <Debug|Release> [--owner <org>]" >&2
  exit 2
fi

bash .github/scripts/ci/bootstrap_conda_tools.sh

bash .github/scripts/ci/ensure_deps.sh \
  --platform "$platform" \
  --owner "$owner" \
  --mpi "$mpi" \
  --openmp "$openmp"

source "$(conda info --base)/etc/profile.d/conda.sh"

env_prefix="/tmp/opflow-src-${mpi}-${openmp}-${build_type}"
conda remove -y -p "$env_prefix" --all >/dev/null 2>&1 || true

install_specs=(
  "cmake >=4.0.2"
  "ninja"
  "pkg-config"
  "c-compiler"
  "cxx-compiler"
  "spdlog"
  "tbb"
  "${owner}::amgcl"
  "vtk"
  "benchmark"
  "gtest"
  "${owner}::hypre * mpi_${mpi}_openmp_${openmp}_*"
)

if [[ "$mpi" == "nompi" ]]; then
  install_specs+=(
    "mpi * mpi_serial"
    "${owner}::tecio * mpi_nompi_*"
    "hdf5 * nompi*"
  )
  opflow_with_mpi="OFF"
else
  install_specs+=(
    "mpi * openmpi"
    "openmpi"
    "${owner}::teciompi * mpi_openmpi_*"
    "hdf5 * mpi_openmpi*"
  )
  opflow_with_mpi="ON"
fi

if [[ "$openmp" == "on" ]]; then
  opflow_with_openmp="ON"
  if [[ "$(uname -s)" == "Darwin" ]]; then
    install_specs+=("llvm-openmp")
  fi
else
  opflow_with_openmp="OFF"
fi

echo "Creating build env: $env_prefix"
conda create -y -p "$env_prefix" \
  --override-channels \
  -c "$owner" \
  -c conda-forge \
  "${install_specs[@]}"

# Some conda activation scripts assume unset toolchain vars are allowed.
set +u
conda activate "$env_prefix"
set -u
hash -r

cmake_bin="$(command -v cmake || true)"
ctest_bin="$(command -v ctest || true)"
if [[ -z "$cmake_bin" || -z "$ctest_bin" ]]; then
  echo "cmake/ctest not found after activating $env_prefix" >&2
  exit 1
fi

if command -v nproc >/dev/null 2>&1; then
  jobs="$(nproc)"
else
  jobs="$(sysctl -n hw.ncpu)"
fi

cmake_args=(
  -S .
  -B build-ci
  -G Ninja
  -DCMAKE_BUILD_TYPE="$build_type"
  -DOPFLOW_BUILD_ALL=OFF
  -DOPFLOW_BUILD_EXAMPLES=ON
  -DOPFLOW_BUILD_TESTS=ON
  -DOPFLOW_BUILD_BENCHMARKS=ON
  -DOPFLOW_BUILD_DOCS=OFF
  -DOPFLOW_ENABLE_MODULE=OFF
  -DOPFLOW_WITH_MPI="$opflow_with_mpi"
  -DOPFLOW_WITH_OPENMP="$opflow_with_openmp"
  -DOPFLOW_WITH_HDF5=ON
)

if [[ "$mpi" != "nompi" ]]; then
  cmake_args+=(
    -DCMAKE_C_COMPILER=mpicc
    -DCMAKE_CXX_COMPILER=mpicxx
    -DMPI_C_COMPILER=mpicc
    -DMPI_CXX_COMPILER=mpicxx
  )
fi

echo "Configuring source tree"
"$cmake_bin" "${cmake_args[@]}"

echo "Building All_CI"
"$cmake_bin" --build build-ci -t All_CI --parallel "$jobs"

echo "Running tests"
"$ctest_bin" --test-dir build-ci --output-on-failure -VV --parallel "$jobs" -C "$build_type"
