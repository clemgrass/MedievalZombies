#!/usr/bin/env bash
# Instala los hooks de git del repo (opcional). Apunta core.hooksPath a
# tools/hooks/, así que los hooks quedan versionados y se actualizan con el repo, sin
# copiarlos a .git/hooks/ (que git no versiona).
#
#   tools/install-hooks.sh
#
# Para desinstalarlos: git config --unset core.hooksPath
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
git -C "$root" config core.hooksPath tools/hooks
echo "Hooks instalados: core.hooksPath = tools/hooks"
