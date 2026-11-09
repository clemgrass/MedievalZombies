# Temas de auditoría

Cada tema es una parte de la auditoría, con sus ADRs, las rutas que lo activan y una
guía de qué mirar. Un solo auditor (`auditor-adr`) recorre los temas activos, uno a la
vez. **La guía orienta, pero manda el texto del ADR.**

## Índice

| Tema | ADRs | Se activa cuando el PR toca… |
|---|---|---|
| [proceso](#proceso) | 0001, 0002, 0014, 0020, 0033 | **Siempre** |
| [docs-y-adrs](#docs-y-adrs) | 0001, 0014 (reglas 8 y 11), `CLAUDE.md` | **Siempre** |
| [cpp-y-convenciones](#cpp-y-convenciones) | 0003, 0009, 0013, 0014 (reglas 8 y 10) | `*.cpp`, `*.hpp`, `*.h`, `*.inl`, `.clang-format`, `.clang-tidy`, `.editorconfig` |
| [arquitectura](#arquitectura) | 0004, 0012, 0016 (consultas directas), 0020 | `engine/**`, `game/**`, cualquier `CMakeLists.txt`, `docs/performance/**` |
| [memoria](#memoria) | 0010 | `engine/core/**`, `engine/gpu/**`, o código que reserve, libere o guarde memoria (arenas, pools, allocators, contenedores) |
| [build-y-licencias](#build-y-licencias) | 0006, 0013 (warnings), 0019, 0008 y 0009 (flags), 0035 (los builds de la CD: SDL3 estática, el loader de Vulkan en runtime, `-ffile-prefix-map`) | `CMakeLists.txt`, `CMakePresets.json`, `cmake/**`, `LICENSE*`, `LICENSES/**`, `REUSE.toml`, `CREDITS.md`, `assets/**`, `tools/third_party/**`, cualquier dependencia nueva |
| [graficos](#graficos) | 0005, 0007 (frames en vuelo y subida de los pedazos), 0008 (proyección y buffers), 0010 (tamaño del *ring buffer*), 0022 (pipeline de render), 0035 (la carga de funciones de Vulkan) | `engine/gpu/**`, `engine/render*/**`, `shaders/**`, o código que llame a Vulkan |
| [matematica](#matematica) | 0008 | `engine/core/math/**`, o código que defina o use vectores, matrices, cuaterniones o comparaciones de `float` |
| [animacion](#animacion) | 0015 | `engine/animation/**`, shaders de skinning, o código que use esqueletos, poses, clips, IK o pesos de vértices |
| [fisica](#fisica) | 0016 | `engine/physics/**`, o código que use cuerpos, formas de colisión, contactos, uniones, consultas o el controlador de personaje |
| [assets](#assets) | 0017, 0007 (lo que el cocinador hace para el corte), 0022 (mipmaps), 0023 (la navmesh), 0024 (los sonidos), 0026 (las fuentes y los textos), 0027 (el mundo: zonas, marcadores y fases), 0028 (los binarios por zona y el global, el reparto y el paquete), 0031 (las cámaras fijas y los rieles), 0034 (las cinemáticas) | `assets/**`, las herramientas de assets en `tools/**`, las reglas de CMake que exportan o cocinan assets, o código que lea el binario cocinado |
| [corte](#corte) | 0007 | `engine/slicing/**`, o código que parta mallas, arme tapas, cree pedazos o lea un `slice_request` |
| [testing](#testing) | 0011, 0007, 0010, 0015, 0016, 0017, 0018, 0023, 0024, 0025, 0026, 0027, 0028, 0031, 0032, 0034 y 0035 (la suite de Windows sin *sanitizers*; TDD en el corte, incluido el campo de distancia de la tapa, la memoria, la animación, la física, el cocinador de assets, incluidos la navmesh, los sonidos, las fuentes, los textos, el mundo, las cinemáticas y los binarios de la carga, la cola del audio, la lógica pura de la UI, el cargador, la tarjeta de memoria, el resorte y el barrido de la cámara, las piezas puras del input y el secuenciador de las cinemáticas; tests después en el combate, la IA, el resto del audio, los VFX, las pantallas de la UI, las fases, el sistema de cámara y la capa de acciones; el *replay* con tiempos de carga al azar y con otro horario de frames; el guardado con el proceso muerto a mitad) | `tests/**`, o cualquier cambio en `engine/**`, `game/**` o en las herramientas de `tools/**` (para ver si trae sus tests) |
| [diagramas](#diagramas) | 0030 | `**/diagramas/**`, `*.d2`, o cualquier cambio en `engine/**` o `game/**` (para ver si cambió un flujo dibujado, o si un sistema nuevo trae su diagrama) |
| [gameplay](#gameplay) | 0018 | `game/**`, `docs/vision/**`, o código que cargue el tajo, gaste la fuerza, emita un `slice_request`, maneje la energía, la pausa de impacto, la espada clavada, los agarres o decida qué pedazo sigue vivo |
| [ia](#ia) | 0023 | `engine/ai/**`, `engine/nav*/**`, `game/**`, o código que decida qué hace un zombie, perciba, reparta fichas de ataque, busque caminos, esquive (ORCA) o genere o lea la navmesh |
| [audio](#audio) | 0024 | `engine/audio/**`, el audio de `engine/platform/**`, `game/**`, o código que abra el dispositivo de audio, mezcle, emita o lea eventos de sonido, maneje voces, emisores o música, o decodifique sonido |
| [vfx](#vfx) | 0025 | `engine/vfx/**`, los shaders de partículas, marcas, estela o postproceso, `game/**`, o código que emita, integre o dibuje partículas, marcas de sangre, la estela, los *frames* de impacto o las capas de la tapa |
| [ui](#ui) | 0026 | `engine/ui/**`, los shaders de la UI, `game/**`, los archivos de textos y las fuentes en `assets/**`, `docs/ui/**`, o código que dibuje el HUD, los subtítulos o los menús, mida o corte texto, mueva el foco o pause el juego |
| [niveles](#niveles) | 0027 | `engine/world*/**`, `game/**`, los `.blend` y los textos de zona y de mundo en `assets/**`, `docs/levels/**`, o código que cargue, active o descargue zonas, cree las entidades de una zona, ejecute fases o triggers, elija las antorchas del frame o recorte por la niebla o las vecinas |
| [recursos](#recursos) | 0028, 0035 (la sección en PE y las carpetas de Windows) | `engine/io*/**`, `engine/resource*/**`, `engine/save*/**`, `engine/platform/**`, `game/**`, `CMakePresets.json`, o código que lea o escriba archivos, pida o libere zonas, use el hilo de I/O o los slots, suba datos de una zona a la GPU, cree o guarde el `VkPipelineCache`, recargue en caliente, o lea o escriba la tarjeta de memoria o las opciones |
| [camara](#camara) | 0031 | `engine/camera*/**`, `game/**`, `docs/camera/**`, los marcadores de cámara en `assets/**`, o código que calcule la cámara, su colisión, la cámara del tajo, la herida que se abre o la sacudida, que apunte la espada o que lea la cámara desde la simulación |
| [input](#input) | 0032 | `engine/input*/**`, el input de `engine/platform/**`, `game/**`, `docs/vision/**`, o código que lea eventos de teclado, mouse o gamepad, arme acciones, aplique zonas muertas o curvas, reparta el input entre los pasos, guarde o reasigne controles, cambie de dispositivo, haga vibrar el mando o espere la presentación antes de leer el input |
| [cinematicas](#cinematicas) | 0034 | `game/**`, los `.blend` de cinemáticas y sus scripts de exportación en `assets/**` y `tools/**`, o código que reproduzca, salte o cocine una cinemática, detenga el mundo durante ella, mezcle su cámara con la del jugador o dibuje sus bandas |
| [herramientas](#herramientas) | 0021 | `CMakePresets.json`, o código que use Dear ImGui, Tracy o RenderDoc, que loguee, que dibuje primitivas de depuración, que declare o lea CVars, que grabe o reproduzca un *replay*, o que mida tiempos o memoria |
| [ci](#ci) | 0029 (CI), 0035 (CD y distribución) | `.github/workflows/**`, `.github/dependabot.yml`, `tools/ci/**`, el *toolchain file* de Windows, los recursos del `.exe` (ícono, `VERSIONINFO`, manifiesto), el `LEEME.txt` o la plantilla de las notas de release, o cualquier cambio en la versión de Blender, de D2, en la fecha del snapshot del toolchain o en el *digest* de las imágenes de la CD |

### Activación por contenido

Además de las rutas, un tema corre si el **contenido** del diff toca su área. Esto
importa sobre todo en los cambios de documentación (un ADR nuevo, una guía):

| Si el diff menciona… | Corre |
|---|---|
| Vectores, matrices, cuaterniones, coordenadas, `float`, una librería de matemática (GLM, Eigen…) | `matematica` |
| Vulkan, render, shaders, profundidad, GPU, luces, sombras, el look PSX (*dithering*, mipmaps, niebla), el escalado, transparencias, la tapa del corte | `graficos` |
| Una dependencia o librería nueva, licencias, assets de terceros, flags del compilador | `build-y-licencias` |
| ECS, capas, módulos, SDL, input, el bucle de juego | `arquitectura` |
| Rasgos de C++ (excepciones, RTTI, punteros inteligentes, templates, `virtual`) | `cpp-y-convenciones` |
| Tests, cobertura, bugs, TDD | `testing` |
| Rendimiento, presupuestos, benchmarks, tiempo de frame, profiling | `arquitectura` (por el reparto del frame) y `herramientas` (por cómo se mide) |
| Logging, Dear ImGui, Tracy, `perf`, RenderDoc, dibujo de depuración, CVars, valores ajustables, *replay*, grabación del input | `herramientas` |
| Memoria, arenas, pools, allocators, `malloc`, `mmap`, `std::pmr`, contenedores, handles, ASan, presupuestos de memoria | `memoria` |
| Animación, skinning, esqueletos, huesos, clips, IK | `animacion` |
| Física, colisiones, cuerpos rígidos, ragdoll, solver, cápsulas, controlador de personaje, la detección del tajo | `fisica` |
| Assets, Blender, glTF, texturas, formatos de archivo, el cocinador, Git LFS | `assets` |
| Corte de mallas, tapas, pedazos, hendidura, `slice_request`, índice soldado | `corte` |
| Diagramas, D2, la leyenda, SVG, Mermaid, PlantUML, Graphviz | `diagramas` |
| Combate, la fuerza o la carga del tajo, armadura, juntas, rebote, la espada clavada, agarres, energía, pausa de impacto, vivo o muerto, tipos de zombie | `gameplay` |
| IA, árbol de comportamiento, máquina de estados, percepción, fichas de ataque, navmesh, A*, *funnel*, caminos, ORCA, evasión, la horda | `ia` |
| Audio, sonido, mezclador, voces, callback de audio, hilo de audio, música, Vorbis, FLAC, paneo, atenuación, el oyente, emisores | `audio` |
| VFX, partículas, sangre, salpicadura, chispas, polvo, fuego, *decals* o marcas, la estela de la espada, los *frames* de impacto, las capas de la tapa, el campo de distancia | `vfx` |
| UI, HUD, menús, pausa, subtítulos, fuentes, texto, idiomas, traducción, foco, íconos de botones, accesibilidad, contraste, daltonismo, el color de acento | `ui` |
| Niveles, escenas, el mundo, zonas, vecinas, *streaming*, precarga, spawns, oleadas, fases, triggers, checkpoints, marcadores, *empties*, *collection instances*, *prefabs* o piezas, la pantalla de carga, el culling por la niebla | `niveles` |
| La carga de recursos, el hilo de I/O, io_uring, `pread`, `mmap`, los slots, el paquete o el ejecutable con los assets, la subida a la GPU, el *staging*, la recarga en caliente (*hot reload*), inotify, el `VkPipelineCache`, la partida, la tarjeta de memoria, el guardado, las opciones del jugador, las carpetas XDG, `SDL_GetPrefPath`, `fsync` | `recursos` |
| Cámara, estilos de cámara, tercera o primera persona, cámaras fijas, rieles, FOV, colisión de la cámara, la cámara del tajo, cámara lenta, rayos X, la herida que se abre, sacudida, trauma, la mira, apuntar la espada, el punto apuntado | `camara` |
| Input, controles, acciones, reasignar, teclas, scancode o keycode, mouse relativo, sensibilidad, stick, zona muerta, curva de respuesta, gatillos, *buffer* de input, toques perdidos, latencia del input, `present_wait`, gamepad, mando, deriva, cambio de dispositivo, vibración, giroscopio, moverse (tanque o relativo a la pantalla) | `input` |
| Cinemáticas, escenas de película, secuenciador, línea de tiempo, tomas, cortes de cámara, la salida a la cámara del jugador, bandas o *letterbox*, saltar una cinemática, *quick time*, secuencia interactiva, el dragón, el *parry* | `cinematicas` |
| CI, CD, GitHub Actions, workflows, runners, chequeos obligatorios, el ruleset, Dependabot, releases, el Arch Linux Archive, el Steam Linux Runtime, *sniper*, Windows, llvm-mingw, Wine, itch.io, `butler`, firma de código, SmartScreen, antivirus, `SHA256SUMS`, atestaciones, releases inmutables, builds reproducibles, `SOURCE_DATE_EPOCH`, *staging*, macOS | `ci` |

**Un solo dueño por regla.** Las flags del compilador las audita solo
`build-y-licencias`, aunque vengan del ADR-0008, 0009 o 0013. Así no aparece el mismo
hallazgo tres veces.

**Mantenimiento:** cuando se acepta un ADR nuevo, se agrega a la fila y a la sección del
tema que corresponda, o se crea un tema nuevo. El tema `docs-y-adrs` avisa si un ADR
aceptado del índice de `docs/README.md` no aparece en ningún tema.

---

## proceso

**ADRs:** 0001 (ADRs), 0002 (tickets), 0014 (colaboración y metodología), 0020
(requisitos y protocolo de medición del rendimiento), 0033 (organización del desarrollo
y roadmap). También `docs/process/git.md`, `docs/process/tickets.md`,
`docs/performance/presupuestos.md` y `docs/plans/roadmap.md`.

Qué mirar:

- La rama sigue `<N>-<descripcion>`, y `N` es el ticket que se cierra.
- El título previsto del PR sigue Conventional Commits, con el tipo en inglés y la
  descripción en español. El tipo corresponde al cambio (`refactor:` no cambia el
  comportamiento, y no se mezcla con un `feat:`).
- El cuerpo lleva `Closes #N`, una línea por issue.
- El ticket **existe en GitHub**, y su número coincide con el de la rama.
- El cambio cumple los **criterios de aceptación** del ticket. Señala cada criterio que
  no se ve cumplido en el diff. En una investigación no hay criterios: el ADR tiene que
  **responder la "Pregunta a decidir"** del ticket, completa.
- No queda ningún `TODO(human)` sin resolver: esos marcadores son del modo dirigido y
  no llegan a `main`.
- Si el ticket es una tarea: tiene label `modo:`, y "Qué debo poder explicar" está
  respondida o pendiente para el autor antes del merge.
- **El átomo (ADR-0033):** una tarea de código es un PR que entrega algo comprobable y
  cabe en una revisión tranquila. Si el diff de producción (sin los tests) pasa de ~400
  líneas, o junta dos entregas que se podrían revisar por separado, es un hallazgo
  `sugerencia`: se parte, o el PR explica por qué no se puede.
- **La trazabilidad (ADR-0033):** la tarea tiene su sección "ADRs" (el primero es el
  dueño, y su área coincide con el label `área:`, salvo las tareas de contenido, que
  llevan `área:contenido` por la Decisión 8), su milestone (el corte en curso) y sus
  dependencias como *blocked by* nativas. En el código no hay épicas ni `tipo:feature`.
- **El roadmap (ADR-0033):** el PR que abre o cierra un milestone actualiza
  `docs/plans/roadmap.md` (las filas, el número de issue de cada átomo, los riesgos).
  Una tarea de código que no aparece en el roadmap, o un milestone que se cierra sin
  actualizarlo, es un hallazgo `importante`.
- Una deuda deliberada nueva lleva `TODO(#N)` y su issue `deuda-técnica`. Un `TODO`
  suelto, sin issue, es un hallazgo.
- Los cambios de rendimiento incluyen la medición antes y después (ADR-0014, regla 9),
  hecha con el **protocolo del ADR-0020**:
  - preset `relwithdebinfo` sin validation layers, con calentamiento y al menos 5
    ejecuciones;
  - p50, p99, p99,9 y máximo en ms, nunca fps promedio;
  - una mejora o un empeoramiento solo cuentan si los intervalos de confianza del 95 %
    no se superponen.
- Un cambio a `docs/performance/presupuestos.md` trae su medición y una línea nueva en el
  historial. Un número sin medir sigue marcado como hipótesis. La excepción: una nota que
  solo cita la decisión de un ADR aceptado, sin cambiar ninguna meta, va en el PR de ese
  ADR, con su línea en el historial y "Ninguna" en la columna de medición.
- Un cambio que sube la carga del juego (más zombies, más pedazos o un sistema nuevo en
  el frame) respeta los requisitos R1 a R5 del ADR-0020, o deja el incumplimiento
  registrado como bug o ticket, según el ADR-0020.
- No hay código de un *spike* (ramas `spike/…`) entrando a `main`.
- Si el cambio agrega código de Claude: el commit previsto lleva `Co-Authored-By`.
- Si hay tags o releases: SemVer `0.y.z` y `CHANGELOG.md` al día.

## docs-y-adrs

**ADRs:** 0001 (formato, estados, índice e inmutabilidad), 0014 (reglas 8 y 11), y las
reglas de `CLAUDE.md` sobre la documentación.

Qué mirar:

- **Idioma:** la documentación va en español.
- **Audiencia genérica:** los ADRs no nombran la universidad objetivo ni usan etiquetas
  de tipo de empresa ("AAA"). Se escribe "la industria", "estudios grandes" o "programas
  de posgrado en ingeniería de videojuegos".
- **Un ADR nuevo o editado** sigue `docs/adr/template.md`: Contexto, Criterios, Opciones,
  Investigación, Decisión, Consecuencias, Cómo lo defiendo y Referencias. Un ADR
  `Aceptado` tiene la Decisión y "Cómo lo defiendo" completas.
- **Índice:** cada ADR nuevo o con estado cambiado tiene su fila en `docs/README.md`, con
  el mismo estado que su encabezado.
- **Diagrama teórico (fe de erratas del ADR-0030, #69):** un ADR que pasa a `Aceptado`
  tiene su caja en `docs/diagramas/teorico/proyecto.d2`, con lo que eligió y su número,
  y las flechas "empujó a" que su texto nombra. Una fe de erratas que cambia lo que
  eligió un ADR corrige su caja. Falta la caja: `importante`.
- **Inmutabilidad (ADR-0014, regla 11):** mientras no exista código del juego, un ADR
  aceptado se puede corregir como fe de erratas, registrada en el campo **Fe de
  erratas** de su encabezado (fecha, ticket y qué cambió). **El código empieza cuando se
  mergea el primer PR con código** (ADR-0033, Decisión 9), aunque solo traiga el CMake
  del esqueleto. Desde ahí, editar la Decisión de un ADR aceptado es `bloqueante`: hace
  falta un ADR nuevo que lo reemplace.
- **No adelantar decisiones:** un texto que da por decidido algo que le toca a un ADR
  pendiente es un hallazgo. Por ejemplo, decidir con qué se firma el `.exe` antes de
  retomar la firma que el ADR-0035 dejó abierta.
- **Coherencia:** un documento nuevo no contradice un ADR aceptado. Si lo contradice,
  reporta las dos citas.
- **Modelo de colaboración (ADR-0014):** los documentos lo describen como trabajo
  colaborativo (el autor dirige y revisa, Claude escribe). No lo presentan como un
  cambio respecto de otra forma de trabajar.
- **Comentarios en el código (ADR-0014, regla 8):** concisos, sin referencias a ADRs, y
  cada bloque se entiende por sí solo.
- **Mapa de temas:** cada ADR `Aceptado` de `docs/README.md` aparece en algún tema de
  este archivo.

## cpp-y-convenciones

**ADRs:** 0003 (C++20), 0009 (subconjunto de C++), 0013 (convenciones y análisis
estático), 0014 (reglas 8 y 10).

Qué mirar:

- **Sin excepciones ni RTTI:** nada de `throw`, `try`, `catch`, `dynamic_cast` ni
  `typeid`. Los errores del programador se tratan con `assert`, y los esperables con
  `result<T, E>`.
- **Propiedad:** `unique_ptr` y handles con generación. `shared_ptr` está prohibido.
- **`virtual` solo en las fronteras de módulo.** Nada de jerarquías profundas.
- **Templates simples, restringidos con `concepts`**, sin metaprogramación compleja.
- **Sin corrutinas.** La recursión del código que corre cada frame tiene un límite.
- **STL por zonas:** lo que el ADR-0009 permite en cada zona. Por ejemplo, no reservar
  memoria dinámica en el camino de cada frame si el ADR lo restringe.
- **Nombres (ADR-0013):** estilo STL en `snake_case`, miembros con prefijo `m_` y
  macros en `MAYÚSCULAS`.
- **Formato:** 4 espacios y 100 columnas, según `.clang-format`.
- **Severidad:** los nombres y el formato son `importante` como máximo. Las flags del
  compilador (`-fno-exceptions`, `-fno-rtti`, warnings) no son de este tema: las audita
  `build-y-licencias`.
- **Headers, no módulos de C++20.**
- **Diseño:** prioridad al diseño orientado a datos (ADR-0014, regla 10). Datos
  contiguos en los caminos calientes, nada de polimorfismo innecesario y módulos
  profundos con interfaces chicas.
- **Comentarios:** concisos, sin referencias a ADRs, y cada bloque se entiende fuera de
  contexto.

## arquitectura

**ADRs:** 0004 (capa de plataforma), 0012 (arquitectura del engine), 0016 (consultas
directas a la física), 0020 (reparto del frame por sistema).

Qué mirar:

- **Capas en una sola dirección:** `platform` → `core` → `gpu` → sistemas → `game`. Una
  capa nunca incluye ni enlaza una capa superior, y las dependencias en CMake respetan
  ese orden.
- **SDL3 solo detrás de la interfaz `platform::`.** Nada fuera de `engine/platform`
  incluye headers de SDL. `SDL_GPU` y `SDL_Renderer` están prohibidos. **Lo mismo con
  Windows** (fe de erratas del ADR-0004, por el ADR-0035): `windows.h` y los tipos de
  Win32 solo dentro de `engine/platform`.
- **`platform::` entrega eventos con tipos propios y su *timestamp*,** nunca eventos de
  SDL. La capa de acciones (un sistema encima de `engine/core`, ADR-0032) arma las
  acciones de cada paso; el gameplay nunca ve una tecla ni un dispositivo.
- **ECS:** de *sparse sets* propio. La entidad es un handle con generación.
- **Bucle:** paso fijo de 60 Hz con acumulador e interpolación, como mucho 8 pasos por
  frame y el frame recortado a 250 ms.
- **Comunicación:** colas de eventos entre sistemas, guardadas en la arena del paso
  (fe de erratas del ADR-0012, por el ADR-0010), para las órdenes que cambian el estado. Las consultas de solo lectura (rayos, barridos, el
  barrido del tajo) son llamadas directas hacia abajo (fe de erratas del ADR-0012, por
  el ADR-0016).
- **Un solo hilo** por ahora. Hay tres excepciones, y ninguna toca el estado del juego:
  los hilos de Tracy en los builds de desarrollo (fe de erratas del ADR-0012, por el
  ADR-0021), el hilo de audio de SDL3, que solo ejecuta comandos y mezcla (fe de
  erratas del ADR-0012, por el ADR-0024; sus reglas las audita `audio`), y el hilo de
  I/O, que solo lee y escribe archivos en memoria reservada (fe de erratas del ADR-0012,
  por el ADR-0028; sus reglas las audita `recursos`). Cualquier otro hilo es una decisión
  nueva.
- **Los VFX corren al final de cada paso, fuera del estado** (fe de erratas del ADR-0012,
  por el ADR-0025): después de la animación y antes de reiniciar la arena del paso. No
  entran en el hash del *replay* y nunca escriben en la simulación; sus reglas las
  audita `vfx`.
- **Reparto del frame (ADR-0020):** `docs/performance/presupuestos.md` separa el costo
  por paso de simulación del costo por frame. Un frame con dos pasos tiene que caber en
  16,7 ms. Lo que depende de la simulación (por ejemplo, la cuota de cortes) se cuenta
  en pasos, no en frames, para no romper el determinismo.
- **Estructura del monorepo:** `engine/`, `game/`, `tools/`, `shaders/`, `tests/`,
  `docs/` y `assets/` (la fuente de los assets, ADR-0017 y ADR-0019).
- **El cocinador de assets** (`tools/`) enlaza solo `engine/core`. Nada de `engine/` ni
  de `game/` enlaza el cocinador ni sus dependencias (ADR-0017).

## memoria

**ADRs:** 0010 (estrategia de memoria). También las filas de memoria de
`docs/performance/presupuestos.md`.

Qué mirar:

- **Regiones:** cada región tiene un sistema dueño, una sola vida, y cae en una fila de
  `presupuestos.md`. Las que usa el frame (arenas, pools, pedazos, *ring buffer*) se
  reservan al iniciar y se tocan enteras en ese momento: en la CPU, con `mmap` y
  `MAP_POPULATE` a través de `platform::` (`engine/core` no llama al sistema operativo
  directamente); en la GPU, con los bloques de `gpu::Allocator`. Pedir memoria al
  sistema operativo o al driver dentro del paso o del render es un hallazgo. **Con el
  ADR-0027, lo que vive es una zona**, con un tope de zonas cargadas a la vez, y el
  ADR-0028 la reparte en **slots fijos**: tantos como el tope, cada uno del tamaño
  fijado en el texto del mundo, en la CPU (memoria de Vulkan visible y con caché) y en la GPU,
  reservados y tocados al iniciar. Un solo bloque lineal para todas las zonas, un montón
  general (TLSF o *buddy*) para ellas, o pedir la memoria de una zona al cargarla son
  `importante`: pierden la garantía del tope (P4 del ADR-0028). La región de recarga
  (solo en desarrollo) es la excepción: usa el allocator de offsets.
- **`gpu::Allocator`:** bloques grandes por tiempo de vida (lo que vive una zona, el
  *ring buffer* para el frame, el allocator de offsets para los pedazos). Una
  asignación de Vulkan por recurso es un hallazgo.
- **Arenas temporales:** la del paso se reinicia al terminar cada paso (ahí van las colas
  de eventos), la del frame al terminar el frame (solo el render), y las dos *scratch*
  vuelven a su marcador al salir de la función que las usó. Guardar en la arena del paso
  algo que se lee en otro paso o en el render es un hallazgo: va en un componente.
- **Las arenas no limpian:** quien pide memoria la inicializa (determinismo, ADR-0016).
- **Objetos de tamaño fijo:** pools con lista libre y `handle<T>` con generación.
- **Pedazos:** su geometría usa el allocator de offsets tipo TLSF, en la CPU y en la GPU.
  Liberar un pedazo pasa por la cola de liberación por número de frame: reusar su lugar
  antes de que terminen sus frames en vuelo es un hallazgo.
- **Descartar pedazos es una decisión lógica:** se cuenta con la cantidad y los bytes
  vivos contra un tope fijo, nunca con lo que liberó la GPU (las *fences* dependen del
  tiempo real y romperían el determinismo).
- **Contenedores:** propios y de capacidad fija, que nunca crecen. La arena se pasa como
  parámetro explícito. **Sin `std::pmr`.** La regla de qué contenedores de la STL se
  permiten en cada zona es de `cpp-y-convenciones`.
- **Cuando se acaba:** las arenas y las tablas del engine abortan con el nombre de la
  región y su pico, en todos los builds. Los pools que llena el juego (pedidos de corte,
  pedazos, ragdolls; y las partículas, las marcas y los campos de distancia del ADR-0025,
  por fe de erratas del ADR-0010) degradan con su regla, y el juego pregunta antes si hay lugar.
  Devolver `nullptr` en silencio, o pedir al heap como respaldo, es un hallazgo. **Dos
  excepciones declaradas** (fe de erratas del ADR-0010, por el ADR-0021): el dibujo de
  depuración descarta lo que no cabe y lo muestra en pantalla, y Tracy con un servidor
  conectado pide memoria mientras corre (por eso nunca se mide así). **La región de UI**
  (ADR-0026) no degrada: llena, aborta con su nombre, como una arena, porque lo que dibuja
  la UI lo decide el autor. Descartar vértices en silencio es un hallazgo. **La región de
  input** (ADR-0032) es la tercera excepción, y la única en Release: llena, suma los
  deltas de los eventos que no caben y nunca pierde un botón, porque lo que la llena es
  el hardware del jugador. Abortar, o perder un botón, es un hallazgo.
- **RelWithDebInfo** cuesta lo mismo que Release, salvo las herramientas del ADR-0021 y
  su región de depuración (fe de erratas del ADR-0010).
- **Revisiones de desarrollo, solo en Debug:** las arenas y los pools envenenan para
  ASan lo que reinician o devuelven, con zonas rojas y asignaciones alineadas a 8 bytes
  como mínimo; patrón de relleno, guardas, y comprobación al cerrar de que cada pool
  quedó vacío. Que RelWithDebInfo las lleve es un hallazgo: es el build que mide el
  ADR-0020. En todos los builds: uso y pico por región, generación de los handles y
  abortar al quedarse sin memoria. Las flags de los sanitizers no son de este tema: las
  audita `build-y-licencias`.
- **El ECS** tiene capacidad fija, reservada al iniciar.
- **Lo de la GPU que no es del allocator** (el *ring buffer* y su tamaño, los frames en
  vuelo, las dos rutas de subida) lo audita `graficos`.
- **Tests:** los casos que pide el ADR-0010 (arena, pool, allocator de offsets con prueba
  de propiedades, cola de liberación, *ring buffer* y los *death tests* de ASan) los
  revisa `testing`.

## build-y-licencias

**ADRs:** 0006 (build y toolchain), 0013 (warnings y análisis estático), 0019
(licencias), 0008 (flags de punto flotante y CPU), 0009 (`-fno-exceptions` y
`-fno-rtti`). Este tema es el **único dueño de las flags del compilador**.

Qué mirar:

- **CMake + Ninja**, con Clang como compilador principal, GCC como secundario y `lld`.
- **Presets:** Debug (ASan + UBSan + validation layers), RelWithDebInfo (con
  `-fno-omit-frame-pointer`, fe de erratas del ADR-0006 por el ADR-0021) y Release.
- **Dear ImGui y Tracy** (ADR-0021) entran con `FetchContent` y una versión fijada, y
  solo en `debug` y `relwithdebinfo`. Si Release los compila o los enlaza, es
  `bloqueante`.
- **Flags:** `-Werror` y la lista ampliada de warnings del ADR-0013, sin `-Weverything`.
  **Por target, nunca globales** (fe de erratas del ADR-0013): un target de interfaz que
  solo enlazan los targets propios. `add_compile_options` con warnings o `-Werror` es un
  hallazgo, porque llega a las dependencias de `FetchContent`.
  `-fno-exceptions` y `-fno-rtti` (ADR-0009). **Sin `-ffast-math` y sin `-march`**
  (ADR-0008).
- **SDL3:** del sistema, con la opción `MZ_VENDOR_SDL3` para usar FetchContent. **Los
  builds de la CD usan la copia, estática** (fe de erratas del ADR-0006, por el
  ADR-0035): un binario publicado que enlaza `libSDL3.so` no arranca en Ubuntu 24.04
  (medido).
- **Vulkan (fe de erratas del ADR-0006, por el ADR-0035):** headers en una versión
  fijada, y **el loader se carga en runtime a través de SDL3**. Enlazar `libvulkan` o
  `vulkan-1.dll` (`Vulkan::Vulkan`) es un hallazgo. Las funciones de Vulkan van en una
  tabla propia de `engine/gpu` (`VK_NO_PROTOTYPES`); eso lo audita `graficos`, y volk
  sería una dependencia nueva.
- **El linker:** `lld`, salvo la Release de Linux de la CD, que enlaza con el `ld` de
  *sniper* (fe de erratas del ADR-0006, por el ADR-0035).
- **Los builds de la CD (ADR-0035):** Linux con GCC 14 en *sniper*, `libstdc++` y
  `libgcc` estáticas; Windows con llvm-mingw y `-static`, desde un *toolchain file*.
  **`-ffile-prefix-map` va en las banderas del preset** (`CMAKE_C_FLAGS` y
  `CMAKE_CXX_FLAGS`), para que alcance a las dependencias; con `add_compile_options`
  después de `add_subdirectory`, no las alcanza. Estas banderas no son las de warnings:
  esas siguen por target.
- **Shaders:** `glslc` y `spirv-val`.
- **Dependencias:** una dependencia nueva necesita su ADR y su fila en `CREDITS.md`.
  Una dependencia **copiada en el repo** (como `tools/third_party/`) lleva además su
  anotación en `REUSE.toml`, con el copyright y la licencia de sus autores, debajo de la
  anotación por defecto. Sin ella, `reuse lint` pasa igual, pero con el autor
  equivocado.
- **Licencias (ADR-0019):** el código va bajo MIT, y la documentación y los assets
  propios bajo CC BY 4.0. Un asset de terceros solo puede ser CC0 o CC BY, **salvo las
  fuentes, que también pueden ser OFL 1.1** (fe de erratas del ADR-0019, por el ADR-0026),
  con su fila en `CREDITS.md` y su anotación en `REUSE.toml` si no es CC BY 4.0 de
  Clemente Grass. Una fuente OFL lleva además `LICENSES/OFL-1.1.txt`. Una fuente con otra
  licencia, o un asset que no es fuente bajo OFL, es `bloqueante`.
- **`uvx reuse lint` sigue pasando.** Puedes ejecutarlo, porque solo lee.

## graficos

**ADRs:** 0005 (API gráfica), 0007 (frames en vuelo y subida de los pedazos), 0008
(proyección y buffers), 0010 (el *ring buffer* dimensionado para el peor caso), 0022
(pipeline de render) y la carga de funciones de Vulkan del ADR-0035.

Qué mirar:

- **Vulkan 1.3**, con *dynamic rendering* y `synchronization2`. Sin *render passes*
  clásicos.
- **Inicialización propia**, sin vk-bootstrap.
- **Las funciones de Vulkan, en una tabla propia** (fe de erratas del ADR-0006, por el
  ADR-0035): `VK_NO_PROTOTYPES`, cargadas con `vkGetInstanceProcAddr` y
  `vkGetDeviceProcAddr` desde el puntero inicial que entrega `platform::` (que lo saca de
  SDL3). Llamar a un `vk*` enlazado, pedirle el puntero a SDL fuera de `platform::`, o
  usar volk sin su ADR, es un hallazgo.
- **`gpu::Allocator` propio** (VMA es solo el plan B, y necesita su ADR).
- **Ring buffer** persistente y mapeado, uno por frame en vuelo, de tamaño máximo por
  frame × frames en vuelo. **2 frames en vuelo** (ADR-0007). El máximo por frame cuenta
  el peor caso de cortes (4 por paso × 8 pasos), las matrices de los huesos de los
  zombies y las de los ragdolls.
- **Los pedazos viven en un pool, no en el *ring*** (ADR-0007): en la ruta dedicada se
  copian del *ring* al pool *device-local*, y en la integrada se escriben directo en el
  pool. Cuándo se libera su lugar (la cola por número de frame) lo audita `memoria`.
- **Las dos rutas:** GPU dedicada e integrada.
- **Validation layers** activas en los builds de desarrollo.
- **Reverse-Z (ADR-0008):** el buffer de profundidad es `D32_SFLOAT`, se limpia a `0.0`
  y se compara con `GREATER` o `GREATER_OR_EQUAL`. Un `LESS` o un clear a `1.0` es
  `bloqueante`.
- **Proyección:** profundidad en [0, 1], Y volteada **en la matriz de proyección** (no
  con un viewport negativo), y `frontFace` = **`VK_FRONT_FACE_COUNTER_CLOCKWISE`**, con
  un test que dibuja un cubo y verifica que se ven sus caras de afuera. `CLOCKWISE` es
  `bloqueante`: dibuja la escena de adentro hacia afuera sin que cambie la silueta.
  Plano lejano infinito solo en la cámara principal.
- **Buffers de GPU:** los `struct` que van a la GPU usan `vec4` o relleno explícito,
  con `static_assert` sobre `sizeof` y `offsetof`.
- **El frame (ADR-0022):** forward, con estas pasadas en este orden: color y
  profundidad a 640×360 (opacos, *screen-door* y los VFX del ADR-0025, sin pasadas
  nuevas), dibujo de depuración, postproceso y **la UI del ADR-0026** (HUD, subtítulos y
  menús, después del postproceso; fe de erratas del ADR-0022) a 640×360, **blit
  *nearest* por el factor entero** al swapchain, e ImGui a la resolución nativa. Un *depth prepass*, un G-buffer o un shadow map son una decisión nueva, no un
  detalle de implementación. Un efecto aplicado después del blit, o un escalado con
  filtro lineal o por el compositor, es `importante`.
- **Barreras escritas a mano** con `synchronization2`, sin *render graph*. **Todos los
  pipelines se compilan al arrancar**, con un `VkPipelineCache`; crear un pipeline
  durante el juego es `importante` (causa tirones).
- **Luz estilizada por píxel:** Lambert con atenuación hasta el radio, ambiente frío, sin
  especular, y un arreglo global de **hasta 16 antorchas** en el uniform del frame:
  **las 16 más cercanas a la cámara** entre las zonas cargadas, con un fundido al entrar
  o salir (fe de erratas del ADR-0027). Elegirlas por otro criterio, o sin fundido, es un
  hallazgo.
  Sombras ***blob***, sin shadow map, dibujadas con *screen-door* en la pasada de color,
  después del piso y con *depth bias*.
- **El look PSX:** texturas *nearest* con mipmaps *nearest* entre niveles; *dithering* a
  15 bits con Bayer 4×4 y niebla lineal al final del fragment shader. El temblor de
  vértices (grilla) y las texturas afines son **CVars** (ADR-0021), constantes en
  Release; las afines, apagadas por defecto. Color LDR en `R8G8B8A8_SRGB`, sin
  antialiasing.
- **Bindless:** un *descriptor set* global con el arreglo de objetos (un SSBO por frame
  en vuelo) y el de texturas (*descriptor indexing*, *partially bound*); el índice del
  objeto va en un *push constant*. Una llamada por objeto; culling por esfera en la CPU,
  **hasta el final de la niebla y solo en la zona actual y sus vecinas** (fe de erratas
  del ADR-0027). Dibujar más allá de la niebla o zonas que no son vecinas es un
  hallazgo.
  El engine verifica al iniciar `descriptorIndexing`, `runtimeDescriptorArray`,
  `descriptorBindingPartiallyBound`, `shaderSampledImageArrayNonUniformIndexing` y
  `shaderDemoteToHelperInvocation` (la usa el `discard` del *screen-door*; fe de erratas
  del ADR-0022, por el ADR-0025).
- **El canal alfa del color es la máscara de personajes** (fe de erratas del ADR-0022,
  por el ADR-0025): 0 en los personajes, 1 en el resto. Un shader que escribe otro alfa,
  o una mezcla que no conserva el alfa del destino, rompe los *frames* de impacto.
- **Pedazos:** dos llamadas, con **índices de 16 bits**; el buffer chico lleva los
  vértices nuevos y copias de los del borde. **La tapa es procedural** según la distancia
  al eje de las cápsulas de los huesos del pedazo, con una textura de detalle en su UV
  plana. Usa las cápsulas del asset, movidas con la paleta congelada del pedazo; usar las
  recortadas del pedazo (ADR-0016) es un hallazgo, porque el hueso no llega a la tapa.
- **Transparencias:** *screen-door* con Bayer 4×4 para la caballera; *alpha blending*
  solo para los VFX del ADR-0025 (partículas, marcas y estela). Los VFX son además la
  excepción a "una llamada por objeto" (usan *instancing*) y a la luz por píxel (luz por
  vértice, sin normal); fe de erratas del ADR-0022, por el ADR-0025. **El swapchain usa
  `VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR`:** otro modo deja transparentes a los personajes.
- **Medir una pasada:** la comparación de antes y después de un PR de rendimiento se
  hace con el reloj de la GPU fijo, y el PR dice a qué frecuencia (ADR-0020).

## matematica

**ADRs:** 0008.

Qué mirar:

- **Biblioteca propia**, sin GLM ni DirectXMath.
- **Tipos concretos con `float`:** `vec2`, `vec3`, `vec4`, `mat3`, `mat4`, `quat` y
  `plane`. Sin `vec<N, T>` genérico.
- **Escalar.** SIMD solo donde el profiler lo justifique, con la medición en el PR. El
  SoA de un pool decidido por diseño (las partículas del ADR-0025) no necesita
  medición: es diseño orientado a datos (fe de erratas del ADR-0008, por el ADR-0025).
  Llevar un bucle a SIMD, sí.
- **Convenciones:** mano derecha, Y arriba, −Z adelante, metros y radianes.
- **Importación de assets:** se convierten a Y arriba y metros, con el frente del modelo
  hacia −Z. Un glTF (frente hacia +Z) se rota 180° sobre Y, en toda la jerarquía. Un
  espejo en Z (`z → −z`) es `bloqueante`, porque invierte la mano.
- **Matrices:** vectores columna (`M * v`), guardados por columnas.
- **Rotaciones:** se guardan e interpolan con cuaterniones. Ángulos de Euler solo en
  debug y editor.
- **Flags de punto flotante:** no son de este tema. `-ffast-math` lo audita
  `build-y-licencias`.
- **Comparaciones de `float`:** nunca `==` salvo con valores exactos.
  `nearly_equal(a, b, tol)`, con una tolerancia explícita. **Excepciones decididas en el
  ADR-0007:** la clasificación del corte por el signo de la distancia (el 0 cuenta como
  positivo) y el índice soldado por posición idéntica bit a bit no usan tolerancia. Las
  audita `corte`.
- **Tests (ADR-0008, Consecuencias):** valores calculados a mano y propiedades
  (`inverse(M) * M ≈ I`, `|q| = 1`, `dot(v, cross(v, w)) ≈ 0`). El TDD y la cobertura
  no son de este tema: los audita `testing`.

## animacion

**ADRs:** 0015 (sistema de animación), con la fe de erratas del ADR-0031 (la guía de la
espada) y la del ADR-0032 (la guía sigue un objetivo, que el stick marca como posición y
el mouse mueve con sus deltas; el peso lo pone la espada, no el input).

Qué mirar:

- **La guía de la espada** (fe de erratas, ADR-0031): cuando no ataca, la caballera es
  clips; al apuntar y mientras carga, el ratón o el stick guían la espada con la IK del
  tajo, **limitada por lo que puede hacer el cuerpo, lenta, precisa y con peso**. Copiar
  el input directo a la IK en cada frame (sin resorte, sin tope de velocidad, sin
  límites de articulación) es `importante`: es lo que se siente "espagueti". **Todos sus
  valores son CVars.**

- **Por capas:** clips para la base, procedural encima (la espada, el torso, los
  agarres) y física para lo que ya no tiene control.
- **El tajo es mixto:** la espada y los brazos no usan clips (la punta sigue un arco en
  el plano del tajo y los brazos llegan con IK analítica de dos huesos), y el cuerpo sí
  lleva un clip (el paso adelante y la carga).
- **LBS con hasta 4 huesos por vértice.** El aspecto rígido de PS1 son pesos de 1,0 en
  el asset, no otra ruta del engine. Cambiar a DQS cambia la Decisión 2: con código
  existente, necesita un ADR nuevo (ADR-0001).
- **Skinning en la GPU** (vertex shader) para dibujar. **La CPU deforma solo el zombie
  que se corta,** en el paso del corte (Decisión 3).
- **El skinning de la CPU es una función pura en una capa de abajo** (junto a la
  matemática de `core`), y hace la misma cuenta que el shader. `slicing` no depende de
  `animation` para usarla.
- **La pose se evalúa en el paso de simulación,** no en el frame de render, y después del
  corte (orden del ADR-0012): el combate y el corte usan la pose del paso anterior. El
  render interpola con *nlerp* y revisa el hemisferio (`dot < 0` → invertir). Una pose
  que depende del frame de render rompe el determinismo.
- **La garantía al corte:** las matrices y los pesos están en la CPU para que el corte
  asigne pesos a los vértices nuevos y el pedazo vivo siga animado. Los pedazos heredan
  la pose anterior del zombie. El método de los pesos no es de este tema: lo decide el
  ADR-0007.
- **El pedazo vivo y el zombie mutilado** conservan el esqueleto completo, con los
  huesos perdidos enmascarados, y reutilizan los mismos clips.
- **La IK de dos huesos acota `d`** a `[|a − b| + ε, a + b − ε]` antes del `acos`.
- El TDD y la cobertura de la animación no son de este tema: los audita `testing`.

## fisica

**ADRs:** 0016 (física), con la fe de erratas del ADR-0031 (la predicción del tajo
especial).

Qué mirar:

- **La predicción del tajo especial** (fe de erratas, ADR-0031): reutiliza la prueba
  sector contra cápsula de la Decisión 7 sobre el arco completo, al soltar, contra las
  poses de ese paso, y es de solo lectura. Una predicción que escriba en el estado o que
  use otra prueba distinta de la del barrido es un hallazgo.

- **Física propia.** Una biblioteca de física (Jolt, Bullet, PhysX) contradice la
  Decisión 1 y necesita su propio ADR.
- **Alcance acotado:** cuerpos dinámicos esféricos y de cápsula; cajas solo estáticas
  (el escenario); contactos con fricción; rótulas con límites; dormir; rayo y barrido de
  esfera; controlador de personaje. Agregar cajas dinámicas, casco convexo, mallas de
  triángulos, física activa, cuerpos blandos o telas cambia la Decisión 2.
- **Solver Soft Step:** impulsos secuenciales con subpasos y restricciones blandas. Se
  construye primero con impulsos secuenciales simples, y después se agregan los
  subpasos y las restricciones blandas.
- **Las cápsulas por hueso son la única forma física del cuerpo del zombie:** detectan
  el tajo, forman el ragdoll y dan la forma de los pedazos (como cuerpo compuesto si son
  varias). El zombie vivo usa además la cápsula de su controlador para moverse. Los
  huesos enmascarados no tienen cápsula activa, y un hueso que no mueve ningún vértice
  (como `root`) no tiene cápsula. Si queda en medio de una cadena, la rótula une a sus
  hijos con el ancestro más cercano con cápsula, y ese hueso se mueve rígido junto a él
  (fe de erratas del ADR-0016, por el ADR-0017).
- **Recortar una cápsula en el plano del corte** recorta su segmento al semiespacio con
  distancia al plano mayor o igual que su radio, para que la cápsula entera quede de su
  lado. Si no queda segmento, sobra una esfera en el extremo más lejano, de radio
  `min(r, dist)`, o nada. **Solo donde el corte separa pedazos:** las cápsulas que solo
  toca una hendidura no cambian (fe de erratas del ADR-0016, por el ADR-0007). Las
  recorta el corte, y la física las lee del pedazo.
- **El zombie muerto entero es un ragdoll, y los pedazos son cuerpos rígidos,** salvo los
  de más de N huesos (configurable), que pasan a ragdoll mientras quede lugar en el tope
  propio de ragdolls activos (fe de erratas del ADR-0016, por el ADR-0018). Los cuerpos
  unidos por una rótula no chocan entre sí.
- **Los personajes son controladores cinemáticos** que se mueven y se sacan de lo que
  penetran, con pruebas de superposición, no con barridos ni como cuerpos dinámicos. El
  empuje es un impulso programado. **Contra los pedazos, el contacto es en un solo
  sentido:** el pedazo se mueve y el controlador no se frena (fe de erratas del ADR-0016,
  por el ADR-0018). La consulta de superposición del agarre es un barrido de esfera de
  largo 0, no una consulta nueva. El avance por paso queda muy por debajo del radio de
  la cápsula: los movimientos rápidos (esquive, empuje) se recortan o se subdividen.
- **El tajo se detecta con el barrido en su plano:** hay golpe si la distancia del
  segmento de una cápsula al sector de anillo es menor o igual que su radio (inflado con
  lo que se movió el centro del arco). La espada es la de los pasos N−2 y N−1. Un test
  discreto por paso pierde golpes.
- **Los datos entre animación, física y corte pasan por componentes del ECS.** Las
  consultas de solo lectura son llamadas directas hacia abajo, y las colas quedan para
  las órdenes que cambian el estado.
- **Fase amplia por fuerza bruta.** Cambiarla a una grilla uniforme necesita una medición
  que lo justifique (ADR-0014). Otra estructura (árbol, *sweep and prune*) cambia la
  Decisión 8.
- **Determinismo:** un hilo, orden fijo (nunca por dirección de memoria ni por hash),
  memoria inicializada y nada que dependa del reloj.

## assets

**ADRs:** 0017 (pipeline de assets), 0007 (lo que el cocinador hace para el corte), 0022
(mipmaps), 0023 (la navmesh), 0024 (los sonidos), 0026 (las fuentes y los textos), 0027
(el mundo: zonas, marcadores y fases), 0028 (los binarios por zona y el global, el
reparto y el paquete de Release), 0031 (las cámaras fijas y los rieles), 0034 (las
cinemáticas).

Qué mirar:

- **Las cámaras fijas y los rieles** (ADR-0031): el cocinador los lee del glTF exportado
  de cada zona, como *empties* (una cámara es un *empty*; un riel, una cadena de
  *empties*, porque glTF no tiene curvas; el volumen de su sector, la escala de un
  *empty*). Leer el `.blend` directo o exportar el riel como una curva convertida en malla
  es un hallazgo. **Falla el build** si una cámara o un riel
  queda dentro del escenario o si algún polígono del piso de la navmesh queda sin
  volumen de cámara. Se colocan en todas las zonas, aunque el jugador elija otro estilo.

- **El camino:** `.blend` → `blender -b` → `.glb` → cocinador → binario. Lo exportado y
  lo cocinado viven en el directorio de build: un `.glb` o un binario cocinado
  **versionado** en el repo es un hallazgo. **La única excepción** son los fixtures de
  los tests del cocinador: `.gltf` chicos en `tests/tools/fixtures/`.
- **cgltf y stb_image** están copiados en `tools/third_party/`, con su fila en
  `CREDITS.md` y su anotación en `REUSE.toml` (esto último lo audita
  `build-y-licencias`).
- **El cocinador sigue el ADR-0009:** sin excepciones ni RTTI, y los errores salen como
  un `result` y un código de salida distinto de cero.
- **Blender, con la versión fijada,** y el build la verifica. Cambiar la versión es un
  cambio deliberado, con todo re-exportado.
- **glTF 2.0 es el único formato de intercambio.** Leer FBX, OBJ, USD o `.blend` directo
  cambia la Decisión 2.
- **cgltf y stb_image viven solo en el cocinador.** Si el juego (`engine/` o `game/`) los
  incluye o los enlaza, es `bloqueante`: el juego carga solo el binario cocinado.
- **El binario cocinado:** cabecera con firma y versión, secciones planas con offsets
  relativos y sin punteros. El cargador valida la firma, la versión y que cada offset
  caiga dentro del archivo antes de usarlo. No hay compatibilidad hacia atrás: si el
  formato cambia, sube la versión.
- **El cocinador falla el build, no solo avisa:** un vértice sin pesos en una malla con
  esqueleto, más de 256
  huesos, una malla que se corta con más de 65 536 vértices (índices de 16 bits,
  ADR-0022), un prefijo de colisión mal escrito o desconocido (incluido `UCX_`), una
  primitiva que no calza, o un hueso conservado que cuelga de uno que se quita. Un
  vértice con más de 4 huesos no es un error: primero se funden los pesos de los huesos
  quitados, y después conserva los 4 de mayor peso, se renormaliza y se cuantiza.
- **Formato de vértice de todas las mallas animadas (ADR-0007),** se corten o no:
  índices de hueso `uint8` × 4 y pesos `unorm8` × 4 que suman exactamente 255. El resto
  de los atributos lo define el ticket del cocinador en `engine/core/asset_format`, con
  32 B por vértice como objetivo. Lo que se cuantiza, se cuantiza antes de usarlo en el
  corte.
- **La rotación de ejes es la del cocinador,** y es su primer paso: 180° sobre Y en toda
  la jerarquía. La regla matemática la audita `matematica`.
- **El recorte de huesos no depende de un rig:** el cocinador no conoce nombres de
  Quaternius ni de ningún rig. Cada rig dice qué huesos conserva, solo se quitan
  subárboles completos, y los pesos de los que se quitan se funden en el ancestro
  conservado más cercano.
- **Los clips** se guardan a la frecuencia de la escena de Blender, con cada canal
  constante reducido a un solo valor y cada clave en el mismo hemisferio que la
  anterior.
- **Las texturas guardan su `VkFormat`:** `R8G8B8A8_SRGB` para el color y `_UNORM` para
  los datos.
- **Las texturas son RGBA8 sin comprimir.** BC7, BC1 o Basis cambian la Decisión 5.
- **Lo que pide el render (ADR-0022):** el cocinador genera los mipmaps de las texturas
  de color, promediando en espacio lineal. La tapa procedural no le pide datos nuevos:
  usa las cápsulas por hueso.
- **Las mallas que se cortan (ADR-0007):**
  - el cocinador **falla el build** si una cáscara no es cerrada y *manifold* (cada
    arista soldada con exactamente dos triángulos, de orientaciones opuestas);
  - el binario trae el índice soldado, las cáscaras y los huesos dominantes de cada una,
    calculados con los pesos ya cuantizados;
  - cómo se marca una malla que se corta lo decide el ticket del cocinador. Usar "tiene
    esqueleto" como criterio contradice el ADR-0007: la caballera también lo tiene;
  - lo que el corte hace en el juego lo audita `corte`.
- **Las cápsulas por hueso** se ajustan en el cocinador, después del recorte de huesos y
  de cuantizar los pesos (ADR-0007), con correcciones a mano. Tiene cápsula todo hueso que mueve algún vértice (si no
  domina en ninguno, se ajusta con los de cualquier peso), y uno que no mueve ninguno no
  la tiene. En el escenario, solo `UBX_`, `UCP_` y `USP_`, más el prefijo que retoca la
  navmesh (ADR-0023).
- **La navmesh del escenario** (ADR-0023) la genera el cocinador desde la colisión y el
  piso, con el radio de inflado como parámetro **del mundo** (el mismo en todas las
  zonas), y la escribe en el binario cocinado: polígonos convexos con vértices en 3D, sus
  portales, una grilla para ubicar un punto y el radio, guardado en la sección de la
  navmesh. **La genera para el mundo armado entero y la parte por zona** (ADR-0027), con
  portales hacia los polígonos de las vecinas. Un radio de inflado por zona es una
  decisión nueva (los bordes no coincidirían).
  Es determinista (los mismos bytes dos veces). Generarla al cargar el nivel o en el
  juego, o leer una navmesh hecha a mano como fuente única, es una decisión nueva.
- **Los sonidos** (ADR-0024): los fuentes son FLAC (un WAV, MP3 u Ogg como fuente es
  una decisión nueva). El cocinador los lee con **dr_flac** (copiado en
  `tools/third_party/`) y deja los efectos en PCM16 mono a 48 kHz y la música en Vorbis
  estéreo a 48 kHz, con **libvorbis y libogg** (paquetes del sistema). **Falla el build**
  si un fuente no está a 48 kHz (no remuestrea), si un sonido 3D no es mono o si la
  tabla de sonidos nombra una toma que no existe. Cocinar dos veces da los mismos bytes.
  dr_flac, libvorbis y libogg en el juego son `bloqueante`; el juego solo enlaza
  stb_vorbis, para la música.
- **Las fuentes y los textos** (ADR-0026): el cocinador rasteriza las fuentes de píxeles
  con **`stb_truetype`** (copiado en `tools/third_party/`) **en su grilla exacta, sin
  antialiasing**, a un atlas `R8` con tablas de glifos y de *kerning*, en ×1 y ×2.
  Rasterizar a un tamaño que no es múltiplo de la grilla (los píxeles se deforman) o con
  antialiasing a 640×360 es `importante`. Los textos son un TSV en UTF-8 por idioma, con
  IDs. **Falla el build** si a un idioma le falta un ID, si la fuente no tiene un carácter
  o si un texto no cabe en su ancho, que sale de un **archivo de anchos en los assets**.
  Declarar el ancho en el código del juego es `importante`: el cocinador no enlaza
  `game/` (ADR-0017). `stb_truetype` en el juego, o
  rasterizar fuentes al cargar, es una decisión nueva. Cocinar dos veces da los mismos
  bytes.
- **El mundo** (ADR-0027): el cocinador **ve todas las zonas a la vez**, aplana las
  instancias con `M' = R·M·R`, da a cada nodo el id del hash del camino completo y
  **ordena cada sección por id**; escribe un binario por zona y uno del mundo. Cocinar
  zona por zona sin ver las demás es `importante` (no puede validar ids únicos, vecinas
  ni la navmesh entre zonas). **Falla el build** por un tipo, marcador o evento
  desconocido, una referencia que no existe o que apunta a otra zona, un patrón sin
  marcadores, una fase sin final, dos caminos con el mismo id, un marcador fuera de la
  navmesh, una vecina olvidada (otra zona a menos de la distancia de entrada sin estar en
  la lista) o un punto de la navmesh con más zonas a menos de la distancia de salida que
  el tope menos uno. Avisar en vez de
  fallar es `importante`. Un cambio solo en un texto no abre Blender. Cocinar dos veces da
  los mismos bytes, también con los nodos en otro orden.
- **Los binarios de la carga** (ADR-0028): con los assets ya cocinados, el cocinador arma
  **un binario por zona y uno global**, cada uno con **una parte de la CPU y una de la
  GPU**. **Va al global** lo que usan los personajes, la UI, los VFX y los sonidos
  comunes, y **todo asset que use más de una zona**; lo demás, a su zona. Un asset
  repetido en dos binarios de zona, o un conteo de uso en runtime para decidirlo, es
  `importante`. **La música** no va al global: va en su propio binario, que se carga a
  la región de audio (ADR-0024). **El binario de arranque** (la fuente, "Cargando…" en
  cada idioma y el fondo) es uno solo para todos los idiomas, y su fuente repetida con
  la del global es a propósito; vive al principio del global, comparte su rango de
  índices y cuenta dentro de su presupuesto (una región propia sin fila en
  `presupuestos.md` es un hallazgo de `memoria`). **Lee del texto del mundo** el tamaño del slot (CPU y
  GPU) y el presupuesto del global, los copia al binario del mundo, reporta los tamaños
  y **falla el build** si una zona no entra, **revisando dos medidas** (el binario entero
  contra el slot de la CPU y la parte de la GPU contra el de la GPU), o si una parte del
  global, sumada a la del binario de arranque, se pasa (medir solo el global es
  `importante`); usar "la zona más grande" como tamaño del slot es `importante` (la
  falla nunca ocurriría). **El índice estable de cada fase vive en el texto del mundo**,
  en una lista a la que solo se le agregan filas (las borradas quedan como retiradas),
  y **cada fila nombra la fase por el id de su zona y un nombre de fase**, nunca por su
  número de orden (insertar una fase lo correría): el cocinador lo copia y falla si una
  fase no tiene fila, si una fila no retirada no tiene fase, si un número se repite o si
  la lista pasa de 256. Un índice que asigna el cocinador sin esa lista, o que sale del
  orden del mundo, es `importante` (corre los bits de las tarjetas guardadas). **Escribe en el
  binario del mundo el peor caso de polígonos** entre las zonas cargadas a la vez (la
  memoria de A*, ADR-0023). **Cada
  archivo se escribe a un temporal y se renombra** (la recarga en caliente nunca lee uno
  a medias): escribirlo directo es `importante`. **Sin compresión:** la tabla de
  secciones guarda el codec (0 = crudo); comprimir es una decisión nueva. **En Release,**
  el mundo, el global, las zonas, el binario de arranque y el de la música van en un
  paquete dentro del ejecutable; en
  desarrollo quedan sueltos, junto al binario de cada asset.
- **La definición del formato** (cabecera y secciones) vive en `engine/core`, sin lógica
  de carga.
- **Un asset de terceros que solo existe como `.glb`** se versiona importado a `.blend`,
  con el original anotado en `CREDITS.md`.
- **Git directo, nunca LFS:** un `.gitattributes` con `filter=lfs` es `bloqueante`.
  Ningún archivo pasa de 50 MiB.
- **Las cinemáticas (fe de erratas, ADR-0034):** el script de exportación usa el modo
  `SCENE`, exporta el FOV con `KHR_animation_pointer` y copia los marcadores a los
  `extras`; el cocinador lee ese canal desde el JSON crudo de cgltf. **El cocinador
  falla el build** por un prefijo de marcador desconocido, un actor que no existe, un
  subtítulo sin id en la tabla de textos, un evento fuera de la lista cerrada, un sonido
  que no existe, una rama que nombra una cinemática que no existe, una cámara dentro del
  escenario, o un tramo de salida que cruza el escenario o pasa a menos de 0,5 m de la
  caballera en alguno de los tres estilos. Que falte una de esas validaciones es
  `importante`.
- Las licencias de los assets de terceros y las filas de `CREDITS.md` no son de este
  tema: las audita `build-y-licencias`.

## corte

**ADRs:** 0007 (corte de mallas).

Qué mirar:

- **Corte geométrico de la superficie, en la CPU.** Simular el corte en el shader,
  cortar en puntos fijos o usar una malla de tetraedros cambia la Decisión 1.
- **El corte usa la pose del paso anterior** (ADR-0015): deforma en la CPU solo el
  zombie que se corta, con la misma cuenta que el shader.
- **Solo se corta la región barrida.** El `slice_request` lleva el plano, la recta donde
  se detuvo la hoja (de la guarda a la punta) con el sentido del barrido, y la pose del
  zombie. **La recta y la pose son del mismo paso: el que emite el pedido.** La pose es
  la vigente en ese paso (la que dejó el paso anterior), la misma que usó el combate con
  las cápsulas. Guardar la pose con que termina ese paso es un hallazgo. Un corte
  completo no lleva recta. Dónde se detiene la hoja, y el roce de la punta, los decide
  el combate con las cápsulas, no el corte con la malla.
- **El corte usa la pose guardada en el pedido,** también cuando se procesa en un paso
  posterior. Usar la pose actual de un zombie diferido es un hallazgo.
- **Contornos contra la región:** un contorno entero dentro se corta completo; uno en
  parte dentro deja una hendidura, sin separar, abierta en cuña (máxima en la boca, 0 en
  el fondo); uno al que le falta un solo segmento se corta completo. El fondo sigue las
  aristas de la malla, con los triángulos del borde partidos en abanico (sin vértices
  en T).
- **Clasificación:** una sola vez por vértice soldado, con una distancia exactamente 0
  del lado positivo. Volver a clasificar un vértice por triángulo, o comparar posiciones
  con tolerancia para encadenar contornos, es un hallazgo.
- **Un punto de corte por arista soldada,** calculado una sola vez desde el extremo de
  menor índice. Los contornos se encadenan por índice, nunca por posición.
- **Lo que hace el cocinador para el corte** (índice soldado, cáscaras, huesos dominantes
  y validación *manifold*) lo audita `assets`. Aquí solo se revisa que el corte no
  recalcule el índice de la malla original en el juego: lo extiende a sus vértices
  nuevos por construcción (cada vértice nuevo apunta a su punto de corte **de su
  lado**), nunca comparando posiciones. Los dos lados de un punto partido tienen
  identificadores distintos, y un punto unido (el fondo de la ranura), uno solo. Darles
  el mismo identificador a las dos caras de una ranura es un hallazgo: cada arista de la
  boca quedaría con 4 caras.
- **Tapas cortando orejas,** propio, en una base del plano con `u × v = n`. Los agujeros
  se buscan solo en la misma cáscara. Los contornos invertidos se triangulan solos, con
  su propia orientación, y los de área cero, como exteriores: descartarlos deja el
  pedazo abierto. Las coordenadas de textura de la tapa salen de una proyección plana en
  esa base.
- **Vértices nuevos des-deformados** (`b = (Σ wᵢ·Mᵢ)⁻¹·p`), con los pesos ya mezclados,
  recortados a 4 y cuantizados a `unorm8` (que suman exactamente 255). Interpolar la
  posición de referencia, o des-deformar antes de cuantizar, es un hallazgo. Si el
  determinante de la parte 3 × 3 queda bajo el umbral, el vértice pasa entero a su hueso
  dominante (y se revisa también el determinante de esa matriz: un hueso con escala 0 no
  se invierte; en ese caso, el último recurso es interpolar). Dividir por un
  determinante sin revisar es un hallazgo. La normal de la
  tapa se des-deforma con la inversa de lo que hace el shader, y se renormaliza.
- **La cuña** desplaza cada vértice de una cara `Δb = A⁻¹·(±(h/2)·n)` en la pose de
  referencia: `+` en la cara del lado positivo, `−` en la del negativo, y `h` como ancho
  total, de 0 en el fondo al máximo en la boca. El vértice del borde de la piel y su
  copia en la tapa, del mismo lado, se desplazan igual. Un mismo signo en las dos caras
  es un hallazgo: la ranura no se abre.
- **El corte escribe en cada pedazo:** la máscara de huesos (los que tienen peso
  distinto de cero en algún vértice del pedazo, no los que dominan), las cápsulas
  recortadas y el índice soldado extendido con sus cáscaras y huesos dominantes. En el
  pedazo vivo, la máscara no quita matrices: la jerarquía completa se sigue evaluando.
  Los pedazos rígidos usan sus matrices congeladas en la pose del corte (ADR-0016), y
  eso no es un hallazgo. Solo se recortan las cápsulas donde el corte separa pedazos: una
  hendidura no las cambia. La geometría de recortar una cápsula vive en `engine/core`
  (la llama el corte, y la especifica y prueba la física).
- **Un corte da N pedazos:** fragmentos conexos, unidos cuando comparten un hueso
  dominante **del mismo lado**. El lado se mira por triángulo: un fragmento con hendidura
  tiene los dos, y se une por cada lado con sus huesos de ese lado. Juntar fragmentos de
  lados distintos por un hueso vuelve a pegar las dos mitades de un hueso cortado.
- **Los pedazos comparten el buffer de vértices del modelo** y guardan sus índices y sus
  vértices nuevos. Copiar todos los vértices de cada pedazo cambia la Decisión 8.
- **Cuota de 4 cortes por paso,** contada en pasos. Los que sobran se reparten en los
  pasos de la pausa de impacto, o en el paso siguiente si no hay pausa. Esperan en un
  almacenamiento propio del corte, no en la arena por frame, con un tope de 20. Antes de
  emitir un pedido, el combate pregunta al corte si queda lugar (consulta directa hacia
  abajo); si no queda, no lo emite y aplica él mismo el empuje. Un canal del corte hacia
  el juego es un hallazgo. Los pedazos bajo el mínimo se descartan.
- **Determinismo:** los identificadores se asignan en el orden de los triángulos, y las
  tablas hash solo se consultan, nunca se recorren.
- **El campo de distancia de la tapa** (fe de erratas del ADR-0007, por el ADR-0025):
  el corte rasteriza el contorno en la base `(u, v)` del plano en una grilla de 64² y le
  aplica una transformada lineal que **propaga la arista más cercana** y mide la
  distancia exacta a ese segmento. Las dos tapas del corte lo comparten. Calcularlo por
  fuerza bruta (cada texel contra cada arista: 3,6 ms con 64²) es `importante`: no cabe
  en el paso del corte. Medir solo hasta los centros de los texeles también es
  `importante`: el error (~5 mm) pasa la capa de piel. **El campo es del corte:** se
  libera con la última tapa que lo usa (cola por número de frame) y, con el pool lleno,
  el corte nuevo reusa el más viejo. Abortar o no liberarlo es un hallazgo.
- El TDD y la cobertura del corte no son de este tema: los audita `testing`.

## testing

**ADRs:** 0011, 0007, 0010, 0015, 0016, 0017, 0018, 0023, 0024, 0025, 0026, 0027, 0028,
0031, 0032, 0034 y 0035 (la suite de Windows de la CD, bajo Wine y sin *sanitizers*;
reparto del TDD en el corte, la memoria, la animación, la física, el
cocinador de assets, el cargador, la tarjeta de memoria, el resorte y el barrido de la
cámara, las piezas puras del input y el secuenciador de las cinemáticas; los tests del
combate, de la IA, del audio, de los VFX, de la UI, de los niveles, del guardado, del
sistema de cámara y de la capa de acciones).

- **Las cinemáticas** (ADR-0034): el muestreo de las pistas, el reloj en pasos, el salto
  y las ramas son piezas puras del secuenciador, en TDD estricto (sin test es
  `bloqueante`), igual que lo que valida el cocinador de cinemáticas. **El test de
  propiedad** (saltar en cualquier paso da el mismo hash que verla entera) es
  obligatorio desde el ticket del secuenciador; que falte es `importante`.

- **El input** (ADR-0032): la zona muerta radial escalada, la curva, la histéresis de los
  gatillos, los contadores de presiones, la regla del paso por *timestamp* (con los
  *timestamps* del futuro y del pasado), el *buffer* en pasos, la lectura de los
  controles de `opciones.txt` y los conflictos son piezas puras, en TDD estricto (sin
  test es `bloqueante`). **La misma traza cruda** (eventos con su *timestamp*) con varios
  horarios de frames tiene que dar **las mismas acciones por paso** desde el ticket de las
  piezas puras, y **el mismo hash** desde el ticket de la guía de la espada, cuando ya
  existe la simulación que apunta. No exigir el hash antes.

- **La cámara** (ADR-0031): el resorte críticamente amortiguado y el barrido de esfera son
  matemática del núcleo, en TDD estricto (sin test es `bloqueante`); el sistema de cámara
  lleva tests después. Un test de *replay* que cambie de estilo de cámara tiene que dar el
  mismo hash.

Qué mirar:

- **GoogleTest.** Los tests de propiedades usan una semilla fija que se imprime al
  fallar.
- **TDD estricto** en la matemática, la memoria, los contenedores, el ECS y las
  funciones del corte. Código nuevo en esas zonas **sin test** es `bloqueante`.
- **En la memoria (ADR-0010):** la arena (alineación, marcador, reinicio, y que
  quedarse sin espacio aborta, con un *death test*); el pool (reutiliza slots, y un
  handle viejo se detecta por su generación); el allocator de offsets (pedir, devolver,
  unir con cada vecino, fallar sin un bloque que alcance, y una prueba de propiedades:
  ningún bloque se superpone y lo libre más lo usado suma el total); la cola de
  liberación (nada se libera antes de que termine su frame); el *ring buffer* (da la
  vuelta sin pisar una región en vuelo); y en Debug con ASan, un *death test* por cada
  error: leer después de reiniciar, escribir pasado el final y usar un slot devuelto.
- **En el corte (ADR-0007),** el TDD y el 100 % de cobertura aplican a todo:
  - clasificar, incluido un vértice exactamente sobre el plano;
  - partir triángulos: el solitario de cada lado, conservando el sentido de giro, y el
    caso de un vértice sobre el plano (queda un triángulo de área cero sin abrir el
    pedazo);
  - el índice soldado con una costura de UV, y su extensión a los vértices nuevos;
  - encadenar contornos (uno abierto falla con un error, no se cuelga);
  - cortar orejas, con agujeros, contornos invertidos, de área cero y puntos alineados;
  - des-deformar (incluida una matriz casi singular y un hueso dominante con escala 0) y
    cuantizar pesos;
  - armar los pedazos, incluido un torso con hendidura que se une con las piernas por sus
    huesos del lado de abajo;
  - qué cápsulas se recortan;
  - la hendidura (estados de los puntos, abanico, cuerda, cuña con 0 en el fondo y
    signos opuestos en cada cara, y el contorno al que le falta un solo segmento);
  - volver a cortar un pedazo ya cortado, incluida una herida abierta (ninguna arista de
    la boca queda con más de dos caras).

  Cada caso va calculado a mano (un cubo da 2 pedazos; una "U" con un hueso distinto en
  cada pata, cortada a través de las dos patas, da 3; dos cubos superpuestos con el mismo
  hueso dan 2). Además:
  - invariantes de malla después de cada corte: cada pedazo cerrado y orientado, ninguna
    arista con más de dos caras y ningún triángulo con índices repetidos. Los triángulos
    de área cero solo se toleran en las tapas de contornos degenerados y al partir un
    triángulo con un vértice sobre el plano, o tan cerca que el punto de corte coincide
    con él. En los cortes completos, cada vértice
    original queda del lado de su clasificación, cada vértice nuevo queda a menos de una
    tolerancia explícita del plano, y el volumen se conserva (medido antes de descartar
    pedazos chicos);
  - un test de determinismo: el mismo `slice_request` dos veces da los mismos bytes;
  - pruebas de propiedades con planos y rectas de detención al azar, incluidos planos que
    pasan justo por vértices;
  - fuzzing del corte: nunca lee fuera de un buffer, y una malla inválida da un error;
  - las mallas de prueba se arman en el código de los tests, no son archivos.
- **En la animación (ADR-0015),** el TDD y el 100 % de cobertura aplican al skinning de
  la CPU (va en la ruta del corte), a la IK de dos huesos, al recorrido de la jerarquía
  y al *nlerp*. Código nuevo en esas piezas sin test es `bloqueante`, como en las demás
  zonas con TDD. La máquina de estados y la mezcla de capas llevan tests después.
- **Cuando exista el renderer,** el skinning trae un test que compara la GPU (con
  lavapipe) con la CPU, vértice a vértice y con tolerancia (ADR-0015).
- **En la física (ADR-0016),** llevan TDD y el 100 % de cobertura las pruebas de cada
  forma que se mueve (esfera, cápsula y la cápsula del controlador) contra cada forma
  (esfera, cápsula y caja estática), el rayo, el barrido de esfera, recortar una cápsula
  en un plano y el barrido del tajo. Código nuevo en esas piezas sin
  test es `bloqueante`. El solver y el controlador llevan tests después, con escenarios
  conocidos (una pila de pedazos que se queda quieta, una esfera que se detiene, un
  ragdoll que no explota). Hay un test de determinismo desde el principio: el mismo
  escenario dos veces, con el mismo hash del estado.
- **En el cocinador de assets (ADR-0017),** llevan TDD y el 100 % de cobertura las
  conversiones puras: la rotación de 180° sobre Y, fundir huesos y pesos, conservar los 4
  huesos de mayor peso, quitar canales constantes, alinear el hemisferio de las claves y
  el ajuste de cápsulas y primitivas (incluido que un hueso que no mueve vértices no
  tiene cápsula). También lo que hace para el corte (ADR-0007): el índice soldado, las
  cáscaras con sus huesos dominantes, la cuantización a `unorm8` que suma 255 y la
  validación *manifold*, con casos que fallan el build (una arista abierta, una con tres
  caras y dos caras vecinas con la misma orientación). Viven en una biblioteca probada en
  `tests/tools/`. Código nuevo ahí sin test es `bloqueante`. Cada propiedad va con casos
  calculados a mano: "rotar dos veces devuelve el original" solo no alcanza, porque no
  detecta un espejo (hace falta la normal de un triángulo). Además: fixtures `.gltf`
  chicos en `tests/tools/fixtures/`, un test de que cocinar dos veces da los mismos
  bytes, y fuzzing del cargador del binario.
- **En el combate (ADR-0018),** el gameplay lleva tests después, no TDD, y la sensación
  se prueba jugando. Las reglas son funciones puras con casos calculados a mano: el área
  de una cápsula perpendicular a la hoja, el umbral con el ángulo y con la junta, la
  curva de carga, la fuerza que sobra entre dos zombies y la regla de vivo o muerto sobre
  máscaras de huesos armadas a mano. La espada clavada y el agarre tienen tests de
  *replay*. Al ticket del corte se suma un caso: volver a cortar una herida abierta con
  el mismo plano y la misma pose, que pasa justo por el fondo de la ranura.
- **En la IA (ADR-0023):** **la generación de la navmesh vive en el cocinador y lleva su
  TDD:** sobre cajas conocidas, polígonos convexos conectados por portales que dejan
  afuera lo inflado, los mismos bytes al cocinar dos veces, y un prefijo de retoque mal
  escrito que falla el build. Código nuevo del generador sin test es `bloqueante`. **La
  IA y la navegación en el juego llevan tests después,** con casos calculados a mano: A*
  y el *funnel* sobre una navmesh armada a mano, el programa lineal de ORCA (dos agentes
  de frente que no chocan), el reparto de fichas (cercanía, ventaja del cono de la
  caballera, desempate) y la evaluación de un árbol chico. La horda entera tiene tests de *replay*
  con el hash del estado.
- **En el audio (ADR-0024):** **la cola de comandos de un productor y un consumidor es
  un contenedor y lleva TDD estricto** (llena, vacía, la vuelta del índice). **Los pasos
  de audio del cocinador llevan su TDD:** leer un FLAC conocido, fallar con 44,1 kHz,
  fallar con un sonido 3D en estéreo y cocinar dos veces los mismos bytes. Código nuevo
  en esas piezas sin test es `bloqueante`. **El resto lleva tests después,** con casos
  calculados a mano: el paneo y la atenuación, el robo de voces y el límite por sonido,
  la conversión del paso al reloj del audio y su reajuste, el limitador y la elección de
  tomas por hash. Además, **un render offline del mezclador** (sin dispositivo, con un
  guion de eventos) contra un hash guardado, comparado con la glibc fija de la CI
  (ADR-0029).
- **En los VFX (ADR-0025):** **el campo de distancia de la tapa lo llena el corte, así
  que lleva el TDD estricto del corte**, con un caso que la compara con la fuerza bruta,
  con una tolerancia en milímetros. Código nuevo ahí sin test es `bloqueante`. **El resto
  lleva tests después,** con casos calculados a mano para el recorte de las marcas y la
  integración.
- **En la UI (ADR-0026):** **TDD estricto** en el cocinador de fuentes y de textos (que
  falle por un ID que falta, un carácter que la fuente no tiene o un texto que no cabe, y
  que cocinar dos veces dé los mismos bytes) y en la lógica pura: UTF-8, *layout*, corte
  de línea, foco y repetición del stick. Código nuevo ahí sin test es `bloqueante`. **Las
  pantallas llevan tests después,** con capturas de referencia comparadas con la
  tolerancia que fije el ADR-0011.
- **En los niveles (ADR-0027):** **TDD estricto** en el cocinador del mundo (cada
  validación con su caso que falla, ids ordenados, y que cocinar dos veces, o con los
  nodos en otro orden, dé los mismos bytes) y en cargar una zona al ECS (las entidades en
  orden de id, el mismo hash). Código nuevo ahí sin test es `bloqueante`. **El sistema de
  fases y triggers lleva tests después**, con el *replay* de cada fase. **Un test de
  *replay* inyecta tiempos de carga al azar** y exige el mismo hash; sin él, las zonas
  que se cargan al caminar no tienen prueba de determinismo (`importante`).
- **En la carga y el guardado (ADR-0028):** **TDD estricto** en el cargador (validar la
  cabecera y los offsets, con archivos rotos a propósito), en el cocinador de los
  binarios de la carga (el reparto, los tamaños y sus fallas) y en **la tarjeta de
  memoria** (el diseño de los bytes, el CRC, los bloques A y B, el bit de partida y la
  lectura de tarjetas de cada versión guardadas como datos de prueba). Código nuevo ahí
  sin test es `bloqueante`. **Un test mata el proceso mientras guarda** y exige una
  partida válida al cargar; sin él, la robustez del guardado no tiene prueba
  (`importante`). **Un test enlaza un paquete chico en un ejecutable de prueba** y lo
  lee por `platform::`, en `debug`; sin él, el camino de Release no se prueba nunca
  (`importante`). **También TDD estricto** en los slots (memoria del núcleo, ADR-0011) y
  en las dos colas sin locks del hilo de I/O (como la cola del audio). La recarga en
  caliente lleva tests después.
- **Cobertura del 100 % de líneas y ramas** en el core y en las zonas con TDD. Las
  ramas de los `assert` se cubren con *death tests*. Una línea imposible de cubrir lleva
  una justificación escrita junto al código.
- **Organización:** un ejecutable por módulo (`tests/core/`, `tests/slicing/`…), más
  `tests/regression/`, y etiquetas de CTest por tipo.
- **Regresión de bugs:** `tests/regression/bug_<N>.cpp` con el test `Bug<N>_…` y la URL
  del issue. Lleva `WILL_FAIL` mientras el bug está abierto, y se le quita en el PR que
  lo arregla.
- **Los tests corren en el preset Debug**, con ASan y UBSan. **Excepción (fe de erratas
  del ADR-0011, por el ADR-0035):** la suite de Windows de la CD corre bajo Wine sin
  *sanitizers*; por eso **cada test que depende de un *sanitizer*** (los *death tests* de
  ASan) **lleva la etiqueta de CTest que lo excluye ahí**. Un *death test* de ASan sin la
  etiqueta es `importante`: rompe la CD.

## diagramas

**ADRs:** 0030 (diagramas técnicos).

Qué mirar:

- **Solo D2.** Un diagrama técnico nuevo en Mermaid, PlantUML, Graphviz o una
  herramienta visual cambia la Decisión 1. La excepción es el piloto guardado como
  historial en `docs/research/diagramas-piloto/`.
- **Cada diagrama `.d2` tiene su SVG al lado** (la leyenda y los fragmentos
  `.ejemplo.d2` no lo llevan, porque solo se importan), con el mismo nombre, generado con
  `--dark-theme`. Un `.d2` cambiado con su SVG sin regenerar es un hallazgo. Se revisa
  el `.d2`, no el diff del SVG.
- **`docs/vision/mapa/` no es técnico:** el mapa del castillo dibuja zonas del juego y
  usa su propia leyenda (`docs/vision/mapa/leyenda.d2`). Solo se revisa que cada SVG
  esté al día con su `.d2`; las reglas de abajo no le tocan (#77, `docs/diagramas/README.md`).
- **`docs/diagramas/teorico/` no es técnico** (fe de erratas del ADR-0030, #69): el
  diagrama teórico del proyecto muestra lo que eligió cada ADR y usa su propia leyenda
  (`docs/diagramas/teorico/leyenda.d2`). Como al mapa, solo se revisa que el SVG esté al
  día con su `.d2` (verificador (b)); las reglas de abajo no le tocan.
- **Cada diagrama importa la leyenda** de `docs/diagramas/leyenda.d2` y usa sus clases.
  Estilos sueltos que repiten o contradicen la leyenda son un hallazgo. Cada diagrama
  tiene título, y todo elemento tiene su propio relleno. Un sistema del ECS es un
  contenedor `system` con sus funciones dentro, y un módulo usa `module`. Una línea
  lleva etiqueta cuando no se explica sola; en un flujo función → dato → función, la
  explica el dato.
- **Nombres reales:** las funciones van con `()` y el nombre del código. El id de cada
  dato es el nombre de su tipo, tal cual (`signed_distances`), un tipo por dato.
- **Ubicación y niveles:** el frame y lo transversal en `docs/diagramas/`; los sistemas
  y procesos de un área en `docs/<área>/diagramas/`. Cada carpeta tiene un `README.md`
  que muestra sus SVG y enlaza al nivel de arriba y a los de abajo. Un link roto es un
  hallazgo.
- **Ejemplos:** en los diagramas de proceso, el ejemplo lo calcula un test y el diagrama
  importa su fragmento `<diagrama>.ejemplo.d2`. Números escritos a mano en un diagrama
  de proceso son un hallazgo.
- **Cuándo:** un PR que le da comportamiento a un sistema nuevo trae su diagrama; un PR
  que cambia un flujo dibujado (el orden de las llamadas, una función nueva, borrada o
  renombrada, o un dato que cambia de tipo) actualiza el diagrama en el mismo PR.
  Faltar a esto es `importante`. Un sistema vacío no lleva diagrama.
- **Verificadores:** cuando existan, el PR los corre y pasan: nombres, SVG, leyenda,
  links, ejemplos, orden estático, tipos, orden dinámico y paso nuevo. La macro de traza
  es la macro de zona del profiling (fe de erratas del ADR-0030, por el ADR-0021): en
  Debug alimenta la traza, y en RelWithDebInfo alimenta a Tracy. Las funciones que
  corren por elemento usan su variante solo de traza, vacía en RelWithDebInfo. Que la traza esté activa
  en RelWithDebInfo o en Release es un hallazgo, porque altera las mediciones de
  rendimiento.
- **Lo que los verificadores no ven** es justo lo que mira este tema: si el diagrama
  describe bien lo que pasa dentro de cada función, si las etiquetas describen bien sus
  datos y si las ramas que el test no recorre están dibujadas.

## gameplay

**ADRs:** 0018 (combate y desmembramiento). También la visión (`docs/vision/README.md`),
que el ADR-0018 actualizó a su versión 3.

Qué mirar:

- **Modelo de fuerza D:** la fuerza se gasta **por área cortada**, por la resistencia del
  material, en el orden en que la hoja alcanza cada parte, y lo que sobra sigue al zombie
  siguiente. **La carne no tiene umbral de entrada; el metal sí.** Un umbral por zombie
  (todo o nada), un costo fijo por parte o la armadura como cáscara cambian la Decisión 1.
- **El combate decide con las cápsulas, nunca con la malla** (también lo pide el
  ADR-0007). El material es por cápsula, en una tabla por tipo de zombie. El umbral del
  metal se divide por `max(|dirección del hueso · normal del plano|, piso)`. Una parte es
  **junta** si el plano cruza su eje cerca de una articulación; juntas como esferas en el
  espacio contradicen la Decisión 2.
- **La carga** sube rápido al principio y cada vez más lento. Si la energía llega a 0
  cargando, la carga se cancela y la caballera entra en recuperación: un golpe debilitado
  contradice la Decisión 1. **La fuerza sale solo de la carga.** La duración del barrido
  baja con la carga, pero la velocidad no suma fuerza; contarla en el gasto es un
  hallazgo. El tajo rápido tiene su propia duración, corta.
- **Bajo el umbral, el tajo rebota** con aturdimiento y recuperación de energía más lenta.
  **El tajo rápido nunca se clava:** corta solo lo que atraviesa entero y, si no, empuja.
  Su plano es horizontal, por el pecho, en la dirección en que mira la caballera, y su
  recta de detención va un margen angular (configurable) antes de la primera parte que no
  atraviesa, para no abrirle una ranura.
- **El pedido de corte se emite al salir o detenerse la hoja,** uno por zombie y por tajo,
  después de preguntar al corte si queda lugar (ADR-0007). **La pausa de impacto es una
  por tajo,** empieza en ese paso, congela solo a la caballera y al zombie golpeado, y su
  duración es configurable (0 la apaga).
- **Escala de tiempo por personaje:** la aplican el gameplay, el combate, la IA y la
  animación. Los cuerpos dinámicos (pedazos y ragdolls) siguen el tiempo global; frenarlos
  dentro del solver es un hallazgo. Un zombie con un pedido de corte en la cola queda con
  escala 0 hasta que se procese.
- **El plano del tajo** pasa por el pecho de la caballera y por el punto apuntado, con la
  dirección de la línea dibujada, y el centro del arco es el pecho. **El punto apuntado es
  el primer impacto del rayo que sale del pecho** (fe de erratas, ADR-0031) contra las
  cápsulas, con un punto de reserva si no toca nada. **Usar la cámara para apuntar es
  `bloqueante`:** la cámara es una opción del jugador y está fuera de la simulación.
  **Por fases:** al apuntar, el ratón o el stick mueven el punto dentro de un cono
  alrededor del rumbo; al cargar, **la dirección del rayo queda fija respecto del rumbo**
  (no un punto del mundo) y giran la línea; la guía termina al soltar, y **el rayo se
  vuelve a lanzar al soltar**. Fijar un punto del mundo al cargar es un hallazgo. El 90° del arco (ADR-0015) es la dirección del pecho al punto apuntado, sin
  su componente sobre la línea.
- **El tajo especial se resuelve al soltar** (fe de erratas, ADR-0031): el combate
  predice el barrido entero contra las poses de ese paso, con una consulta de solo
  lectura; si cumple las reglas, los pedidos de corte salen en ese paso. Una predicción
  que escriba en el estado, o que use la cámara o el reloj, es `bloqueante`. Los tajos
  normales piden el corte al salir o detenerse la hoja.
- **La espada clavada** es una secuencia: el zombie avanza por la hoja **con el esqueleto
  fijo en la pose del corte** (solo se mueve su raíz), los demás van a una escala de
  tiempo baja, y cada pulsación gasta energía. Terminar el corte es un `slice_request`
  **completo con el mismo plano y la misma pose** sobre la herida abierta. Si el zombie
  llega o la energía llega a 0, pasa al agarre. En la cabeza, el zombie muere y la hoja
  queda libre; en un pedazo del suelo, la hoja no se clava.
- **Vivo o muerto** corre en el gameplay del paso siguiente al corte, sobre las máscaras
  de huesos de los pedazos: vive el pedazo con la cabeza completa que conserva el torso
  o el segmento superior de un brazo o de una pierna. Esa regla dentro del corte es un
  hallazgo (el corte no sabe de vivos ni muertos, ADR-0012). **Los pedazos nacen sin
  clasificar:** en el paso del corte, la física no les crea cuerpo y la animación no los
  evalúa; un cuerpo creado antes de la clasificación es un hallazgo. La cabeza, el torso
  y los segmentos de cada extremidad se declaran en la tabla del tipo de zombie, no en
  el cocinador.
- **Los pedazos del suelo se cortan.** Dos reglas conviven para los pedazos chicos: el
  combate no emite el pedido si la parte que se separaría es más delgada que la hoja
  (medido con las cápsulas), y el corte sigue descartando los pedazos bajo su mínimo
  (ADR-0007). Pasa a ragdoll el pedazo con más de N huesos (N configurable). Los
  personajes empujan los pedazos sin frenarse.
- **El agarre** se detecta con una consulta de superposición al final del ataque del
  zombie, que necesita un brazo útil. Es en tiempo real para todos, se resuelve
  presionando rápido y con eventos, y al zafarse libera de todos. Si la agarran cargando,
  hay un margen para soltar el tajo. **Accesibilidad:** la secuencia y el agarre tienen
  una alternativa de mantener en las opciones; que falte es `importante`.
- **Todos los números del combate se ajustan sin recompilar** (materiales, curva, piso
  del ángulo, radio de junta, pausa, aturdimiento, escalas de tiempo, N de los ragdolls,
  mínimo de zombies para la cámara del tajo). Un número del combate fijo en el código es
  un hallazgo.
- **Determinismo:** ninguna regla usa azar ni el reloj; la secuencia lee el input por
  paso.
- Los tests del combate no son de este tema: los audita `testing`.

## ia

**ADRs:** 0023 (IA de los zombies y navegación).

Qué mirar:

- **El orden dentro del gameplay:** percepción → fichas → decisión → camino → ORCA →
  velocidad deseada en un componente. **La IA no mueve a nadie:** mueve la física, con
  esa velocidad, en su turno del paso. Una IA que escriba la posición es un hallazgo.
- **Los zombies deciden con un BT plano:** los nodos de cada tipo en un arreglo
  compartido, con los hijos contiguos, y las hojas despachadas con un `switch` sobre su
  id. Un BT con nodos virtuales, memoria asignada al decidir o recursión sin el máximo de
  profundidad que verifica el constructor rompe el ADR-0009 (lo audita
  `cpp-y-convenciones`). Se evalúa desde la raíz en cada decisión; las acciones
  comprometidas (preparar el ataque, agarrar, recuperarse) avanzan fuera del árbol.
  **El punto del agarre lo marca el temporizador de la tabla del tipo**, y el clip se
  reproduce escalado a esa duración; que la consulta del agarre dependa del clip (un
  evento de animación, otro reloj) es un hallazgo.
  **Los jefes** (el alquimista, que el ADR llama "el mago") son una FSM propia con
  `switch`; un jefe metido en el árbol común es un hallazgo. El rey y la reina ya no son
  jefes. Si los mini jefes (el cazador y su caballo, el prisionero gigante) llevan FSM o
  el BT común sigue pendiente (fe de erratas del ADR-0023, #77).
- **Los árboles se arman en C++ al iniciar,** fuera del paso. **Los números de cada
  tipo** (velocidad, alcance, tiempos del ataque, radios de percepción) van en la tabla
  de datos del tipo y se ajustan sin recompilar. Un número fijo en el código es un
  hallazgo.
- **Percepción:** vista con distancia, ángulo y **el rayo de la física**; oído por
  eventos de ruido que emite el combate; contagio entre vecinos. **El ruido va en un
  arreglo fijo del mundo,** no en una cola del paso: una cola de la arena del paso se
  borra antes de que la IA la lea, y el ruido se pierde. Con el arreglo lleno, el ruido
  nuevo reemplaza al de radio más chico si es más grande; si no, se descarta y un
  contador lo registra. Un ruido que se pierde sin contarlo es un hallazgo (lo audita
  `memoria`). En el corte vertical, un zombie alertado no olvida.
- **Fichas de ataque** (3 para empezar, configurable): la ficha va al más cercano, con
  un descuento si está dentro del **cono alrededor del rumbo de la caballera** del paso
  anterior (fe de erratas, ADR-0031; leer la cámara es `bloqueante`: elegir otro estilo
  cambiaría la partida), y el desempate es por
  entidad. Se toma al empezar a preparar el ataque y se devuelve al terminar de
  recuperarse, **o antes** si su dueño muere, pierde los brazos útiles, queda ensartado
  o pasa a escala 0 por un pedido de corte. Una ficha que no vuelve en esos casos es
  `importante`: la horda se queda sin fichas. Los que no tienen ficha esperan en anillo.
  **No usan ficha** el guardaespaldas, los jefes y el agarre que sale de la espada
  clavada (lo lleva el combate, ADR-0018). Fuera de esos casos, un zombie de la horda
  común que ataca sin ficha es `importante`: rompe la justicia que pide el ADR-0018.
- **La navmesh:** polígonos convexos con portales, generada por el cocinador (lo audita
  `assets`). Una grilla, *waypoints* o un campo de flujo como representación son una
  decisión nueva. **Recast/Detour no es una dependencia:** incluirlo lo audita
  `build-y-licencias`. **Al cargar el mundo,** el juego verifica que ningún radio de las
  tablas de tipos supere el radio de inflado guardado en la navmesh, y si lo supera,
  falla con un mensaje; que falte esa verificación es un hallazgo.
- **Caminos:** A* sobre los polígonos y *funnel*. Si el zombie ve a su objetivo, camina
  directo; esa línea de vista es **un rayo sobre la navmesh**, no el de la física.
  Seguimiento reactivo hacia un punto más adelante. El camino es un arreglo fijo de 32
  esquinas en `nav_agent`. **La memoria de A* se reserva al iniciar**, con el peor caso
  de polígonos que el cocinador escribe en el binario del mundo (ADR-0023, ADR-0028);
  reservarla al cargar una zona, o que una búsqueda asigne memoria, lo audita `memoria`.
- **El recálculo** se reparte por **el índice estable del *handle***, cada 30 pasos
  (configurable) o antes si el objetivo se aleja del final del camino. Repartir por la
  posición en el arreglo denso del *sparse set* es `importante`: esa posición cambia
  cuando muere otro zombie, y con ella el turno, así que un camino puede quedar viejo
  hasta ~59 pasos y los recálculos se juntan. No rompe el determinismo: con un hilo y
  destrucción diferida, las muertes se repiten en el mismo orden.
- **ORCA entre agentes:** hasta 10 vecinos a menos de 3 m, horizonte de 1 s, la mitad
  del esquive para cada uno, y todo el esquive contra la caballera y contra un zombie
  quieto (preparando el ataque, agarrando, congelado o ensartado). Los muros los
  resuelven la navmesh y el controlador, que sigue sacando a los zombies de lo que
  penetran (ADR-0016).
- **El zombie mutilado:** el pedazo vivo sigue con el mismo árbol y su nuevo estado de
  locomoción (ADR-0018), y su camino se descarta. Sin brazos útiles, no ataca ni pide
  ficha; el que se retuerce no se mueve ni decide. Una sola navmesh, inflada por el
  radio mayor del corte vertical (0,45 m).
- **Escala de tiempo:** la IA multiplica sus temporizadores por la escala de cada
  personaje; con escala 0 no decide ni pide camino. **El zombie ensartado no decide.**
- **Determinismo:** sin azar ni reloj; la cola abierta de A*, las fichas y los vecinos
  de ORCA desempatan por índice; los valores ajustables son CVars que afectan la
  simulación (ADR-0021).
- **Depuración (ADR-0021):** dibujo de la navmesh, los caminos, la velocidad deseada
  contra la elegida, el cono de la vista, los ruidos y quién tiene ficha; el panel del
  zombie elegido con el recorrido de su árbol; contadores de A*, fichas y vecinos en el
  registro por frame.
- Los tests de la IA no son de este tema: los audita `testing`.

## audio

**ADRs:** 0024 (audio). También la fe de erratas que hizo en el ADR-0004 (SDL3 solo
saca el sonido) y en el ADR-0012 (el hilo de audio, segunda excepción al hilo único), y
la fe de erratas del ADR-0032 en este ADR (la vibración lee el arreglo de eventos).

Qué mirar:

- **SDL3 solo abre el dispositivo y llama al callback** (`SDL_OpenAudioDeviceStream`).
  Usar `SDL_BindAudioStreams` para mezclar, `SDL_PutAudioStreamData` desde el hilo
  principal para empujar audio, SDL3_mixer o miniaudio es una decisión nueva. **Ningún
  tipo de SDL sale de `platform::`:** el engine recibe un buffer de float estéreo (lo
  audita también `arquitectura`).
- **El contrato del callback:** no asigna memoria, no toma locks, no hace I/O, no
  registra y no llama a nada que bloquee. Solo toca memoria reservada al iniciar. Un
  `malloc`, un `std::vector` que crece, un mutex, un `printf`, una macro de log, una
  lectura de disco o libvorbis (que asigna al decodificar) dentro del callback es
  `bloqueante`: el audio se corta. Una zona de Tracy en el callback está permitida en
  los builds de desarrollo.
- **El callback solo ejecuta.** Las decisiones (qué voz, a quién robar, ganancias,
  cuándo termina un sonido) se toman en el hilo principal, que lleva una copia de cada
  voz. Que el callback decida un robo o elija una toma es un hallazgo.
- **Comunicación:** comandos por una cola de un productor y un consumidor sin locks
  (1024 para empezar; un comando que no entra se descarta y se cuenta). De vuelta, solo
  atómicos: el reloj del audio y los contadores. Cualquier otro dato compartido entre
  los dos hilos es `bloqueante`.
- **Liberar sonidos:** un sonido cocinado no se libera mientras una voz lo lee. Antes de
  liberarlo, el hilo principal para sus voces y espera a que el reloj del audio pase el
  bloque de la orden. Liberar sin esa espera es `bloqueante` (el callback lee memoria
  liberada).
- **Los eventos de sonido** van en un arreglo fijo del sistema de audio, con vida de
  frame (por ejemplo, 256), no en una cola del paso (se borra al terminar el paso) ni en
  la arena del frame (es del render). Lleno: el nuevo reemplaza al de menor prioridad si
  es más importante; si no, se descarta y se cuenta. **La vibración (ADR-0032) es un
  segundo lector,** que solo lee, después del audio: el arreglo se vacía al final del
  frame. **Vaciarlo antes (por ejemplo, al leerlo el audio) es `importante`:** la
  vibración se queda sin eventos.
- **El tiempo:** cada evento suena en el tiempo de su paso más un desfase con el reloj
  del audio (latencia de partida: 2 pasos, 33 ms). **El desfase solo se reajusta**
  cuando un evento caería en el pasado o demasiado lejos. Recalcularlo en cada frame
  desde el reloj del audio es `importante`: vuelve el temblor de ±1 bloque. Tocar el
  sonido "al principio del próximo bloque", sin tiempo, es `importante`: dos golpes de
  pasos seguidos se funden.
- **Determinismo:** el audio nunca escribe en el estado de la simulación ni usa su
  generador de azar. Las variaciones salen de un hash del paso, la entidad y la capa.
  Usar el generador de la simulación es `bloqueante`: el replay deja de dar el mismo
  hash.
- **El mezclador:** 64 voces; cuatro prioridades (crítica, alta, normal, baja); un
  sonido nuevo solo roba una voz de prioridad menor (la más baja, después la que menos
  suena, después la más vieja); la voz robada baja a 0 con una rampa de un bloque; límite
  de instancias por sonido. Rampa de ganancia por bloque, retardo dentro del bloque,
  remuestreo lineal solo si el *pitch* no es 1. Buses de música, mundo y UI; la pausa
  congela el bus del mundo. Un limitador al final.
- **El sonido 3D:** distancia desde la caballera y paneo desde la cámara (el oyente
  híbrido); paneo de potencia constante; atenuación inversa con límite y una distancia
  máxima por sonido; pasa bajos de "atrás". Se calcula una vez por frame en el hilo
  principal, con las posiciones interpoladas.
- **La música:** comprimida en memoria (Vorbis) y decodificada en el callback con
  stb_vorbis y su buffer propio. El hilo principal abre el track y se lo pasa al callback;
  desde ahí, solo el callback lo toca. Estados exploración, combate y jefe, con
  *crossfade* y un tiempo mínimo por estado. Leer la música del disco en el callback es
  `bloqueante`.
- **El corte:** las capas de la tabla del ADR-0024 según el material (carne, cota,
  junta, placa, rebote, hoja clavada) y la fuerza normalizada a `Fmax`, con varias tomas
  por capa y ±5 % de *pitch* y volumen. **La pausa de impacto no congela el audio.** La
  cámara del tajo baja el tono del bus del mundo (×0,6 de partida, CVar).
- **Los valores ajustables** (latencia, buffer de 512, prioridades, límites, distancias,
  tono de la cámara lenta, *crossfade*) son CVars o datos de la tabla de sonidos. Un
  número fijo en el código es un hallazgo.
- **Depuración (ADR-0021):** panel de voces en ImGui; contadores de voces, robos,
  descartes, eventos tarde, comandos descartados, CPU máxima del callback y llamadas
  tarde en el registro por frame.
- Los tests del audio los audita `testing`; los sonidos en el cocinador, `assets`; las
  dependencias (dr_flac, libvorbis, libogg, stb_vorbis), `build-y-licencias`.

## vfx

**ADRs:** 0025 (VFX y partículas), con la fe de erratas del ADR-0026 (el color de
acento de la sangre). También la fe de erratas que hizo en el ADR-0007 (el
campo de distancia de la tapa), el ADR-0008 (el SoA de un pool es diseño; el SIMD espera
al profiler), el ADR-0010 (la región de VFX, las marcas y los campos), el ADR-0012 (los
VFX al final del paso), el ADR-0018 (la tabla `material → efecto`) y el ADR-0022 (la
máscara en el alfa, el swapchain `OPAQUE`, la feature del *screen-door* y las tres
excepciones de los VFX: *alpha*, *instancing* y luz por vértice).

Qué mirar:

- **Las partículas, en la CPU, en SoA, con dos pools fijos** reservados al iniciar
  (chicas y grandes); **las marcas y los campos de distancia, en bloques de la GPU** (ver
  más abajo). Simular en la GPU (un *compute
  shader* de partículas) es una decisión nueva.
- **Al final del paso, fuera del estado.** Los VFX leen los eventos y las poses del paso
  y nunca escriben en la simulación. Que el gameplay, la física o la IA lean una
  partícula o una marca es `bloqueante`: lo cosmético pasa a cambiar el *replay*.
  Integrar con el tiempo real del frame en vez del `dt` del paso es `importante`.
- **El azar es un `pcg` del número de paso, la entidad y el índice.** Usar el generador
  de la simulación es `bloqueante` (cambia el hash); usar `rand()` o el reloj es
  `importante` (el *replay* deja de repetir los efectos).
- **El tiempo de cada efecto es el de su dueño:** la estela usa el tiempo propio de la
  caballera (se congela con la pausa de impacto); la salpicadura, el del mundo, así que
  sale en el contacto. La cámara lenta escala todos.
- **Se dibujan en `pos − vel · (1 − α) · dt`** con el α de la interpolación: van un
  paso atrás, como todo lo que dibuja el render. `pos + vel · α · dt` extrapola y es
  `importante`: la sangre se despega del muñón. Guardar la posición anterior de cada
  partícula es un hallazgo menor (memoria de más). La estela termina en la pose
  interpolada de la espada.
- **Dibujo:** quads con *instancing* (4 vértices por instancia). *Vertex pulling* o
  *point sprites* son una decisión nueva (los primeros miden +45 % en esta GPU; los
  segundos no se estiran). **La mezcla por tipo:** sangre con *screen-door* (sin
  ordenar); polvo, humo y estela con *alpha* premultiplicado, ordenados con *radix
  sort*; chispas y fuego aditivos. Sangre aditiva es `importante` (brilla). *Alpha* sin
  ordenar es `importante`.
- **Todo dentro de la pasada de color, después de lo opaco**, en el orden *screen-door*
  → marcas → *alpha* → aditivo. Una pasada nueva o una pasada de color partida para los
  VFX es una decisión nueva.
- **La sangre:** salpicadura en el contacto, chorros en pulsos y gotas que mueren al
  tocar el suelo. **Un rayo hacia abajo por estallido** (consulta del ADR-0016), **que
  solo toca lo estático**, da el plano del suelo. Un rayo por gota es `importante` (gasta
  la física en algo cosmético); un rayo que toca cuerpos, también (la sangre queda a la
  altura de una rodilla).
- **Las marcas son *decals* de malla:** la CPU recorta los triángulos estáticos contra la
  caja (Sutherland–Hodgman) al crear la marca, y la escribe una vez en un buffer
  persistente de la GPU, en un anillo de lugares fijos. Un lugar reemplazado pasa por la
  cola por número de frame: reusarlo mientras un frame en vuelo lo lee es `bloqueante`.
  Subir las marcas cada frame por el *ring buffer* es un hallazgo. *Decals* proyectados
  (que manchan a los personajes) o pintura en una máscara son una decisión nueva. Un
  recorte que pasa de 64 vértices descarta lo que sobra y lo cuenta.
- **El color de la sangre es el color de acento** (fe de erratas, ADR-0026): un valor
  del frame (rojo, magenta o cian) que leen las partículas de sangre, las marcas y el rojo
  de las siluetas de los *frames* de impacto. Un rojo fijo en un shader o en el código es
  `importante`: el modo para quien no distingue el rojo deja de funcionar.
- **La tabla `material → efecto`** vive junto a la de materiales por cápsula del
  ADR-0018. Elegir el efecto con un `if` por material en el código es un hallazgo.
- **Los *frames* de impacto:** siluetas en blanco, negro y el color de acento (rojo por
  defecto; fe de erratas, ADR-0026) en el postproceso a
  640×360, antes del blit, leyendo el alfa del color como máscara. **Solo en los cortes
  cargados que cortan y en la cámara del tajo,** y con una opción para apagarlos.
  Dispararlos en cada tajo, o sin la opción, es `importante` (fotosensibilidad).
- **Las capas de la tapa** leen el campo de distancia del corte (filtrado lineal) y el
  material de la cápsula. Calcular la distancia por píxel contra las aristas del
  contorno es `importante`: costó +1,15 ms en primer plano con 203 aristas.
- **Pool lleno:** la partícula nueva no nace, y un contador lo muestra; la marca nueva
  reemplaza a la más vieja; el corte nuevo reusa el campo de distancia más viejo. Crecer,
  pedir al heap o fallar en silencio es un hallazgo (lo audita también `memoria`).
- **Los valores** (cantidades, tamaños, vidas, la fracción de gotas que manchan, los
  grosores de las capas, el tope de partículas grandes, la duración de los *frames* de
  impacto) son CVars, constantes en Release. Un número fijo en el código es un hallazgo.
- **Depuración (ADR-0021):** contadores de partículas vivas y descartadas, marcas y
  recortes truncados en ImGui.
- Los tests de los VFX y el TDD del campo de distancia los audita `testing`; el
  presupuesto (0,5 ms de GPU; 0,2 ms de CPU que corren en cada paso), `arquitectura`.

## ui

**ADRs:** 0026 (UI, HUD y menús). También la fe de erratas que hizo en el ADR-0010 (la
región de UI), el ADR-0019 (las fuentes OFL), el ADR-0021 (el menú de opciones), el
ADR-0022 (la pasada de UI) y el ADR-0025 (el color de acento de la sangre y de las
siluetas). También la fe de erratas del ADR-0031 en este ADR (la cámara en las opciones,
la mira en el mundo y la ayuda según el estilo), la del ADR-0032 (los controles fuera
de la simulación, los íconos con umbral, los menús fijos y el cursor encerrado) y la del
ADR-0034 (las cinemáticas: sin HUD, las bandas, el anillo de saltar y "Saltar" en la
pausa).

Qué mirar:

- **La cámara en la UI** (fe de erratas, ADR-0031): las opciones suman el estilo, el FOV
  de la tercera y de la primera persona, el cierre al apuntar por estilo, la sacudida y
  la cámara del tajo. **La mira está en el mundo,** no en el centro de la pantalla. **La
  energía en la espalda es la ubicación por defecto solo con la tercera persona;** con
  la primera persona y con las fijas y los rieles, junto a la mira.

- ***Immediate mode* propio:** cada pantalla es una función que llama a sus controles
  cada frame; el estado de la UI es el foco, y el valor de cada opción vive en el juego.
  **Estado por ID solo para lo que se anima,** en un arreglo fijo. Un árbol de controles
  que se guarda (*retained*), una biblioteca de UI (RmlUi, microui) o Dear ImGui en las
  pantallas del juego son una decisión nueva. **ImGui solo en desarrollo:** una pantalla
  del juego que lo use no compila en Release (lo audita también `build-y-licencias`).
  Cuando ImGui tiene el foco del teclado o del mouse, la UI del juego no recibe ese
  input.
- **A 640×360, en su pasada, después del postproceso y antes del blit:** una sola llamada
  por frame, con alfa premultiplicado y **sin escribir el alfa del destino** (la máscara
  de personajes del ADR-0025). Dibujar la UI del juego a la resolución nativa (después del
  blit) es una decisión nueva; escribir el alfa del destino es `importante`; dibujarla
  antes del postproceso es `importante` (los *frames* de impacto la invierten).
- **Las fuentes:** Jersey 10 (texto, 18 px por em) y Jacquard 12 (títulos, 21 px por
  em), cocinadas en su grilla exacta, en ×1 (18 y 21 px por em) y ×2 (36 y 42). **El ×2 es una
  opción para los subtítulos, los avisos de botón y el HUD; los menús quedan en ×1.**
  **Sin opción de otra fuente** (decisión del autor). Una tercera fuente o una opción de
  fuente es una decisión nueva.
- **Los textos:** nunca en el código; siempre un ID de la tabla del idioma. Un texto que
  ve el jugador escrito en el código es `importante`. Los plurales van por la regla de
  cada idioma. **Las tablas de todos los idiomas se cargan al iniciar:** cambiar de idioma
  en vivo no carga nada; cargar un archivo al cambiarlo es `importante` (pide memoria en
  el frame, ADR-0010).
- **0 HUD por defecto.** Las ayudas (energía, carga, avisos de botón) se prenden **una
  por una** y van en el cuerpo (por defecto), junto a la mira o en las esquinas. **La
  línea del tajo se ve siempre que se apunta.** Una ayuda prendida por defecto es
  `importante`. **El juego nunca presenta las ayudas como dificultad:** un texto que
  diga "fácil" o "difícil" al prenderlas es `importante`.
- **Ningún dato depende solo del color** (Game Accessibility Guidelines, básico): las
  barras son color hueso sobre una placa oscura, y la energía baja parpadea. Un dato que
  solo cambia de color es `importante`. **Los acentos rojos leen el color de acento**
  (rojo, magenta o cian); un rojo fijo es `importante`.
- **Contraste (ISO/IEC 40500:2025, WCAG 2.2),** medido contra la parte de menos
  contraste del fondo: 4,5:1 para todo el texto y 3:1 para las barras, los arcos, los
  íconos y el foco. Un elemento nuevo sin su medición en la lista viva
  (`docs/ui/accesibilidad.md`) es un hallazgo.
- **Subtítulos siempre:** el jugador elige el estilo (con fondo, por defecto, con
  opacidad ajustable, o sin fondo) y el tamaño, **pero no los puede apagar**. Una opción
  que los apague es `bloqueante` (decisión del autor). El nombre de quien habla, en hueso,
  no en rojo. Los subtítulos de sonidos son aparte y sí se apagan.
- **La pausa:** una pila de pantallas; solo la de arriba recibe input. La pausa deja el
  paso fijo en 0 pasos, congela el bus del mundo y deja sonar el de la UI (ADR-0024).
  Avanzar pasos durante la pausa es `bloqueante` (cambia el *replay*). **Una opción que
  cambia la simulación** (apagar la cámara del tajo) se graba como evento del paso si
  cambia durante una grabación; no grabarla es `bloqueante`. Grabar las que no la cambian
  (ayudas, subtítulos, idioma, color y **los controles**: la sensibilidad, las curvas, la
  reasignación y la alternativa de mantener, que el ADR-0032 aplica antes del paso) es un
  hallazgo menor.
- **Los íconos y los menús** (fe de erratas, ADR-0032): un dispositivo cuenta como usado
  con un botón o con un eje que pasa la zona muerta, no con cualquier evento; las
  acciones de los menús son fijas, con alternativas (flechas y WASD, Enter y Espacio,
  Esc; la cruceta, el stick, A/Cruz y B/Círculo); el cursor queda encerrado en la ventana
  mientras tiene el foco.
- **La navegación:** listas con un índice de foco, *hover* del mouse, el stick con
  repetición (0,35 s y después cada 0,12 s) e íconos del último dispositivo, dibujados
  con primitivas. Una pantalla que no se puede manejar sin mouse es `bloqueante`.
- **Las opciones** (Decisión 6) tienen las ayudas, los subtítulos, el idioma, el color de
  acento, el tamaño del texto, la opacidad de las placas, los *frames* de impacto, los
  volúmenes del ADR-0024 y los controles del ADR-0032. Se guardan en `opciones.txt` (el
  formato y dónde los audita `recursos`). "Continuar" solo aparece si la tarjeta de
  memoria tiene el bit de partida, y "Nueva partida" pide confirmar si lo tiene
  (ADR-0028).
- **Memoria y presupuesto:** la región de UI con topes fijos (8192 vértices por frame,
  más las tablas de las fuentes y de los textos);
  **0 asignaciones por frame** (el corte de línea se hace cuando cambia el subtítulo, no
  cada frame). Un `std::string` armado cada frame es un hallazgo (lo audita también
  `memoria`). Metas: 0,2 ms de GPU y 0,1 ms de CPU.
- **Los valores** (posiciones, tiempos, colores, la repetición del stick) son CVars,
  constantes en Release. Un número fijo en el código es un hallazgo.
- **Depuración (ADR-0021):** un panel de ImGui con los controles, el foco, el idioma y el
  dispositivo, y las cajas de cada control en el dibujo de depuración.
- **Las cinemáticas (fe de erratas, ADR-0034):** mientras dura una cinemática **no hay
  HUD**; la UI dibuja **bandas negras de 1,85:1** (7 px arriba y abajo a 640×360), que
  aparecen con el corte de entrada y se retiran durante el tramo de salida; **los
  subtítulos van sobre la imagen**, con las opciones del jugador; **el anillo** se llena
  mientras se mantiene la acción "saltar"; y **la pausa suma "Saltar"**. Otra proporción
  o un HUD visible durante una cinemática son una decisión nueva. En una secuencia
  interactiva, el HUD que el gameplay necesite.
- Las fuentes y los textos en el cocinador los audita `assets`; sus licencias,
  `build-y-licencias`; los tests, `testing`; las acciones y la reasignación de controles,
  `input`.

## niveles

**ADRs:** 0027 (niveles y escenas). También la fe de erratas que hizo en el ADR-0010 (la
vida de una zona y el tope), el ADR-0012 (la zona entra a la simulación en un paso; 0
pasos si no llegó), el ADR-0017 (el mundo en el cocinador), el ADR-0021 (el encabezado
del *replay* y el contador de esperas), el ADR-0022 (el culling y las 16 antorchas), el
ADR-0023 (la navmesh partida por zona), el ADR-0024 (la música por zona), el ADR-0026
(la pantalla de carga) y la visión (el mundo continuo).

Qué mirar:

- **Blender es el editor.** Las piezas van como *collection instances*; los marcadores
  (spawns, puntos de invocación, triggers, checkpoints, inicio, sonidos de ambiente),
  como *empties* con la propiedad `tipo` y sus valores propios. Un editor de niveles
  propio, TrenchBroom u otro formato de escena son una decisión nueva. Un complemento de
  Blender necesita su ticket.
- **El mundo, partido en zonas:** un `.blend` y un texto por zona, y un texto del mundo
  (zonas, vecinas, inicio, radio de inflado, tope de zonas cargadas y las cuatro
  distancias de carga y de simulación). **El corte
  vertical es un mundo de una zona.** Un nivel suelto que no sea una zona del mundo es
  una decisión nueva.
- **"Dónde" va en el glTF y "cuándo y cuántos" en el texto.** Fases, oleadas o
  condiciones escritas como propiedades en Blender son un hallazgo; posiciones escritas
  a mano en el texto, también.
- **Los ids:** el FNV-1a de 64 bits del camino completo, que empieza con la zona; únicos
  en el mundo. **Cada sección se ordena por id y las entidades se crean en ese orden.**
  Crear entidades en el orden del glTF (o de cualquier recorrido de Blender) es
  `bloqueante`: cambia los *handles* y el hash del *replay*. **Un texto de zona solo
  nombra objetos de su zona:** una vecina puede estar cargada sin simular.
- **Las fases son datos:** una lista cerrada de comandos (aparecer, gotear por un grupo,
  subir el máximo, el jefe y sus puntos de invocación) y de finales (todos muertos, N
  muertes, el jefe muerto), ejecutada por un sistema genérico de `game/`. **No hay C++
  por zona:** una función de `game/` con las fases de una zona es `importante`; algo
  nuevo suma una palabra a la lista, con su test. **Sin lenguaje de *script* y sin
  director adaptativo:** cualquiera de los dos es una decisión nueva. Los grupos
  (`puerta.*`) se recorren en orden de nombre.
- **Los triggers** son un volumen (la escala del *empty*) y un evento de una lista
  cerrada, revisados **en el paso fijo** contra la posición de la caballera. Revisarlos
  en el render o con el estado interpolado es `bloqueante`.
- **El disco nunca decide qué pasa. Cargada no es lo mismo que simulando:**
  - **la carga** (el disco): una vecina de la zona actual se pide a menos de 35 m de su
    borde y se descarga a más de 65 m, si no simula;
  - **la simulación** (el paso fijo): una zona entra (su colisión, su navmesh, sus
    triggers, sus entidades) a menos de 10 m, y si no llegó, **el frame corre 0 pasos**
    y el acumulador no avanza; sale a más de 20 m, y **las entidades que están en la
    zona en ese paso** (según su polígono de la navmesh, o su posición si no tienen
    uno) se destruyen en orden de id; no las que creó la zona y siguieron a la
    caballera. **Las fases y la
    música empiezan al cruzar el borde** (0 m, revisado en el paso fijo), no al entrar a
    la simulación (10 m); usar "entrar" sin decir cuál es un hallazgo.

  Activar una zona cuando termina de cargar, o crear o destruir sus entidades fuera del
  paso fijo, es `bloqueante` (P8: un hash distinto por cada disco). **Los portales de la
  navmesh hacia una zona que no simula están cerrados.** **El tope de zonas cargadas:**
  se pide primero la más cercana, y con el tope lleno solo se descarga una más lejana
  que no simula; nunca se aborta. Pedir en otro orden, o descargar una zona más cercana
  que la que se pide, es `bloqueante` (P8b: se bloqueó en todas las corridas). **Las
  distancias y el tope son datos del mundo:** una sola fuente, el texto del mundo, y el
  juego usa los del binario, que también guarda la curva del peor caso y la entrada
  máxima. Una CVar o un número fijo en el código que los cambie sin pasar por esa
  validación es `importante` (rompe la garantía del cocinador). En desarrollo, **el
  panel del mundo** los cambia en vivo, valida cada cambio contra la curva (una salida no
  entera usa el metro siguiente) y rechaza el que rompa la garantía; exige entrada <
  salida y carga < descarga; **el tope en vivo nunca pasa el del binario**, con el que se
  reservó la memoria al iniciar (pasarlo en vivo es `importante`: pide memoria fuera del
  inicio, ADR-0010). Un cambio de la entrada o la salida durante una grabación
  se graba como evento del paso. El panel escribe los valores aceptados en el texto del
  mundo.
- **Los checkpoints:** un marcador de una zona ligado a una fase. Volver es cargar la
  zona del checkpoint detrás de la pantalla de carga, aplicar desde ahí la regla de carga
  (la más cercana primero, con el tope) y entrar en la fase desde cero; las fases
  terminadas siguen terminadas. Cargar de golpe la zona y todas sus vecinas es
  `importante` (puede pasar el tope). **La semilla de una fase sale del id de la zona y del número de la fase:**
  una semilla que cambie entre intentos es una decisión nueva. Cómo se guarda (la
  tarjeta de memoria del ADR-0028) lo audita `recursos`.
- **Lo que se dibuja:** lo cargado, con la esfera contra el frustum, **hasta el final de
  la niebla**, y solo la zona actual y sus vecinas. **Las 16 antorchas** son las más
  cercanas a la cámara entre las zonas cargadas, con fundido al entrar o salir. La
  antorcha es una pieza (luz, fuego y sonido), y su radio es la propiedad `radio`; usar
  la intensidad del glTF es un hallazgo (llega en unidades físicas).
- **La música va por zona** (en su texto) y un trigger puede cambiar el *track*; los
  estados (exploración, combate, jefe) siguen el ADR-0024.
- **La pantalla de carga** aparece al empezar y al volver a un checkpoint, **nunca al
  caminar**.
- **Depuración (ADR-0021):** el dibujo de la navmesh, la colisión, los radios de las
  antorchas, los triggers, los marcadores por fase y los bordes de las zonas con su
  estado; un panel con las zonas cargadas, las esperas y el tiempo de cada carga; el
  contador de frames que esperaron a una zona en el registro por frame.
- El cocinador del mundo (validaciones, secciones, navmesh) lo audita `assets`; sus
  tests, `testing`; la memoria de las zonas, `memoria`; el culling y las antorchas en el
  renderer, `graficos`; cómo se carga una zona sin cortar el frame (el hilo de I/O, los
  slots y la subida), `recursos`.

## recursos

**ADRs:** 0028 (recursos en runtime y guardado). También la fe de erratas que hizo en el
ADR-0004 (los archivos en `platform::`), el ADR-0006 (el paquete en Release), el ADR-0012
(el hilo de I/O y el orden del frame), el ADR-0021 (el corte de la grabación), el
ADR-0022 (la subida y el `VkPipelineCache`), el ADR-0024 (los sonidos en los slots), el
ADR-0026 (la pantalla de carga, el indicador y "Continuar") y el ADR-0027 (qué es una
zona cargada y el checkpoint guardado). La memoria de los slots la audita `memoria`; el
cocinador de los binarios, `assets`; sus tests, `testing`.

Qué mirar:

- **El hilo principal nunca espera al disco.** Leer o escribir un archivo en el hilo
  principal (`read`, `fread`, `pread`, `fsync`, cargar "de una vez"), tocar como memoria
  un archivo mapeado (`mmap`: las faltas de página caen en quien lo toca) o usar
  `SDL_AsyncIO` es `bloqueante` (P1: de 2 a 36 ms por frame; `SDL_AsyncIO`, de 10 a
  15 ms en el frame que pide). io_uring como reemplazo del hilo es una decisión nueva.
  **Vale también para los archivos de depuración:** el log, el registro por frame y los
  bloques del *replay* (ADR-0021) se le entregan al hilo de I/O, y en desarrollo también
  el archivo de CVars cambiadas y los valores que el panel del mundo escribe en el texto
  del mundo (los únicos que escriben dentro del repo). **Hay dos excepciones, y no son
  hallazgos:** la etapa cero del arranque (antes del primer frame, el hilo principal
  espera `opciones.txt` y el binario de arranque) y un error fatal (el hilo principal
  espera a que el hilo de I/O vacíe el log antes de abortar).
- **El hilo de I/O:** abre, lee (`pread` en bloques de 1 MB) y escribe (con su `fsync`),
  y nada más. **No toca el estado del juego, no usa allocators y solo escribe en memoria
  reservada al iniciar.** Se comunica por **dos colas de un productor y un consumidor sin
  locks**: pedidos y avisos. Un lock, un `std::mutex`, una asignación de memoria o un
  acceso al ECS desde ese hilo es `bloqueante`. El orden de los pedidos lo decide el hilo
  principal (la zona más cercana primero, ADR-0027); el hilo los atiende en orden.
  **Las primitivas de archivo y de hilo están en `platform::`.** **Son dos piezas:** el
  hilo de I/O con sus colas, encima de `engine/core` y sin conocer la GPU, y **el
  cargador** (los slots, la validación y la subida), un sistema encima de `engine/gpu`.
  Llamar a POSIX o crear un hilo fuera de `platform::`, o que el hilo de I/O use
  `gpu::`, es `importante`.
- **El orden del frame:** al empezar, el hilo principal mira los avisos, valida la
  cabecera y los offsets de lo que llegó y decide los pedidos; antes de las pasadas del
  render, graba la subida. Mirar los avisos en medio del paso fijo es `importante`.
- **Usado tal como llega:** el binario no se parsea ni se copia; se valida (firma,
  versión y que cada offset caiga dentro) y se usa en el slot. Copiar recurso por recurso
  a asignaciones propias es `importante` (P2: de 2 a 4 ms por zona).
- **Los slots y la subida:** el slot de la CPU es memoria de Vulkan **visible y con
  caché** (`HOST_CACHED`); usar un tipo sin caché para leer la navmesh o la colisión es
  `importante` (memoria de escritura combinada, lenta para leer). **El slot de la CPU es
  el *staging*:** un buffer de *staging* aparte o un `memcpy` en el hilo principal es un
  hallazgo. **La subida tiene un presupuesto de 2 MB por frame**; subir una zona entera
  en un frame es `importante`. **Una zona está cargada cuando llegó del disco y su subida
  terminó en la GPU**: hacerla entrar a la simulación o dibujarla antes es `bloqueante`.
  Al descargarla, primero se paran las voces que leen sus sonidos (ADR-0024). El slot de
  la GPU se libera con la cola por número de frame; **el de la CPU espera además a que el
  reloj del audio pase el bloque de la orden de parar**. Liberarlo solo con la cola por
  frame es `importante` (el audio tiene más latencia que dos frames). **Cada slot tiene
  un rango fijo de índices del arreglo de texturas** (más uno para el global y otro para
  la recarga), y las imágenes se ligan dentro de su bloque: una asignación de Vulkan por
  textura es un hallazgo.
- **El global:** se carga al arrancar y vive todo el programa. Liberar algo del global
  con una zona es un hallazgo. **Su región de la CPU guarda solo su parte de la CPU:**
  la parte de la GPU sube al arrancar a través de los slots de zona, que todavía están
  libres; un *staging* propio del global, o su parte de la GPU guardada también en la
  CPU, es `importante` (no entra en su presupuesto). Los sonidos comunes viven ahí y el
  audio los lee sin copiarlos; **la música es la excepción**: su propio binario, en la
  región de audio (ADR-0024).
- **El paquete de Release** va dentro del ejecutable, en una sección propia, y **el hilo
  de I/O lo lee del propio ejecutable con `pread`**. Usar el símbolo de esa sección como
  memoria (es `mmap`) es `bloqueante`. Un paquete pegado al final del ejecutable, o
  archivos sueltos en Release, es una decisión nueva. **El juego nunca escribe junto al
  ejecutable.** **En PE** (fe de erratas, ADR-0035), la sección se encuentra por la tabla
  de secciones y su tamaño es `VirtualSize`; usar `SizeOfRawData` (redondeado al
  `FileAlignment`) es `importante`: lee basura al final. El nombre de la sección cabe en
  8 bytes.
- **El arranque:** primero la etapa cero (`opciones.txt`, el binario de arranque y su
  pipeline, antes del primer frame); después, detrás de la pantalla de carga, el mundo,
  el global (su parte de la GPU, por los slots de zona), la música, los pipelines y la
  zona de la partida. **El `VkPipelineCache`** vive en la carpeta de caché, con **una
  cabecera propia** (firma, versión, `vendorID`, `deviceID`, `driverVersion`,
  `pipelineCacheUUID`, tamaño y hash) que se valida antes de pasárselo al driver; pasarle
  los bytes sin validar es `importante`. Si no coincide, se empieza con uno vacío. Se
  escribe con temporal y `rename`, en el hilo de I/O.
- **La pantalla de carga es fija** (el fondo y "Cargando…"), sin barra. **Si la
  simulación espera a una zona,** el frame queda quieto y el indicador aparece **pasados
  0,5 s**.
- **La recarga en caliente, solo en desarrollo:** inotify sobre la carpeta de lo
  cocinado, una vez por frame, a través de `platform::`. Se recargan los shaders, las
  texturas (también las de los VFX y el atlas de las fuentes), las mallas y las tablas de
  sonidos y de textos. **Lo viejo se retira con la cola por número de frame:** destruir
  un pipeline, una imagen o un buffer que un frame en vuelo puede usar es `bloqueante`
  (el prototipo lo hizo con una textura). Un set de descriptores se actualiza solo al
  empezar su frame, nunca en vuelo. Lo recargado va a la región de recarga, no al slot.
  **Recargar algo que cambia la simulación (una malla) corta la grabación del *replay***
  en ese paso, con una nota en el registro; seguir grabando es `bloqueante` (el *replay*
  ya no reproduce). La recarga compilada en Release es un hallazgo.
- **La tarjeta de memoria:** `tarjeta.bin`, **8 KB fijos**, en la carpeta de datos
  (`SDL_GetPrefPath`), con **dos bloques de 4 KB, A y B**. Cada bloque: firma, versión,
  secuencia, banderas (**el bit 0 dice si hay partida**), hash del mundo, zona, fase
  (por su índice estable; su número de orden dentro de la zona es `importante`), fases
  terminadas (256 bits, cada uno según el índice estable de la fase, de la lista
  del texto del mundo), bytes reservados en cero y CRC-32 al final; en
  *little-endian*, con el diseño fijado por `static_assert`. **Guardar pisa el bloque más
  viejo** (`pwrite` y `fsync`, en el hilo de I/O); **cargar se queda con el válido de
  secuencia más alta.** Escribir los dos bloques en el mismo guardado, escribir encima del
  único válido, o guardar sin CRC es `bloqueante` (P7: escribir encima perdió el 52 % de
  las partidas). **Una sola partida**, que cada checkpoint pisa; **nada se guarda al
  salir**, y volver al morir no escribe. Un campo nuevo solo ocupa bytes reservados;
  cambiar el lugar de uno existente es `bloqueante` (rompe las tarjetas viejas). **Si la
  zona del checkpoint ya no existe** en el mundo, se vuelve al checkpoint anterior que
  exista, o al principio. **Guardar dentro del ejecutable es una decisión nueva**
  (descartado: una actualización lo borra).
- **Las opciones:** `opciones.txt` en la carpeta de configuración
  (`$XDG_CONFIG_HOME/<juego>/`, por defecto `~/.config/<juego>/`), separado de la
  partida. Texto `clave = valor`, UTF-8, con la versión primero y sin CRC. Al leer: una
  clave desconocida se ignora, una que falta toma su valor por defecto, un valor fuera de
  rango se lleva al rango y una línea rota se ignora y se registra. Abortar o perder
  todas las opciones por una línea rota es `importante`. Se escriben al salir de su
  pantalla, con temporal, `fsync` y `rename`, en el hilo de I/O.
- **Las carpetas:** datos con `SDL_GetPrefPath`; configuración, caché y estado (el log,
  el registro y los *replays*) con XDG, armadas por `platform::`. Escribir en otra carpeta (la del ejecutable, `$HOME` suelto) es
  `importante`. Los nombres de la organización y del juego, una vez elegidos, no
  cambian. **En Windows** (fe de erratas, ADR-0035): la tarjeta de memoria y las opciones
  en `%APPDATA%\<juego>\` (`SDL_GetPrefPath`); la caché de pipelines y los archivos de
  depuración en `%LOCALAPPDATA%\<juego>\`.
- **Depuración:** el panel de ImGui de la carga (pedidos en vuelo y lo que tardó cada
  uno, los slots y su zona, la subida por frame y las recargas) y zonas de Tracy en el
  hilo de I/O. **Las metas de carga** (≤ 3 s al arrancar, ≤ 1 s al volver a un
  checkpoint) son hipótesis de `presupuestos.md`: cambian con una medición del
  protocolo.

## camara

**ADRs:** 0031 (sistema de cámara). También la fe de erratas que hizo en el ADR-0012 (la
cámara corre en el render, después de interpolar), el ADR-0015 (la guía de la espada),
el ADR-0016 (la predicción del tajo especial y el barrido de esfera desde el render),
el ADR-0017 (las cámaras fijas y los rieles en el cocinador), el ADR-0018 (el apuntado
desde el pecho y el tajo especial resuelto al soltar), el ADR-0021 (el *replay* guarda
si la cámara del tajo está apagada), el ADR-0022 (la herida que se abre), el ADR-0023 (la
ventaja del cono de la caballera), el ADR-0025 (la sacudida y los *frames* de impacto en
la cámara lenta), el ADR-0026 (las opciones de cámara y la ayuda según el estilo), el
ADR-0027 (los marcadores de cámara) y la visión (versión 5).

Qué mirar:

- **La cámara es una opción del jugador, y en ningún estilo la mueve él.** Tres estilos:
  tercera persona por detrás (por defecto), fijas y rieles por sector, y primera persona.
  Un control para girar o acercar la cámara, una órbita libre o un *lock-on* son una
  decisión nueva.
- **La cámara está fuera de la simulación.** Se calcula una vez por frame, en el render,
  después de interpolar el estado, y nada del paso fijo la lee. **Que la simulación lea
  la cámara (el apuntado, la IA, el combate) es `bloqueante`:** elegir otro estilo
  cambiaría la partida y el *replay* dejaría de dar el mismo hash. Su consulta de pared
  es de solo lectura contra el escenario estático.
- **Los estilos:**
  - **tercera persona:** sigue el rumbo con un resorte, con los valores de la visión
    (2,3 m, 1,9 m, 0,5 m a la izquierda, 51°; al apuntar, 2,0 m y 30°). **Mientras apunta
    se queda con el rumbo que tenía al empezar a apuntar**;
  - **fijas y rieles:** el volumen del sector decide la cámara; la fija gira con un
    resorte, el riel va al punto de la curva más cercano a la caballera; **corte seco
    entre tomas**; al apuntar no cambian;
  - **primera persona:** el ojo en la cabeza, con la cabeza y el cuello ocultos; mira hacia
    el rumbo del cuerpo, **sin el balanceo de la cabeza** (que marea); 70°, **sin zoom al
    apuntar** por defecto.

  **Cuánto se cierra el FOV al apuntar** es una opción por estilo, hasta 0.
- **El suavizado:** resortes críticamente amortiguados de forma cerrada, con el dt del
  frame. **Un `lerp` fijo por frame es `importante`** (P6: ×5 entre 30 y 144 FPS).
- **La colisión** (solo la tercera persona): barrido de esfera de 0,2 m contra el
  escenario estático, no contra los personajes; se acerca con un resorte rápido sin
  pasar el límite de una esfera de 0,1 m, y se aleja lento. **Un rayo en vez de la
  esfera es `importante`** (P6: el plano cercano corta la pared en las esquinas). La
  caballera en *screen-door* a menos de ~1 m (ADR-0022, lo audita `graficos`).
- **La cámara del tajo acompaña la hoja, en los tres estilos:** del lado de la normal del
  plano, con la mitad de la hoja, y después se queda en el corte. Cámara lenta ×0,02
  mientras la hoja cruza y ×0,2 durante 0,9 s; vuelve con un corte seco. Se puede
  apagar, y **apagarla cambia la simulación** (quita la cámara lenta): el encabezado del
  *replay* lo guarda y un cambio durante la grabación es un evento del paso. Olvidarlo
  es `bloqueante`.
- **La herida que se abre:** mientras la hoja cruza, la tapa se ve solo del lado barrido
  (un semiplano en el shader) y el pedazo de arriba gira alrededor del borde del lado sin
  barrer. **El giro es solo de presentación:** que toque el cuerpo físico del pedazo o
  el hash es un hallazgo; al salir la hoja vuelve a 0 con un resorte, no de golpe. No hay
  huesos semitransparentes ni pasada propia.
- **La sacudida:** trauma con ruido suave, trauma², **solo rotación y solo en la
  presentación**. Una sacudida que mueva la posición de la cámara o el rayo de apuntar es
  un hallazgo. Se puede apagar.
- **Los *frames* de impacto** dentro de la cámara del tajo duran 2 o 3 frames reales, no
  de simulación.
- **Todo valor de la cámara y de la guía de la espada es una CVar** (ADR-0021): se ajusta
  jugando. Un número de sensación fijo en el código es `importante`. **Salvo las opciones
  del jugador** de la Decisión 10 (el estilo, el FOV, el cierre al apuntar, la sacudida y
  la cámara del tajo), que van en `opciones.txt`; su valor por defecto sí es una CVar.
- **Depuración:** una cámara libre que no toca la partida.
- El apuntado desde el pecho y la predicción del tajo especial los audita `gameplay`; la
  ventaja del cono, `ia`; la guía de la espada, `animacion`; las cámaras fijas y los
  rieles en el cocinador, `assets`; las opciones en el menú, `ui`; los tests del resorte y
  del barrido, `testing`.

## input

**ADRs:** 0032 (input y controles). También la fe de erratas que hizo en la visión
(versión 6: el movimiento, caminar y el paso lateral al apuntar, esquivar respecto del
rumbo), el ADR-0010 (la región de input), el ADR-0012 (el orden del frame), el ADR-0015
(la guía sigue un objetivo), el ADR-0018 (el stick por posición, el paso lateral,
esquivar respecto del rumbo, el *buffer* y los márgenes en pasos, la alternativa de
mantener como disparador), el ADR-0020 (la latencia), el ADR-0021 (la regla del input por
paso), el ADR-0024 (la vibración lee su arreglo de eventos), el ADR-0026 (las opciones fuera de la simulación, los íconos, los menús fijos y el cursor
encerrado), el ADR-0031 (el movimiento y la guía), el ADR-0034 (el contexto de la
cinemática, saltar reasignable y la secuencia para las secuencias interactivas) y
`presupuestos.md`.

Qué mirar:

- **El gameplay habla con acciones, nunca con teclas ni dispositivos.** Que `game/**` o un
  sistema de la simulación pregunte por una tecla, un botón o el tipo de mando es
  `importante`: rompe la reasignación. Contextos apilados (el juego, la secuencia de la
  hoja clavada, del agarre y de las secuencias interactivas, la cinemática con solo
  saltar y pausar, el menú), el de arriba gana; ImGui encima de todos, en
  desarrollo. **"Guiar" es una sola acción** que el gameplay interpreta según la fase.
- **Los botones llevan el estado y contadores de presiones y de sueltas,** que se acumulan
  hasta que un paso los consume. **Solo el estado, o un flanco por frame, es
  `importante`** (P2: se pierde el 54 % de los toques a 144 FPS, o el 29 % a 20 FPS). Los
  gatillos son botones con un umbral con histéresis.
- **Cada evento va al paso que cubre su *timestamp*,** y lo que cae después del último
  paso del frame espera al siguiente. **Darle todo al primer paso, repartirlo o meterlo
  todo en el frame actual es `importante`:** el apunte cambia con los FPS (P2: hasta 39°).
  Un *timestamp* del futuro se toma como el momento de la lectura; uno de un paso que ya
  corrió va al primer paso del frame. No acotarlos es `importante` (los teclados
  virtuales de Wayland mandan *timestamps* inválidos).
- **La simulación recibe las acciones ya procesadas:** **mover** (avance y giro con
  tanque; una dirección del mundo en la fija y los rieles), **la guía** (la posición del
  stick normalizada y absoluta, o el delta del mouse en grados, según el último
  dispositivo usado; el recorte al cono es de la simulación) y **los botones** con sus
  contadores. Sumar el stick y el mouse, en vez de usar el último, es una decisión nueva.
  Convertir el tanque a una dirección del mundo antes del paso es `importante`: el rumbo
  cambia de un paso a otro. **Que la simulación lea
  la sensibilidad, una curva, la reasignación o la opción de mantener es `importante`:**
  esas opciones están fuera del *replay*. **Que lea la cámara para convertir el movimiento
  es `bloqueante`** (también lo ve `camara`): la conversión a una dirección del mundo la
  hace la capa de input, con la cámara del último frame.
- **El mouse es relativo y crudo:** cuentas sin la aceleración del sistema
  (`SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE` apagado), una sensibilidad en grados por
  cuenta y sin filtro. **Multiplicar el delta del mouse por el dt es `importante`** (P4:
  el mismo gesto da de 11° a 40° según los FPS). **Un filtro o suavizado es
  `importante`:** el peso lo pone la espada (ADR-0015), no el input.
- **El stick:** zona muerta radial escalada y una curva (exponente 2 de partida). **Una
  zona muerta axial es `importante`** (P3: se desvía hasta 42°). **El stick marca una
  posición, nunca una velocidad,** en las dos fases: al apuntar, a fondo es el borde del
  cono y soltarlo vuelve al rumbo; al cargar, su ángulo es el de la línea. Un stick que
  marque una velocidad, o una ayuda de apuntado, son una decisión nueva.
- **Moverse:** tanque en la tercera y la primera persona; en la fija y los rieles,
  relativo a la pantalla, **manteniendo la dirección** mientras el stick no cambie,
  aunque la toma corte. **Mientras apunta, camina muy despacio** (bastante más lento que
  un zombie) y no gira el rumbo: izquierda y derecha dan un paso lateral. **Esquivar lee
  el stick respecto del rumbo** (adelante, atrás o a un lado), aunque caminar sea de
  tanque.
- **La reasignación:** todo, por dispositivo; hasta dos entradas por acción en el teclado
  y el mouse. **Se guarda la posición** (el scancode) con una tabla de nombres propia, y
  se muestra la letra de la distribución del jugador. **Guardar la letra, o los nombres
  de `SDL_GetScancodeName` (no son estables), es `importante`.** Un conflicto en el mismo
  contexto se intercambia; Esc y Start/Options siempre pausan; los menús son fijos, con
  alternativas.
- **El *buffer* se cuenta en pasos** (6 de partida para esquivar, el tajo rápido e
  interactuar), como CVar. **En milisegundos, o con el reloj del frame, es
  `importante`:** el *buffer* vive en la simulación y entra al hash. **Soltar la carga no
  tiene *buffer*;** una cola de acciones es una decisión nueva.
- **La latencia: el modo D.** Antes de leer el input, el frame espera a que se muestre el
  frame de hace dos (`VK_KHR_present_wait2` con `VK_KHR_present_id2`, la misma generación
  que usa el ADR-0021; mezclarla con la primera es un hallazgo); sin la extensión, el modo
  A. El modo es una
  CVar. **Esperar al frame anterior (B o C) es `importante`:** pone la CPU y la GPU en
  serie y pierde frames (P5). **Leer el input sin esa espera, con más frames en la cola,
  es `importante`** (82 contra 43 ms).
- **Los dispositivos:** todos funcionan a la vez; los íconos cambian con un botón o con un
  eje que pasa la zona muerta, **no con cualquier evento** (P6: un mando quieto con
  deriva los cambia 292 veces por minuto); pausa si se desconecta el mando en uso; solo la
  base de mandos de SDL3 (un archivo de mapeos nuevo lo ve `build-y-licencias`). **En los
  menús, el cursor queda encerrado en la ventana mientras tiene el foco.**
- **La vibración sale de los eventos del paso,** una vez por frame, después del audio, y
  **los lee del arreglo de eventos del audio** (ADR-0024), que vive el frame. Leerlos de
  las colas del paso (que mueren con el paso) es `importante`. Es de la presentación. **Que la vibración escriba estado de la simulación, o entre al
  hash, es `importante`.** Nunca es la única señal, se apaga y se regula (XAG 110).
- **La región de input** (2048 eventos por frame): **llena, suma los deltas y nunca
  pierde un botón.** Abortar, o descartar un botón, es `importante`. El tamaño de la
  región lo audita `memoria`.
- **Valores:** los de sensación son CVars (la zona muerta, la curva, el *buffer*, la
  histéresis, la velocidad al apuntar, el modo de latencia). **Las opciones del jugador**
  (los controles, la sensibilidad de 0,25× a 4×, invertir un eje, la zona muerta interior,
  mantener o alternar, la alternativa de mantener, la vibración) van en `opciones.txt`.
- La grabación del *replay* (solo en `debug` y `relwithdebinfo`) la audita
  `herramientas`; la guía de la espada, `animacion`; los íconos y el menú de controles,
  `ui`; los tests de las piezas puras, `testing`; que los tipos de SDL no salgan de
  `platform::`, `arquitectura`.

## cinematicas

**ADRs:** 0034 (cinemáticas). También la fe de erratas que hizo en el ADR-0012 (el paso
durante una cinemática), el ADR-0017 (la exportación desde Blender y lo que valida el
cocinador), el ADR-0021 (el panel), el ADR-0026 (sin HUD, las bandas, saltar), el
ADR-0027 (el evento de trigger y el comando de fase), el ADR-0028 (la sección del
binario), el ADR-0031 (la cámara de la cinemática y la salida), el ADR-0032 (el contexto
de la cinemática y la secuencia) y `presupuestos.md`.

Qué mirar:

- **Solo en el engine.** Un decodificador de video, o una cinemática pregrabada, es una
  decisión nueva (y una dependencia, que ve `build-y-licencias`).
- **Se arman en Blender:** un `.blend` por cinemática, los actores (`actor.<nombre>`) en
  la NLA, las cámaras y sus cortes por marcadores, y **marcadores con prefijo de una lista
  cerrada** (`sub:`, `sonido:`, `evento:`, `salida`). Un editor propio de cinemáticas, o
  un formato escrito a mano, es una decisión nueva. **La exportación usa el modo `SCENE`,
  el FOV por `KHR_animation_pointer` y copia los marcadores a los `extras`;** perder los
  marcadores o el FOV es `importante`. Que el cocinador no falle por lo que el ADR-0017
  lista para las cinemáticas lo ve `assets`.
- **El reloj es el número de paso.** Que el secuenciador avance con el dt del frame, o
  con un reloj de pared, es `bloqueante`: la partida cambia con los FPS (P2: a 30 y a
  144 FPS cambian 19 o 20 de los 20 hashes de control; a 60 FPS, 1 sin ruido en el dt y
  20 con ±20 %). **La cámara de la cinemática se calcula una vez por
  frame, desde el tiempo interpolado,** y no entra al hash; que la simulación la lea es
  `bloqueante` (también lo ve `camara`).
- **Dos tipos.** **En una cinemática, el mundo se detiene:** en el paso corren solo el
  secuenciador y sus actores; la física, la IA y el combate no. Que alguno de ellos corra
  durante una cinemática es `importante`: el salto deja de dar la misma partida (P2: 0 de
  172). Los VFX y el sonido ambiente siguen. **En una secuencia interactiva, el mundo
  corre,** el input se restringe con el contexto "secuencia" a lo que la escena permite,
  y **no se salta**.
- **Saltar = los actores a su última clave y los eventos de la partida que faltan, en
  orden;** los de la presentación (sonidos, subtítulos) se descartan. Saltar sin aplicar
  los eventos de la partida, o adelantando los pasos, es `importante`. **Saltar es una
  acción reasignable** del contexto de la cinemática (se mantiene 1 s, una CVar; por
  defecto, las teclas de "aceptar") y el *replay* la graba; leer la tecla directamente
  es `importante` (también lo ve `input`). Se puede saltar siempre, también la primera
  vez; la pausa ofrece "Saltar", que entra como la acción en el primer paso al reanudar.
- **Las transiciones:** de ida, un corte seco. **De vuelta, desde `salida`, la cámara de
  Blender se mezcla con *smoothstep* hacia la cámara del estilo del jugador, calculada
  en vivo.** Una mezcla lineal suelta, un resorte o un corte seco al devolver el control
  son una decisión nueva (P3: la mezcla lineal atraviesa a la caballera). **El cocinador
  barre el tramo de salida en los tres estilos** y falla si cruza el escenario o pasa a
  menos de 0,5 m de la caballera; validarlo en un solo estilo es `importante`.
- **Dónde y qué la dispara:** en el binario de su zona, o en el global si la usan varias;
  la disparan el evento de trigger o el comando de fase "cinemática" (ADR-0027). C++ por
  cinemática, o un lenguaje de *script*, es una decisión nueva.
- **El audio y los subtítulos:** los subtítulos son ids de la tabla de textos del
  ADR-0026 (un texto suelto dentro de la cinemática es `importante`: no se traduce); los
  sonidos son eventos del paso con la latencia fija del ADR-0024.
- **El estilo:** el mismo render a 640×360 con el look PSX; **sin HUD; bandas de 1,85:1**
  (7 px) que aparecen con el corte de entrada y se retiran durante la salida; los
  subtítulos, sobre la imagen. Otra proporción es una decisión nueva.
- **Las ventanas de una secuencia interactiva se miden en pasos y son CVars.** En
  milisegundos, o con el reloj del frame, es `importante`.
- Los tests del secuenciador y el test de propiedad del salto los audita `testing`; el
  panel de ImGui, `herramientas`; las bandas y el anillo de saltar, `ui`.

## herramientas

**ADRs:** 0021 (herramientas de depuración, profiling y logging). También la fila de la
región de depuración en `docs/performance/presupuestos.md` y la fe de erratas del
ADR-0034 (el panel de cinemáticas).

Qué mirar:

- **Una frontera por dependencia:** solo un módulo incluye `imgui.h`. Tracy tiene un
  módulo por capa: la zona de CPU en una capa de abajo y la de GPU (`TracyVkZone`) en
  `gpu`. El resto del engine usa la macro de zona propia y una interfaz propia para los
  paneles. Un `#include` de ImGui o de Tracy fuera de esos módulos es un hallazgo. Los
  hilos de Tracy son la única excepción al hilo único del ADR-0012 (fe de erratas del
  ADR-0012), y no tocan el estado del juego.
- **La macro de zona tiene dos variantes:** zona y traza para las funciones normales, y
  solo traza (vacía en `relwithdebinfo` y en Release) para las que corren por elemento:
  por vértice, por triángulo o por arista. Una zona de Tracy en una función por elemento
  es un hallazgo, y también lo es una función dibujada sin ninguna de las dos variantes
  (lo audita `diagramas`).
- **ImGui sin sus backends:** el input le llega desde `platform::`, y sus listas de
  vértices las dibuja el renderer propio (pipeline propio, atlas como textura, datos por
  el *ring buffer*). Usar `imgui_impl_sdl3` o `imgui_impl_vulkan` contradice la Decisión 1
  (además, el primero saca SDL fuera de `platform::`). Sus asignaciones van a la región de
  depuración con `ImGui::SetAllocatorFunctions`.
- **Tracy siempre con `TRACY_ON_DEMAND`:** sin esa definición, Tracy guarda todo en
  memoria hasta que se conecta un servidor, sin tope. Que falte es `bloqueante`, porque
  ensucia R4 en las mediciones del protocolo. La versión de Tracy es la misma en el
  cliente y en el servidor.
- **El registro por frame no depende de Tracy:** guarda, por frame, el número de frame,
  los pasos que corrió y cuánto tardó cada uno, el tiempo de CPU de cada sistema, los
  timestamps de la GPU, el *present id* y su tiempo de presentación (que llega varios
  frames después), la memoria con la escala de R4 del ADR-0020 (`RssAnon` de
  `/proc/self/statm` más el `heapUsage` de `VK_EXT_memory_budget`), el driver de video y
  los contadores del juego (zombies activos, pedazos vivos, ragdolls activos y, por paso,
  los cortes procesados y los que esperan) y **si el frame esperó a una zona** (ADR-0027:
  0 pasos). Esos frames llevan su propio contador y no cambian cómo se calcula R1
  (ADR-0020); contarlos como de recuperación es un hallazgo. Usar `VmRSS` o `VmHWM` para R4 es un
  hallazgo: cuentan dos veces la memoria de Vulkan mapeada. El registro se escribe en
  bloques fijos de la región de depuración; el que se llena se le entrega al hilo de I/O
  (ADR-0028), que lo escribe en la carpeta de estado, así que no crece con la sesión y
  el hilo principal no espera al disco. Leer el protocolo desde una captura de Tracy contradice la
  Decisión 4.
- **Los frames de recuperación de un tirón externo** se marcan y se reportan aparte (R1
  con y sin ellos, y cuántos). Descartarlos es un hallazgo.
- **Timestamps de GPU:** el que abre una medición va en una etapa que espera a los
  semáforos del frame, no en `TOP_OF_PIPE`, que mide también la espera del swapchain.
- **R3 se mide en Wayland** (el primer píxel de `VK_EXT_present_timing`), con
  `VK_KHR_present_wait2` de respaldo. RenderDoc corre en XWayland
  (`SDL_VIDEO_DRIVER=x11`), con una tecla que dispara la captura por su API *in-app*.
- **Logging:** cinco niveles (error, aviso, info, debug, traza) y una categoría por
  módulo. Se formatea con `std::format_to_n` a un *ring buffer* de entradas fijas, sin
  asignar memoria, y una vez por frame (y siempre antes de abortar por un error fatal)
  se le entrega al hilo de I/O, que lo escribe en la carpeta de estado (ADR-0028).
  Escribir el log desde el hilo principal es un hallazgo. `printf` en el engine, spdlog o un formato diferido contradicen la
  Decisión 5. Por preset: los cinco niveles en `debug`, hasta info en `relwithdebinfo`,
  y solo error y aviso en `release`; los demás se eliminan al compilar.
- **Dibujo de depuración:** *immediate mode*, con duración opcional en pasos, en la
  región de depuración. Guardar sus primitivas en la arena del paso o en la del frame es
  un hallazgo: la del paso se borra antes del render. Si se llena, descarta las
  primitivas nuevas y muestra cuántas descartó (excepción declarada a la regla de
  abortar del ADR-0010). Del corte muestra lo que lista la Decisión 6, normales
  incluidas.
- **CVars:** cada valor ajustable sin recompilar es una CVar registrada con nombre, tipo,
  valor por defecto, rango y si afecta la simulación. En Release es una constante. Un
  valor de tuning editable en Release, o leído de un archivo en Release, contradice la
  Decisión 7. Los valores de ajuste de la cámara y los controles que fija la visión
  (distancia, altura, campo de visión, ventanas de input) son CVars; las opciones del
  jugador (sensibilidad, invertir un eje) no lo son (ADR-0028). **Excepción (ADR-0027):**
  los valores del mundo (distancias de carga y de simulación y tope) son datos de los
  assets, no CVars: vienen del binario del mundo y en desarrollo los cambia el panel del
  mundo, que valida contra el binario y no deja que el tope pase el del binario. Registrarlos como CVars
  editables sin esa validación es `importante`.
- ***Replay*:** graba el input de cada paso (no los eventos de SDL ni el input del
  frame), con un encabezado (versión del formato, commit, el mundo, el checkpoint de
  partida, es decir zona, fase y fases terminadas, y los valores del mundo en uso
  (ADR-0027), semilla y todas las CVars del inicio) y un hash de control cada 60 pasos. En desarrollo, cada sesión se
  graba entera desde que empieza el nivel, volcada a disco por bloques. Un anillo con
  solo los últimos minutos es un hallazgo: no se puede reproducir sin el estado del
  comienzo de su ventana. Un cambio de una CVar que afecta
  la simulación durante una grabación se graba como evento de su paso. El hash se compara
  con la misma glibc: en la misma máquina, o en la CI contra un hash guardado en la
  máquina del autor (ADR-0029; lo audita `ci`). Los controles de tiempo (pausa, avance de un paso, cámara lenta) no cambian
  el `dt`.
- **Memoria:** contadores de uso, pico y fallos en cada región, en todos los builds
  (también los pide el ADR-0010). `TracyAllocN` solo en el pool de pedazos; en una
  arena, manda miles de eventos por paso.
- **Presets:** una macro por herramienta, con la combinación de cada preset en
  `CMakePresets.json`. Release no compila ninguna herramienta, salvo los contadores de
  memoria y el log de errores y avisos. En `relwithdebinfo`, el protocolo mide con la
  interfaz oculta y el dibujo de depuración apagado. Las flags del compilador no son de
  este tema: las audita `build-y-licencias`.
- **La región de depuración** existe solo en `debug` y `relwithdebinfo`, y su tamaño
  está en `presupuestos.md`, junto con la memoria base de Tracy (~14 MiB). Cada parte
  tiene el allocator y la regla al llenarse que fija la tabla de Consecuencias del
  ADR-0021. Lo que tenga que ver con su reserva y sus límites lo audita `memoria`.
- **Dependencias de desarrollo:** Dear ImGui, Tracy y `renderdoc_app.h` (MIT) entran a
  `CREDITS.md` en el ticket que las suma al build. Eso lo audita `build-y-licencias`.
- **El panel de cinemáticas (fe de erratas, ADR-0034):** solo en desarrollo; reproduce,
  pausa, avanza paso a paso y salta, y al ir hacia atrás solo muestra las poses y la
  cámara, sin aplicar los eventos de la partida. **Usarlo para pausar, avanzar o ir hacia
  atrás corta la grabación,** como la recarga en caliente; seguir grabando después es
  `importante`. **Saltar es una acción** y se graba como las demás; el encabezado del
  *replay* no cambia.

## ci

**ADRs:** 0029 (la CI) y 0035 (la CD y la distribución, que reemplazó la Decisión 10
del ADR-0029). También el uso de `docs/performance/presupuestos.md` como límite del
*replay*.

Qué mirar:

- **El entorno:** los jobs que compilan o prueban corren en un contenedor
  `archlinux:latest` con el `mirrorlist` en la fecha del snapshot del Arch Linux
  Archive, leída del único archivo de `tools/ci/` que la guarda, y `pacman -Syyuu`. Una
  fecha escrita en el YAML contradice la Decisión 2; un job de build en el runner Ubuntu
  nativo o una imagen propia en GHCR contradicen la Decisión 1. Mezclar el archivo con mirrors al día también:
  la wiki de Arch lo desaconseja. La única excepción es la corrida nocturna, que usa
  `archlinux:latest` sin fecha y nunca bloquea.
- **Un cambio de la fecha** va en su propio PR `build:`. Si cambia la glibc, el mismo PR
  vuelve a grabar los hashes guardados (el del *replay* y el del render offline del
  audio, ADR-0024) y lo dice en su descripción.
- **Blender y D2** se instalan desde su tarball oficial, con la versión exacta y la
  verificación de su `sha256` (`SHA256SUMS` en D2). Instalarlos con `pacman`, o bajarlos
  sin verificar, contradice la Decisión 2. La versión es la misma que fijan el ADR-0017 y
  el ADR-0030.
- **Dos etapas:** primero los chequeos rápidos, que no compilan (formato, `reuse lint`,
  `actionlint`, reglas del ADR-0009 y los verificadores de diagramas (b), (c) y (d));
  después, en paralelo, Clang en `debug` con sus tests y luego los verificadores (e) y
  (h), GCC en `debug` con sus tests, la cobertura con la meta del 100 %, `clang-tidy`
  junto con los verificadores (a), (f), (g) e (i), compilar `relwithdebinfo` y `release`,
  y el *replay*. Un verificador que necesita compilar o correr los tests puesto en la
  etapa rápida es un hallazgo: falla siempre o no verifica nada. Un job que exige un ADR y no está, o que
  está pero no puede fallar (por ejemplo, la cobertura que solo informa), es un hallazgo.
- **`ci-ok` es el único chequeo obligatorio:** corre con `if: always()`, depende de todos
  los jobs del workflow y falla si alguno falló o se canceló. **Un `paths:` o
  `paths-ignore:` en el workflow de PR es `bloqueante`**: deja el PR esperando un
  chequeo que nunca llega. Saltar un job se hace con condiciones, a partir de un job que
  decide qué cambió. Un job nuevo que no está en los `needs` de `ci-ok` no bloquea nada,
  y eso también es un hallazgo.
- **Cuándo:** en `pull_request` hacia `main` y en `push` a `main`. La nocturna
  (`schedule`) nunca es obligatoria.
- **El YAML solo elige el entorno, instala y llama a un script de `tools/ci/`.** Comandos
  de build o de tests escritos en el YAML contradicen la Decisión 6: el autor no podría
  repetirlos con el mismo comando. Lo que es de CMake usa los *workflow presets*.
- **El *replay*** compara contra el hash guardado y también dos ejecuciones del mismo
  proceso. Compilado en `relwithdebinfo`, mide los picos de memoria por región y falla si
  pasan su fila de `presupuestos.md`; los contadores del juego fallan solo si pasan un
  tope duro de su ADR (la cuota de cortes por paso, los pedidos pendientes, el tope de
  ragdolls). Usar las filas de capacidad (R5) como tope es un hallazgo: R5 es un mínimo,
  y un escenario de carga daría un rojo falso. Medir los picos en `debug` también: las
  zonas rojas de ASan los inflan. **Nada mide tiempo en la CI:** un umbral de tiempo,
  o un benchmark que pueda fallar por tiempo, contradice la Decisión 8. Las
  instrucciones contadas solo informan, en el resumen del job.
- **Seguridad:** `permissions: contents: read` en cada workflow y más solo en el job que
  lo necesite; **cada `uses:` fijado por su SHA completo**, con la versión en un
  comentario (una etiqueta como `@v7` es un hallazgo); `.github/dependabot.yml` con el
  ecosistema `github-actions`; nunca `pull_request_target`; **ningún secreto en la CI**
  (el único de la CD, abajo); ningún runner propio (`runs-on: self-hosted`). Un valor del evento (título, rama, cuerpo de un
  PR) usado directo dentro de un `run:` es `bloqueante` por inyección: va por `env:`.
- **Velocidad:** caché de `/var/cache/pacman/pkg` con la fecha como clave, y de Blender
  y D2 con sus versiones; `cancel-in-progress` solo en los PRs. Si una corrida real pasa
  de 10 minutos, se anota y se abre un ticket.
- **CD (ADR-0035): el disparo y el orden.** Corre solo con un tag `v*`. **El paquete de
  assets se cocina una vez** (en el contenedor de la CI, con Blender) y viaja como
  artefacto a Linux y a Windows; cocinarlo en cada plataforma es un hallazgo (los dos
  podrían llevar bytes distintos). Después: tests, Release dos veces, `strip` y
  empaquetado por plataforma; las pruebas de humo; `SHA256SUMS`, una atestación por
  paquete y **una release en borrador**. Publicar directo, sin borrador, contradice la
  Decisión 7.
- **CD: el toolchain.** **Linux, en el SDK de *sniper* con GCC 14; Windows, con
  llvm-mingw cruzando desde Linux**, con Wine y lavapipe instalados en el job desde los
  paquetes de la imagen. **Las dos imágenes, fijadas por *digest*** (`@sha256:…`); un tag
  como `latest` es `importante`: rompe la reproducibilidad. Publicar un binario
  compilado en el contenedor de Arch es `bloqueante`: no arranca en distros con una glibc
  más vieja (medido). Una imagen propia en GHCR contradice la D1 del ADR-0029. MSVC, o
  `clang-cl` en un runner de Windows, es una decisión nueva.
- **CD: las pruebas.** Linux: los tests de `debug` en *sniper* **sin** las comparaciones
  contra un hash guardado (el *replay* y el render offline del audio, ADR-0024; la del
  *replay* de dos ejecuciones en el mismo proceso sí corre). Windows: los tests bajo Wine,
  sin *sanitizers*, sin hashes guardados y **sin los tests que dependen de un
  *sanitizer*** (los *death tests* de ASan del ADR-0010, excluidos por una etiqueta de
  CTest; un *death test* sin la etiqueta hace fallar la CD); configurar, compilar y
  probar en la misma imagen. **El runner `windows-latest` es la única excepción a la D6
  del ADR-0029:** solo corre la prueba de humo; compilar ahí, o sumar otro job que no se
  repite en local, es un hallazgo. **Las pruebas de humo corren el ejecutable final**, el mismo que se empaqueta
  (contenedores de la última Ubuntu LTS y de la Debian estable más vieja con soporte, y
  un runner `windows-latest`); probar un binario recompilado aparte es un hallazgo:
  rompe "lo que se publica es lo que se probó". Correr un hash guardado fuera de la CI, o
  los tests en `release` (los *death tests* no pasan), es un hallazgo.
- **CD: reproducible.** El job exporta `SOURCE_DATE_EPOCH` con la fecha del commit (las
  banderas, como `-ffile-prefix-map`, las audita `build-y-licencias`). **Cada Release se
  compila dos veces en rutas distintas y el job falla si los bytes difieren;** que la
  comparación solo informe es un hallazgo.
- **CD: el paquete.** `MedievalZombies-<versión>-linux-x86_64.tar.gz` y
  `MedievalZombies-<versión>-windows-x86_64.zip`: el ejecutable sin símbolos, `LICENSE`,
  **`LICENSES/`** (MIT, CC BY 4.0 y OFL 1.1: la OFL y el MIT piden su texto junto a cada
  copia; que falte es `importante`), `CREDITS.md` y el `LEEME.txt` en español e inglés; el `.exe` con ícono, `VERSIONINFO` y
  manifiesto. **Sin empaquetadores (UPX) ni datos pegados al final del PE:** atraen las
  heurísticas de los antivirus; es `importante`. AppImage, Flatpak o un instalador son
  una decisión nueva. Los símbolos quedan como artefacto del workflow, sin publicarse.
- **CD: la firma.** **Por ahora no se firma** (se retoma cuando haya un juego jugable).
  Un paso de firma, o un certificado comprado, es una decisión nueva. Las notas de la
  release explican el aviso de SmartScreen y cómo verificar el SHA-256 y la atestación.
  **No se sube a VirusTotal antes de publicar;** un job que lo hace es una decisión nueva.
- **CD: los secretos y los permisos.** **Un solo secreto, `BUTLER_API_KEY`, dentro del
  entorno `itch.io`** (aprobación del autor, solo tags `v*`). Un secreto del repo, o
  usado fuera de ese entorno, es `bloqueante`. Solo los jobs de release tienen
  `contents: write`, `id-token: write` y `attestations: write`. **El job de itch.io
  corre al publicar el borrador y sube los mismos archivos** de la release; recompilar o
  re-empaquetar ahí es un hallazgo. `butler` en una versión fijada con su `sha256`.
- **macOS queda fuera** del flujo (ADR-0035, dimensión 5): un job de macOS es una
  decisión nueva.
- **Puedes ejecutar `actionlint`** sobre los workflows si está instalado, porque solo
  lee.
