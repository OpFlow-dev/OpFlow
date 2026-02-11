#!/usr/bin/env bash
set -euo pipefail

mpi=""
openmp=""
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

if [[ -z "$mpi" || -z "$openmp" ]]; then
  echo "Usage: run_preflight_job.sh --mpi <nompi|openmpi> --openmp <on|off> [--owner <org>] [--platform <subdir>]" >&2
  exit 2
fi

missing_file="$(mktemp -t opflow-missing-deps-XXXXXX)"
trap 'rm -f "$missing_file"' EXIT

croot="$(conda config --show croot | awk '{print $2}')"
local_channel="file://${croot}"

if ! bash .github/scripts/ci/ensure_deps.sh \
  --platform "$platform" \
  --owner "$owner" \
  --mpi "$mpi" \
  --openmp "$openmp" \
  --channel "$local_channel" \
  --missing-file "$missing_file"; then
  echo "Dependency check failed. Trying to bootstrap missing dependencies..."
  bash .github/scripts/ci/bootstrap_missing_deps.sh \
    --missing-file "$missing_file" \
    --platform "$platform" \
    --owner "$owner" \
    --mpi "$mpi" \
    --openmp "$openmp"

  bash .github/scripts/ci/ensure_deps.sh \
    --platform "$platform" \
    --owner "$owner" \
    --mpi "$mpi" \
    --openmp "$openmp" \
    --channel "$local_channel" \
    --missing-file "$missing_file"
fi
