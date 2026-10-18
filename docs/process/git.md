# Convenciones git

## Commits

Se usa [Conventional Commits](https://www.conventionalcommits.org/): el tipo va en
inglés y la descripción en español.

```
<tipo>: <descripción corta en minúscula>

<cuerpo opcional: el porqué del cambio>

Refs #N
```

| Tipo | Para qué |
|---|---|
| `feat` | Funcionalidad nueva |
| `fix` | Corrección de un bug |
| `docs` | Documentación (ADRs, guías, specs) |
| `refactor` | Cambio interno que no altera el comportamiento |
| `test` | Tests |
| `build` | Sistema de build, toolchain, dependencias |
| `chore` | Mantenimiento: configuración del repo, scripts |

Ejemplo: `feat: depth pass para shadow maps`.

## Ramas

Cada ticket tiene su rama, con nombre `<N>-<descripcion-corta>`, donde `N` es el
número del issue.

```bash
git switch -c 42-shadow-map-depth-pass
```

## Integración: PR con squash merge

1. Al terminar el ticket, se abre un PR hacia `main`. **El título del PR sigue
   Conventional Commits**, porque se convierte en el commit de `main`. El cuerpo
   lleva `Closes #N`.
2. Se integra con **squash merge**: todo el trabajo del ticket queda como **un solo
   commit** en `main`.
3. Al hacer merge, GitHub cierra el issue `#N` y el tablero lo mueve a `Hecho`.

```bash
gh pr create --title "feat: depth pass para shadow maps" --body "Closes #42"
gh pr merge --squash --delete-branch
```

Así queda la cadena que puede seguir un evaluador:
**ADR (por qué) → issue (qué) → commit en `main` (cómo)**.

## Coautoría

Los commits que incluyen código o texto escrito por Claude terminan con la línea:

```
Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
```

El nombre del modelo cambia según la versión de Claude que se use. GitHub muestra a
Claude como coautor del commit, así que la autoría queda en el historial de git, commit
por commit (ADR-0014).

## `main` protegida

**Nunca se trabaja directo en `main`, ni siquiera para una errata.** Todo cambio entra
por rama y PR. El *ruleset* "Proteger main" del repo lo hace cumplir, sin excepciones:

- PR obligatorio para cualquier cambio;
- las conversaciones del PR se resuelven antes del merge;
- sin *force push* y sin borrar la rama;
- cuando exista la CI (ADR-0029), también la CI en verde.

## Antes de abrir el PR

1. Correr la **skill de auditoría** contra los ADRs (ticket #52).
2. Revisar juntos sus hallazgos. Cada uno se corrige, se ignora (con el motivo escrito en
   el PR) o queda como deuda técnica, con su issue `deuda-técnica`.
3. Completar la lista de revisión de la plantilla del PR.
