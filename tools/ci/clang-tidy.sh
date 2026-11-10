#!/usr/bin/env bash
# clang-tidy sobre el C++ de engine/ y game/, con la configuración de .clang-tidy y las
# banderas reales de cada archivo, sacadas de la base de compilación de CMake
# (compile_commands.json).
#
#   tools/ci/clang-tidy.sh [carpeta-de-build]
#
# La carpeta de build por defecto es build/debug. Si todavía no tiene la base de
# compilación, la configura con el preset debug; no hace falta compilar.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
build_dir="${1:-$root/build/debug}"

if [[ ! -f "$build_dir/compile_commands.json" ]]; then
    if [[ $# -gt 0 ]]; then
        echo "No existe $build_dir/compile_commands.json." >&2
        exit 1
    fi
    (cd "$root" && cmake --preset debug >/dev/null)
fi

# run-clang-tidy corre un clang-tidy por archivo, en paralelo, y falla si alguno falla. El
# último argumento filtra los archivos de la base de compilación: solo el código propio.
run-clang-tidy -quiet -p "$build_dir" "^$root/(engine|game)/"
