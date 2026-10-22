# Temas de auditoría

Cada tema es una parte de la auditoría, con sus ADRs, las rutas que lo activan y una
guía de qué mirar. Un solo auditor (`auditor-adr`) recorre los temas activos, uno a la
vez. **La guía orienta, pero manda el texto del ADR.**

## Índice

| Tema | ADRs | Se activa cuando el PR toca… |
|---|---|---|
| [proceso](#proceso) | 0001, 0002, 0014, 0020 | **Siempre** |
| [docs-y-adrs](#docs-y-adrs) | 0001, 0014 (reglas 8 y 11), `CLAUDE.md` | **Siempre** |
| [cpp-y-convenciones](#cpp-y-convenciones) | 0003, 0009, 0013, 0014 (reglas 8 y 10) | `*.cpp`, `*.hpp`, `*.h`, `*.inl`, `.clang-format`, `.clang-tidy`, `.editorconfig` |
| [arquitectura](#arquitectura) | 0004, 0012, 0016 (consultas directas), 0020 | `engine/**`, `game/**`, cualquier `CMakeLists.txt`, `docs/performance/**` |
| [memoria](#memoria) | 0010 | `engine/core/**`, `engine/gpu/**`, o código que reserve, libere o guarde memoria (arenas, pools, allocators, contenedores) |
| [build-y-licencias](#build-y-licencias) | 0006, 0013 (warnings), 0019, 0008 y 0009 (flags) | `CMakeLists.txt`, `CMakePresets.json`, `cmake/**`, `LICENSE*`, `LICENSES/**`, `REUSE.toml`, `CREDITS.md`, `assets/**`, `tools/third_party/**`, cualquier dependencia nueva |
| [graficos](#graficos) | 0005, 0007 (frames en vuelo y subida de los pedazos), 0008 (proyección y buffers), 0010 (tamaño del *ring buffer*) | `engine/gpu/**`, `engine/render*/**`, `shaders/**`, o código que llame a Vulkan |
| [matematica](#matematica) | 0008 | `engine/core/math/**`, o código que defina o use vectores, matrices, cuaterniones o comparaciones de `float` |
| [animacion](#animacion) | 0015 | `engine/animation/**`, shaders de skinning, o código que use esqueletos, poses, clips, IK o pesos de vértices |
| [fisica](#fisica) | 0016 | `engine/physics/**`, o código que use cuerpos, formas de colisión, contactos, uniones, consultas o el controlador de personaje |
| [assets](#assets) | 0017, 0007 (lo que el cocinador hace para el corte) | `assets/**`, las herramientas de assets en `tools/**`, las reglas de CMake que exportan o cocinan assets, o código que lea el binario cocinado |
| [corte](#corte) | 0007 | `engine/slicing/**`, o código que parta mallas, arme tapas, cree pedazos o lea un `slice_request` |
| [testing](#testing) | 0011, 0007, 0010, 0015, 0016 y 0017 (TDD en el corte, la memoria, la animación, la física y el cocinador de assets) | `tests/**`, o cualquier cambio en `engine/**`, `game/**` o en las herramientas de `tools/**` (para ver si trae sus tests) |
| [diagramas](#diagramas) | 0030 | `**/diagramas/**`, `*.d2`, o cualquier cambio en `engine/**` o `game/**` (para ver si cambió un flujo dibujado, o si un sistema nuevo trae su diagrama) |

### Activación por contenido

Además de las rutas, un tema corre si el **contenido** del diff toca su área. Esto
importa sobre todo en los cambios de documentación (un ADR nuevo, una guía):

| Si el diff menciona… | Corre |
|---|---|
| Vectores, matrices, cuaterniones, coordenadas, `float`, una librería de matemática (GLM, Eigen…) | `matematica` |
| Vulkan, render, shaders, profundidad, GPU | `graficos` |
| Una dependencia o librería nueva, licencias, assets de terceros, flags del compilador | `build-y-licencias` |
| ECS, capas, módulos, SDL, input, el bucle de juego | `arquitectura` |
| Rasgos de C++ (excepciones, RTTI, punteros inteligentes, templates, `virtual`) | `cpp-y-convenciones` |
| Tests, cobertura, bugs, TDD | `testing` |
| Rendimiento, presupuestos, benchmarks, tiempo de frame, profiling | `arquitectura` (por el reparto del frame) |
| Memoria, arenas, pools, allocators, `malloc`, `mmap`, `std::pmr`, contenedores, handles, ASan, presupuestos de memoria | `memoria` |
| Animación, skinning, esqueletos, huesos, clips, IK | `animacion` |
| Física, colisiones, cuerpos rígidos, ragdoll, solver, cápsulas, controlador de personaje, la detección del tajo | `fisica` |
| Assets, Blender, glTF, texturas, formatos de archivo, el cocinador, Git LFS | `assets` |
| Corte de mallas, tapas, pedazos, hendidura, `slice_request`, índice soldado | `corte` |
| Diagramas, D2, la leyenda, SVG, Mermaid, PlantUML, Graphviz | `diagramas` |

**Un solo dueño por regla.** Las flags del compilador las audita solo
`build-y-licencias`, aunque vengan del ADR-0008, 0009 o 0013. Así no aparece el mismo
hallazgo tres veces.

**Mantenimiento:** cuando se acepta un ADR nuevo, se agrega a la fila y a la sección del
tema que corresponda, o se crea un tema nuevo. El tema `docs-y-adrs` avisa si un ADR
aceptado del índice de `docs/README.md` no aparece en ningún tema.

---

## proceso

**ADRs:** 0001 (ADRs), 0002 (tickets), 0014 (colaboración y metodología), 0020
(requisitos y protocolo de medición del rendimiento). También `docs/process/git.md`,
`docs/process/tickets.md` y `docs/performance/presupuestos.md`.

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
- Si el ticket es feature o tarea: tiene label `modo:`, y "Qué debo poder explicar" está
  respondida o pendiente para el autor antes del merge.
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
- **Inmutabilidad (ADR-0014, regla 11):** mientras no exista código del juego, un ADR
  aceptado se puede corregir como fe de erratas, registrada en el campo **Fe de
  erratas** de su encabezado (fecha, ticket y qué cambió). Cuando existe código (`engine/` o
  `game/` con fuentes), editar la Decisión de un ADR aceptado es `bloqueante`: hace
  falta un ADR nuevo que lo reemplace.
- **No adelantar decisiones:** un texto que da por decidido algo que le toca a un ADR
  pendiente es un hallazgo. Por ejemplo, fijar el formato de las escenas antes del
  ADR-0027, o decidir el *hot reload* antes del ADR-0028.
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
  incluye headers de SDL. `SDL_GPU` y `SDL_Renderer` están prohibidos.
- **El input se expone como estado** (teclas y botones en el frame), no como eventos
  crudos de SDL.
- **ECS:** de *sparse sets* propio. La entidad es un handle con generación.
- **Bucle:** paso fijo de 60 Hz con acumulador e interpolación, como mucho 8 pasos por
  frame y el frame recortado a 250 ms.
- **Comunicación:** colas de eventos entre sistemas, guardadas en la arena del paso
  (fe de erratas del ADR-0012, por el ADR-0010), para las órdenes que cambian el estado. Las consultas de solo lectura (rayos, barridos, el
  barrido del tajo) son llamadas directas hacia abajo (fe de erratas del ADR-0012, por
  el ADR-0016).
- **Un solo hilo** por ahora.
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
  sistema operativo o al driver dentro del paso o del render es un hallazgo. La memoria
  que vive un nivel la decide el ADR-0028.
- **`gpu::Allocator`:** bloques grandes por tiempo de vida (lineal para el nivel, el
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
  pedazos, ragdolls) degradan con su regla, y el juego pregunta antes si hay lugar.
  Devolver `nullptr` en silencio, o pedir al heap como respaldo, es un hallazgo.
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
- **Presets:** Debug (ASan + UBSan + validation layers), RelWithDebInfo y Release.
- **Flags:** `-Werror` y la lista ampliada de warnings del ADR-0013, sin `-Weverything`.
  `-fno-exceptions` y `-fno-rtti` (ADR-0009). **Sin `-ffast-math` y sin `-march`**
  (ADR-0008).
- **SDL3:** del sistema, con la opción `MZ_VENDOR_SDL3` para usar FetchContent.
- **Shaders:** `glslc` y `spirv-val`.
- **Dependencias:** una dependencia nueva necesita su ADR y su fila en `CREDITS.md`.
  Una dependencia **copiada en el repo** (como `tools/third_party/`) lleva además su
  anotación en `REUSE.toml`, con el copyright y la licencia de sus autores, debajo de la
  anotación por defecto. Sin ella, `reuse lint` pasa igual, pero con el autor
  equivocado.
- **Licencias (ADR-0019):** el código va bajo MIT, y la documentación y los assets
  propios bajo CC BY 4.0. Un asset de terceros solo puede ser CC0 o CC BY, con su fila
  en `CREDITS.md` y su anotación en `REUSE.toml` si no es CC BY 4.0 de Clemente Grass.
- **`uvx reuse lint` sigue pasando.** Puedes ejecutarlo, porque solo lee.

## graficos

**ADRs:** 0005 (API gráfica), 0007 (frames en vuelo y subida de los pedazos), 0008
(proyección y buffers), 0010 (el *ring buffer* dimensionado para el peor caso).

Qué mirar:

- **Vulkan 1.3**, con *dynamic rendering* y `synchronization2`. Sin *render passes*
  clásicos.
- **Inicialización propia**, sin vk-bootstrap.
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
  con un viewport negativo), y `frontFace` ajustado a ese volteo. Plano lejano infinito
  solo en la cámara principal.
- **Buffers de GPU:** los `struct` que van a la GPU usan `vec4` o relleno explícito,
  con `static_assert` sobre `sizeof` y `offsetof`.

## matematica

**ADRs:** 0008.

Qué mirar:

- **Biblioteca propia**, sin GLM ni DirectXMath.
- **Tipos concretos con `float`:** `vec2`, `vec3`, `vec4`, `mat3`, `mat4`, `quat` y
  `plane`. Sin `vec<N, T>` genérico.
- **Escalar.** SIMD y SoA solo donde el profiler lo justifique, con la medición en el PR.
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

**ADRs:** 0015 (sistema de animación).

Qué mirar:

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

**ADRs:** 0016 (física).

Qué mirar:

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
- **El zombie muerto entero es un ragdoll, y los pedazos son cuerpos rígidos,** con un
  tope propio de ragdolls activos. Los cuerpos unidos por una rótula no chocan entre sí.
- **Los personajes son controladores cinemáticos** que se mueven y se sacan de lo que
  penetran, con pruebas de superposición, no con barridos ni como cuerpos dinámicos. El
  empuje es un impulso programado. El avance por paso queda muy por debajo del radio de
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

**ADRs:** 0017 (pipeline de assets), 0007 (lo que el cocinador hace para el corte).

Qué mirar:

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
  huesos, un prefijo de colisión mal escrito o desconocido (incluido `UCX_`), una
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
  la tiene. En el escenario, solo `UBX_`, `UCP_` y `USP_`.
- **La definición del formato** (cabecera y secciones) vive en `engine/core`, sin lógica
  de carga.
- **Un asset de terceros que solo existe como `.glb`** se versiona importado a `.blend`,
  con el original anotado en `CREDITS.md`.
- **Git directo, nunca LFS:** un `.gitattributes` con `filter=lfs` es `bloqueante`.
  Ningún archivo pasa de 50 MiB.
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
- El TDD y la cobertura del corte no son de este tema: los audita `testing`.

## testing

**ADRs:** 0011, 0007, 0010, 0015, 0016 y 0017 (reparto del TDD en el corte, la memoria,
la animación, la física y el cocinador de assets).

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
- **Cobertura del 100 % de líneas y ramas** en el core y en las zonas con TDD. Las
  ramas de los `assert` se cubren con *death tests*. Una línea imposible de cubrir lleva
  una justificación escrita junto al código.
- **Organización:** un ejecutable por módulo (`tests/core/`, `tests/slicing/`…), más
  `tests/regression/`, y etiquetas de CTest por tipo.
- **Regresión de bugs:** `tests/regression/bug_<N>.cpp` con el test `Bug<N>_…` y la URL
  del issue. Lleva `WILL_FAIL` mientras el bug está abierto, y se le quita en el PR que
  lo arregla.
- **Los tests corren en el preset Debug**, con ASan y UBSan.

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
  se activa solo donde corren los tests: activa en RelWithDebInfo o en Release es un
  hallazgo, porque altera las mediciones de rendimiento.
- **Lo que los verificadores no ven** es justo lo que mira este tema: si el diagrama
  describe bien lo que pasa dentro de cada función, si las etiquetas describen bien sus
  datos y si las ramas que el test no recorre están dibujadas.
