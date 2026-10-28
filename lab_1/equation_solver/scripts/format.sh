#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
if (( $# != 0 )); then
    echo "Usage: $0" >&2
    exit 1
fi
if ! command -v clang-format >/dev/null 2>&1; then
    echo "clang-format is required. Install it and try again." >&2
    exit 1
fi

files=("$project_dir/main.cpp")
while IFS= read -r -d '' file; do
    files+=("$file")
done < <(find "$project_dir/core" "$project_dir/cli" "$project_dir/tests" \
    -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) -print0)

clang-format -i --style=file "${files[@]}"
echo "Formatted ${#files[@]} C++ files."
