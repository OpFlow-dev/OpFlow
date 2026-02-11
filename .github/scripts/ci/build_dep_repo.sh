#!/usr/bin/env bash
set -euo pipefail

repo_url=""
repo_ref="main"
recipe_path=""
variants=""
owner=""
platform="osx-arm64"
channels=("conda-forge" "opflow-dev")

while [[ $# -gt 0 ]]; do
  case "$1" in
    --repo-url)
      repo_url="$2"
      shift 2
      ;;
    --repo-ref)
      repo_ref="$2"
      shift 2
      ;;
    --recipe-path)
      recipe_path="$2"
      shift 2
      ;;
    --variants)
      variants="$2"
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

if [[ -z "$repo_url" || -z "$recipe_path" || -z "$variants" ]]; then
  echo "Usage: build_dep_repo.sh --repo-url <url> --recipe-path <path> --variants '{mpi: nompi, openmp: off}' [--owner <anaconda-org>]" >&2
  exit 2
fi

workdir="$(mktemp -d -t opflow-dep-build-XXXXXX)"
trap 'rm -rf "$workdir"' EXIT

echo "Cloning dependency repo: $repo_url ($repo_ref)"
git clone --depth 1 --branch "$repo_ref" "$repo_url" "$workdir/repo"

recipe_abs="$workdir/repo/$recipe_path"
if [[ ! -e "$recipe_abs" ]]; then
  echo "Recipe path does not exist: $recipe_abs" >&2
  exit 1
fi

channel_args=()
for c in "${channels[@]}"; do
  channel_args+=("-c" "$c")
done

echo "Building recipe: $recipe_abs"
conda build "$recipe_abs" \
  --override-channels \
  --platform "$platform" \
  "${channel_args[@]}" \
  --variants "$variants" \
  --no-anaconda-upload

mapfile -t outputs < <(conda build "$recipe_abs" --output --variants "$variants")

echo "Built outputs:"
printf '  %s\n' "${outputs[@]}"

if [[ -n "$owner" ]]; then
  if ! command -v anaconda >/dev/null 2>&1; then
    echo "anaconda command not found; cannot upload outputs" >&2
    exit 1
  fi
  anaconda -s https://api.anaconda.org upload -u "$owner" --force "${outputs[@]}"
fi
