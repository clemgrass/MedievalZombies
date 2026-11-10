#!/usr/bin/env bash
# Lista la versión instalada de cada paquete de tools/ci/toolchain.txt, una línea
# "paquete versión" por paquete, o "paquete -" si no está instalado.
#
#   tools/ci/toolchain-versions.sh
#
# Corre igual en la máquina del autor y en el contenedor (tools/ci/run.sh
# toolchain-versions). tools/ci/compare-toolchain.sh compara las dos salidas.
set -euo pipefail

dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

mapfile -t packages < <(sed -e 's/#.*//' -e 's/[[:space:]]//g' -e '/^$/d' \
    "$dir/toolchain.txt")

for package in "${packages[@]}"; do
    # pacman -Q escribe "paquete versión", o falla si no está instalado.
    pacman -Q "$package" 2>/dev/null || echo "$package -"
done
