#!/usr/bin/env bash
set -euo pipefail

platform="osx-arm64"
mpi=""
openmp=""
owner="opflow-dev"
channels=()

while [[ $# -gt 0 ]]; do
  case "$1" in
    --platform)
      platform="$2"
      shift 2
      ;;
    --mpi)
      mpi="$2"
      shift 2
      ;;
    --openmp)
      openmp="$2"
      shift 2
      ;;
    --owner)
      owner="$2"
      shift 2
      ;;
    --channel)
      channels+=("$2")
      shift 2
      ;;
    *)
      echo "Unknown argument: $1" >&2
      exit 2
      ;;
  esac
done

if [[ -z "$mpi" || -z "$openmp" ]]; then
  echo "Usage: ensure_deps.sh --mpi <nompi|openmpi> --openmp <on|off> [--platform <subdir>] [--owner <org>]" >&2
  exit 2
fi

if [[ ${#channels[@]} -eq 0 ]]; then
  channels=("$owner" "conda-forge")
fi

channel_args=()
for c in "${channels[@]}"; do
  channel_args+=("-c" "$c")
done

missing=()

check_spec() {
  local spec="$1"
  if conda search --override-channels --platform "$platform" "${channel_args[@]}" "$spec" >/dev/null 2>&1; then
    echo "[OK] $spec"
  else
    echo "[MISS] $spec"
    missing+=("$spec")
  fi
}

check_spec "cmake >=4.0.2"
check_spec "ninja"
check_spec "spdlog"
check_spec "tbb"
check_spec "amgcl"
check_spec "vtk"
check_spec "benchmark"
check_spec "gtest"

if [[ "$mpi" == "nompi" ]]; then
  check_spec "mpi * mpi_serial"
  check_spec "tecio * mpi_nompi_*"
  check_spec "hdf5 * nompi*"
else
  check_spec "mpi * openmpi"
  check_spec "openmpi"
  check_spec "teciompi * mpi_openmpi_*"
  check_spec "hdf5 * mpi_openmpi*"
fi

check_spec "opflow-hypre * mpi_${mpi}_openmp_${openmp}_*"

if [[ "$openmp" == "on" && "$(uname -s)" == "Darwin" ]]; then
  check_spec "llvm-openmp"
fi

if [[ ${#missing[@]} -gt 0 ]]; then
  echo "Dependency check failed for platform=${platform}, mpi=${mpi}, openmp=${openmp}" >&2
  printf 'Missing specs:\n' >&2
  printf '  - %s\n' "${missing[@]}" >&2
  exit 1
fi

echo "All dependencies available for platform=${platform}, mpi=${mpi}, openmp=${openmp}."
