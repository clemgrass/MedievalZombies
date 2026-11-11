#!/usr/bin/env bash
# Cobertura de la segunda etapa de la CI: corre el workflow preset coverage, que
# configura, compila, corre los tests y mide, con la meta del 100 % en líneas y ramas en
# las carpetas de tests/CMakeLists.txt. Falla si la meta no se cumple.
#
#   tools/ci/coverage.sh
#
# Lo mismo corre sin el script, en cualquier máquina con el toolchain:
#
#   cmake --workflow --preset coverage
#
# El reporte queda en build/coverage/coverage/: el HTML en html/ y el resumen en
# summary.txt, con cada línea y cada rama sin cubrir. En GitHub Actions, el resumen va
# también al resumen del job ($GITHUB_STEP_SUMMARY), cumpla o no la meta.
set -uo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
summary="$root/build/coverage/coverage/summary.txt"

# Sin el resumen de una corrida anterior: si el build falla antes de medir, no tiene que
# quedar uno viejo que parezca de esta.
rm -f "$summary"

# Desde la raíz: cmake --workflow busca CMakePresets.json en la carpeta actual.
cd "$root"
cmake --workflow --preset coverage
status=$?

if [[ -n "${GITHUB_STEP_SUMMARY:-}" ]]; then
    {
        echo "## Cobertura"
        echo
        if [[ -f "$summary" ]]; then
            echo '```text'
            cat "$summary"
            echo '```'
        else
            echo "No hay resumen: el build o los tests fallaron antes de medir."
        fi
    } >>"$GITHUB_STEP_SUMMARY"
fi

exit "$status"
