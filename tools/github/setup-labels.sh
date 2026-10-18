#!/usr/bin/env bash
# Configura labels y milestones del repo. Se puede correr varias veces.
set -euo pipefail

REPO="clemgrass/MedievalZombies"

# Labels por defecto de GitHub que no usamos.
for label in accessibility bug documentation duplicate enhancement \
  "good first issue" "help wanted" invalid question wontfix; do
  gh label delete "$label" --repo "$REPO" --yes 2>/dev/null || true
done

create() { gh label create "$1" --repo "$REPO" --color "$2" --description "$3" --force; }

for t in épica feature tarea bug investigación; do
  create "tipo:$t" 5319e7 "Tipo de ticket"
done
for a in graphics slicing physics animation ai gameplay tooling docs; do
  create "área:$a" 0e8a16 "Área del proyecto"
done
create "owner:autor" fbca04 "Lo escribe el autor (núcleo)"
create "owner:claude" fbca04 "Lo puede escribir Claude (periférico)"

existing=$(gh api "repos/$REPO/milestones?state=all" -q '.[].title')
for m in "M0 – Base" "M1 – Decisiones fundacionales"; do
  if ! grep -qxF "$m" <<<"$existing"; then
    gh api "repos/$REPO/milestones" -f title="$m" >/dev/null
  fi
done
