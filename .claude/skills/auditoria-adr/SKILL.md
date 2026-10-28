---
name: auditoria-adr
description: Audita los cambios de una rama de MedievalZombies contra los ADRs aceptados, con un solo agente auditor que recorre en secuencia los temas que tocan (proceso, docs, C++, arquitectura, memoria, build y licencias, gráficos, matemática, animación, física, assets, corte, testing, diagramas, gameplay, herramientas), y presenta los hallazgos para decidirlos con el autor. Úsala siempre antes de abrir un PR, al terminar un ticket y antes de dar los comandos de commit/push/`gh pr create`, y también cuando el autor pida auditar, revisar contra los ADRs, verificar que algo cumple lo decidido, o escriba /auditoria-adr, aunque no diga "auditoría".
---

# Auditoría de un PR contra los ADRs

Los ADRs de `docs/` son las decisiones del proyecto. Con muchos ADRs, es fácil que el
autor o Claude se salten una regla sin darse cuenta. Esta skill compara **lo que se
hizo** en la rama con **lo que se decidió**, tema por tema, antes de que el cambio
llegue a `main`.

La hace **un solo auditor** (el agente `auditor-adr`, con Opus y esfuerzo alto), que
recibe el contexto completo y va por partes: un tema a la vez, acumulando los hallazgos
en un solo informe.

El auditor es objetivo: no busca llevar la contraria ni opinar de gustos. Sus
hallazgos no se aplican solos. **El autor y Claude los revisan juntos**, y cada uno se
corrige, se ignora con un motivo, o queda como deuda técnica con su issue.

## Paso 1: reunir el contexto

1. **Ticket:** sale del nombre de la rama (`<N>-descripcion`). Léelo con
   `gh issue view <N>`, que trae los criterios de aceptación (o la "Pregunta a decidir",
   si es una investigación), el label `modo:` y "Qué debo poder explicar".
   - Si la rama no sigue el formato, pregúntale al autor qué ticket cierra.
   - Si el ticket todavía no existe en GitHub, usa el texto que te dé el autor. El
     tema `proceso` lo reporta: hay que crear el issue (con aprobación del autor)
     antes del PR, y la rama tiene que llevar su número.
2. **La base de comparación** es el punto donde la rama salió de `main` (o de
   `origin/main`, si no existe una rama `main` local). Así, lo que entró a `main` después
   no aparece como un cambio de la rama:
   ```bash
   BASE=$(git rev-parse --verify -q main >/dev/null && echo main || echo origin/main)
   MB=$(git merge-base "$BASE" HEAD)
   ```
3. **Archivos cambiados respecto de la base.** El autor hace el commit después de la
   auditoría, así que el trabajo suele estar **sin commitear**. Junta las tres fuentes:
   ```bash
   git diff --name-only "$MB"...HEAD            # commits de la rama
   git diff --name-only HEAD                    # cambios sin commitear (staged y unstaged)
   git ls-files --others --exclude-standard     # archivos nuevos sin seguimiento
   ```
   **Excluye `.claude/skills/*/evals/fixtures/**`:** son casos de prueba de las skills,
   con errores puestos a propósito.
4. **ADRs aceptados:** el índice está en `docs/README.md`.

## Paso 2: elegir los temas

Lee `references/temas.md`. Su índice dice qué rutas activan cada tema:

- **`proceso` y `docs-y-adrs` corren siempre.**
- Los demás temas corren si algún archivo cambiado cae en sus rutas, **o** si el
  contenido del diff toca su área según la tabla "Activación por contenido" de
  `temas.md`. Esto importa sobre todo en los cambios de documentación: un ADR que nombra
  una librería o toma una decisión técnica activa el tema dueño de esa decisión.

El orden de los temas es el del índice de `temas.md`: `proceso` y `docs-y-adrs` primero.

Antes de lanzar el auditor, dile al autor en una línea qué temas va a revisar y por qué.

## Paso 3: preparar el contexto

El auditor recibe todo en un solo archivo, `.git/AUDITORIA.md`. Vive dentro de `.git/`,
así que nunca entra al repo, y se sobrescribe en cada auditoría. Así el diff no pasa por
tu contexto, y el archivo sigue ahí para revisar otra vez después de corregir.

1. **Escribe la cabecera** con la herramienta de escritura de archivos:

   ```
   # Contexto de la auditoría

   Ticket: #<N> — <título>
   Rama: <rama>   Base: <main u origin/main>, desde <MB>
   Título previsto del PR: <título>

   ## Texto del ticket
   <cuerpo completo de `gh issue view <N>`, o el texto que dio el autor>

   ## Temas, en este orden
   1. proceso: ADR-0001 (docs/adr/0001-….md), ADR-0002 (…), …
   2. docs-y-adrs: …
   3. <tema>: <ADRs con su ruta, según docs/README.md>

   ## Archivos cambiados
   <lista, sin los fixtures de las evals>

   ## Diff
   ```

2. **Agrega el diff con la shell,** sin leerlo. Incluye los commits de la rama, lo que
   no está commiteado y los archivos nuevos, sin los fixtures:

   ```bash
   EXCL=':(exclude).claude/skills/*/evals/fixtures/**'
   {
     git diff "$MB" -- . "$EXCL"
     git ls-files --others --exclude-standard -- . "$EXCL" |
       while IFS= read -r f; do git diff --no-index -- /dev/null "$f"; done
   } >> "$(git rev-parse --git-dir)/AUDITORIA.md"
   ```

## Paso 4: lanzar el auditor

Lanza **un solo agente** de tipo `auditor-adr` (`.claude/agents/auditor-adr.md`). Su
definición ya fija el modelo (`opus`) y el esfuerzo (`high`). El prompt es corto, porque
todo está en el contexto:

```
Contexto de la auditoría: .git/AUDITORIA.md
Revisa los temas en el orden del contexto, uno a la vez, y entrega un solo informe.
```

Si el tipo `auditor-adr` no está disponible en la sesión, usa `general-purpose` con
modelo `opus` y esfuerzo `high`, y pídele que lea primero `.claude/agents/auditor-adr.md`
y lo siga como sus instrucciones.

Espera a que el auditor avise que terminó. Guarda su nombre o su id: lo vas a necesitar
para la segunda revisión.

Si no puedes lanzar agentes, haz tú mismo la auditoría, tema por tema, siguiendo
`.claude/agents/auditor-adr.md`. Avísale al autor que la auditoría corrió sin el agente.

## Paso 5: verificar

1. **Verifica cada hallazgo** antes de mostrarlo: abre el archivo en la línea citada y
   el ADR en la regla citada. Descarta los que no se sostienen, y dilo ("descarté 2
   hallazgos que no se sostenían: …"). Un hallazgo falso le cuesta tiempo al autor.
2. **Revisa que no queden duplicados.** El auditor ya los une; si se le escapó alguno,
   queda uno, con todos sus ADRs.
3. **Ordena** por severidad: `bloqueante`, después `importante`, después `sugerencia`.

## Paso 6: presentar y decidir con el autor

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

## Paso 7: cerrar

Con las decisiones del autor:

1. **Corregir:** aplica los cambios. Si tocan más de un archivo o cambian la lógica,
   pide una segunda revisión de los temas afectados:
   1. rehace `.git/AUDITORIA.md` (Paso 3), porque el diff cambió;
   2. mándale un mensaje al **mismo auditor** (`SendMessage`, con su nombre o su id)
      con los temas que tiene que revisar de nuevo. Conserva lo que ya leyó, así que no
      vuelve a leer todos los ADRs;
   3. si el auditor ya no está disponible (por ejemplo, en otra sesión), lanza uno nuevo
      solo con esos temas en el contexto.
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
