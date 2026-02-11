#!/usr/bin/env python3
"""Generate GitHub Actions matrix JSON from conda_build_config.yaml."""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import sys
from typing import Dict, List


def parse_conda_build_config(path: pathlib.Path) -> Dict[str, List[str]]:
    data: Dict[str, List[str]] = {}
    current: str | None = None

    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.split("#", 1)[0].rstrip()
        if not line.strip():
            continue

        key_match = re.match(r"^([A-Za-z0-9_-]+):\s*$", line)
        if key_match:
            current = key_match.group(1)
            data[current] = []
            continue

        value_match = re.match(r"^\s*-\s*(.+?)\s*$", line)
        if value_match and current is not None:
            data[current].append(value_match.group(1))

    missing = [name for name in ("mpi", "openmp") if not data.get(name)]
    if missing:
        raise ValueError(
            f"Missing required variant keys in {path}: {', '.join(missing)}"
        )

    return data


def package_matrix(variants: Dict[str, List[str]]) -> Dict[str, List[Dict[str, str]]]:
    include: List[Dict[str, str]] = []
    for mpi in variants["mpi"]:
        for openmp in variants["openmp"]:
            include.append({"mpi": mpi, "openmp": openmp})
    return {"include": include}


def source_matrix(
    variants: Dict[str, List[str]], build_types: List[str]
) -> Dict[str, List[Dict[str, str]]]:
    include: List[Dict[str, str]] = []
    for mpi in variants["mpi"]:
        for openmp in variants["openmp"]:
            for build_type in build_types:
                include.append(
                    {"mpi": mpi, "openmp": openmp, "build_type": build_type}
                )
    return {"include": include}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--recipe-config",
        default="conda/recipe/conda_build_config.yaml",
        help="Path to conda build variant config",
    )
    parser.add_argument(
        "--mode",
        choices=["all", "package", "source"],
        default="all",
        help="Which matrix to generate",
    )
    parser.add_argument(
        "--build-types",
        default="Debug,Release",
        help="Comma-separated build types for source matrix",
    )
    parser.add_argument(
        "--github-output",
        default="",
        help="If set, write output key=value pairs for GitHub Actions",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    config_path = pathlib.Path(args.recipe_config)
    if not config_path.exists():
        print(f"error: config file not found: {config_path}", file=sys.stderr)
        return 1

    build_types = [x.strip() for x in args.build_types.split(",") if x.strip()]
    if not build_types:
        print("error: --build-types resolved to empty list", file=sys.stderr)
        return 1

    variants = parse_conda_build_config(config_path)

    payloads: Dict[str, Dict[str, List[Dict[str, str]]]] = {}
    if args.mode in {"all", "package"}:
        payloads["package_matrix"] = package_matrix(variants)
    if args.mode in {"all", "source"}:
        payloads["source_matrix"] = source_matrix(variants, build_types)

    if args.github_output:
        out_path = pathlib.Path(args.github_output)
        with out_path.open("a", encoding="utf-8") as f:
            for key, value in payloads.items():
                f.write(f"{key}={json.dumps(value, separators=(',', ':'))}\n")

    if args.mode == "all":
        print(json.dumps(payloads, indent=2, sort_keys=True))
    elif args.mode == "package":
        print(json.dumps(payloads["package_matrix"], indent=2, sort_keys=True))
    else:
        print(json.dumps(payloads["source_matrix"], indent=2, sort_keys=True))

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
