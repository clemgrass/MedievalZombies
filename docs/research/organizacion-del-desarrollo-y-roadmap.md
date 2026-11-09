# Prototipo en papel de la organización del desarrollo

Nota de investigación del [ADR-0033](../adr/0033-organizacion-del-desarrollo-y-roadmap.md).
Guarda los datos que sostienen la decisión, para poder repetirlos.

Fecha: 2026-11-09. **No hay código del juego todavía**, así que el prototipo de este ADR
no es un programa. Es el **plan armado en borrador y medido con dos scripts**: una tabla
con todos los tickets que piden los ADRs aceptados, sus dependencias y un reparto en
milestones. Los scripts verifican que el reparto respete las dependencias. Todo vive
fuera del repo; lo que entra al repo es lo que el autor decida.

| Paso | Qué |
|---|---|
| P1 | Inventario de las listas "Tickets, en orden" de los ADRs |
| P2 | Listas para los ADRs que no tienen una |
| P3 | Prueba del "átomo" sobre dos listas reales |
| P4 | Grafo de dependencias: ciclos, ruta crítica y anchura |
| P5 | Esqueleto andante y cortes, validados contra el grafo |
| P6 | Mecánica de GitHub, solo lectura (sin crear issues) |
| P7 | Registro de riesgos y lista de contenido del corte vertical |

## P1. Inventario

**Solo los 11 ADRs de M2 tienen una lista de tickets en orden,** con 118 tickets en
total:

| ADR | Tickets | ADR | Tickets |
|---|---|---|---|
| 0022 render | 11 | 0027 niveles | 12 |
| 0023 IA | 11 | 0028 recursos | 11 |
| 0024 audio | 11 | 0031 cámara | 10 |
| 0025 VFX | 10 | 0032 input | 11 |
| 0026 UI | 12 | 0034 cinemáticas | 10 |
| | | 0035 CD | 9 |

**38 de esos 118 (el 32 %) dicen "junto al ADR-…":** el mismo trabajo pertenece a dos
ADRs. Los que más aparecen como pareja son el 0018 (combate) y el 0021 (depuración),
con 6 cada uno. Un ticket, entonces, puede apuntar a más de un ADR.

**Hay trabajo repetido entre listas,** que tiene que ser un solo ticket:

| Trabajo | Aparece en |
|---|---|
| El culling por zonas visibles | 0022 (ticket 7) y 0027 (ticket 11) |
| La navmesh de la arena en el cocinador | 0023 (ticket 2) y 0027 (ticket 3) |
| Grabar las acciones en el *replay* | 0021 (sin lista) y 0032 (ticket 9) |
| La recarga en caliente de la tabla de sonidos | 0024 (ticket 11) y 0028 (ticket 10) |

**Y hay una dependencia escondida:** la tabla propia de funciones de Vulkan
(`VK_NO_PROTOTYPES`) está en el ticket 3 del ADR-0035, que "llega con el primer tag de
corte vertical". Pero sin esa tabla no se dibuja el primer píxel: tiene que estar en el
esqueleto andante.

## P2. Las listas que faltan

Los ADRs de M1 decidieron los sistemas centrales sin una lista de tickets: el corte
(0007), la animación (0015), la física (0016), el combate (0018), el cocinador (0017),
el ECS y el game loop (0012), la memoria (0010), la depuración (0021) y la CI (0029).
Tampoco las fundaciones: el build (0006), las convenciones (0013), el testing (0011),
las utilidades (0009), la matemática (0008) y `platform::` (0004). **Es justo el
corazón del juego.**

El borrador de listas suma **86 tickets** más, armados desde la Decisión y las
Consecuencias de cada ADR. Por ejemplo, el del corte sigue el orden de su lista de tests
(clasificar → contornos → tapa → des-deformar → pedazos → hendidura → volver a cortar).
Con eso, el inventario completo tiene **204 tickets**.

Lo que salió al armarlas:

- **El combate (0018) no tiene ningún ticket con TDD estricto:** son reglas de juego,
  "tests después" según el ADR-0011. El corte, el cocinador y el ECS sí lo llevan casi
  en todo (12 de 14, 8 de 8 y 4 de 4).
- **El corte de una malla estática no necesita esqueletos.** El índice soldado, las
  cáscaras y la validación *manifold* (ADR-0007) solo piden el cocinador mínimo. Los
  huesos dominantes y la unión de pedazos por hueso sí piden el esqueleto. Partir esos
  dos tickets en "estático" y "con hueso" acorta la ruta hasta el primer corte (ver P4).
- **Ocho tickets son paneles de ImGui** o contadores de un sistema (VFX, IA, audio, UI,
  recursos, input, cinemáticas y la cámara libre). Son chicos y casi iguales entre sí.

## P3. La prueba del átomo

El autor pidió, al abrir el ticket, que **un issue sea un átomo**: una pieza que entrega
algo puntual, que hace avanzar el juego y que es fácil de implementar. Su ejemplo: un
auto es demasiado grande, un motor todavía es grande, un cilindro ya es del tamaño justo.

La industria tiene dos formulaciones de la misma idea:

- **INVEST** (Bill Wake, 2003): una historia tiene que ser *Independent, Negotiable,
  Valuable, Estimable, Small, Testable*. Wake compara la historia con un pastel de
  capas: se corta **en vertical, atravesando todas las capas**, y no capa por capa.
- **Small CLs** (Google): "el tamaño correcto de un CL es un cambio autocontenido", que
  hace una sola cosa e incluye sus tests. 100 líneas suele estar bien y 1000 suele ser
  demasiado.

Y un dato sobre el revisor: en el estudio de SmartBear con el equipo de Cisco (2006,
2500 revisiones), **la densidad de defectos encontrados cae mucho por encima de 200 a
400 líneas por revisión**, y por encima de unas 500 líneas por hora. En este proyecto,
el revisor es el autor (ADR-0014): el tamaño del ticket se mide contra su revisión.

La regla de prueba, en cuatro preguntas:

1. **¿Entrega algo que se puede comprobar solo?** Un test que pasa, algo que se ve en
   pantalla o una regla del build que falla.
2. **¿Cabe en una revisión tranquila?** Del orden de 200 a 400 líneas de código de
   producción, sin contar los tests.
3. **¿Es indivisible?** Si se parte en dos y **cada mitad sigue cumpliendo la pregunta
   1**, es divisible: se parte.
4. **¿No es demasiado chico?** Si una mitad no entrega nada que se pueda comprobar,
   no se parte: el átomo es la pieza más chica que todavía vale un PR.

Aplicada a dos listas reales, con un tamaño estimado (no hay código: las líneas son un
orden de magnitud, sacado de implementaciones conocidas y de los prototipos):

| Lista | Antes | Después | Qué cambió |
|---|---|---|---|
| ADR-0022 (render) | 11 | 13 | La malla con reverse-Z se parte (destino de 640×360 con profundidad / *pipeline* y malla). ImGui se separa del dibujo de depuración. La mitad del culling por zonas se va al ticket del ADR-0027. **El blit (ticket 6) va primero:** sin él no se ve nada |
| ADR-0007 (corte, con lo suyo del cocinador) | 11 | 16 | Los datos del corte en el cocinador se parten en dos (estático / con hueso). La tapa se parte (orejas sin agujeros / con agujeros). Los pedazos se parten (conexos / unidos por hueso). El corte de punta a punta se parte (pedazos al ECS / cápsulas recortadas e índice extendido). La hendidura se separa de la cuña |

**Las listas crecen entre un 20 y un 45 %** al llevarlas a átomos, y **el orden de una
lista no siempre es el orden de construcción**: la lista del ADR-0022 sigue la lógica
del frame, no la de "qué hace falta para ver algo". Ninguno de los 22 tickets probados
salió demasiado chico. Los candidatos a "demasiado chico" son los ocho paneles del P2.

## P4. El grafo de dependencias

Un script lee la tabla (`id`, ADR, TDD, dependencias, título) y calcula el grafo:

- **204 tickets, ninguna dependencia rota, ningún ciclo.**
- **La ruta crítica tiene 15 tickets en serie,** y pasa por el cocinador y el corte:

  ```
  build → tests → utilidades → formato de asset → cocinador mínimo → datos del corte
  → clasificar → contornos → tapa → pedazos conexos → pedazos por hueso
  → corte completo → fuerza del combate → pausa de impacto → reloj por personaje
  ```

- **Lo más arriesgado del proyecto (el corte) está en la ruta crítica y lejos del
  inicio.** Con los tickets completos de cada lista, el primer corte de un zombie con
  esqueleto (`CT6`) necesita **34 tickets** terminados antes. Un corte mínimo de una
  malla estática, con una tecla y un plano fijo, necesita **25**.
- **Lo que más bloquea** (cuántos tickets dependen directo de él): la primera malla en
  pantalla (12), la capa de acciones del input (11), el corte completo (10) y el
  cocinador mínimo (8).
- **La anchura llega a 29 tickets** que se podrían hacer en paralelo en un mismo nivel
  del grafo. Para una persona con un asistente, **lo que limita no son las
  dependencias, sino la revisión** (ADR-0014).

## P5. El esqueleto andante y los cortes

Primero, la definición. **Alistair Cockburn:** "una implementación diminuta del sistema
que realiza una pequeña función de punta a punta". **Steve Freeman y Nat Pryce**
(*Growing Object-Oriented Software, Guided by Tests*, cap. 4) agregan que esa porción,
la más delgada posible de funcionalidad real, **se compila, se despliega y se prueba de
punta a punta de forma automática**. Para ellos, el despliegue es parte del esqueleto.
**Jeff Patton** (*story mapping*, 2008) lo dibuja como la primera franja horizontal de
un mapa: debajo de la "columna vertebral" de capacidades, cada franja es una release
que toma un poco de cada columna.

Con el grafo, un reparto posible (cada ticket a un milestone, sin que nadie dependa de
algo que llega después):

| Milestone | Qué se juega al cerrarlo | Tickets | TDD estricto |
|---|---|---|---|
| M3, esqueleto andante | Una sala con una malla estática cocinada desde Blender. Una tecla la parte en dos por un plano fijo, y los pedazos se separan. CI en verde | 39 | 17 |
| M4, corte 1: "el tajo" | La caballera apunta, carga y suelta. El maniquí con esqueleto se parte por donde apuntó y los pedazos caen con física | 58 | 20 |
| M5, corte 2: "todo pesa" | Carne y metal: rebote, tajo rápido, hendidura, hoja clavada, pausa de impacto, ragdoll, sangre y sonido del corte | 28 | 5 |
| M6, corte 3: "la horda" | Zombies que caminan, persiguen y agarran, en la arena cocinada, con checkpoints y el look PSX completo | 35 | 7 |
| M7, corte 4: el corte vertical objetivo | El alquimista y su guardaespaldas, la cámara del tajo, los menús, las opciones, los subtítulos y la accesibilidad | 30 | 4 |
| Producción (sin planificar) | Varias zonas, el castillo, las cinemáticas | 14 | 1 |

**El script lo valida: 0 errores.** Dos cosas salen del reparto:

- **M4 pesa el doble que los demás,** y 9 de sus 58 tickets son de la CD: el ADR-0035
  los trae "con el primer tag de corte vertical". **Si la CD entra en el esqueleto**
  (con el paquete del ejecutable que necesita), **M3 queda en 50 y M4 en 47**, y el
  script también lo valida. Es lo que pide la definición de Freeman y Pryce (el
  esqueleto se despliega), y adelanta el riesgo de Windows. Cambiarlo exige una fe de
  erratas en el ADR-0035.
- **El TDD estricto se concentra al principio** (17 a 20 tickets en M3 y M4, contra 4 a
  7 después): las fundaciones, el cocinador y el corte son núcleo (ADR-0011). Los
  primeros milestones son más lentos por ticket.

## P6. La mecánica de GitHub

Solo lectura: no se creó ni se editó ningún issue.

| Qué | Hallazgo |
|---|---|
| **Dependencias** | `gh` 2.102 las maneja nativas: `gh issue create --blocked-by` y `gh issue edit --add-blocked-by`. En el tablero, un issue bloqueado lleva el ícono "Blocked". `gh issue view` ya muestra los campos `blocked-by` y `blocking` en este repo |
| **Sub-issues** | Hasta 100 por padre y 8 niveles. **Un issue tiene un solo padre:** un ticket compartido entre dos ADRs no puede colgar de dos épicas |
| **Vista Roadmap** | Ubica cada ítem con **campos de fecha o de iteración**. Los milestones aparecen solo como líneas verticales de marca. **El Project no tiene campos de fecha ni de iteración,** y el proyecto no tiene fechas |
| **Tipos de issue** | `issueTypes` da `null`: no existen en un repo personal. El tipo sigue siendo el label `tipo:` |
| **Campos del Project** | `Status`, `Prioridad` y `Tamaño` (S, M, L), más los de GitHub (`Milestone`, `Parent issue`, `Sub-issues progress`, `Labels`) |
| **Épicas hoy** | **En la práctica, épica = milestone:** #5 fue la épica de M1 y #35 es la de M2. `tickets.md` dice otra cosa: "Épica: un área completa (Renderer, Corte de mallas)" |

## P7. Riesgos y contenido

### Registro de riesgos

Barry Boehm (el modelo en espiral, 1988) propone ordenar el trabajo por riesgo: lo que
más puede hundir el proyecto se ataca primero, cuando todavía es barato cambiar de
rumbo.

| # | Riesgo | Dónde se ve primero | Ya medido |
|---|---|---|---|
| 1 | **El corte en mallas reales:** casos degenerados, LBS, determinismo | M3 (estático) y M4 (con esqueleto) | Prototipo del ADR-0007: 1975/2000 planos al azar con todos los pedazos cerrados |
| 2 | **La sensación del tajo:** que pese y no sea flácido (ADR-0031) | M4 | No se puede medir antes de jugarlo |
| 3 | **El contenido:** mallas cerradas y *manifold* para cortar | M4, con el primer zombie | El maniquí de Quaternius tiene 61 cáscaras encajadas y una arista del torso con más de dos caras: no pasa la validación sin arreglarlo en Blender |
| 4 | **El presupuesto de GPU en la UHD 620** con 50 zombies y 100 pedazos | M6 (la horda); antes, con una escena de estrés | Prototipos de los ADR-0022 y 0025 |
| 5 | **El determinismo del *replay*** entre compiladores y builds | M3 (hash del ECS) y M4 (*replay* en la CI) | Varios prototipos: el mismo hash con Clang y GCC de -O0 a -O3 |
| 6 | **Windows:** el port de `platform::` y llvm-mingw | M3 si la CD entra al esqueleto; si no, M4 | Prototipo del ADR-0035, bajo Wine |
| 7 | **La IA de la horda:** el costo de ORCA y que los agarres sean justos | M6 | Prototipo del ADR-0023: 200 zombies en 1,0 ms p99 |
| 8 | **El alcance y el cuello de botella de la revisión** para una persona | Siempre | El P4: hasta 29 tickets en paralelo y un solo revisor |

### El contenido del corte vertical

La visión fija el alcance: la arena, tres tipos de zombie en la primera oleada, la horda
y la pelea contra el alquimista. El origen sigue el ADR-0017 (terceros CC0 o CC BY para
empezar y para lo genérico, propios para lo que define al juego) y el ADR-0019.

| Asset | Qué incluye | Exigencia | Origen posible |
|---|---|---|---|
| **La caballera** | Malla, rig (~23 huesos, ADR-0017), espada gruesa. Clips: quieta, caminar, paso lateral lento, esquivar, postura de carga, recuperación, aturdida, agarrada, muerte | No se corta | Maniquí CC0 de Quaternius para empezar; la espada y la armadura, propias |
| **Zombie campesino, con cota y con armadura completa** | Malla y material por cápsula. Clips: caminar, trotar, atacar, agarrar, cojear, arrastrarse (con dos brazos y con uno), ensartado | **Se cortan:** cáscaras cerradas y *manifold* (ADR-0007) | Propio (provisional, según el ADR-0033: el maniquí CC0 con material por cápsula) |
| **El guardaespaldas** | Armadura máxima, mismos clips | Se corta | Propio (ídem) |
| **El alquimista** | Malla y clips: invocar, aturdido | Se corta | Propio (ídem) |
| **La arena** | Ruinas tipo coliseo, colisión con prefijos (ADR-0017), hasta 16 antorchas, marcadores de aparición | La navmesh sale de su colisión (ADR-0023) | Propia, con texturas CC0 |
| **Texturas** | PNG de baja resolución y la textura de detalle de la carne de la tapa (ADR-0022) | Estilo PSX | Propias o CC0 |
| **Sonidos** | Los del corte por capa (carne, hueso, metal), el rebote, pasos, gruñidos, agarre y la música | — | Los del corte, grabados por el autor o CC0; el resto, CC0, CC BY o síntesis propia (ADR-0024) |
| **Fuentes y textos** | Jersey 10 y Jacquard 12, tablas en español e inglés | OFL (ADR-0019) | Ya elegidas (ADR-0026) |

## Lo que el prototipo no prueba

- **Las líneas por ticket son estimaciones,** no mediciones: no existe código. La primera
  vez que un ticket real se pase de 400 líneas, la regla se revisa con datos.
- **No mide la velocidad.** Cuántos tickets por semana salen, con TDD estricto y la
  revisión del autor, recién se sabe en M3. Por eso el reparto no tiene fechas.
- **Las listas del P2 son un borrador:** las escribió Claude desde los ADRs, y el autor
  no las revisó una por una.
- **No se creó ningún issue** en GitHub: la mecánica del P6 sale de la documentación y
  de `gh --help`.

## Cómo repetirlo

Los scripts vivieron en el scratchpad de la sesión y no se guardaron: son unas 150
líneas de Python sin dependencias, fáciles de rehacer. **Los datos sí están en el
repo:** las tablas del [roadmap](../plans/roadmap.md) tienen las mismas columnas que la
tabla del prototipo (fila, ADRs, de qué depende, TDD), ya con el trabajo repetido
juntado.

- **El grafo** lee una fila por ticket y calcula:
  - las dependencias rotas (las que nombran una fila que no existe);
  - los ciclos, con una búsqueda en profundidad;
  - la profundidad de cada fila (el camino más largo desde una raíz) y la ruta crítica
    (el camino más largo de todos);
  - la anchura por nivel (cuántas filas tienen la misma profundidad);
  - cuántas filas dependen directo de cada una;
  - la clausura de una fila: todo lo que tiene que estar hecho antes.
- **El reparto** lee qué filas van a cada milestone y falla si una fila no tiene
  milestone, si aparece dos veces o si depende de una que llega en un milestone
  posterior. También cuenta las filas y el TDD estricto de cada milestone.
- **El generador del roadmap** ordena las filas de cada milestone de forma topológica
  (primero las que no dependen de nada dentro del milestone).
