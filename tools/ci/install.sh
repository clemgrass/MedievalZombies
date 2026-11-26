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
# apuntando a una fecha, pacman -Syy baja las bases de datos de ese día y pacman -Suu deja
# todo el sistema en sus versiones (las dos u permiten bajar de versión). No se mezcla con
# mirrors al día: un paquete de otra fecha rompería la foto.
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

# Las bases de datos de los repositorios (core.db, extra.db) de una fecha del archivo no
# cambian nunca, así que la CI las guarda en su caché con la fecha como clave, como los
# paquetes. Si ya están las de esta fecha, no se vuelven a bajar: el Arch Linux Archive a
# veces no responde, y con las dos cachés un job no lo toca. La marca dice de qué fecha
# son, porque la imagen puede traer sus propias bases de datos, de otro día; se escribe
# solo después de bajarlas bien.
sync_dir=/var/lib/pacman/sync
sync_mark="$sync_dir/mz-archive-date"

# Sirven si son de esta fecha y están todas: los repositorios los decide el pacman.conf de
# la imagen, que no entra en la clave de la caché. Si la imagen suma uno, su base de datos
# falta, y hay que bajarlas otra vez en vez de fallar en -Suu.
sync_is_current() {
    [[ "$(cat "$sync_mark" 2>/dev/null)" == "$date" ]] || return 1
    local repos repo
    mapfile -t repos < <(pacman-conf --repo-list)
    # Sin repositorios (pacman-conf falló: set -e no ve un error dentro de < <(...)), no se
    # puede saber cuáles faltan.
    [[ ${#repos[@]} -gt 0 ]] || return 1
    for repo in "${repos[@]}"; do
        [[ -f "$sync_dir/$repo.db" ]] || return 1
    done
}

if ! sync_is_current; then
    pacman -Syy --noconfirm
    echo "$date" >"$sync_mark"
fi

echo "Instalando el toolchain del $date: ${packages[*]}"
pacman -Suu --noconfirm --needed "${packages[@]}"
