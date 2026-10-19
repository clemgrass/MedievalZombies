---
name: auditoria-adr
description: Audita los cambios de una rama de MedievalZombies contra los ADRs aceptados, con un subagente auditor por tema (proceso, docs, C++, arquitectura, build y licencias, gráficos, matemática, animación, testing), y presenta los hallazgos para decidirlos con el autor. Úsala siempre antes de abrir un PR, al terminar un ticket y antes de dar los comandos de commit/push/`gh pr create`, y también cuando el autor pida auditar, revisar contra los ADRs, verificar que algo cumple lo decidido, o escriba /auditoria-adr, aunque no diga "auditoría".
---

# Auditoría de un PR contra los ADRs

Los ADRs de `docs/` son las decisiones del proyecto. Con muchos ADRs, es fácil que el
autor o Claude se salten una regla sin darse cuenta. Esta skill compara **lo que se
hizo** en la rama con **lo que se decidió**, tema por tema, antes de que el cambio
llegue a `main`.

Los auditores son objetivos: no buscan llevar la contraria ni opinar de gustos. Sus
hallazgos no se aplican solos. **El autor y Claude los revisan juntos**, y cada uno se
corrige, se ignora con un motivo, o queda como deuda técnica con su issue.

## Paso 1: reunir el contexto

1. **Ticket:** sale del nombre de la rama (`<N>-descripcion`). Léelo con
   `gh issue view <N>`, que trae los criterios de aceptación (o la "Pregunta a decidir",
   si es una investigación), el label `modo:` y "Qué debo poder explicar".
   - Si la rama no sigue el formato, pregúntale al autor qué ticket cierra.
   - Si el ticket todavía no existe en GitHub, usa el texto que te dé el autor. El
     auditor de `proceso` lo reporta: hay que crear el issue (con aprobación del autor)
     antes del PR, y la rama tiene que llevar su número.
2. **La base de comparación** es `main`. Si no existe una rama `main` local, usa
   `origin/main`:
   ```bash
   BASE=$(git rev-parse --verify -q main >/dev/null && echo main || echo origin/main)
   ```
3. **Archivos cambiados respecto de la base.** El autor hace el commit después de la
   auditoría, así que el trabajo suele estar **sin commitear**. Junta las tres fuentes:
   ```bash
   git diff --name-only "$BASE"...HEAD          # commits de la rama
   git diff --name-only HEAD                    # cambios sin commitear (staged y unstaged)
   git ls-files --others --exclude-standard     # archivos nuevos sin seguimiento
   ```
   Para un auditor, el diff completo sale de `git diff "$BASE"` más el contenido de los
   archivos nuevos.
   **Excluye `.claude/skills/*/evals/fixtures/**`:** son casos de prueba de las skills,
   con errores puestos a propósito.
4. **ADRs aceptados:** el índice está en `docs/README.md`.

## Paso 2: elegir los auditores

Lee `references/temas.md`. Su índice dice qué rutas activan cada tema:

- **`proceso` y `docs-y-adrs` corren siempre.**
- Los demás temas corren si algún archivo cambiado cae en sus rutas, **o** si el
  contenido del diff toca su área según la tabla "Activación por contenido" de
  `temas.md`. Esto importa sobre todo en los cambios de documentación: un ADR que nombra
  una librería o toma una decisión técnica activa el tema dueño de esa decisión.

Antes de lanzarlos, dile al autor en una línea qué temas vas a correr y por qué.

## Paso 3: lanzar los auditores en paralelo

Lanza **un subagente por tema, todos en el mismo mensaje**, para que corran a la vez.
Usa el tipo de agente `auditor-adr` (`.claude/agents/auditor-adr.md`). Si ese tipo no
está disponible en la sesión, usa `general-purpose` y pídele que lea primero
`.claude/agents/auditor-adr.md` y lo siga como sus instrucciones.

El prompt de cada auditor lleva:

```
Tema: <tema>
Tu sección: .claude/skills/auditoria-adr/references/temas.md, sección "<tema>"
Rama: <rama>   Base: <main u origin/main>
Índice de ADRs: docs/README.md (rutas y estados de cada ADR)
Ticket: #<N> — <título>
<cuerpo del ticket, o "ver `gh issue view <N>`">
Archivos cambiados (respecto de main): <lista>
Diff: `git diff <base> -- <rutas de tu tema>` más el contenido completo de los archivos nuevos: <lista>
Título previsto del PR: <título>   (solo para `proceso`)
```

Espera a que terminen **todos** los auditores antes de consolidar: cada uno avisa al
terminar.

Si no puedes lanzar subagentes, haz tú mismo cada auditoría, tema por tema y en
secuencia, siguiendo `.claude/agents/auditor-adr.md`. Avísale al autor que la
auditoría corrió sin subagentes.

## Paso 4: consolidar

Cuando terminen todos:

1. **Verifica cada hallazgo** antes de mostrarlo: abre el archivo en la línea citada y
   el ADR en la regla citada. Descarta los que no se sostienen, y dilo ("descarté 2
   hallazgos que no se sostenían: …"). Un hallazgo falso le cuesta tiempo al autor.
2. **Une los duplicados.** Si dos temas reportan lo mismo, queda uno, con los dos ADRs.
3. **Ordena** por severidad: `bloqueante`, después `importante`, después `sugerencia`.

## Paso 5: presentar y decidir con el autor

Muestra esta tabla, en español:

```
## Auditoría de #<N> · <k> temas · <b> bloqueantes, <i> importantes, <s> sugerencias

| # | Severidad | Tema | Dónde | Hallazgo | ADR | Propuesta de Claude |
|---|---|---|---|---|---|---|
| 1 | bloqueante | cpp | `engine/core/x.hpp:12` | usa `shared_ptr` | 0009 | Corregir: `unique_ptr` |
```

Debajo, por cada hallazgo, la solución completa del auditor. Termina con los temas que
no tuvieron hallazgos ("Sin hallazgos: arquitectura, testing"), si hay alguno.

En la columna "Propuesta de Claude" va lo que recomiendas:

| Propuesta | Cuándo |
|---|---|
| **Corregir** | Se arregla en este PR |
| **Ignorar** | No aplica. Va con el motivo, que queda escrito en el PR |
| **Deuda técnica** | Se acepta por ahora, con su issue `deuda-técnica` y un `TODO(#N)` |
| **Acción del autor** | Algo que no está en el diff y que hace el autor, por ejemplo responder "Qué debo poder explicar" en el issue |
| **Otro ticket** | Es real pero queda fuera del alcance de este ticket. Se propone un issue aparte |

Es solo una propuesta: **la decisión es del autor**.
Pregúntale qué hace con cada uno, y no corrijas nada antes de que responda. Un
`bloqueante` solo se ignora si el autor lo decide de forma explícita.

## Paso 6: cerrar

Con las decisiones del autor:

1. **Corregir:** aplica los cambios. Si tocan más de un archivo o cambian la lógica,
   vuelve a correr solo los temas afectados.
2. **Deuda técnica:** propón el issue con label `deuda-técnica` y créalo solo si el autor
   lo aprueba. Agrega `TODO(#N)` en el código, en el lugar del hallazgo.
3. **Cuerpo del PR:** escribe `.git/PR_BODY.md` siguiendo
   `.github/pull_request_template.md`, con `Closes #N` y la sección **Auditoría
   contra los ADRs** completa: por cada hallazgo, una línea con su decisión. Si no hubo
   hallazgos, escribe "Sin hallazgos (temas: …)". Ese archivo vive dentro de `.git/`,
   así que nunca entra al repo.
4. Recién ahí dale al autor los comandos del PR:
   ```bash
   git add …
   git commit -m "<tipo>: <descripción>" -m "Refs #<N>" -m "Co-Authored-By: Claude <modelo> <noreply@anthropic.com>"
   git push -u origin <rama>
   gh pr create --title "<tipo>: <descripción>" --body-file .git/PR_BODY.md
   ```
