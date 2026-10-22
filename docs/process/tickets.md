# Guía de tickets: GitHub Issues + Projects

Esta guía es para quien nunca usó GitHub Projects. El porqué de esta herramienta está
en el ADR-0002.

- **Repo:** <https://github.com/clemgrass/MedievalZombies>
- **Tablero:** el Project #1 `MedievalZombies` del perfil `clemgrass` (épicas,
  features, tareas e investigaciones)
- **Bugs:** el Project #2 `MedievalZombies – Bug hunting`, separado del tablero para no
  mezclar arreglos con features (ADR-0011). Ver [Bugs](#bugs)

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
| Área | `área:graphics`, `área:slicing`, `área:physics`, `área:animation`, `área:ai`, `área:gameplay`, `área:audio`, `área:ui`, `área:tooling`, `área:docs` | Qué parte del proyecto toca |
| Modo | `modo:dirigido`, `modo:propuesto` | Cómo se trabaja la feature o la tarea (ADR-0014): el autor dirige el diseño, o Claude lo propone |
| Deuda | `deuda-técnica` | Un atajo deliberado que hay que pagar. Su `TODO(#N)` en el código apunta al issue |

Cada formulario de issue pone su label de `tipo:` automáticamente, **pero solo si el
issue se crea desde la web**. El label de `área:` siempre se agrega a mano.

**El `modo` se elige en el formulario, pero el label `modo:` se pone al refinar.**
Sin ese label, una feature o una tarea no puede pasar a `Listo`. Las investigaciones no
llevan modo.

## Campos del tablero

| Campo | Valores |
|---|---|
| Estado (en GitHub se llama `Status`) | `Backlog` → `Listo` → `En progreso` → `En revisión` → `Hecho` |
| Prioridad | `Alta`, `Media`, `Baja` |
| Tamaño | `S`, `M`, `L` |

## Definición de Listo y de Hecho

- **Listo:** tiene criterios de aceptación, label `modo:` (si es feature o tarea),
  Tamaño, y su ADR
  relacionado (si aplica) está `Aceptado`.
- **Hecho:** cumple los criterios de aceptación, está mergeado con `Closes #N`, y (si
  es feature o tarea) el autor respondió "Qué debo poder explicar", recorriendo el
  diagrama del flujo cuando lo hay. Si el ticket cambia un flujo dibujado, o le da
  comportamiento a un sistema nuevo, su diagrama está al día (ADR-0030).

## Cómo hago…

Cada acción tiene su ruta en la web y su comando `gh` equivalente.

### …crear un issue

- **Web:** *Issues* → *New issue* → elegir el formulario (Épica, Feature, Tarea,
  Investigación o Bug).
- **gh:** `gh issue create --web` abre el selector de formularios en el navegador.
  Si creas el issue solo con `gh`, sin pasar por el formulario, tienes que poner los
  labels tú: `gh issue create --title "…" --label "tipo:tarea" --body "…"`.

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
  `gh project item-edit 1 --owner clemgrass --url <url-del-issue> --field Status --value "En progreso"`
  (el `1` es el número del Project)

### …cerrar un ticket desde un PR

Ver [`git.md`](git.md). En corto: rama `<N>-descripcion`, PR con `Closes #N` y squash
merge. El issue se cierra solo y el tablero lo pasa a `Hecho`.

### …buscar tickets

- **Web:** en la barra de filtros, por ejemplo `label:"tipo:investigación" is:open`
  (las comillas son necesarias por los dos puntos).
- **gh:** `gh issue list --label "tipo:investigación" --milestone "M1 – Decisiones fundacionales"`

Ojo: la búsqueda filtrada usa el índice de búsqueda de GitHub, así que un issue recién
creado puede tardar unos segundos en aparecer.

## Bugs

El porqué de todo esto está en el ADR-0011, sección 6. Los bugs son issues normales del
repo, con label `tipo:bug`, pero **viven en el Project #2**, no en el tablero de
features.

### Campos del Project de bugs

| Campo | Valores | De dónde sale |
|---|---|---|
| Estado (`Status`) | `Sin confirmar` → `Confirmado` → `En progreso` → `En revisión` → `Cerrado` | El ciclo de vida de Bugzilla (`UNCONFIRMED`, `NEW`, `ASSIGNED`, `RESOLVED`, `CLOSED`) |
| Severidad | `S1 – Crítica`, `S2 – Mayor`, `S3 – Menor`, `S4 – Cosmética` | IEEE 1044. **Cuánto daño hace** en el juego |
| Prioridad | `Alta`, `Media`, `Baja` | **Cuándo se arregla.** Es independiente de la severidad |
| Reproducibilidad | `Siempre`, `A veces`, `No reproducible` | ISO/IEC/IEEE 29119-3 |
| Resolución | `FIXED`, `WONTFIX`, `DUPLICATE`, `WORKSFORME`, `INVALID` | Bugzilla. Solo se llena al cerrar |
| Test de regresión | Texto: ruta y nombre del test | ADR-0011, sección 6.3 |

El formulario pide la severidad de quien reporta. La del Project es la definitiva, que se
fija al clasificar el bug (puede ser distinta).

### Ciclo de vida de un bug

1. **Reportar.** *New issue* → **Bug**. El formulario pone `tipo:bug` y agrega el issue
   al Project #2 en `Sin confirmar`. El label `área:` se agrega a mano, como siempre.
2. **Confirmar.** Reproducirlo y llenar Severidad, Prioridad y Reproducibilidad en el
   Project. Si se puede reproducir con código, **se escribe el test de regresión ahora**,
   antes de arreglarlo:
   - va en `tests/regression/bug_<N>.cpp` y se llama `Bug<N>_<descripción>`, con la URL
     del issue en un comentario;
   - verifica el comportamiento **correcto**, así que falla mientras el bug existe;
   - se registra en CTest con `WILL_FAIL` (falla esperada), para que la suite siga en
     verde;
   - su ruta y nombre van al campo Test de regresión.

   Estado: `Confirmado`.
3. **Arreglar.** Rama `<N>-descripcion` y estado `En progreso`. Cuando el arreglo
   funciona, el test **pasa de forma inesperada** y CTest lo marca. En el mismo PR se
   **quita `WILL_FAIL`**: desde ahí es un test de regresión normal, que queda para
   siempre.
4. **Cerrar.** El PR lleva `Closes #N`. Al hacer merge, el issue se cierra y el Project
   lo pasa a `Cerrado`. **La Resolución se llena a mano**: `FIXED`.

### Bugs sin test

No todo bug se puede reproducir con un test (una falla del driver, un problema de timing,
algo que pasó una vez). **Se registra igual**: se llena "Por qué no tiene test" en el
formulario y se deja vacío el campo Test de regresión. Si no se vuelve a ver, se cierra
como *not planned* con Resolución `WORKSFORME`, y queda como historial.

| Resolución | Cuándo | Cómo se cierra en GitHub |
|---|---|---|
| `FIXED` | Arreglado, con su test si se pudo | `Closes #N` en el PR (*completed*) |
| `WONTFIX` | Es un bug, pero no se va a arreglar | *Close as not planned* |
| `DUPLICATE` | Ya estaba reportado | *Close as duplicate*, enlazando el original |
| `WORKSFORME` | No se pudo reproducir | *Close as not planned* |
| `INVALID` | No era un bug | *Close as not planned* |

### Con `gh`

```bash
# Reportar sin el formulario: poner el label y agregarlo al Project #2 a mano.
gh issue create --title "…" --label "tipo:bug" --label "área:slicing" --body "…"
gh project item-add 2 --owner clemgrass --url <url-del-issue>

# Cambiar un campo del Project de bugs.
gh project item-edit 2 --owner clemgrass --url <url-del-issue> --field Severidad --value "S1 – Crítica"

# Listar los bugs abiertos.
gh issue list --label "tipo:bug"
```

## Errores comunes

Lo que más confundió en la primera sesión con el tablero:

- **No aparecen Prioridad ni Tamaño en el issue.** En la barra lateral, dentro de
  *Projects*, hay que desplegar `MedievalZombies`. Es más fácil editarlos en la vista
  **Tabla**, haciendo clic en la celda.
- **La Definición de Listo no la impone GitHub.** Un ticket se puede mover a `Listo`
  sin `owner:` ni Tamaño. La regla la cumplimos nosotros.
- **Los cambios en una vista no se guardan solos.** Después de renombrar o filtrar una
  vista, hay que presionar **Save** en el menú ▾ de la pestaña.
- **El botón verde del PR por defecto es *Create a merge commit*.** Nuestra convención
  es **Squash and merge**, que está en la flecha ▾ al lado del botón.
- **La rama no se borra sola.** Después del merge, presiona **Delete branch** en el PR.
- **Si renombras opciones de `Status`, no las borres.** Los workflows apuntan al id de
  la opción, no a su nombre.
- **En bash, `&&` va al final de la línea**, no al inicio de la siguiente:

  ```bash
  comando1 && \
    comando2
  ```
