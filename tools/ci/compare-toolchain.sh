#!/usr/bin/env bash
# Compara el toolchain de la máquina del autor con el del snapshot de la CI: los
# paquetes de tools/ci/toolchain.txt, en las versiones de tools/ci/archive-date.
#
#   tools/ci/compare-toolchain.sh
#
# Lista cada paquete cuya versión difiere y sale con 1; si todo coincide, sale con 0.
# Se corre después de actualizar la máquina: si algo difiere, la fecha nueva va en un
# PR build: (ver docs/process/ci.md). Necesita Docker, porque instala el snapshot en el
# contenedor para preguntarle sus versiones.
set -euo pipefail

dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
date="$(tr -d '[:space:]' <"$dir/archive-date")"

declare -A mine snapshot
while read -r package version; do
    mine[$package]="$version"
done < <(bash "$dir/toolchain-versions.sh")
# tr: con una terminal, docker run -t termina las líneas con \r\n.
while read -r package version; do
    snapshot[$package]="$version"
done < <("$dir/run.sh" toolchain-versions | tr -d '\r')

different=0
for package in "${!mine[@]}"; do
    if [[ "${mine[$package]}" != "${snapshot[$package]:-}" ]]; then
        if [[ $different -eq 0 ]]; then
            printf '%-14s %-36s %s\n' "Paquete" "Tu máquina" "Snapshot $date"
        fi
        printf '%-14s %-36s %s\n' "$package" "${mine[$package]}" "${snapshot[$package]:--}"
        different=$((different + 1))
    fi
done

if [[ $different -gt 0 ]]; then
    echo "$different paquete(s) difieren del snapshot $date." >&2
    exit 1
fi
echo "Los ${#mine[@]} paquetes del toolchain coinciden con el snapshot $date."
