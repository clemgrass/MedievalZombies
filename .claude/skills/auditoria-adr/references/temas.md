# Temas de auditoría

Cada tema es un auditor (`auditor-adr`) con sus ADRs, las rutas que lo activan y una
guía de qué mirar. **La guía orienta, pero manda el texto del ADR.**

## Índice

| Tema | ADRs | Se activa cuando el PR toca… |
|---|---|---|
| [proceso](#proceso) | 0001, 0002, 0014, 0020 | **Siempre** |
| [docs-y-adrs](#docs-y-adrs) | 0001, 0014 (reglas 8 y 11), `CLAUDE.md` | **Siempre** |
| [cpp-y-convenciones](#cpp-y-convenciones) | 0003, 0009, 0013, 0014 (reglas 8 y 10) | `*.cpp`, `*.hpp`, `*.h`, `*.inl`, `.clang-format`, `.clang-tidy`, `.editorconfig` |
| [arquitectura](#arquitectura) | 0004, 0012, 0016 (consultas directas), 0020 | `engine/**`, `game/**`, cualquier `CMakeLists.txt`, `docs/performance/**` |
| [build-y-licencias](#build-y-licencias) | 0006, 0013 (warnings), 0019, 0008 y 0009 (flags) | `CMakeLists.txt`, `CMakePresets.json`, `cmake/**`, `LICENSE*`, `LICENSES/**`, `REUSE.toml`, `CREDITS.md`, `assets/**`, `tools/third_party/**`, cualquier dependencia nueva |
| [graficos](#graficos) | 0005, 0008 (proyección y buffers) | `engine/gpu/**`, `engine/render*/**`, `shaders/**`, o código que llame a Vulkan |
| [matematica](#matematica) | 0008 | `engine/core/math/**`, o código que defina o use vectores, matrices, cuaterniones o comparaciones de `float` |
| [animacion](#animacion) | 0015 | `engine/animation/**`, shaders de skinning, o código que use esqueletos, poses, clips, IK o pesos de vértices |
| [fisica](#fisica) | 0016 | `engine/physics/**`, o código que use cuerpos, formas de colisión, contactos, uniones, consultas o el controlador de personaje |
| [assets](#assets) | 0017 | `assets/**`, las herramientas de assets en `tools/**`, las reglas de CMake que exportan o cocinan assets, o código que lea el binario cocinado |
| [testing](#testing) | 0011, 0015, 0016 y 0017 (TDD en la animación, la física y el cocinador de assets) | `tests/**`, o cualquier cambio en `engine/**`, `game/**` o en las herramientas de `tools/**` (para ver si trae sus tests) |

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
| Animación, skinning, esqueletos, huesos, clips, IK | `animacion` |
| Física, colisiones, cuerpos rígidos, ragdoll, solver, cápsulas, controlador de personaje, la detección del tajo | `fisica` |
| Assets, Blender, glTF, texturas, formatos de archivo, el cocinador, Git LFS | `assets` |

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
- **Comunicación:** colas de eventos por frame, guardadas en la arena del frame, para
  las órdenes que cambian el estado. Las consultas de solo lectura (rayos, barridos, el
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
  `min(r, dist)`, o nada.
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

**ADRs:** 0017 (pipeline de assets).

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
  quitados, y después conserva los 4 de mayor peso y se renormaliza.
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
- **Las cápsulas por hueso** se ajustan en el cocinador, después del recorte de huesos,
  con correcciones a mano. Tiene cápsula todo hueso que mueve algún vértice (si no
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

## testing

**ADRs:** 0011, 0015, 0016 y 0017 (reparto del TDD en la animación, la física y el
cocinador de assets).

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
  tiene cápsula). Viven en una biblioteca probada en
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
