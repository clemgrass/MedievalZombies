# ADR-0012: Arquitectura del engine

- **Estado:** Aceptado
- **Fecha:** 2026-10-18
- **Ticket:** #22
- **Reemplaza / Reemplazado por:** —
- **Fe de erratas:** 2026-10-19 (#29): la comunicación por colas es para las órdenes que
  cambian el estado. Las consultas de solo lectura (por ejemplo, a la física) son
  llamadas directas hacia abajo (ADR-0016). Se aclara en la Decisión 4 y en
  Consecuencias.
  2026-10-21 (#19): las colas de eventos viven en la arena del paso, que se reinicia al
  terminar cada paso de simulación. La arena del frame queda para el render (ADR-0010).
  Se corrige en el orden del frame, la Decisión 4 y Consecuencias.
  2026-10-28 (#43): Tracy (ADR-0021) arranca 4 hilos propios en los builds de
  desarrollo. Son una excepción al hilo único: leen sus propias colas de zonas y las
  mandan por red, sin tocar el estado del juego, así que no afectan el determinismo. El
  engine sigue en un hilo. Se corrige en la Decisión 5.

## Contexto y problema

Ya está decidido **con qué** se construye el engine: C++20 con un subconjunto acotado
(ADR-0003, ADR-0009), SDL3 detrás de una interfaz propia (ADR-0004), Vulkan 1.3
(ADR-0005) y CMake (ADR-0006). Falta decidir **cómo se organiza por dentro**:

1. cómo se representan las cosas del juego (protagonista, zombies, pedazos de zombie,
   objetos del escenario);
2. en qué módulos se divide el código y quién puede depender de quién;
3. cómo fluye un frame: input, simulación, render;
4. cómo se comunican los sistemas entre sí, por ejemplo "la espada golpeó a este
   zombie";
5. si el engine es de un hilo o de varios;
6. cómo se separa el engine del juego dentro del monorepo.

Hay tres restricciones que vienen del corte de mallas:

- **Cada corte crea entidades nuevas en tiempo de ejecución:** los dos pedazos, cada
  uno con su malla, su cuerpo físico y su tiempo de vida.
- La memoria de esos pedazos vive en **pools con presupuesto**, y se referencian con
  **handles con generación** (ADR-0009).
- El ADR-0009 necesita que **"el código que corre cada frame"** sea identificable.

## Criterios

1. **Rendimiento:** uso de la caché y costo por entidad.
2. **Costo de crear y destruir entidades en tiempo de ejecución,** porque cada corte lo
   hace.
3. **Claridad:** que se entienda dónde vive cada dato y quién lo modifica.
4. **Facilidad para agregar sistemas** (un tipo de zombie nuevo, una mecánica nueva).
5. **Aprendizaje y defensa** frente a un evaluador.
6. **Simplicidad** para una sola persona.

## Opciones

### 1. Modelo de entidades

| Opción | Cómo se ve | A favor | En contra |
|---|---|---|---|
| **A. Jerarquía de clases** | `entity` → `character` → `zombie` → `armored_zombie`, con `virtual update()` | Intuitivo y "tradicional" | Choca con el ADR-0009 (sin jerarquías profundas ni `virtual` por entidad en el frame). La herencia rígida se rompe cuando aparece un "zombie con armadura que también es un pedazo cortado". Mal uso de la caché |
| **B. Objeto con componentes** | Un `game_object` con una lista de componentes polimórficos, al estilo de Unity o Unreal | Flexible y conocido | Cada componente es un objeto separado en memoria, con llamada virtual. Mismo choque con el ADR-0009 |
| **C. ECS** | La entidad es solo un **handle**. Los componentes son **datos** guardados en arrays contiguos. Los **sistemas** son funciones que recorren todas las entidades que tienen ciertos componentes | Muy buen uso de la caché, crear entidades es barato (según la variante), y encaja con los handles del ADR-0009. Es lo que usó Overwatch | Más abstracto al principio. Un ECS genérico es un proyecto en sí mismo |
| **D. Tablas específicas orientadas a datos** | Sin ECS genérico: un array por tipo de cosa (`zombies`, `slice_pieces`, `props`) en formato estructura de arrays | Lo más simple y lo más rápido para pocos tipos de entidad. Muy fácil de razonar | Agregar un tipo nuevo implica tocar el código de cada sistema. Se vuelve rígido si el juego crece |

#### Si se elige ECS, hay dos variantes de almacenamiento

| Variante | Cómo funciona | Quién la usa | Costo |
|---|---|---|---|
| **Archetypes** | Las entidades con exactamente el mismo conjunto de componentes se guardan juntas | Flecs, Unity DOTS | La iteración es la más rápida a gran escala, pero **agregar o quitar un componente mueve la entidad** de tabla, lo que es caro |
| **Sparse sets** | Cada tipo de componente tiene su propio array denso, más un índice disperso por entidad | EnTT | **Crear y modificar entidades es barato.** La iteración sobre varios componentes a la vez es algo más lenta a gran escala |

Un estudio de Eurographics (CGVC 2025) que compara las dos variantes confirma justo ese
trade-off.

### 2. Módulos y dirección de las dependencias

Propuesta de capas, donde **cada capa solo depende de las de abajo**:

```
game/          el juego: gameplay, tipos de zombie, la protagonista, niveles
───────────────────────────────────────────────────────────────────────────
engine/slicing   engine/physics   engine/animation   engine/renderer   (sistemas)
───────────────────────────────────────────────────────────────────────────
engine/gpu       abstracción de Vulkan (allocator de la GPU, ring buffers)
───────────────────────────────────────────────────────────────────────────
engine/core      memoria (arenas, pools), matemática, contenedores, result, handles, assert
───────────────────────────────────────────────────────────────────────────
engine/platform  ventana, input y tiempo (SDL3 queda escondido aquí)
```

- Las opciones son **capas estrictas** (como el diagrama) o **módulos sin regla de
  dirección**.
- Con capas estrictas, CMake puede **hacer cumplir la regla**: cada módulo es un target,
  y solo enlaza (`target_link_libraries`) con los de abajo. Una dependencia hacia arriba
  no compila.

### 3. Flujo del frame (game loop)

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Paso variable** | `update(dt)` con el tiempo real del frame | La física se vuelve inestable y no determinista: un frame lento produce un salto grande |
| **Paso fijo con acumulador e interpolación** | La simulación avanza en pasos fijos (por ejemplo 60 Hz). El render dibuja tan rápido como pueda, **interpolando** entre los dos últimos estados simulados | Más complejo. Hay que guardar el estado anterior para interpolar |
| **Paso fijo, sin interpolación** | Simulación y render atados a la misma frecuencia | Si el render no alcanza esa frecuencia, el juego se ve en cámara lenta. Si el monitor es más rápido, no se aprovecha |

La referencia clásica es *Fix Your Timestep!* de Glenn Fiedler. Overwatch, por ejemplo,
simula en pasos fijos de 16 ms.

Orden propuesto dentro de un frame:

```
1. platform: vaciar la cola de eventos → estado de input        (ADR-0004)
2. mientras el acumulador ≥ paso fijo:
     simulación: gameplay → combate → corte → física → animación
     reiniciar la arena del paso                                 (ADR-0010)
3. render: interpolar el estado → grabar comandos → enviar a la GPU  (ADR-0005)
4. fin de frame: reiniciar la arena del frame                    (ADR-0010)
```

### 4. Comunicación entre sistemas

| Opción | Ejemplo |
|---|---|
| **Llamadas directas** | El sistema de combate llama a `slicing::cut(zombie, plane)` |
| **Colas de eventos por frame** | El combate escribe un evento `slice_request { entidad, plano }` en una cola. El sistema de corte la vacía en su turno. La cola vive en la arena por frame |
| **Observadores o callbacks** | El combate notifica a quien se haya suscrito |

Las colas hacen **explícito el orden**: el corte siempre ocurre en su paso del frame,
nunca en medio de otro sistema. Además no asignan memoria (viven en la arena), y se
pueden registrar para depurar.

### 5. Hilos

| Opción | Qué significa |
|---|---|
| **Un hilo al principio** | Todo en el hilo principal. Los datos se organizan para poder paralelizar después (arrays de componentes, sistemas sin estado oculto) |
| **Sistema de jobs desde el inicio** | Un pool de hilos que ejecuta tareas en paralelo |

Con una CPU de portátil y un proyecto que recién empieza, el paralelismo se puede
agregar cuando el profiling lo pida (ADR-0014).

### 6. Engine y juego en el monorepo

```
engine/    biblioteca: platform, core, gpu, renderer, physics, animation, slicing
game/      ejecutable: depende de engine/
tools/     herramientas (scripts del repo, más adelante herramientas de assets)
shaders/   GLSL que el build compila a SPIR-V (ADR-0006)
tests/     pruebas (ADR-0011)
docs/      documentación
```

## Investigación

- **Overwatch (GDC 2017, Timothy Ford).** Blizzard usó un ECS para todo el gameplay:
  unos 46 sistemas y 103 tipos de componentes en el cliente. Según la charla, el ECS
  ayudó a contener la complejidad a medida que agregaban features, y redujo la
  superficie del código de red a tres sistemas. La simulación corre en pasos fijos de
  16 ms.
- **Game Programming Patterns (Robert Nystrom).** Los capítulos *Game Loop*, *Update
  Method* y *Component* explican la evolución de la jerarquía de clases a los
  componentes, y por qué el loop separa la actualización del render.
- **Fix Your Timestep! (Glenn Fiedler).** Es la referencia clásica del paso fijo con
  acumulador. El tiempo sobrante que no alcanza para un paso completo se arrastra al
  frame siguiente, y el render **interpola** entre el estado anterior y el actual para
  evitar tirones visuales.
- **Archetypes frente a sparse sets** (Eurographics CGVC 2025). Los sparse sets hacen
  más baratas las modificaciones de entidades pero escalan peor al iterar. Los
  archetypes iteran mejor a gran escala pero cuesta más cambiar la composición de una
  entidad. **Para este juego, donde cada corte crea entidades nuevas, ese costo de
  creación pesa.**
- **Mike Acton, Data-Oriented Design** (ya citado en el ADR-0003): el engine
  transforma datos. La arquitectura se diseña alrededor de **cómo fluyen los datos**,
  no alrededor de cómo se clasifican los objetos.

## Decisión

| # | Dimensión | Elección |
|---|---|---|
| 1 | Modelo de entidades | **ECS propio y simple, con sparse sets.** La entidad es un `handle` con generación (ADR-0009) |
| 2 | Módulos | **Capas estrictas** (`platform` → `core` → `gpu` → sistemas → `game`), que CMake hace cumplir: cada módulo es un target y solo enlaza con los de abajo |
| 3 | Game loop | **Paso fijo a 60 Hz con acumulador**, e **interpolación** en el render. Para evitar la espiral de la muerte: **máximo 8 pasos de simulación por frame** (el valor por defecto de Godot) y **tiempo de frame limitado a 250 ms** (el de *Fix Your Timestep!*). Los dos son constantes ajustables, y se revisan con profiling en la GPU de desarrollo |
| 4 | Comunicación | **Colas de eventos entre sistemas** para las órdenes que cambian el estado, guardadas en la arena del paso (ADR-0010). Las **consultas de solo lectura** a un módulo de abajo (un rayo, un barrido) son llamadas directas. Dentro de un mismo módulo, llamadas directas |
| 5 | Hilos | **Un hilo al principio.** Los datos se organizan para poder paralelizar cuando el profiling lo pida. La única excepción son los hilos de Tracy en los builds de desarrollo, que no tocan el estado del juego (ADR-0021) |
| 6 | Monorepo | `engine/`, `game/`, `tools/`, `shaders/`, `tests/`, `docs/` |

## Consecuencias

- **"El código que corre cada frame"** (ADR-0009) son los sistemas que se ejecutan
  dentro del paso de simulación y del render. Ahí aplican las reglas estrictas: nada de
  contenedores que asignen memoria, nada de `virtual` por entidad, y recursión solo
  acotada.
- **Cada módulo de `engine/` es un target de CMake.** Si se agrega una dependencia
  hacia arriba, el build falla.
- **El ECS necesita su propio diseño y sus tickets:** almacenamiento con sparse sets,
  consultas de varios componentes a la vez y destrucción diferida (no se destruye una
  entidad mientras un sistema la está recorriendo). Su memoria se diseña junto con el
  ADR-0010.
- **Límite de pasos:** si el acumulador todavía tiene tiempo después de 8 pasos, se
  descarta. En una máquina que no da abasto, el juego se ve en cámara lenta en vez de
  congelarse. Como referencia: Unity limita el frame a 0,333 s (unos 20 pasos a 60 Hz)
  y Godot a 8 pasos por frame. Este proyecto usa el límite de Godot, más estricto, por
  el hardware modesto.
- **La interpolación obliga a guardar el estado anterior** de lo que se dibuja (como
  mínimo, las transformaciones).
- **Las colas de eventos** son tipadas (por ejemplo `slice_request`) y viven en la
  arena del paso, así que desaparecen solas al terminar el paso. Lo que un paso le deja
  al render va en componentes (ADR-0010).
- **Las consultas no van por colas:** un sistema que necesita una respuesta en el momento
  (el combate preguntando qué tocó la espada, la cámara preguntando si hay una pared) la
  pide con una llamada directa a un módulo de abajo. Una consulta no cambia el estado,
  así que no altera el orden explícito que dan las colas.
- **La estructura del monorepo** se crea con el primer ticket de código. Hoy solo
  existen `docs/`, `.github/` y `tools/`.

## Cómo lo defiendo

**En el ADR-0003 dijiste que la orientación a objetos es lo tradicional. ¿Por qué
ahora ECS?**
Ha sido un clásico de la industria y es lo tradicional, pero en mi opinión ECS es más
interesante y calza mejor con este proyecto, porque es mejor para el rendimiento.

**¿Por qué sparse sets si los archetypes iteran más rápido?**
Siento que nunca se van a alcanzar tantas entidades al mismo tiempo. Además, el
momento más crítico es el corte, porque hay que crear geometría en tiempo real. Con
sparse sets, la entidad nueva se crea rápido. Con archetypes sería lenta justo en el
momento más crítico y más demandante del juego.

**Si un frame tarda 50 ms, ¿cuántos pasos de física corre tu loop, y qué pasa si el
siguiente también tarda?**
Corre 3 pasos (50 / 16,6), y lo que sobra queda en el acumulador para el frame
siguiente. Si los frames siguen tardando, cada frame más lento obliga a simular más
pasos, y simular más pasos hace más lento el frame siguiente. Sin límite, el juego se
congela. Por eso primero pusimos un límite, siguiendo un estándar: máximo 8 pasos por
frame y un frame de 250 ms como máximo. Con el límite, el juego se ve en cámara lenta
en vez de congelarse, y eso se va a probar y mantener en equipos de menor rendimiento
como el de desarrollo.

**Recórreme un frame completo: el jugador aprieta "atacar", ¿qué pasa en cada parte del
engine hasta que ve al zombie partido en dos?**
Se registra el input, empieza la animación y la espada recorre un arco. Durante el
arco se revisa constantemente si la espada hizo contacto. Cuando lo hace, se calcula el
plano en que toca, para calcular las normales que quedarían expuestas y así poder
recalcular la geometría que hay que rellenar.

Después se valida si ese zombie sigue vivo, en términos de seguir con su IA, o si el
pedazo que cayó no es suficiente para matarlo. Técnicamente, solo se crean 2 entidades
si se logra cortar, y deben provenir de la primera. Hay que calcular qué pedazo queda
muerto y cuál vivo: por ejemplo, un brazo solo queda muerto y el resto del zombie
sigue vivo; pero si se separa la cabeza, la cabeza y el resto quedan muertos. Eso,
igual que la fuerza necesaria para cortar a un zombie, se define mejor en otro ADR
(ADR-0018). La física también afecta todo esto.

Luego se renderiza todo lo más rápido posible, para que parezca que siempre hubo
geometría y assets dentro del zombie. Finalmente, se reinicia la memoria del frame.

## Referencias

- Timothy Ford, *Overwatch Gameplay Architecture and Netcode*, GDC 2017:
  <https://gdcvault.com/play/1024001/-Overwatch-Gameplay-Architecture-and>
- Robert Nystrom, *Game Programming Patterns*: <https://gameprogrammingpatterns.com/>
  (capítulos *Game Loop*, *Update Method* y *Component*)
- Glenn Fiedler, *Fix Your Timestep!*: <https://gafferongames.com/post/fix_your_timestep/>
- *Run-time Performance Comparison of Sparse-set and Archetype Entity-Component
  Systems*, Eurographics CGVC 2025: <https://diglib.eg.org/handle/10.2312/cgvc20251224>
- Mike Acton, *Data-Oriented Design and C++*, CppCon 2014:
  <https://isocpp.org/blog/2015/01/cppcon-2014-data-oriented-design-and-c-mike-acton>
- Jason Gregory, *Game Engine Architecture*, 4.ª ed., capítulos de arquitectura en
  tiempo de ejecución y game loop.
