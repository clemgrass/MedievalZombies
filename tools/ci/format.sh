#!/usr/bin/env bash
# Chequeo de formato del C++. Es un job de la etapa rápida de la CI: no compila nada.
#
#   tools/ci/format.sh [--fix] [archivos...]
#
# Sin archivos, revisa todo el C++ versionado en git. Falla si algún archivo no sigue
# .clang-format, y muestra dónde. Con --fix, formatea los archivos en su lugar.
#
# Los archivos que calzan con un .clang-format-ignore (los fixtures de los tests) se
# saltan, aunque se pasen por nombre.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

fix=false
if [[ "${1:-}" == "--fix" ]]; then
    fix=true
    shift
fi

files=("$@")
if [[ ${#files[@]} -eq 0 ]]; then
    # Desde la raíz, para que los mensajes muestren rutas relativas al repo.
    cd "$root"
    mapfile -t files < <(git ls-files -- '*.cpp' '*.h' '*.hpp')
fi
if [[ ${#files[@]} -eq 0 ]]; then
    exit 0
fi

# El estilo se pasa explícito: un archivo fuera del repo (una copia en el build, por
# ejemplo) usaría el estilo por defecto si clang-format lo buscara por carpetas.
style="--style=file:$root/.clang-format"

if $fix; then
    clang-format "$style" -i "${files[@]}"
else
    clang-format "$style" --dry-run --Werror "${files[@]}" || {
        echo "Hay archivos sin formatear. Arréglalos con: tools/ci/format.sh --fix" >&2
        exit 1
    }
fi
