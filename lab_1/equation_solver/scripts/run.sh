#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
"$project_dir/scripts/build.sh" "$@"
build_type="${1:-Debug}"
build_dir="$project_dir/build/$build_type"

# Multi-configuration generators add another configuration directory.
if [[ -x "$build_dir/$build_type/eqsolver" ]]; then
    exec "$build_dir/$build_type/eqsolver"
fi
exec "$build_dir/eqsolver"
