#!/usr/bin/env bash
set -euo pipefail

mpi=""
openmp=""
owner="opflow-dev"
platform="osx-arm64"
output_file=""

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
    --output-file)
      output_file="$2"
      shift 2
      ;;
    *)
      echo "Unknown argument: $1" >&2
      exit 2
      ;;
  esac
done

if [[ -z "$mpi" || -z "$openmp" ]]; then
  echo "Usage: run_package_job.sh --mpi <nompi|openmpi> --openmp <on|off> [--owner <org>]" >&2
  exit 2
fi

bash .github/scripts/ci/ensure_deps.sh \
  --platform "$platform" \
  --owner "$owner" \
  --mpi "$mpi" \
  --openmp "$openmp"

variants="{mpi: ${mpi}, openmp: ${openmp}}"

echo "Building conda package with variants: $variants"
conda build conda/recipe \
  --override-channels \
  -c "$owner" \
  -c conda-forge \
  --variants "$variants" \
  --no-anaconda-upload

mapfile -t outputs < <(conda build conda/recipe --output --variants "$variants")

resolved=()
for pkg in "${outputs[@]}"; do
  if [[ -f "$pkg" ]]; then
    resolved+=("$pkg")
  fi
done

if [[ ${#resolved[@]} -eq 0 ]]; then
  echo "No built package output was found for variants: $variants" >&2
  exit 1
fi

echo "Built package files:"
printf '  %s\n' "${resolved[@]}"

if [[ -n "$output_file" ]]; then
  printf '%s\n' "${resolved[@]}" > "$output_file"
fi
