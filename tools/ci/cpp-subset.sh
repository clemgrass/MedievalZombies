#!/usr/bin/env bash
# Chequeos del subconjunto de C++ que el compilador no puede hacer cumplir. Es un job de
# la etapa rápida de la CI: no compila nada.
#
#   tools/ci/cpp-subset.sh [archivos...]
#
# Sin archivos, revisa el C++ versionado de engine/, game/ y tests/, sin los fixtures de
# tests/cmake/ y tests/tooling/, que rompen las reglas a propósito. Falla con el archivo,
# la línea y la regla que se rompe.
#
# Las excepciones y la RTTI no se buscan aquí: -fno-exceptions y -fno-rtti cambian el
# lenguaje que acepta el compilador. std::shared_ptr y las corrutinas, en cambio, son C++
# válido, y por eso se buscan aquí.
#
# Límites: busca en el texto, no en el AST. Ignora lo que va después de //, pero no los
# comentarios /* */ ni el contenido de los strings, y no ve un nombre armado con un macro.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

files=("$@")
if [[ ${#files[@]} -eq 0 ]]; then
    # Desde la raíz, para que los mensajes muestren rutas relativas al repo.
    cd "$root"
    mapfile -t files < <(git ls-files -- \
        'engine/*.cpp' 'engine/*.h' 'engine/*.hpp' 'game/*.cpp' 'game/*.h' 'game/*.hpp' \
        'tests/*.cpp' 'tests/*.h' 'tests/*.hpp' \
        ':(exclude)tests/cmake/cases/**' ':(exclude)tests/tooling/fixtures/**')
fi
if [[ ${#files[@]} -eq 0 ]]; then
    exit 0
fi

# Cada regla: patrón (grep -E, sobre la línea sin su comentario) y mensaje.
rules=(
    '\b(shared_ptr|make_shared|allocate_shared|weak_ptr|enable_shared_from_this)\b'
    'std::shared_ptr está prohibido (ADR-0009, Decisión 6). Para objetos de larga vida, std::unique_ptr; para lo que vive en un pool, un handle.'

    '\bco_(await|yield|return)\b|#[[:space:]]*include[[:space:]]*<coroutine>'
    'Las corrutinas están prohibidas (ADR-0009, Decisión 7): asignan memoria por defecto.'
)

failed=false
for file in "${files[@]}"; do
    for ((i = 0; i < ${#rules[@]}; i += 2)); do
        pattern="${rules[i]}"
        message="${rules[i + 1]}"
        # awk numera las líneas y les quita el comentario //; grep busca la regla.
        while IFS= read -r line_number; do
            echo "$file:$line_number: $message"
            failed=true
        done < <(awk '{ sub(/\/\/.*/, ""); print NR "\t" $0 }' "$file" |
            grep -E $'^[0-9]+\t.*('"$pattern"')' | cut -f1)
    done
done

if $failed; then
    exit 1
fi
