#!/usr/bin/env bash
# Instala el toolchain y las herramientas de la CI en el contenedor archlinux:latest, con
# los repositorios de Arch congelados en la fecha de tools/ci/archive-date.
#
#   tools/ci/install.sh
#
# Corre como root y solo dentro de un contenedor: reemplaza el mirrorlist del sistema y
# puede bajar de versión cualquier paquete. Lo llaman la CI y tools/ci/run.sh.
#
# El Arch Linux Archive guarda una foto diaria de los repositorios. Con el mirrorlist
# apuntando a una fecha, pacman -Syyuu deja todo el sistema en las versiones de ese día
# (las dos u permiten bajar de versión). No se mezcla con mirrors al día: un paquete de
# otra fecha rompería la foto.
set -euo pipefail

dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [[ ! -f /.dockerenv ]]; then
    echo "install.sh solo corre dentro del contenedor de la CI." >&2
    echo "En tu máquina, usa: tools/ci/run.sh <job>" >&2
    exit 1
fi

date="$(tr -d '[:space:]' <"$dir/archive-date")"
if [[ ! "$date" =~ ^[0-9]{4}/[0-9]{2}/[0-9]{2}$ ]]; then
    echo "tools/ci/archive-date tiene que ser una fecha AAAA/MM/DD, y dice: '$date'" >&2
    exit 1
fi

# Las listas, sin comentarios ni líneas vacías.
mapfile -t packages < <(cat "$dir/toolchain.txt" "$dir/tools.txt" |
    sed -e 's/#.*//' -e 's/[[:space:]]//g' -e '/^$/d')

echo "Server = https://archive.archlinux.org/repos/$date/\$repo/os/\$arch" \
    >/etc/pacman.d/mirrorlist
echo "Instalando el toolchain del $date: ${packages[*]}"
pacman -Syyuu --noconfirm --needed "${packages[@]}"
