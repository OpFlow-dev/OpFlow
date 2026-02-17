#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../" && pwd)"
FIXTURE_DIR="$ROOT_DIR/.github/scripts/ci/tests/fixtures"

fail() {
  echo "[FAIL] $*" >&2
  exit 1
}

assert_contains() {
  local text="$1"
  local needle="$2"
  [[ "$text" == *"$needle"* ]] || fail "expected output to contain: $needle"
}

assert_not_contains() {
  local text="$1"
  local needle="$2"
  [[ "$text" != *"$needle"* ]] || fail "expected output NOT to contain: $needle"
}

run_with_mock_conda() {
  local script="$1"
  shift
  PATH="$FIXTURE_DIR:$PATH" bash "$script" "$@" 2>&1
}

echo "[CASE] ensure_deps should fail when dependency is missing"
set +e
ensure_out="$(
  run_with_mock_conda "$ROOT_DIR/.github/scripts/ci/ensure_deps.sh" \
    --platform linux-64 \
    --owner opflow-dev \
    --mpi openmpi \
    --openmp on
)"
ensure_rc=$?
set -e
[[ $ensure_rc -ne 0 ]] || fail "ensure_deps should fail when dependency is missing"
assert_contains "$ensure_out" "Dependency check failed"
assert_contains "$ensure_out" "opflow-dev::hypre * mpi_openmpi_openmp_on_*"

echo "[CASE] preflight should fail-fast without bootstrap"
set +e
preflight_out="$(
  run_with_mock_conda "$ROOT_DIR/.github/scripts/ci/run_preflight_job.sh" \
    --platform linux-64 \
    --owner opflow-dev \
    --mpi openmpi \
    --openmp on
)"
preflight_rc=$?
set -e
[[ $preflight_rc -ne 0 ]] || fail "run_preflight_job should fail on missing dependencies"
assert_contains "$preflight_out" "Dependency check failed"
assert_not_contains "$preflight_out" "Trying to bootstrap missing dependencies"
assert_not_contains "$preflight_out" "bootstrap_missing_deps"

echo "[CASE] package/source scripts should not contain bootstrap calls"
if rg -n "bootstrap_missing_deps" \
  "$ROOT_DIR/.github/scripts/ci/run_package_job.sh" \
  "$ROOT_DIR/.github/scripts/ci/run_source_job.sh" \
  "$ROOT_DIR/.github/scripts/ci/run_preflight_job.sh" >/dev/null; then
  fail "bootstrap_missing_deps references still exist in ci runner scripts"
fi

echo "[PASS] fail-fast dependency policy checks passed"
