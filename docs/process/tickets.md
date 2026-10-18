# Guía de tickets: GitHub Issues + Projects

Esta guía es para quien nunca usó GitHub Projects. El porqué de esta herramienta está
en el ADR-0002.

- **Repo:** <https://github.com/clemgrass/MedievalZombies>
- **Tablero:** el Project `MedievalZombies` del perfil `clemgrass`

## Conceptos

| Concepto | Qué es | Equivalente en Jira |
|---|---|---|
| **Issue** | Una unidad de trabajo dentro del repo. Tiene título, descripción, comentarios y un número (`#12`) | Ticket |
| **Sub-issue** | Un issue hijo de otro. El padre muestra una barra de progreso de sus hijos | Epic → Story → Sub-task |
| **Label** | Etiqueta de color para clasificar y filtrar | Label / Component |
| **Milestone** | Agrupa issues hacia una meta, con su barra de progreso | Release / Sprint |
| **Project** | Tablero que vive en el perfil y *toma* issues del repo. Tiene campos propios y varias vistas (Kanban, tabla, roadmap) del mismo trabajo | Board |
| **`Closes #N`** | Escrito en un PR, cierra el issue `#N` al hacer merge | Integración de Jira con Git |

Una idea clave: **el issue vive en el repo y el Project solo lo muestra.** Los labels y
el milestone son del issue. Estado, Prioridad y Tamaño son campos del Project.

## Jerarquía

| Nivel | Qué es | Ejemplo |
|---|---|---|
| Épica | Un área completa | "Renderer", "Corte de mallas" |
| Feature | Una capacidad concreta, sub-issue de una épica | "Shadow maps" |
| Tarea | Se termina en 1 a 3 sesiones de trabajo, con criterios de aceptación claros | "Depth pass para shadow map" |

**Si una tarea no cabe en 1 a 3 sesiones, se parte en tareas más chicas.**

## Labels

| Grupo | Labels | Para qué |
|---|---|---|
| Tipo | `tipo:épica`, `tipo:feature`, `tipo:tarea`, `tipo:bug`, `tipo:investigación` | Qué clase de ticket es. Una `tipo:investigación` produce un ADR |
| Área | `área:graphics`, `área:slicing`, `área:physics`, `área:animation`, `área:ai`, `área:gameplay`, `área:tooling`, `área:docs` | Qué parte del proyecto toca |
| Owner | `owner:autor`, `owner:claude` | Quién escribe el código (regla B, ver `CLAUDE.md`) |

Cada formulario de issue pone su label de `tipo:` automáticamente. El de `área:` se
agrega a mano.

**El `owner` se elige en el formulario, pero el label `owner:` se pone al refinar.**
Sin ese label, el ticket no puede pasar a `Listo`.

## Campos del tablero

| Campo | Valores |
|---|---|
| Estado | `Backlog` → `Listo` → `En progreso` → `En revisión` → `Hecho` |
| Prioridad | `Alta`, `Media`, `Baja` |
| Tamaño | `S`, `M`, `L` |

## Definición de Listo y de Hecho

- **Listo:** tiene criterios de aceptación, label `owner:`, Tamaño, y su ADR
  relacionado (si aplica) está `Aceptado`.
- **Hecho:** cumple los criterios de aceptación, está mergeado con `Closes #N`, y (si
  es `owner:autor`) el autor respondió "Qué debo poder explicar".

## Cómo hago…

Cada acción tiene su ruta en la web y su comando `gh` equivalente.

### …crear un issue

- **Web:** *Issues* → *New issue* → elegir el formulario (Épica, Feature, Tarea,
  Investigación).
- **gh:** `gh issue create --template "Tarea"`

### …agregar un sub-issue

- **Web:** abrir el issue padre → *Create sub-issue* (nuevo) o *Add existing issue*
  (uno que ya existe).
- **gh:** `gh issue create --parent 12` (nuevo) o `gh issue edit 34 --parent 12`
  (existente).

### …poner un label o un milestone

- **Web:** en la barra lateral del issue, *Labels* y *Milestone*.
- **gh:** `gh issue edit 34 --add-label "área:graphics" --milestone "M1 – Decisiones fundacionales"`

### …mover un ticket de estado

- **Web:** en el tablero, arrastrar la tarjeta a otra columna. También se puede
  cambiar *Status* en la barra lateral del issue.
- **gh:**
  `gh project item-edit --owner clemgrass --url <url-del-issue> --field Status --value "En progreso"`

### …cerrar un ticket desde un PR

Ver [`git.md`](git.md). En corto: rama `<N>-descripcion`, PR con `Closes #N` y squash
merge. El issue se cierra solo y el tablero lo pasa a `Hecho`.

### …buscar tickets

- **Web:** en la barra de filtros, por ejemplo `label:"tipo:investigación" is:open`
  (las comillas son necesarias por los dos puntos).
- **gh:** `gh issue list --label "tipo:investigación" --milestone "M1 – Decisiones fundacionales"`

Ojo: la búsqueda filtrada usa el índice de búsqueda de GitHub, así que un issue recién
creado puede tardar unos segundos en aparecer.
