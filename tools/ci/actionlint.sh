#!/usr/bin/env bash
# Revisa los workflows de GitHub Actions: la estructura, las expresiones ${{ }} y el
# shell de cada run: (con shellcheck). Es un job de la etapa rápida de la CI.
#
#   tools/ci/actionlint.sh
#
# Revisa todos los archivos de .github/workflows/.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

cd "$root"
actionlint
