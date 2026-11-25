#!/usr/bin/env bash
# Job de la CI: instala el Blender fijado en tools/ci/blender-version (con la caché) y
# comprueba que corre sin interfaz y que es esa versión.
#
#   tools/ci/blender.sh
#
# Falla si la instalación falla (por ejemplo, un sha256 que no coincide) o si la primera
# línea de `blender -b --version` no es "Blender <versión fijada>".
set -euo pipefail

dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

blender="$(bash "$dir/install-blender.sh")"
version="$(sed -n 's/^version=//p' "$dir/blender-version" | tr -d '[:space:]')"

# La salida entera, y después su primera línea: con `| head -n 1`, blender podría morir
# por SIGPIPE y pipefail lo contaría como un fallo.
output="$("$blender" -b --version)"
first_line="${output%%$'\n'*}"
echo "$output"

# El tarball oficial dice "Blender 5.2.2 LTS": la versión, y después un espacio o nada.
if [[ "$first_line" != "Blender $version" && "$first_line" != "Blender $version "* ]]; then
    echo "Se esperaba Blender $version y el binario dice: $first_line" >&2
    exit 1
fi
