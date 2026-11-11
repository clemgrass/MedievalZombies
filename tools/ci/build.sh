#!/usr/bin/env bash
# Build de la segunda etapa de la CI: corre un workflow preset de CMakePresets.json, que
# configura, compila y, si el preset los incluye, corre los tests.
#
#   tools/ci/build.sh <preset>
#
#   debug           Clang con ASan, UBSan y LSan: configurar, compilar y probar
#   debug-gcc       lo mismo con GCC, el compilador secundario
#   relwithdebinfo  configurar y compilar, sin tests
#   release         configurar y compilar, sin tests
#
# Lo mismo corre sin el script, en cualquier máquina con el toolchain:
#
#   cmake --workflow --preset <preset>
#
# Los warnings son errores en todas las configuraciones (COMPILE_WARNING_AS_ERROR): un
# warning rompe el job aunque solo lo dé uno de los dos compiladores.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

if [[ $# -ne 1 ]]; then
    echo "Uso: tools/ci/build.sh <preset>   (debug, debug-gcc, relwithdebinfo, release)" >&2
    exit 2
fi

# Desde la raíz: cmake --workflow busca CMakePresets.json en la carpeta actual.
cd "$root"
cmake --workflow --preset "$1"
