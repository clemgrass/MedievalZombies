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
# También revisa las reglas de SDL de la capa de plataforma, que CMake no puede hacer
# cumplir porque los headers del sistema se ven desde cualquier target: SDL solo se
# incluye en los .cpp de engine/platform (y en sus tests), y su renderer y su API gráfica
# no se usan.
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

# Cada regla: patrón (grep -E, sobre la línea sin su comentario), mensaje y las carpetas
# donde la regla no vale (grep -E sobre la ruta relativa al repo; vacío: vale en todo el
# repo).
rules=(
    '\b(shared_ptr|make_shared|allocate_shared|weak_ptr|enable_shared_from_this)\b'
    'std::shared_ptr está prohibido (ADR-0009, Decisión 6). Para objetos de larga vida, std::unique_ptr; para lo que vive en un pool, un handle.'
    ''

    '\bco_(await|yield|return)\b|#[[:space:]]*include[[:space:]]*<coroutine>'
    'Las corrutinas están prohibidas (ADR-0009, Decisión 7): asignan memoria por defecto.'
    ''

    # Solo en los .cpp de platform, no en sus headers públicos: esos los incluye el resto
    # del engine. Los tests de platform también: inyectan eventos de SDL para probar la
    # traducción.
    '#[[:space:]]*include[[:space:]]*[<"]SDL3/'
    'SDL solo se incluye en engine/platform/src (ADR-0004): el resto del engine, y los headers públicos de platform, usan la interfaz de platform::, sin tipos de SDL.'
    '^(engine/platform/src|tests/platform)/'

    # La API gráfica de SDL (SDL_GPUDevice, SDL_CreateGPUDevice...) y su renderer 2D
    # (SDL_Renderer, SDL_RenderPresent...), por nombre o por su header.
    '\bSDL_[A-Za-z0-9_]*(GPU|Render)|SDL3/SDL_(gpu|render)\.h'
    'SDL_GPU y SDL_Renderer están prohibidos (ADR-0004): el renderer es propio, sobre Vulkan.'
    ''
)

failed=false
for file in "${files[@]}"; do
    # La ruta relativa al repo, también si llegó absoluta (los fixtures de los tests).
    relative="${file#"$root"/}"
    for ((i = 0; i < ${#rules[@]}; i += 3)); do
        pattern="${rules[i]}"
        message="${rules[i + 1]}"
        allowed_in="${rules[i + 2]}"
        if [[ -n "$allowed_in" ]] && grep -qE "$allowed_in" <<<"$relative"; then
            continue
        fi
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
