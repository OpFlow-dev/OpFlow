#!/usr/bin/env bash
set -euo pipefail

need_conda_build=0

if ! conda build --help >/dev/null 2>&1; then
  need_conda_build=1
fi

if [[ $need_conda_build -eq 0 ]]; then
  echo "conda-build is already available."
  exit 0
fi

echo "Installing required conda tooling: conda-build"
conda install -y -n base -c conda-forge conda-build

if ! conda build --help >/dev/null 2>&1; then
  echo "Failed to provision conda-build." >&2
  exit 1
fi

echo "conda-build is ready."
