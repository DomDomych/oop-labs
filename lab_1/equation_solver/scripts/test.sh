#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
if ! command -v ctest >/dev/null 2>&1; then
    echo "CTest is required. Install CMake and try again." >&2
    exit 1
fi

"$project_dir/scripts/build.sh" "$@"
build_type="${1:-Debug}"
cd -- "$project_dir/build/$build_type"

test_list="$(ctest -N -C "$build_type")"
if [[ "$test_list" == *"Total Tests: 0"* ]]; then
    echo "No tests registered yet. Add tests and register them with CMake add_test()." >&2
    exit 1
fi

ctest -C "$build_type" --output-on-failure
