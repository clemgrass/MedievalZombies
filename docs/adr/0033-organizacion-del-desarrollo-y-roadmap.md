# ADR-0033: Organización del desarrollo y roadmap

- **Estado:** Aceptado
- **Fecha:** 2026-11-09
- **Ticket:** #75
- **Reemplaza / Reemplazado por:** —

## Contexto y problema

Con este ADR se cierra M2: después viene el código. Ya existen **la mecánica de los
tickets** (ADR-0002 y `docs/process/tickets.md`) y **el método** (ADR-0014: esqueleto
andante, cortes verticales, *spikes*, Kanban y un tag por corte). La spec base dejó los
milestones de código "para cuando M1 esté cerrado", y cada ADR de sistema de M2 trae
su lista de "Tickets, en orden". Pero **nadie decidió cómo se convierte todo eso en un
plan**: qué se construye primero, en qué milestone, en qué tamaño de ticket y cómo se
sigue.

Al juntar el material aparecen huecos y tensiones:

| Qué | Dónde |
|---|---|
| **Solo los 11 ADRs de M2 tienen una lista de tickets** (118 en total). Los sistemas de M1, que son el corazón del juego (corte, animación, física, combate, cocinador, ECS, game loop, memoria, depuración, CI y las fundaciones), no tienen ninguna | ADR-0004 a 0021 y 0029 |
| **El 32 % de los tickets de M2 se comparte con otro ADR** ("junto al ADR-…"), y hay trabajo repetido entre listas | P1 de la [nota](../research/organizacion-del-desarrollo-y-roadmap.md) |
| **Las épicas:** `tickets.md` dice "épica = un área completa (Renderer)", pero en la práctica fueron una por milestone (#5 en M1, #35 en M2) | ADR-0002, `tickets.md` |
| **El tamaño del ticket:** `tickets.md` dice "de 1 a 3 sesiones de trabajo", sin una regla para partirlo ni un límite de lo que se puede revisar | `tickets.md` |
| **La CD llega "con el primer tag de corte vertical",** pero su ticket 3 trae la tabla propia de funciones de Vulkan, que hace falta para el primer píxel | ADR-0035 |
| **La vista Roadmap del Project** necesita campos de fecha, y el proyecto no tiene fechas | P6 de la nota |

**Lo que el autor ya fijó al abrir el ticket:** un issue que termina en un PR tiene que
ser **un átomo**. Un auto es demasiado grande, un motor todavía es grande, pero un
cilindro, aunque tenga piezas más chicas, ya es del tamaño justo: **entrega algo puntual,
que hace avanzar el juego y que es fácil de implementar.**

Hay que decidir:

1. **los milestones de código:** cuántos cortes, qué se juega al cerrar cada uno y su
   tag;
2. **las épicas:** por sistema, por corte, la matriz de los dos o ninguna;
3. **el ticket átomo:** la regla de tamaño y forma, qué significan S, M y L y si
   sobrevive el nivel "feature";
4. **cómo se parte un ADR en tickets:** quién, cuándo, cómo se marcan las dependencias
   y cómo apunta cada ticket a su ADR;
5. **los ADRs sin lista de tickets:** dónde se escriben sus listas y cuándo;
6. **el roadmap:** dónde vive y cómo se mantiene;
7. **los riesgos:** cómo se registran y cómo ordenan el trabajo;
8. **el contenido del corte vertical:** qué assets, de dónde y cuándo entran;
9. **la transición de M2 a M3:** qué cierra M2 y cuándo "empieza el código" (ADR-0014,
   regla 11);
10. **la revisión del plan:** cuándo se vuelve a mirar, sin ceremonias nuevas.

Lo que ya está decidido y limita este ADR:

- **ADR-0002:** GitHub Issues y Projects; épica → feature → tarea; los labels `tipo:`,
  `área:` y `modo:`; los campos `Status`, `Prioridad` y `Tamaño`.
- **ADR-0011:** TDD estricto en el núcleo (el corte, el cocinador, el ECS, la
  matemática); "tests después" en el resto.
- **ADR-0014:** esqueleto andante y después cortes verticales; Kanban sin límites por
  columna; todo entra por PR; un tag SemVer `0.y.z` por corte; los ADRs aceptados se
  corrigen como fe de erratas **solo mientras no exista código**.
- **ADR-0030 y el diagrama teórico:** la Definición de Hecho pide el diagrama al día.
- **ADR-0035:** la CD publica Linux y Windows desde el primer tag de corte vertical.
- **La visión:** el alcance del corte vertical objetivo (la arena, tres tipos de zombie,
  la horda y la pelea contra el alquimista).
- **`CLAUDE.md`:** Claude solo crea issues con la aprobación del autor; trabajo en piezas
  chicas; ritmo pausado.

**Fuera de alcance:**

| Qué | Lo decide |
|---|---|
| Las reglas de cada sistema | Su ADR |
| Las ramas, los commits y el merge | `docs/process/git.md` |
| El flujo de los bugs | ADR-0011 y `tickets.md` |
| Lo que la historia pide y ningún ADR cubre (el parry, la carga infinita, los avisos del tutorial, la IA del mini-jefe, el acompañante, el caballo) | Sus ADRs futuros, después del corte vertical |

## Criterios

Los cinco del ticket, más cuatro:

1. **Flujo simple** para una sola persona con un asistente (ADR-0014).
2. **Avance jugable en cada milestone:** cada uno termina en algo que se juega.
3. **Riesgos atacados temprano.**
4. **Trazabilidad:** de cada ticket a su ADR, y de vuelta.
5. **Aprendizaje:** gestión de producción como en la industria.
6. **Atomicidad:** la regla del autor.
7. **Revisable por el autor:** su revisión es el cuello de botella (ADR-0014).
8. **Encaje con lo decidido:** ADR-0002, 0011, 0014 y 0030.
9. **Sin fechas:** importa el orden, no el calendario.

## Opciones

### 1. Los milestones de código

| Opción | Qué significa | A favor | En contra |
|---|---|---|---|
| **Uno por corte vertical** (ADR-0014) | M3 = esqueleto andante; M4 en adelante, un corte cada uno, con su criterio de salida jugable y su tag | Lo que ya dice el ADR-0014. Cada milestone es una demo | Hay que decidir cuántos cortes y qué entra en cada uno |
| **Un solo milestone hasta el corte vertical objetivo** | M3 = esqueleto, M4 = todo lo demás | Menos planificación | Un milestone de ~150 tickets no se ve avanzar; contradice "un tag por corte" |
| **Por sistema** | M3 = renderer, M4 = física… | Cada sistema "termina" | Es la opción horizontal que el ADR-0014 descartó |

**El reparto medido** (P5 de la nota), con cada ticket en un milestone y validado contra
el grafo de dependencias:

| Milestone | Qué se juega al cerrarlo | Tickets |
|---|---|---|
| M3, esqueleto andante | Una malla estática cocinada desde Blender; una tecla la parte en dos por un plano fijo | 39 |
| M4, "el tajo" | La caballera apunta, carga y suelta; el maniquí con esqueleto se parte por donde apuntó y los pedazos caen con física | 58 |
| M5, "todo pesa" | Carne y metal: rebote, tajo rápido, hendidura, hoja clavada, pausa de impacto, ragdoll, sangre y sonido | 28 |
| M6, "la horda" | Zombies que persiguen y agarran en la arena, checkpoints, look PSX completo | 35 |
| M7, el corte vertical objetivo | El alquimista, la cámara del tajo, los menús, las opciones, los subtítulos y la accesibilidad | 30 |
| Producción | Varias zonas, el castillo y las cinemáticas: sin planificar | 14 |

**Dónde entra la CD:**

| Opción | M3 / M4 | A favor | En contra |
|---|---|---|---|
| **Con el primer corte (M4)**, como dice el ADR-0035 | 39 / 58 | Sin fe de erratas | M4 pesa el doble que los demás. Windows se ve tarde |
| **En el esqueleto (M3)** | 50 / 47 | Freeman y Pryce: el esqueleto andante **se despliega**. Adelanta el riesgo de Windows. Equilibra M3 y M4 | Fe de erratas en el ADR-0035. El esqueleto tarda más en cerrarse |

### 2. Las épicas

| Opción | Qué significa | A favor | En contra |
|---|---|---|---|
| **Épica = sistema** (lo que dice `tickets.md`) | "Renderer", "Corte"…, con sub-issues en varios milestones | Barra de progreso por sistema | Una épica que vive de M3 a M7 dice poco. Repite el label `área:`. Un ticket compartido solo tiene un padre |
| **Épica = corte** (lo que se hizo en M1 y M2) | Una épica por milestone | La costumbre del repo | Repite el milestone, que ya tiene su barra de progreso |
| **Matriz: épica = sistema, milestone = corte** | Lo que Jeff Patton llama *story map*: la columna vertebral de capacidades, cortada en franjas de release | La vista más rica: qué sistema avanza en qué corte | Dos jerarquías que mantener a mano |
| **Sin épicas en el código:** milestone = corte, `área:` = sistema, ADR en el cuerpo | El milestone y los labels ya dan las dos dimensiones de la matriz | Lo más simple. Nada repetido | Se pierde la barra de progreso por sistema (se filtra por label) |

### 3. El ticket átomo

**La regla de tamaño:**

| Opción | Qué mide |
|---|---|
| **Solo sesiones** (`tickets.md`: de 1 a 3 sesiones) | El tiempo de quien implementa. Con Claude escribiendo, mide poco |
| **Solo líneas** | El diff. Fácil de medir, fácil de cumplir partiendo mal |
| **Cuatro preguntas** (P3 de la nota): ¿entrega algo comprobable solo? ¿cabe en una revisión tranquila (del orden de 200 a 400 líneas de producción, sin los tests)? ¿es indivisible (partido, cada mitad todavía entregaría algo)? ¿no es demasiado chico? | La entrega y la revisión, que es lo que pide el autor. Viene de INVEST, de *Small CLs* de Google y del estudio de SmartBear con Cisco |

**Medido sobre dos listas reales:** al aplicar las cuatro preguntas, la del render pasa
de 11 a 13 tickets y la del corte, de 11 a 16. **Las listas crecen entre un 20 y un
45 %,** y su orden no siempre es el de construcción.

**El nivel "feature":**

| Opción | Jerarquía |
|---|---|
| **Se mantiene** | Épica → feature → tarea |
| **Se quita** | (Épica →) tarea. Cada ticket átomo es una tarea, y la "feature" es el ADR o el corte |

**El campo `Tamaño` (S, M, L):** sin uso, con el significado de hoy (sesiones) o
atado a la revisión (por ejemplo, S hasta ~100 líneas, M hasta ~250, L hasta ~400, y lo
que pase de L se parte).

### 4. Cómo se parte un ADR en tickets

| Tema | Opciones |
|---|---|
| **Quién** | Claude propone el lote y el autor lo aprueba antes de crear nada (`CLAUDE.md`). No hay otra opción real |
| **Cuándo** | **Todos ahora** (204 issues de una vez); **todos al abrir cada milestone**; o **justo a tiempo**, con *rolling wave planning*: el milestone en curso al detalle, el siguiente en el roadmap y el resto como lista |
| **Dependencias** | **Nativas de GitHub** (`gh issue create --blocked-by`, con el ícono "Blocked" en el tablero); solo en el texto del issue; o ninguna |
| **De ticket a ADR** | Una sección "ADRs" en el cuerpo del issue, que admite varios (el 32 % de los tickets tiene dos); el padre (una sola épica por ticket); un label por ADR (unos 35 labels); o un campo de texto en el Project |

### 5. Los ADRs sin lista de tickets

| Opción | Qué significa | En contra |
|---|---|---|
| **Fe de erratas en cada ADR**, con su lista | Cada ADR queda con su lista, como los de M2 | Dieciséis fe de erratas en un solo PR. La lista del ADR envejece apenas se parte en átomos |
| **La lista vive solo en el roadmap** | Los ADRs de M1 no cambian; el roadmap es el único lugar con todos los tickets | Un ADR de M1 no dice cómo se construye |
| **Se escribe al abrir cada milestone** | Justo a tiempo | Sin un plan completo antes de M3, la ruta crítica no se ve |

### 6. El roadmap

| Opción | A favor | En contra |
|---|---|---|
| **Un documento vivo en `docs/plans/`**, con los milestones, sus criterios de salida, la tabla de tickets con sus dependencias y el registro de riesgos | Se versiona y se revisa en un PR. Un evaluador lo lee sin GitHub | Hay que mantenerlo al día con los issues |
| **La vista Roadmap del Project** | Visual | **Necesita campos de fecha o de iteración** (P6): habría que inventar fechas o crear iteraciones sin duración real |
| **Una vista Tabla del Project agrupada por milestone** | Muestra los issues reales, con el ícono de bloqueado | Solo muestra lo que ya es issue |
| **Generado desde la tabla** (un script valida las dependencias, como el del P5) | Las dependencias no se pueden romper sin que falle algo | Una herramienta más que mantener |

### 7. Los riesgos

| Opción | Qué significa |
|---|---|
| **Un registro en el roadmap**, con el corte donde cada riesgo se ve primero | Lo que propone Boehm: ordenar por riesgo. Ver la tabla del P7 |
| **Un issue por riesgo** | Se ve en el tablero, pero un riesgo no es trabajo que se cierra |
| **Además, *spikes* antes del corte que los expone** (ADR-0014) | Por ejemplo, el *spike* de Windows del ADR-0035 |

Los ocho riesgos medidos: el corte en mallas reales, la sensación del tajo, el
contenido (el maniquí de Quaternius no pasa la validación *manifold* sin arreglarlo),
el presupuesto de GPU con 50 zombies, el determinismo, Windows, la IA de la horda y el
alcance para una persona.

### 8. El contenido del corte vertical

La lista completa está en el P7 de la nota: la caballera, cuatro tipos de zombie que se
cortan, el alquimista, la arena, las texturas, los sonidos, las fuentes y los textos.

| Tema | Opciones |
|---|---|
| **Qué es propio** (ADR-0017: mixto) | Solo lo que define al juego (la caballera, la espada, los zombies) o también lo genérico |
| **Cuándo entra cada asset** | En el corte que lo usa primero, como un ticket más; o un milestone de contenido aparte |
| **Su label** | No existe un `área:` para el contenido: se agrega uno (por ejemplo `área:contenido`, con su fe de erratas en `tickets.md`) o se usa el del sistema que lo consume |

### 9. La transición de M2 a M3

| Tema | Opciones |
|---|---|
| **Qué cierra M2** | Este ADR aceptado, la épica #35 y el milestone M2 cerrados, y la línea de estado de `CLAUDE.md` actualizada |
| **Cuándo "empieza el código"** (fin de las fe de erratas, ADR-0014 regla 11) | Al mergear el primer PR con código; al abrir M3; o al cerrar el esqueleto andante |
| **Lo pendiente de M2** | Las pruebas con el DualShock (ADR-0026 P6 y ADR-0032) quedan como tickets de M4 o M7, o como deuda de la investigación |

### 10. La revisión del plan

| Opción | Qué significa |
|---|---|
| **Nunca** | El roadmap se escribe una vez |
| **Al cerrar cada milestone** | Una revisión corta: se juega el tag, se actualizan el roadmap y los riesgos, y se planifica el siguiente milestone al detalle (*rolling wave*) |
| **Cada cierto tiempo** | Como Scrumban; el ADR-0014 lo descartó |

## Investigación

Los datos están en la
[nota de investigación](../research/organizacion-del-desarrollo-y-roadmap.md): un
prototipo en papel, con una tabla de **204 tickets** (los 118 de M2 y 86 escritos para
los ADRs que no tenían lista), un grafo de dependencias y un reparto en milestones que
un script valida. Lo principal:

- **El grafo no tiene ciclos.** La ruta crítica tiene 15 tickets en serie y pasa por el
  cocinador y el corte: **lo más arriesgado del proyecto está en la ruta crítica.** Un
  corte mínimo de una malla estática necesita 25 tickets; uno de un zombie con
  esqueleto, 34.
- **Lo que limita no son las dependencias, sino la revisión:** hay niveles del grafo con
  29 tickets que se podrían hacer en paralelo.
- **Las listas crecen entre un 20 y un 45 %** al partirlas en átomos.
- **El TDD estricto se concentra al principio** (17 a 20 tickets en M3 y M4, contra 4 a
  7 después).
- **GitHub:** dependencias nativas con `gh` 2.102; un solo padre por issue; la vista
  Roadmap necesita fechas; los tipos de issue no existen en un repo personal.

### Cómo lo hace la industria

- **Las etapas de un estudio** antes de producción son el prototipo, que se tira, y el
  **corte vertical**, una porción chica del juego con calidad final que decide si se
  pasa a producción (Clinton Keith, *Agile Game Development*; ya citado en el ADR-0014).
- **Esqueleto andante:** para Cockburn, "una implementación diminuta del sistema que
  realiza una pequeña función de punta a punta". Para Freeman y Pryce, la porción más
  delgada de funcionalidad real que se compila, **se despliega** y se prueba de forma
  automática.
- **Story mapping** (Jeff Patton, 2008): las capacidades del sistema en una fila (la
  columna vertebral) y las releases como franjas horizontales que toman un poco de cada
  una. La primera franja es el esqueleto andante.
- **INVEST** (Bill Wake, 2003) y **Small CLs** (Google): el tamaño de una unidad de
  trabajo se mide por lo que entrega y por lo que se puede revisar.
- **El estudio de SmartBear con Cisco** (2006, 2500 revisiones): la eficacia de la
  revisión cae por encima de 200 a 400 líneas.
- ***Rolling wave planning*** (PMBOK): lo cercano se planifica al detalle y lo lejano a
  grandes rasgos, y se refina a medida que se sabe más.
- **Desarrollo guiado por riesgos** (Barry Boehm, el modelo en espiral): primero lo que
  más puede hundir el proyecto.

## Decisión

Elijo un milestone por corte vertical, sin épicas en el código, tareas átomo y un
roadmap vivo que manda sobre las listas de cada ADR. El esqueleto andante es una malla
estática cocinada desde Blender que una tecla parte en dos: así el corte, que es el
riesgo más grande, aparece en el primer milestone, y la CD entra desde ese mismo
milestone para que el esqueleto también se despliegue.

| # | Dimensión | Elección |
|---|---|---|
| 1 | Milestones | **Uno por corte vertical, cada uno con su criterio de salida jugable y su tag:** **M3, esqueleto andante** (`v0.1.0`): una sala con una malla estática cocinada desde Blender; una tecla la parte en dos por un plano fijo y los pedazos se separan, sin física; la CI en verde y la CD deja el borrador de Linux y Windows. **M4, "el tajo"** (`v0.2.0`): la caballera apunta, carga y suelta, el maniquí con esqueleto se parte por donde apuntó y los pedazos caen con física. **M5, "todo pesa"** (`v0.3.0`): carne y metal, rebote, tajo rápido, hendidura, hoja clavada, pausa de impacto, ragdoll, sangre y sonido del corte. **M6, "la horda"** (`v0.4.0`): zombies que persiguen y agarran en la arena cocinada, checkpoints y el look PSX completo. **M7, el corte vertical objetivo** (`v0.5.0`): lo que pide la visión. Lo que viene después (varias zonas, el castillo, las cinemáticas) se planifica al cerrar M7. **La CD entra en M3**, no en el primer corte (fe de erratas en el ADR-0035) |
| 2 | Épicas | **No se usan en el código.** El milestone es el corte, y el label `área:` es el sistema: las dos dimensiones de la matriz, sin nada que mantener a mano. Los ADRs de cada ticket van en una sección de su cuerpo. `tipo:épica` queda solo para agrupar trabajo que no es código, como las épicas #5 y #35 |
| 3 | El ticket átomo | **Cuatro preguntas.** (1) ¿Entrega algo que se puede comprobar solo: un test que pasa, algo en pantalla, una regla del build que falla? (2) ¿Cabe en una revisión tranquila, del orden de 200 a 400 líneas de producción sin contar los tests? (3) ¿Es indivisible: partido en dos, alguna mitad dejaría de cumplir la (1)? (4) ¿No es demasiado chico: vale un PR? **Cada átomo es una `tarea`**: el nivel "feature" se quita (nunca se creó un issue `tipo:feature`). **`Tamaño` se ata a la revisión:** S hasta ~100 líneas de producción, M hasta ~250 y L hasta ~400. Lo que pasa de L se parte |
| 4 | De un ADR a sus tickets | **Al abrir cada milestone, Claude propone el lote completo de ese milestone** (los átomos, sus dependencias y sus ADRs) y el autor lo aprueba antes de crear ningún issue. **Las dependencias son las nativas de GitHub** (`--blocked-by`). **El cuerpo de cada tarea tiene una sección "ADRs"**, con uno o varios: el primero es el dueño, y su área da el label `área:`. El trabajo repetido entre listas es un solo ticket |
| 5 | Los ADRs sin lista | **La lista vive solo en el roadmap,** que es la fuente única del orden. Los ADRs de M1 no cambian, y las listas de los ADRs de M2 quedan como la intención original de cada uno |
| 6 | El roadmap | **Un documento vivo, `docs/plans/roadmap.md`:** cada milestone con su criterio de salida, su tag y su tabla de tickets (con sus ADRs, sus dependencias y el número de issue cuando existe), y el registro de riesgos. **El milestone en curso va en átomos; los siguientes, como filas del inventario** (*rolling wave planning*). En el Project, **una vista Tabla agrupada por milestone**, sin la vista Roadmap ni fechas inventadas |
| 7 | Los riesgos | **Un registro en el roadmap,** con el corte donde cada riesgo se ve primero, cómo se ataca y su estado. Los riesgos ordenan los cortes. **Un *spike* solo cuando un riesgo pide una respuesta antes de su corte**, como el de Windows del ADR-0035 |
| 8 | El contenido | **Cada asset entra como una tarea en el corte que lo usa primero,** con un label nuevo, **`área:contenido`**. M4 empieza con el maniquí CC0 de Quaternius, arreglado en Blender para que pase la validación *manifold* (así el riesgo del contenido aparece temprano). La caballera, la espada y los zombies propios llegan a lo largo de los cortes, hasta M7. **Mientras tanto, los tres tipos de zombie de M5, el guardaespaldas y el alquimista usan el mismo maniquí CC0, con su propio material por cápsula** (carne, cota o placa), para probar las reglas sin esperar el arte. El diseño de los propios se decide al abrir el milestone que los usa. Los sonidos del corte, como dice el ADR-0024 |
| 9 | La transición | **M2 se cierra con el merge del PR de este ADR:** la épica #35 y el milestone M2 se cierran, y `CLAUDE.md` pasa a decir M3. **"Empieza el código" cuando se mergea el primer PR con código:** desde ahí rige la inmutabilidad estricta del ADR-0014, y un cambio de decisión pide un ADR nuevo. Las pruebas pendientes con el DualShock (ADR-0026 y ADR-0032) son tareas de M7 |
| 10 | La revisión del plan | **Al cerrar cada milestone,** en el mismo PR que actualiza el roadmap: el autor juega el tag, se actualizan los riesgos y el roadmap, y Claude prepara el lote del milestone siguiente. Sin otras ceremonias (ADR-0014) |

## Consecuencias

### El camino de un milestone

```
abrir el milestone
  → Claude parte sus filas del roadmap en átomos (las cuatro preguntas)
  → el autor aprueba el lote → los issues, con --blocked-by y su sección "ADRs"
  → Kanban: un átomo, una rama, un PR (ADR-0014), con la auditoría antes de cada PR
  → milestone vacío: tag v0.N.0 → la CD deja el borrador → el autor juega el tag
  → PR de cierre: roadmap y riesgos al día → abrir el siguiente
```

### Lo que se gana

- **El riesgo más grande aparece primero:** el corte de una malla estática cierra M3, y
  el de un zombie con esqueleto, M4.
- **Cada milestone termina en algo que se juega,** con su tag y, desde M3, con su
  borrador para Linux y Windows.
- **El tamaño de cada PR lo fija la revisión del autor,** que es el cuello de botella.
- **Un solo lugar para el orden** (el roadmap), y la trazabilidad de cada tarea a uno o
  varios ADRs.
- **Nada se planifica de más:** solo el milestone en curso va en átomos.

### Lo que se sacrifica

- **M3 tarda más en cerrarse:** además del esqueleto, lleva la CD y el port de
  `platform::` a Windows.
- **No hay barra de progreso por sistema:** se filtra el tablero por `área:`.
- **Más tickets:** las listas crecen entre un 20 y un 45 % al partirlas en átomos.
- **Las listas de los ADRs de M2 envejecen:** el roadmap manda, y no se corrigen.
- **El roadmap se mantiene a mano,** en el PR que abre o cierra cada milestone.

### Fe de erratas

| Documento | Qué cambia |
|---|---|
| ADR-0002 | La jerarquía pasa a épica (solo fuera del código) → tarea. Los milestones de código son cortes |
| ADR-0014 | La Decisión 6: también lleva tag el esqueleto andante (`v0.1.0`), porque la CD entra en él. Las Decisiones 1.2 y 1.3: el label `modo:` y "Qué debo poder explicar" son de cada tarea, porque el nivel "feature" ya no existe |
| ADR-0035 | Sus tickets llegan con el esqueleto andante (M3), no con el primer corte. La tabla propia de funciones de Vulkan pasa a la base de Vulkan del esqueleto |
| Visión | La CD publica desde el esqueleto andante |
| `docs/process/tickets.md` | La jerarquía, el átomo, `Tamaño`, las épicas, la sección "ADRs", las dependencias nativas, `área:contenido` y la Definición de Listo |
| `.github/ISSUE_TEMPLATE/` | Se quita el formulario Feature. La tarea cambia su descripción (el átomo) y su campo "ADR relacionado" pasa a "ADRs" (uno o varios). La épica dice que es solo para lo que no es código |
| `tools/github/` | El label `área:contenido`, y fuera `tipo:feature` |

### Archivos nuevos

- **`docs/plans/roadmap.md`,** en este mismo PR: los cinco milestones con sus criterios
  y sus tags, las filas del inventario de la nota repartidas por milestone y el registro
  de riesgos. Partir M3 en átomos es el primer paso de M3, después del merge.
- **La caja del ADR-0033** en la franja de proceso del diagrama teórico.
- **El tema de proceso** de la skill de auditoría suma este ADR.

### Abierto

- **Los umbrales de líneas** (S, M y L): se revisan con los primeros PRs reales de M3.
- **El diseño de los zombies propios:** cuántos modelos, y si el guardaespaldas y el
  alquimista son modelos propios o variantes de otro. Se decide al abrir M5 y M7. Hasta
  entonces, el maniquí CC0 con material por cápsula.
- **Un script que valide las dependencias del roadmap,** como el del prototipo, si la
  tabla a mano empieza a fallar.
- **Los milestones de producción,** que se planifican al cerrar M7, junto con lo que la
  historia pide y ningún ADR cubre.
- **Si se publica cada tag:** la CD deja un borrador, y el autor decide si lo publica
  (ADR-0035). Publicar el esqueleto andante no es obligatorio.

## Cómo lo defiendo

**¿Por qué no planificaste todo el juego en detalle desde el principio?**
Porque lo lejano se planifica mal. Hice el inventario completo (unos 200 tickets, con
sus dependencias) para ver la ruta crítica y los riesgos, pero solo el milestone en
curso se parte en tareas y se convierte en issues. Lo que aprenda en M4 cambia M5, y
así no hay decenas de issues que corregir. En gestión de proyectos se llama *rolling
wave planning*.

**¿Qué es tu esqueleto andante y por qué ese?**
Una malla estática que sale de Blender, pasa por el cocinador, se dibuja con Vulkan y se
parte en dos con una tecla, con la CI y la CD funcionando. Atraviesa todas las capas
del engine con lo mínimo, y además toca lo más arriesgado del proyecto, que es el
corte. Si el corte tiene un problema de fondo, lo veo en el primer milestone y no
después de meses.

**¿Por qué la CD entra en el esqueleto y no en el primer corte?**
Porque un esqueleto andante, según Freeman y Pryce, se compila, se despliega y se prueba
de punta a punta. Si no se despliega, le falta una capa. Además, Windows es un riesgo: no
tengo una máquina con Windows y el port de `platform::` es trabajo de verdad. Prefiero
descubrirlo al principio. Y equilibra los milestones: M3 y M4 quedan con unas 50 filas del
inventario cada uno, en vez de 39 y 58 (al partirlas en átomos, son más tareas).

**¿Qué es un ticket "átomo"?**
Una tarea que termina en un PR que entrega algo que se puede comprobar solo y que yo
puedo revisar con calma. El ejemplo que uso es el de un auto: es demasiado grande, el
motor todavía es grande, pero un cilindro ya tiene el tamaño justo. Lo pruebo con cuatro
preguntas: si entrega algo, si cabe en una revisión, si se puede partir en dos piezas
que entreguen algo cada una y si no es demasiado chico.

**¿Por qué 200 a 400 líneas?**
Porque yo reviso todo el código, y mi revisión es el cuello de botella. El estudio de
SmartBear con Cisco mostró que, por encima de ese rango, una revisión encuentra cada vez
menos defectos, y la guía de Google pide cambios chicos y autocontenidos. Es un orden de
magnitud, no una ley: lo voy a revisar con los primeros PRs reales.

**¿Por qué no usas épicas en el código?**
Porque no agregan nada. El milestone ya dice en qué corte va una tarea, y el label de
área dice de qué sistema es: esas son las dos dimensiones. Además, un tercio de las
tareas pertenece a dos ADRs, y en GitHub un issue tiene un solo padre. Con épicas por
sistema, tendría que elegir uno y perder el otro.

**¿Cómo sabes de dónde sale cada tarea?**
Cada tarea tiene una sección "ADRs" con uno o varios, y el primero es el dueño. Y el
roadmap tiene una fila por trabajo, con sus ADRs, de qué depende y el número del issue.
Se puede ir del ADR a la tarea y de la tarea al ADR.

**¿Qué riesgo te preocupa más?**
El corte en mallas reales, y por eso está en el esqueleto. Le sigue la sensación del
tajo: eso no se mide en papel, solo jugando. Por eso todo lo que da esa sensación es
ajustable, y M4 termina justo cuando se puede jugar. También el contenido: el maniquí
gratis que uso no pasa la validación de malla cerrada, así que lo arreglo en el primer
corte.

**¿Por qué no usaste la vista Roadmap de GitHub?**
Porque esa vista pone cada tarea en una línea de tiempo, y necesita fechas. Este
proyecto no tiene fechas: importa el orden, no el calendario. Inventar fechas sería
mentirle al tablero. El roadmap es un documento en el repo, y el tablero agrupa por
milestone.

**¿Por qué el orden de construcción no es el de las listas de cada ADR?**
Porque cada ADR escribió su lista pensando en su sistema. Al juntarlas aparecieron
dependencias cruzadas, trabajo repetido y cosas fuera de orden: por ejemplo, el render
pone el blit sexto, pero sin él no se ve nada. El roadmap es el único lugar que mira
todo junto, así que es el que manda.

**¿Cuándo dejas de corregir los ADRs?**
Cuando se mergea el primer PR con código. Hasta ahí, corregir un ADR es una fe de
erratas. Después, cambiar una decisión pide un ADR nuevo que reemplace al anterior.

**¿Cómo sabes si el plan sigue sirviendo?**
Al cerrar cada milestone juego el tag, reviso los riesgos y actualizo el roadmap antes
de abrir el siguiente. No hay otras reuniones ni ceremonias: es un proyecto de una
persona.

## Referencias

- Alistair Cockburn, *Walking Skeleton*, citado en:
  <https://www.mattblodgett.com/2020/09/start-with-walking-skeleton.html>
- Steve Freeman y Nat Pryce, *Growing Object-Oriented Software, Guided by Tests*
  (Addison-Wesley, 2009), cap. 4 y 10:
  <https://www.oreilly.com/library/view/growing-object-oriented-software/9780321574442/>
- Jeff Patton, *The new user story backlog is a map* (2008):
  <https://jpattonassociates.com/the-new-backlog/>
- Bill Wake, *INVEST in Good Stories, and SMART Tasks* (2003):
  <https://xp123.com/invest-in-good-stories-and-smart-tasks/>
- Google, *Small CLs*:
  <https://google.github.io/eng-practices/review/developer/small-cls.html>
- SmartBear, *Best Practices for Code Review* (el estudio con Cisco):
  <https://smartbear.com/learn/code-review/best-practices-for-peer-code-review/>
- *The Largest Case Study of Code Reviews—Ever*:
  <https://agileconnection.com/article/largest-case-study-code-reviews-ever>
- *Rolling Wave Planning*:
  <https://thedigitalprojectmanager.com/project-management/rolling-wave-planning/>
- Barry Boehm, *A Spiral Model of Software Development and Enhancement*, IEEE Computer
  21(5), 1988.
- Clinton Keith, *Agile Game Development: Build, Play, Repeat*, 2.ª ed. (2020):
  <https://www.informit.com/title/0136527817>
- GitHub Docs, *Creating issue dependencies*:
  <https://docs.github.com/en/issues/tracking-your-work-with-issues/using-issues/creating-issue-dependencies>
- GitHub Docs, *Adding sub-issues*:
  <https://docs.github.com/en/issues/tracking-your-work-with-issues/using-issues/adding-sub-issues>
- GitHub Docs, *Customizing the roadmap layout*:
  <https://docs.github.com/en/issues/planning-and-tracking-with-projects/customizing-views-in-your-project/customizing-the-roadmap-layout>
