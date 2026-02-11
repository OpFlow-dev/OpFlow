#!/usr/bin/env bash
set -euo pipefail

missing_file=""
mpi=""
openmp=""
owner="opflow-dev"
platform="osx-arm64"
upload_owner=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --missing-file)
      missing_file="$2"
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

if [[ -z "$missing_file" || -z "$mpi" || -z "$openmp" ]]; then
  echo "Usage: bootstrap_missing_deps.sh --missing-file <file> --mpi <nompi|openmpi> --openmp <on|off> [--owner <org>] [--platform <subdir>]" >&2
  exit 2
fi

if [[ ! -f "$missing_file" ]]; then
  echo "Missing spec file does not exist: $missing_file" >&2
  exit 1
fi

if [[ "${OPFLOW_BOOTSTRAP_UPLOAD:-0}" == "1" ]]; then
  upload_owner="$owner"
fi

croot="$(conda config --show croot | awk '{print $2}')"
local_channel="file://${croot}"

missing_specs=()
while IFS= read -r line; do
  [[ -n "$line" ]] && missing_specs+=("$line")
done < <(awk 'NF {print $0}' "$missing_file" | sort -u)

if [[ ${#missing_specs[@]} -eq 0 ]]; then
  echo "No missing specs to bootstrap."
  exit 0
fi

need_tecio_nompi=0
need_teciompi=0
need_opflow_hypre=0
unresolved=()

for spec in "${missing_specs[@]}"; do
  case "$spec" in
    "tecio * mpi_nompi_*")
      need_tecio_nompi=1
      ;;
    "teciompi * mpi_openmpi_*")
      need_teciompi=1
      ;;
    "opflow-hypre "*)
      need_opflow_hypre=1
      ;;
    *)
      unresolved+=("$spec")
      ;;
  esac
done

if [[ $need_tecio_nompi -eq 1 || $need_teciompi -eq 1 ]]; then
  tecio_repo_url="${OPFLOW_TECIO_REPO_URL:-https://github.com/Luohaothu/tecio.git}"
  tecio_repo_ref="${OPFLOW_TECIO_REPO_REF:-master}"
  tecio_recipe_path="${OPFLOW_TECIO_RECIPE_PATH:-conda/meta.yaml}"

  if [[ $need_tecio_nompi -eq 1 ]]; then
    build_args=(
      --repo-url "$tecio_repo_url"
      --repo-ref "$tecio_repo_ref"
      --recipe-path "$tecio_recipe_path"
      --platform "$platform"
      --variants "{mpi: nompi}"
      --channel "$local_channel"
      --channel "$owner"
      --channel "conda-forge"
    )
    if [[ -n "$upload_owner" ]]; then
      build_args+=(--owner "$upload_owner")
    fi
    bash .github/scripts/ci/build_dep_repo.sh "${build_args[@]}"
  fi

  if [[ $need_teciompi -eq 1 ]]; then
    build_args=(
      --repo-url "$tecio_repo_url"
      --repo-ref "$tecio_repo_ref"
      --recipe-path "$tecio_recipe_path"
      --platform "$platform"
      --variants "{mpi: openmpi}"
      --channel "$local_channel"
      --channel "$owner"
      --channel "conda-forge"
    )
    if [[ -n "$upload_owner" ]]; then
      build_args+=(--owner "$upload_owner")
    fi
    bash .github/scripts/ci/build_dep_repo.sh "${build_args[@]}"
  fi
fi

if [[ $need_opflow_hypre -eq 1 ]]; then
  hypre_repo_url="${OPFLOW_HYPRE_REPO_URL:-}"
  hypre_repo_ref="${OPFLOW_HYPRE_REPO_REF:-master}"
  hypre_recipe_path="${OPFLOW_HYPRE_RECIPE_PATH:-conda/recipe/meta.yaml}"

  if [[ -z "$hypre_repo_url" ]]; then
    unresolved+=("opflow-hypre bootstrap requested but OPFLOW_HYPRE_REPO_URL is not set")
  else
    build_args=(
      --repo-url "$hypre_repo_url"
      --repo-ref "$hypre_repo_ref"
      --recipe-path "$hypre_recipe_path"
      --platform "$platform"
      --variants "{mpi: ${mpi}, openmp: ${openmp}}"
      --channel "$local_channel"
      --channel "$owner"
      --channel "conda-forge"
    )
    if [[ -n "$upload_owner" ]]; then
      build_args+=(--owner "$upload_owner")
    fi
    bash .github/scripts/ci/build_dep_repo.sh "${build_args[@]}"
  fi
fi

if [[ ${#unresolved[@]} -gt 0 ]]; then
  echo "Cannot auto-bootstrap some missing dependencies:" >&2
  printf '  - %s\n' "${unresolved[@]}" >&2
  exit 1
fi

echo "Dependency bootstrap completed for platform=${platform}, mpi=${mpi}, openmp=${openmp}."
