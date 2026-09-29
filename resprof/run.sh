#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

preset="${PRESET:-mingw-debug}"
build_dir="build/$preset"

if [[ ! -f "$build_dir/build.ninja" ]]; then
    cmake --preset "$preset"
fi

cmake --build --preset "$preset"

exec "./$build_dir/resprof.exe" "$@"