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

## Excepción: commits directos a `main`

Solo para cambios triviales de documentación (una errata, un enlace roto). Si tienen
relación con un ticket, llevan `Refs #N`. Esa referencia no cierra el issue.
