#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
build_type="${1:-Debug}"

if (( $# > 1 )) || [[ "$build_type" != Debug && "$build_type" != Release ]]; then
    echo "Usage: $0 [Debug|Release]" >&2
    exit 1
fi

if ! command -v cmake >/dev/null 2>&1; then
    echo "CMake is required. Install it and try again." >&2
    exit 1
fi

build_dir="$project_dir/build/$build_type"
cmake -S "$project_dir" -B "$build_dir" -DCMAKE_BUILD_TYPE="$build_type"
cmake --build "$build_dir" --config "$build_type" --parallel
