# Temas de auditoría

Cada tema es un auditor (`auditor-adr`) con sus ADRs, las rutas que lo activan y una
guía de qué mirar. **La guía orienta, pero manda el texto del ADR.**

## Índice

| Tema | ADRs | Se activa cuando el PR toca… |
|---|---|---|
| [proceso](#proceso) | 0001, 0002, 0014, 0020 | **Siempre** |
| [docs-y-adrs](#docs-y-adrs) | 0001, 0014 (reglas 8 y 11), `CLAUDE.md` | **Siempre** |
| [cpp-y-convenciones](#cpp-y-convenciones) | 0003, 0009, 0013, 0014 (reglas 8 y 10) | `*.cpp`, `*.hpp`, `*.h`, `*.inl`, `.clang-format`, `.clang-tidy`, `.editorconfig` |
| [arquitectura](#arquitectura) | 0004, 0012, 0020 | `engine/**`, `game/**`, cualquier `CMakeLists.txt`, `docs/performance/**` |
| [build-y-licencias](#build-y-licencias) | 0006, 0013 (warnings), 0019, 0008 y 0009 (flags) | `CMakeLists.txt`, `CMakePresets.json`, `cmake/**`, `LICENSE*`, `LICENSES/**`, `REUSE.toml`, `CREDITS.md`, `assets/**`, cualquier dependencia nueva |
| [graficos](#graficos) | 0005, 0008 (proyección y buffers) | `engine/gpu/**`, `engine/render*/**`, `shaders/**`, o código que llame a Vulkan |
| [matematica](#matematica) | 0008 | `engine/core/math/**`, o código que defina o use vectores, matrices, cuaterniones o comparaciones de `float` |
| [animacion](#animacion) | 0015 | `engine/animation/**`, shaders de skinning, o código que use esqueletos, poses, clips, IK o pesos de vértices |
| [testing](#testing) | 0011, 0015 (TDD en la animación) | `tests/**`, o cualquier cambio en `engine/**` o `game/**` (para ver si trae sus tests) |

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
| Animación, skinning, esqueletos, huesos, clips, IK, ragdoll | `animacion` |

**Un solo dueño por regla.** Las flags del compilador las audita solo
`build-y-licencias`, aunque vengan del ADR-0008, 0009 o 0013. Así no aparece el mismo
hallazgo tres veces.

**Mantenimiento:** cuando se acepta un ADR nuevo, se agrega a la fila y a la sección del
tema que corresponda, o se crea un tema nuevo. El auditor `docs-y-adrs` avisa si un ADR
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
  historial. Un número sin medir sigue marcado como hipótesis.
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
  pendiente es un hallazgo. Por ejemplo, nombrar una herramienta de assets antes del
  ADR-0017.
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

**ADRs:** 0004 (capa de plataforma), 0012 (arquitectura del engine), 0020 (reparto del
frame por sistema).

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
- **Comunicación:** colas de eventos por frame, guardadas en la arena del frame.
- **Un solo hilo** por ahora.
- **Reparto del frame (ADR-0020):** `docs/performance/presupuestos.md` separa el costo
  por paso de simulación del costo por frame. Un frame con dos pasos tiene que caber en
  16,7 ms. Lo que depende de la simulación (por ejemplo, la cuota de cortes) se cuenta
  en pasos, no en frames, para no romper el determinismo.
- **Estructura del monorepo:** `engine/`, `game/`, `tools/`, `shaders/`, `tests/` y
  `docs/`.

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
- **Licencias (ADR-0019):** el código va bajo MIT, y la documentación y los assets
  propios bajo CC BY 4.0. Un asset de terceros solo puede ser CC0 o CC BY, con su fila
  en `CREDITS.md` y su anotación en `REUSE.toml` si no es CC BY 4.0 de Clemente Grass.
- **`uvx reuse lint` sigue pasando.** Puedes ejecutarlo, porque solo lee.

## graficos

**ADRs:** 0005 (API gráfica), 0008 (proyección y buffers).

Qué mirar:

- **Vulkan 1.3**, con *dynamic rendering* y `synchronization2`. Sin *render passes*
  clásicos.
- **Inicialización propia**, sin vk-bootstrap.
- **`gpu::Allocator` propio** (VMA es solo el plan B, y necesita su ADR).
- **Ring buffer** persistente y mapeado, uno por frame en vuelo, de tamaño máximo por
  frame × frames en vuelo.
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
  `nearly_equal(a, b, tol)`, con una tolerancia explícita.
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

## testing

**ADRs:** 0011, 0015 (reparto del TDD en la animación).

Qué mirar:

- **GoogleTest.** Los tests de propiedades usan una semilla fija que se imprime al
  fallar.
- **TDD estricto** en la matemática, la memoria, los contenedores, el ECS y las
  funciones del corte. Código nuevo en esas zonas **sin test** es `bloqueante`.
- **En la animación (ADR-0015),** el TDD y el 100 % de cobertura aplican al skinning de
  la CPU (va en la ruta del corte), a la IK de dos huesos, al recorrido de la jerarquía
  y al *nlerp*. Código nuevo en esas piezas sin test es `bloqueante`, como en las demás
  zonas con TDD. La máquina de estados y la mezcla de capas llevan tests después.
- **Cuando exista el renderer,** el skinning trae un test que compara la GPU (con
  lavapipe) con la CPU, vértice a vértice y con tolerancia (ADR-0015).
- **Cobertura del 100 % de líneas y ramas** en el core y en las zonas con TDD. Las
  ramas de los `assert` se cubren con *death tests*. Una línea imposible de cubrir lleva
  una justificación escrita junto al código.
- **Organización:** un ejecutable por módulo (`tests/core/`, `tests/slicing/`…), más
  `tests/regression/`, y etiquetas de CTest por tipo.
- **Regresión de bugs:** `tests/regression/bug_<N>.cpp` con el test `Bug<N>_…` y la URL
  del issue. Lleva `WILL_FAIL` mientras el bug está abierto, y se le quita en el PR que
  lo arregla.
- **Los tests corren en el preset Debug**, con ASan y UBSan.
