#!/usr/bin/env bash
set -euo pipefail

# Fallback launcher for environments where OpenMPI cannot spawn MPI processes
# reliably (e.g., local hostname is not resolvable). It strips mpiexec options
# and runs the target command directly in singleton mode.

if [[ $# -eq 0 ]]; then
  echo "Usage: mpiexec-singleton.sh <mpiexec args> <command>" >&2
  exit 2
fi

args=("$@")
argc=$#
idx=0

while [[ $idx -lt $argc ]]; do
  arg="${args[$idx]}"
  case "$arg" in
    -n|-np|--np|--n)
      idx=$((idx + 2))
      ;;
    --host|--hostfile|--bind-to|--map-by|--mca)
      idx=$((idx + 2))
      ;;
    --oversubscribe|--report-bindings)
      idx=$((idx + 1))
      ;;
    --)
      idx=$((idx + 1))
      break
      ;;
    -*)
      idx=$((idx + 1))
      ;;
    *)
      break
      ;;
  esac
done

if [[ $idx -ge $argc ]]; then
  echo "No executable found in mpiexec invocation: $*" >&2
  exit 2
fi

exec "${args[@]:$idx}"
