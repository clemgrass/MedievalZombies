# Guía de tickets: GitHub Issues + Projects

Esta guía es para quien nunca usó GitHub Projects. El porqué de esta herramienta está
en el ADR-0002.

- **Repo:** <https://github.com/clemgrass/MedievalZombies>
- **Tablero:** el Project #1 `MedievalZombies` del perfil `clemgrass` (épicas,
  tareas e investigaciones)
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
| Épica | Agrupa trabajo que **no es código**, como las decisiones de un milestone de investigación | "M2 – Decisiones de sistemas" (#35) |
| Tarea | Un **átomo**: un PR que entrega algo comprobable y cabe en una revisión tranquila | "Tapa del corte: cortar orejas sin agujeros" |

**En el código no hay épicas** (ADR-0033): el milestone es el corte vertical y el label
`área:` es el sistema. El nivel "feature" ya no existe.

### El átomo

Un ticket que termina en un PR es un átomo. Antes de crearlo, se le hacen cuatro
preguntas (ADR-0033):

1. **¿Entrega algo que se puede comprobar solo?** Un test que pasa, algo que se ve en
   pantalla o una regla del build que falla.
2. **¿Cabe en una revisión tranquila?** Del orden de 200 a 400 líneas de código de
   producción, sin contar los tests.
3. **¿Es indivisible?** Si se parte en dos y cada mitad todavía entregaría algo
   comprobable, se parte.
4. **¿No es demasiado chico?** Si una mitad no entrega nada, no se parte: el átomo es la
   pieza más chica que todavía vale un PR.

Un auto es demasiado grande; un motor, todavía grande; un cilindro, aunque tenga piezas
más chicas, ya es del tamaño justo.

## Labels

| Grupo | Labels | Para qué |
|---|---|---|
| Tipo | `tipo:épica`, `tipo:tarea`, `tipo:bug`, `tipo:investigación` | Qué clase de ticket es. Una `tipo:investigación` produce un ADR |
| Área | `área:core`, `área:graphics`, `área:slicing`, `área:physics`, `área:animation`, `área:ai`, `área:gameplay`, `área:audio`, `área:ui`, `área:tooling`, `área:docs`, `área:contenido` | Qué parte del proyecto toca. En el código, es el sistema. `área:core` es todo el código de `engine/platform` y `engine/core` (memoria, matemática, utilidades, ECS, game loop, formatos y lo que viva ahí): manda la carpeta. `área:tooling` es el build, la CI, la CD, el cocinador y las herramientas fuera de esas carpetas. `área:contenido` es para los assets: mallas, rigs, clips, texturas, sonidos y niveles |
| Modo | `modo:dirigido`, `modo:propuesto` | Cómo se trabaja la tarea (ADR-0014): el autor dirige el diseño, o Claude lo propone |
| Deuda | `deuda-técnica` | Un atajo deliberado que hay que pagar. Su `TODO(#N)` en el código apunta al issue |

Cada formulario de issue pone su label de `tipo:` automáticamente, **pero solo si el
issue se crea desde la web**. El label de `área:` siempre se agrega a mano.

**El `modo` se elige en el formulario, pero el label `modo:` se pone al refinar.**
Sin ese label, una tarea no puede pasar a `Listo`. Las investigaciones no llevan modo.

## Campos del tablero

| Campo | Valores |
|---|---|
| Estado (en GitHub se llama `Status`) | `Backlog` → `Listo` → `En progreso` → `En revisión` → `Hecho` |
| Prioridad | `Alta`, `Media`, `Baja` |
| Tamaño | `S`, `M`, `L`, según las líneas de producción que va a revisar el autor: S hasta ~100, M hasta ~250 y L hasta ~400. Lo que pasa de L se parte (ADR-0033) |

## Definición de Listo y de Hecho

- **Listo:** tiene criterios de aceptación, label `modo:` (si es tarea), Tamaño, su
  sección "ADRs" y sus dependencias marcadas (si es tarea de código), y sus ADRs están
  `Aceptado`.
- **Hecho:** cumple los criterios de aceptación, está mergeado con `Closes #N`, y (si
  es tarea) el autor respondió "Qué debo poder explicar", recorriendo el
  diagrama del flujo cuando lo hay. Si el ticket cambia un flujo dibujado, o le da
  comportamiento a un sistema nuevo, su diagrama está al día (ADR-0030).

## Cómo se organiza el código

Lo decide el ADR-0033. El plan completo vive en el [roadmap](../plans/roadmap.md).

- **Un milestone por corte vertical:** M3 es el esqueleto andante y M4 a M7 son los
  cortes, cada uno con un criterio de salida que se juega y su tag (`v0.1.0` a
  `v0.5.0`).
- **Al abrir un milestone,** Claude parte sus filas del roadmap en átomos y propone el
  lote completo. El autor lo aprueba antes de que se cree ningún issue. Los milestones
  siguientes quedan como filas del roadmap, sin issues.
- **Cada tarea dice a qué ADRs pertenece,** en su sección "ADRs": uno o varios, y el
  primero es el dueño. Su área da el label `área:`, salvo en las tareas de contenido,
  que llevan `área:contenido`, y en las tareas cuyo código vive en `engine/platform` o
  `engine/core`, que llevan `área:core` sin importar el área de su ADR dueño: **manda la
  carpeta** (fe de erratas del ADR-0033, #153). Por ejemplo, el formato de asset vive en
  `engine/core` y lleva `área:core`, aunque su ADR dueño sea el de los assets. Si un ADR
  deja que el ticket decida la carpeta, el label se ajusta cuando el ticket la decide. Si
  una tarea escribe en varias carpetas, **manda la de la mayor parte de su código de
  producción**.
- **Las dependencias son las nativas de GitHub:** una tarea que necesita otra se crea con
  `--blocked-by`, y el tablero la marca como bloqueada.
- **Al cerrar un milestone,** el autor juega el tag y un PR actualiza el roadmap y los
  riesgos, antes de abrir el siguiente.

## Cómo hago…

Cada acción tiene su ruta en la web y su comando `gh` equivalente.

### …crear un issue

- **Web:** *Issues* → *New issue* → elegir el formulario (Épica, Tarea, Investigación
  o Bug).
- **gh:** `gh issue create --web` abre el selector de formularios en el navegador.
  Si creas el issue solo con `gh`, sin pasar por el formulario, tienes que poner los
  labels tú: `gh issue create --title "…" --label "tipo:tarea" --body "…"`.

### …agregar un sub-issue

- **Web:** abrir el issue padre → *Create sub-issue* (nuevo) o *Add existing issue*
  (uno que ya existe).
- **gh:** `gh issue create --parent 12` (nuevo) o `gh issue edit 34 --parent 12`
  (existente).

### …marcar una dependencia

- **Web:** en la barra lateral del issue, *Relationships* → *Mark as blocked by*.
- **gh:** `gh issue create --blocked-by 40,41` (nuevo) o
  `gh issue edit 42 --add-blocked-by 40` (existente).

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
   - va en `tests/regression/bug_<N>.cpp` y su suite se llama `Bug<N>_<Descripción>`
     (por ejemplo, `TEST(Bug123_ThirdCutLeavesHole, ...)`), con la URL del issue en un
     comentario;
   - verifica el comportamiento **correcto**, así que falla mientras el bug existe;
   - se registra en CTest con `mz_add_regression_test(BUG <N> XFAIL)` en
     `tests/regression/CMakeLists.txt`: `XFAIL` es `WILL_FAIL` (falla esperada), para
     que la suite siga en verde. Si solo falla con un *sanitizer* encendido, también
     `SANITIZER`. La función registra además `regression.bug<N>.selected`, que se pone en
     rojo si el filtro no encuentra el test (por ejemplo, con el número mal escrito);
   - si el bug hace caer el proceso (un `assert`, un *segfault*), la sentencia va dentro
     de `EXPECT_EXIT({ …; std::exit(0); }, ::testing::ExitedWithCode(0), "")`: CTest
     marca en rojo un proceso muerto por una señal aunque tenga `WILL_FAIL`, y así la
     caída queda en un proceso hijo;
   - su ruta y nombre van al campo Test de regresión.

   Estado: `Confirmado`.
3. **Arreglar.** Rama `<N>-descripcion` y estado `En progreso`. Cuando el arreglo
   funciona, el test **pasa de forma inesperada** y CTest lo marca. En el mismo PR se
   **quita `XFAIL`**: desde ahí es un test de regresión normal, que queda para
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
