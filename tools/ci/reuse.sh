#!/usr/bin/env bash
# Chequeo de licencias: cada archivo del repo declara su licencia y su autor, con
# REUSE.toml o con un encabezado SPDX. Es un job de la etapa rápida de la CI.
#
#   tools/ci/reuse.sh
#
# Falla con la lista de archivos sin licencia. reuse solo mira lo que git versiona, así
# que necesita un clon con .git: un build/ sin versionar no cuenta.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

reuse --root "$root" lint
