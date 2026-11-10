#!/usr/bin/env bash
# Decide qué partes del repo cambió un PR, para que la CI salte lo que no hace falta con
# condiciones dentro del workflow y nunca con paths:. Un chequeo obligatorio saltado por
# paths: nunca se reporta, y deja el PR esperándolo para siempre.
#
#   tools/ci/changes.sh <base> <head>
#
# Escribe una línea clave=true|false por cada parte, en el formato de $GITHUB_OUTPUT.
# Hoy hay una sola parte:
#
#   code   el C++, el build y lo que los revisa: engine/, game/, tests/, cmake/, los
#          CMakeLists.txt y CMakePresets.json de la raíz, las reglas .clang-*, los
#          scripts de tools/ci/, los workflows, y cualquier .cpp, .h o .hpp, esté
#          donde esté.
#
# Compara <head> con su ancestro común con <base> (git diff base...head): solo cuenta lo
# que cambió el PR, no lo que entró a main mientras tanto. Sin <base> (un push a main),
# todo cuenta como cambiado: main se prueba entero.
set -euo pipefail

base="${1:-}"
head="${2:-HEAD}"

code_pattern='^(engine|game|tests|cmake|tools/ci|\.github/workflows)/'
code_pattern+='|^(CMakeLists\.txt|CMakePresets\.json|\.clang-format|\.clang-format-ignore|\.clang-tidy)$'
code_pattern+='|\.(cpp|h|hpp)$'

if [[ -z "$base" ]]; then
    echo "code=true"
    exit 0
fi

changed="$(git diff --name-only "$base...$head")"

if grep -Eq "$code_pattern" <<<"$changed"; then
    echo "code=true"
else
    echo "code=false"
fi
