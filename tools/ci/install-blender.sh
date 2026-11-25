#!/usr/bin/env bash
# Instala el Blender de tools/ci/blender-version desde su tarball oficial, verificado con
# su sha256, y escribe en stdout la ruta del binario. Lo usan la CI y la máquina del
# autor, sin root:
#
#   blender="$(tools/ci/install-blender.sh)"
#   "$blender" -b --version
#
# Los mensajes van a stderr: stdout es solo la ruta, para que quien lo llama la use tal
# cual. Nunca usa el blender del PATH.
#
# La instalación queda en $MZ_TOOLS_DIR/blender-<versión>, que por defecto es
# ${XDG_CACHE_HOME:-~/.cache}/medievalzombies. Es la caché: si ahí ya está la versión con
# el mismo sha256 (lo dice el marcador .mz-sha256), no baja nada. Si el sha256 del pin
# cambió, reinstala.
#
# El tarball se baja y se extrae en una carpeta temporal dentro de $MZ_TOOLS_DIR, y la
# carpeta final aparece recién con un mv al terminar: una corrida cortada a la mitad no
# deja una instalación incompleta que parezca buena. Un flock evita que dos corridas a la
# vez se pisen.
#
# MZ_BLENDER_MIRROR cambia el servidor (por defecto, https://download.blender.org/release).
# Los tests lo usan para servir un tarball falso con file://.
set -euo pipefail

dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
pin="$dir/blender-version"

# El valor de una clave del pin: la línea clave=valor, sin espacios.
pinned() {
    sed -n "s/^$1=//p" "$pin" | tr -d '[:space:]'
}

version="$(pinned version)"
sha256="$(pinned sha256)"
if [[ ! "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "tools/ci/blender-version: version tiene que ser X.Y.Z, y dice: '$version'" >&2
    exit 1
fi
if [[ ! "$sha256" =~ ^[0-9a-f]{64}$ ]]; then
    echo "tools/ci/blender-version: sha256 tiene que tener 64 dígitos hexadecimales," \
        "y dice: '$sha256'" >&2
    exit 1
fi

# download.blender.org agrupa las versiones por serie: Blender5.2/blender-5.2.2-...
name="blender-$version-linux-x64"
mirror="${MZ_BLENDER_MIRROR:-https://download.blender.org/release}"
url="$mirror/Blender${version%.*}/$name.tar.xz"

tools_dir="${MZ_TOOLS_DIR:-${XDG_CACHE_HOME:-$HOME/.cache}/medievalzombies}"
install_dir="$tools_dir/blender-$version"
marker="$install_dir/.mz-sha256"

mkdir -p "$tools_dir"
exec {lock}>"$tools_dir/.blender.lock"
flock "$lock"

if [[ -f "$marker" && "$(<"$marker")" == "$sha256" ]]; then
    echo "Blender $version ya está en la caché: $install_dir" >&2
    echo "$install_dir/blender"
    exit 0
fi

tmp="$(mktemp -d "$tools_dir/.blender-XXXXXX")"
trap 'rm -rf "$tmp"' EXIT

echo "Bajando $url" >&2
curl --fail --location --silent --show-error --retry 3 --output "$tmp/$name.tar.xz" "$url"

actual="$(sha256sum "$tmp/$name.tar.xz" | cut -d ' ' -f 1)"
if [[ "$actual" != "$sha256" ]]; then
    echo "El sha256 de $name.tar.xz no coincide con tools/ci/blender-version." >&2
    echo "  fijado: $sha256" >&2
    echo "  bajado: $actual" >&2
    echo "No se instaló nada." >&2
    exit 1
fi

echo "Extrayendo en $install_dir" >&2
tar -xJf "$tmp/$name.tar.xz" -C "$tmp"
if [[ ! -x "$tmp/$name/blender" ]]; then
    echo "El tarball no trae $name/blender." >&2
    exit 1
fi
echo "$sha256" >"$tmp/$name/.mz-sha256"

# Una instalación vieja (de otro sha256) se reemplaza entera.
rm -rf "$install_dir"
mv -T "$tmp/$name" "$install_dir"
echo "$install_dir/blender"
