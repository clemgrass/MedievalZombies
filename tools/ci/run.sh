#!/usr/bin/env bash
# Corre un job de la CI en la máquina del autor, en el mismo contenedor y con la misma
# fecha que la CI. Necesita Docker.
#
#   tools/ci/run.sh <job> [argumentos...]
#
# <job> es el nombre de un script de tools/ci/ sin el .sh (format, cpp-subset, reuse,
# actionlint...), y los argumentos pasan tal cual al script. Ejemplos:
#
#   tools/ci/run.sh format
#   tools/ci/run.sh format --fix
#
# El repo se monta en /src. install.sh corre como root y su salida solo se muestra si
# falla; el job corre con el usuario del autor, para que lo que escriba (format --fix)
# no quede como root. Los paquetes bajados y las bases de datos de los repositorios quedan
# en dos volúmenes de Docker por fecha (mz-ci-pacman-AAAA-MM-DD y
# mz-ci-pacman-sync-AAAA-MM-DD), como las dos cachés de la CI: la segunda corrida no le
# pide nada al Arch Linux Archive.
#
# Las herramientas que no salen de pacman (Blender) se instalan en la misma caché de la
# máquina (${MZ_TOOLS_DIR:-${XDG_CACHE_HOME:-~/.cache}/medievalzombies}), montada en
# /opt/mz-tools como en la CI: un tarball bajado sirve para la máquina y para el
# contenedor.
#
# /src/build es un volumen anónimo, que se borra al terminar: el contenedor compila desde
# cero, como la CI, y no toca el build/ de la máquina. Ese build/ está configurado con las
# rutas de la máquina, y CMake no acepta una caché creada en otra carpeta.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

if [[ $# -eq 0 ]]; then
    echo "Uso: tools/ci/run.sh <job> [argumentos...]" >&2
    exit 2
fi
job="$1"
shift
if [[ ! -f "$root/tools/ci/$job.sh" ]]; then
    echo "No existe el job tools/ci/$job.sh." >&2
    exit 2
fi

date="$(tr -d '[:space:]' <"$root/tools/ci/archive-date")"
volume="mz-ci-pacman-${date//\//-}"
sync_volume="mz-ci-pacman-sync-${date//\//-}"

# Con una terminal, -t mantiene los colores de las herramientas.
tty=()
if [[ -t 1 ]]; then
    tty=(-t)
fi

# Dentro del contenedor: instalar, y después pasar al usuario del autor con setpriv
# (de util-linux, ya en la imagen). HOME=/tmp porque ese usuario no existe en el
# contenedor y no tiene carpeta propia.
# shellcheck disable=SC2016 # Las variables se expanden dentro del contenedor.
inside='
if ! bash tools/ci/install.sh >/tmp/install.log 2>&1; then
    cat /tmp/install.log >&2
    echo "Falló la instalación del toolchain." >&2
    exit 1
fi
chown "$HOST_UID:$HOST_GID" /src/build
job="$1"
shift
exec setpriv --reuid="$HOST_UID" --regid="$HOST_GID" --clear-groups \
    env HOME=/tmp bash "tools/ci/$job.sh" "$@"
'

# Los puntos de montaje de /src/build y de la caché de herramientas: si no existieran,
# Docker los crearía como root.
tools_dir="${MZ_TOOLS_DIR:-${XDG_CACHE_HOME:-$HOME/.cache}/medievalzombies}"
mkdir -p "$root/build" "$tools_dir"

docker run --rm "${tty[@]}" \
    --volume "$root:/src" --workdir /src \
    --volume /src/build \
    --volume "$volume:/var/cache/pacman/pkg" \
    --volume "$sync_volume:/var/lib/pacman/sync" \
    --volume "$tools_dir:/opt/mz-tools" --env MZ_TOOLS_DIR=/opt/mz-tools \
    --env HOST_UID="$(id -u)" --env HOST_GID="$(id -g)" \
    archlinux:latest \
    bash -c "$inside" bash "$job" "$@"
