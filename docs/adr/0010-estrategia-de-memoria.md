# ADR-0010: Estrategia de memoria

- **Estado:** Aceptado
- **Fecha:** 2026-10-21
- **Ticket:** #19
- **Reemplaza / Reemplazado por:** —
- **Fe de erratas:** 2026-10-28 (#43): el ADR-0021 suma herramientas de depuración a
  RelWithDebInfo. Ese build cuesta lo mismo que Release salvo esas herramientas, de
  costo despreciable y medido, y una región de depuración que cuenta en R4. Hay dos
  excepciones declaradas: el dibujo de depuración descarta lo que no cabe en vez de
  abortar (Decisión 8), y Tracy con un servidor conectado pide memoria mientras corre
  (Decisión 2), por lo que nunca se mide así. Además, el ADR-0021 decide cómo se ven el
  uso y los picos de cada región: contadores en todos los builds y un panel en los de
  desarrollo. Se corrige en Consecuencias (detección de errores y las excepciones a la
  regla de abortar) y en Abierto.

## Contexto y problema

En C++, el proyecto decide de dónde sale cada byte y cuándo se libera (ADR-0003). Este
ADR decide **cómo se reserva y se libera la memoria del engine, y cómo se garantiza que
alcance**, en especial durante el corte, que crea pedazos nuevos en medio de un paso de
simulación.

Hay que decidir:

1. cómo se **organiza** la memoria: por tiempo de vida, por sistema, o las dos cosas;
2. **de dónde sale** y **cuándo se toca** por primera vez;
3. cómo se maneja la **memoria temporal**: la arena por frame del ADR-0012, en un frame
   que puede tener de 0 a 8 pasos;
4. cómo viven los **objetos de tamaño fijo** que nacen y mueren en cualquier momento
   (cuerpos, pedidos de corte, ragdolls);
5. cómo viven los **datos de tamaño variable** que nacen y mueren en cualquier momento:
   los índices y los vértices nuevos de cada pedazo;
6. cómo subasigna **`gpu::Allocator`** (ADR-0005), cuánto mide el ***ring buffer*** y
   cuándo se libera lo que la GPU todavía puede estar leyendo;
7. cómo reciben memoria los **contenedores**: con `std::pmr` o sin él (lo dejó abierto el
   ADR-0009);
8. qué pasa **cuando la memoria se acaba**;
9. cómo se **detectan los errores** de memoria dentro de los allocators propios;
10. la **capacidad del ECS**: fija o paginada (lo dejó abierto el ADR-0012).

Lo que ya está decidido y limita este ADR:

- **ADR-0003:** "una arena por frame sirve para el trabajo temporal del corte, y un pool
  con presupuesto mantiene la geometría nueva".
- **ADR-0005:** Vulkan 1.3, con un solo heap de memoria compartido con la CPU en la
  UHD 620. `gpu::Allocator` es propio y subasigna, y su diseño se comparte con este ADR.
  La geometría dinámica sube por un *ring buffer* mapeado, una región por frame en vuelo,
  dimensionado para el peor caso. VMA es el plan B, detrás de la misma interfaz.
- **ADR-0006:** el preset Debug corre con ASan y UBSan.
- **ADR-0009:** sin excepciones. Los contenedores que asignan (`std::vector`) solo fuera
  del código que corre cada frame; en ese código, contenedores propios sobre arenas y
  pools. Handles con generación para lo que vive en pools. `std::shared_ptr` prohibido.
- **ADR-0011:** TDD estricto en la memoria y los contenedores.
- **ADR-0012:** un solo hilo, ECS con *sparse sets*, paso fijo de 60 Hz con hasta 8 pasos
  por frame, colas de eventos en la arena por frame, y la arena se reinicia **al terminar
  el frame**. La memoria vive en `engine/core`, y la de la GPU, en `engine/gpu`.
- **ADR-0016:** determinismo: toda la memoria se inicializa, y nada se recorre por
  dirección de memoria. Hay un tope de ragdolls activos.
- **ADR-0007:** 2 frames en vuelo. Hasta 32 cortes en un frame (4 por paso × 8 pasos).
  Los cortes que esperan (hasta 20, cada uno con su pose de ~4 KB) viven en memoria
  propia del corte, no en la arena por frame. Cada pedazo tiene sus índices y un buffer
  chico con sus vértices nuevos, y comparte el buffer de vértices del modelo. Liberar un
  pedazo espera a que terminen los frames en vuelo que lo usan.
- **ADR-0020:** R4 (pico de memoria ≤ 1 GiB de meta, 1,5 GiB obligatorio) y los
  presupuestos de `docs/performance/presupuestos.md` (hipótesis): pedazos 32 MiB, arena
  por frame 16 MiB, subida de geometría 2 × 8 MiB. **Límites duros y degradación
  diseñada** al superarlos.

**Fuera de alcance:**

| Qué | Lo decide |
|---|---|
| Cómo se cargan y se administran los assets en runtime | ADR-0028 |
| Las herramientas para ver la memoria (contadores en pantalla, profiler) | ADR-0021 |
| Cómo se dibuja un pedazo desde dos buffers | ADR-0022 |
| El diseño interno del ECS (consultas, destrucción diferida) | Ticket del ECS (ADR-0012) |
| Hilos: los allocators de este ADR no se comparten entre hilos | ADR-0012 (un hilo) |

## Criterios

Los cuatro del ticket, más dos que salieron de la pregunta y de la investigación:

1. **Costo predecible por frame:** ni el allocator ni el sistema operativo agregan un
   costo variable dentro del frame.
2. **Sin pausas:** ningún frame espera una compactación, una limpieza o un *page fault*
   grande.
3. **Garantía de que alcanza:** cuánta memoria usa el juego se sabe al iniciar, y lo que
   no alcance falla al iniciar, no en el minuto 40.
4. **Detección de errores:** los errores de memoria dentro de los allocators propios se
   ven en Debug, con los sanitizers del ADR-0006.
5. **Simplicidad** para una sola persona.
6. **Aprendizaje y valor para defenderlo.**

## Opciones

### 1. Cómo se organiza la memoria

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Por sistema** | Cada sistema (corte, física, render) tiene su presupuesto y lo administra como quiere | Dentro de cada sistema se repiten los mismos problemas de vida: qué dura un paso, qué dura un nivel. Cada uno los resuelve por su lado |
| **Por tiempo de vida** | La memoria se agrupa por cuánto dura: permanente (todo el programa), nivel, frame, paso. Todo lo que vive lo mismo se libera junto (Fleury; Gregory) | Un sistema que usa varias vidas no tiene un número propio de memoria, y el presupuesto por sistema del ADR-0020 se pierde |
| **Por sistema y por tiempo de vida** | Cada sistema tiene regiones con nombre, y cada región tiene una sola vida: por ejemplo, "corte: temporales del paso", "corte: pedidos pendientes", "pedazos: geometría" | Más regiones que nombrar y dimensionar. Es justo la tabla de memoria de `presupuestos.md`, con una columna más |
| ***Tagged heap*** (Naughty Dog) | Bloques de 2 MiB que se asignan a una etiqueta ("juego", "render", "GPU") y se liberan todos juntos cuando esa etiqueta termina. Así no se dimensiona cada arena para su peor caso | Resuelve un problema de muchos hilos y muchas arenas: en *The Last of Us Remastered* desperdiciaban de 100 a 200 MiB dimensionando todo para el peor caso. Con un hilo y pocas arenas, el desperdicio es chico |

### 2. De dónde sale la memoria y cuándo se toca

**Linux promete la memoria, pero no la entrega:** `mmap` (y `malloc`, que lo usa) devuelve
direcciones sin páginas detrás. La página se entrega, llena de ceros, la primera vez que
se escribe. Medido: ~0,7 ms por MiB, y una sola página llegó a costar 2,4 ms. Los
temporales de un corte en memoria nueva costaron ~100 µs, contra ~6 µs en memoria ya
tocada (ver Investigación).

| Opción | Cómo funciona | Problema |
|---|---|---|
| **`malloc` a demanda** | Cada cosa pide memoria cuando la necesita, también en el frame | Lo que el ADR-0009 ya prohíbe en el código de cada frame: un costo que depende del estado del heap (hasta 161 µs medidos en las llamadas de un corte) y *page faults* que caen donde caen |
| **Un bloque por región al iniciar, tocado al iniciar** | Cada región pide su bloque (con `mmap`) al arrancar el juego o el nivel, del tamaño de su presupuesto, y lo toca entero (`MAP_POPULATE` o escribir un byte por página) | Cada región es una reserva aparte. La suma total solo se conoce sumando |
| **Una sola reserva para todo, repartida** | Al arrancar, una sola reserva del total de R4, tocada, que se reparte en las regiones con un allocator lineal. Es el estilo de *Handmade Hero* | La reserva grande ocupa toda la RAM del presupuesto aunque el juego use menos. Agrandar una región exige recompilar o una configuración |
| **Reservar direcciones, entregar de a poco** | Se reserva un rango virtual enorme (por ejemplo, 64 GiB, que no ocupa RAM) y cada arena crece dentro de él a medida que se usa (Fleury) | La arena crece sola: se pierde el tope duro, y el primer toque de cada página nueva cae en el frame que crece |

**Una pregunta dentro de esta dimensión: ¿qué se toca al iniciar?** Tocar todos los
presupuestos de la CPU (~400 MiB) costaría de 0,15 a 0,3 s al arrancar. Lo que el frame
usa (arenas, pools, *ring buffer*) es mucho menos que eso.

### 3. La memoria temporal

El ADR-0012 reinicia la arena **al terminar el frame**, pero un frame puede tener hasta 8
pasos. Lo que un paso escribe en la arena (las colas, los temporales del corte) se
acumula hasta el final del frame, aunque nadie lo lea después de su paso.

**Cuánto suma:** los temporales de un corte en un zombie de ~3000 vértices son ~240 KB
(estimación, ver Investigación). Con 32 cortes en un frame, son ~7,7 MB si nadie los
libera antes del final del frame: la mitad de los 16 MiB de la arena.

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Una arena por frame** (como dice el ADR-0012) | Todo lo temporal va a una arena que se reinicia al terminar el frame | Su peor caso se multiplica por 8 (los pasos de un frame después de un tirón). Un dato de un paso puede leerse por error en el paso siguiente, y "funciona" |
| **Una arena por paso y una por frame** | La simulación usa una arena que se reinicia al terminar **cada paso**. El render usa otra, que se reinicia al terminar el frame | Dos arenas que nombrar. Lo que un paso le deja al render (por ejemplo, para interpolar) no puede ir en la arena del paso: va en componentes. Sería una fe de erratas del ADR-0012 |
| **Arenas *scratch* con marcador** | Además de lo anterior, una función que necesita memoria de trabajo guarda la posición de una arena *scratch*, la usa y vuelve a esa posición al salir, como una pila (Fleury). El corte libera sus 240 KB al terminar cada corte | Hay que cuidar que nada de lo que sobrevive a la función quede en la *scratch*. Fleury usa dos *scratch* para cuando una función recibe una arena y también necesita otra temporal |
| **Arena con doble buffer** | Dos arenas por frame que se alternan, para que el frame siguiente pueda leer lo del anterior | Solo hace falta si la GPU o el frame siguiente leen esa memoria. Aquí, lo que lee la GPU va por el *ring buffer* (ADR-0005), no por la arena |

### 4. Objetos de tamaño fijo

Pedidos de corte pendientes (20, ADR-0007), cuerpos físicos, ragdolls (con su tope,
ADR-0016), entidades del ECS y los metadatos de cada pedazo: todos tienen tamaño fijo,
un tope y una vida que no sigue al frame.

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Pool con lista libre** | Un arreglo de N slots. Los libres se encadenan dentro de sí mismos: pedir y devolver es O(1), sin fragmentación | Nada impide usar un puntero a un slot después de devolverlo |
| **Pool con handles con generación** (ADR-0009) | Lo mismo, y cada slot guarda una generación que sube al devolverlo. El handle lleva índice y generación: uno viejo se detecta al usarlo | 4 bytes más por slot y una comparación en cada acceso. Es lo que el ADR-0009 ya eligió para lo que vive en pools |
| **Arreglo denso con *swap and pop*** | Los vivos quedan juntos al principio: se recorren sin saltar huecos. Al borrar, el último ocupa el lugar del borrado | Mover elementos cambia sus índices: hace falta una tabla de handles aparte. Es lo que ya hace un *sparse set* del ECS |

### 5. Datos de tamaño variable: los pedazos

Cada pedazo tiene sus índices y sus vértices nuevos (ADR-0007), y su tamaño cambia con
cada corte. El pool vive en la GPU (*device-local* en una GPU dedicada, o el mismo heap
en la integrada) y su contabilidad, en la CPU. Simulado con un millón de cortes (ver
Investigación):

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Slots del tamaño máximo** | Cada pedazo ocupa un slot fijo, como un pool | Un pedazo que se vuelve a cortar acumula vértices nuevos y puede no caber (2 fallos en un millón con 64 KiB). Lo reservado en el pico: 6,75 MiB |
| **Buddy** (potencias de 2) | Bloques que se parten en mitades y se vuelven a unir con su compañero | Desperdicia ~40 % al redondear (4,03 MiB en el pico), y el pool tiene que ser una potencia de 2 |
| **TLSF** (*Two-Level Segregated Fit*, Masmano y otros) | Listas de bloques libres por rango de tamaño, en dos niveles, con mapas de bits: pedir y devolver son O(1), y los bloques vecinos libres se unen. Es el que usa el *OffsetAllocator* de Aaltonen para la GPU, que solo maneja offsets | Más código y más casos que los otros dos. Fragmentación externa chica: en la simulación, un allocator de buen ajuste (lo que TLSF aproxima) no falló con un 23 % de margen sobre lo vivo (2,85 MiB en el pico) |
| **Compactar** | Cualquier allocator, y cuando se fragmenta, mover los pedazos para juntar el espacio libre | Mover datos de la GPU es un pico de trabajo y de sincronización. Los handles lo permitirían (ADR-0009), pero la simulación muestra que no hace falta |

**Un allocator de offsets separa la contabilidad de la memoria:** solo devuelve números
(dónde empieza cada bloque), sin tocar lo que administra. Así, el mismo código sirve
para un bloque de la CPU y para un `VkBuffer` de la GPU.

### 6. La memoria de la GPU

**Cómo subasigna `gpu::Allocator`:**

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Una asignación de Vulkan por recurso** | Cada buffer o textura tiene su `vkAllocateMemory` | La especificación solo garantiza 4096 asignaciones (`maxMemoryAllocationCount`), y cada una es cara. El ADR-0005 ya descartó esto |
| **Bloques grandes por tiempo de vida** | Un bloque por vida, y en cada uno el allocator que le corresponde: lineal para lo que vive un nivel (mallas y texturas), el *ring buffer* para lo que vive un frame, y un allocator de offsets para los pedazos | Hay que respetar la alineación de cada recurso y, entre buffers y texturas *optimal* en un mismo bloque, `bufferImageGranularity` |
| **VMA** | La biblioteca de AMD hace lo anterior, con más casos | El ADR-0005 la dejó como plan B, solo si lo propio bloquea el avance |

**Cuándo se libera lo que la GPU puede estar leyendo:**

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Cola de liberación por número de frame** | Liberar un pedazo lo anota con el número del frame actual. Cuando la *fence* de ese frame indica que la GPU terminó (con 2 frames en vuelo, dos frames después), se libera de verdad. Es el `HasFrameCompleted(frame)` de Naughty Dog | El pool necesita margen para lo que espera liberarse |
| **Esperar a la GPU** | Al liberar, esperar a que la GPU termine | Una pausa en el frame: lo contrario del criterio 2 |

**El tamaño del *ring buffer*:** el ADR-0005 lo dimensiona para el peor caso. La cuenta
está en Investigación: ~2,3 MB por frame, contra la hipótesis de 8 MiB.

### 7. Cómo reciben memoria los contenedores

| Opción | Cómo funciona | Problema |
|---|---|---|
| **`std::pmr`** | Contenedores estándar (`std::pmr::vector`) que piden memoria a un `memory_resource`, que puede ser una arena (`monotonic_buffer_resource`) o un pool | Cada pedido es una llamada virtual, y el ADR-0009 reserva `virtual` para las fronteras entre módulos. El vector sigue creciendo: en una arena, cada crecimiento abandona el buffer anterior. Si la arena se acaba, pide al recurso de respaldo (por defecto, el heap) sin avisar, o lanza `bad_alloc`, que sin excepciones aborta |
| **Contenedores propios de capacidad fija** | `fixed_vector<T>` y similares reciben su capacidad y su memoria al crearse, de una arena o de un pool, y nunca crecen. La arena se pasa como parámetro explícito (`arena&`) a la función que la usa | Hay que escribirlos (el ADR-0009 ya los pidió). Hay que saber la capacidad al crearlos, lo que obliga a pensar el peor caso |
| **Contenedores con el allocator como parámetro de template** | Al estilo de EASTL: `vector<T, Alloc>` | Cada allocator cambia el tipo: un `vector<int, arena_alloc>` no se puede pasar donde se espera un `vector<int, pool_alloc>`. Más templates, y el ADR-0009 pide templates simples |

### 8. Cuando la memoria se acaba

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Siempre abortar** | Quedarse sin memoria es un bug de dimensionamiento: el engine se detiene con el nombre de la región y su pico | Lo que el juego llena con sus reglas (pedazos, pedidos) también abortaría, aunque tenga una salida natural |
| **Siempre devolver un error** | Cada pedido devuelve un `result` (ADR-0009), y quien lo pide decide | Cada llamada a una arena tendría su `if`. La mayoría de esos `if` no tienen nada razonable que hacer |
| **Según el tipo de memoria** | Las arenas y las tablas del engine abortan con un mensaje claro: su tamaño es un contrato. Los pools que llena el juego (pedazos, pedidos de corte, ragdolls) tienen una regla de degradación, y el juego **pregunta antes** si hay lugar, como ya hace el combate con los cortes (ADR-0007) | Dos comportamientos que distinguir. Es la regla 6 del ADR-0020 (límites duros y degradación diseñada) aplicada a la memoria |

**La degradación de cada pool lleno** (si se elige la tercera):

| Pool | Regla posible | Ya decidido en |
|---|---|---|
| Pedidos de corte | El combate no emite el pedido: el zombie solo recibe el empuje | ADR-0007 |
| Pedazos | El más viejo se descarta (con la salpicadura del ADR-0025) | ADR-0020 |
| Ragdolls | El más viejo se duerme o se congela | ADR-0016 |

### 9. Detección de errores

Medido: **ASan no ve nada dentro de una arena o de un pool propios.** Para ASan, la arena
es un solo bloque válido. Leer un dato del frame anterior, escribir un byte de más o
usar un slot ya devuelto pasan sin aviso. Con envenenamiento manual, los tres errores se
detectan (ver Investigación).

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Solo ASan, como está** | ASan revisa lo que viene de `malloc` | No ve los errores de los allocators propios, que son casi toda la memoria del frame |
| **Envenenamiento manual en Debug** | Las arenas y los pools llaman a `ASAN_POISON_MEMORY_REGION` al reiniciar o devolver, y a `ASAN_UNPOISON_MEMORY_REGION` al entregar, con zonas rojas entre asignaciones. Fuera de ASan, las macros no generan código | Solo en Debug. ASan trabaja de a 8 bytes, así que las asignaciones y las zonas rojas se alinean a 8 |
| **Lo anterior, más revisiones propias en todos los builds de desarrollo** | Además de ASan: llenar con un patrón la memoria que se reinicia o se devuelve (por ejemplo, `0xDD`), guardas al final de cada asignación que se revisan al reiniciar, la generación de los handles (ADR-0009), el pico de cada región y, al cerrar, la comprobación de que cada pool quedó vacío | Un costo en RelWithDebInfo. Release no lo lleva. Valgrind también se puede avisar de los allocators propios (*client requests*), pero el ADR-0006 no lo usa |

**El patrón también ayuda al determinismo (ADR-0016):** una arena reutilizada devuelve lo
que dejó el frame anterior. Si alguien lo lee sin inicializarlo, el resultado depende de
la historia, y una partida grabada deja de reproducirse. Con un patrón fijo, ese error
se vuelve visible y repetible.

### 10. La capacidad del ECS

Un *sparse set* tiene un arreglo disperso indexado por entidad, y un arreglo denso con
los componentes (ADR-0012).

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Capacidad fija, reservada al iniciar** | Un máximo de entidades (por ejemplo, 4096) y, por tipo de componente, su arreglo disperso y su arreglo denso del tamaño máximo | Memoria reservada aunque no se use. Con 4096 entidades y 32 tipos de componente, los arreglos dispersos suman 512 KiB: poco, frente a los 64 MiB de "mundo y ECS" |
| **Paginada** | El arreglo disperso se divide en páginas que se piden al usarlas, como EnTT | Pedir una página es asignar memoria en el frame (al crear un pedazo): necesita un pool de páginas con su tope, que es otra forma de capacidad fija |
| **Que crezca** | Arreglos que se agrandan al llenarse | Lo que el ADR-0009 prohíbe en el código de cada frame |

## Investigación

### Prototipo medido

Tres programas en C++20, medidos en la máquina de referencia (ADR-0020) el 2026-10-21,
fuera del protocolo. El detalle y cómo repetirlos están en
[`../research/memoria.md`](../research/memoria.md).

| Qué | Resultado |
|---|---|
| Llamadas a `malloc`/`free` de un corte (~240 KB en 16 bloques, con un heap ocupado por el resto del programa) | p50 1,8 µs, p99 37 µs, máximo 161 µs |
| Las mismas llamadas a una arena | p50 0,04 µs, p99 0,17 µs |
| Escribir los temporales del corte en memoria ya tocada | p50 5,9 µs (arena) a 9,3 µs (`malloc`) |
| **Escribirlos en memoria nueva** (arena fría) | **p50 103 µs, p99 318 µs** |
| Primer toque de memoria nueva | ~0,7 ms por MiB; una sola página llegó a 2,4 ms |
| `MAP_POPULATE` | Adelanta el costo a la reserva (~0,4 ms por MiB) y después el primer toque es casi gratis |
| `mlock` | Limitado a 8 MiB para el usuario: no sirve para fijar el presupuesto |
| ASan sin envenenamiento: uso después del reinicio de una arena, desborde dentro de la arena, uso de un slot devuelto | **Ninguno detectado** |
| ASan con envenenamiento manual | Los tres detectados (`use-after-poison`) |
| Pedazos vivos en el pico (100 pedazos, un millón de cortes) | ~2,85 MiB. El pedazo más grande: ~66 KB |
| Fragmentación | Slots de 64 KiB: fallan pedazos re-cortados. Buddy: 4,03 MiB reservados en el pico. *Good fit* (como TLSF): 0 fallos con 3,5 MiB |

Lo que mostró, además de los números:

- **El costo del corte no está en el allocator, está en la memoria nueva.** Pedir a una
  arena es gratis, y `malloc` cuesta poco en la mediana. Lo que pesa es tocar páginas por
  primera vez: ~100 µs por corte, el 5 % del presupuesto de "combate y corte" por cada
  corte. Una arena que nunca se tocó antes del primer corte lo paga justo ahí.
- **Los presupuestos de memoria de los pedazos y de la subida son hipótesis holgadas**
  (ver Cuentas). Son hipótesis del ADR-0020, y se corrigen con su protocolo, no aquí.

### Cuentas

Con los datos del ADR-0007, el ADR-0017, el ADR-0020 y el prototipo:

| Qué | Cuenta | Tamaño |
|---|---|---|
| Temporales de un corte | Estimación para ~3000 vértices y ~4800 triángulos | ~240 KB |
| Temporales de 32 cortes, si duran hasta el final del frame | 32 × 240 KB | ~7,7 MB |
| Subida de un corte | Índices de los pedazos (~4800 triángulos × 12 B) + vértices nuevos (hasta 200 × 32 B) + triángulos nuevos | ~70 KB |
| *Ring buffer*, por frame | 32 cortes × 70 KB + matrices de los zombies (74 KB) + 10 ragdolls de 23 huesos × 64 B (15 KB) + 100 pedazos rígidos × 64 B (6 KB) | **~2,3 MB**, contra la hipótesis de 8 MiB |
| Si un pedazo se dibuja con dos llamadas (ADR-0022) | Más las copias de los vértices del borde: ~20 KB por corte | +0,6 MB |
| Pool de pedazos | Pico vivo (2,85 MiB) + lo que espera liberarse en un pico de 32 cortes por frame, durante 2 frames (2 × 2,2 MB) + margen de fragmentación (~25 %) | **~9 MiB**, contra la hipótesis de 32 MiB |
| Pedidos de corte pendientes | 20 × (pose de 65 huesos × 64 B) | ~83 KB (~29 KB con el rig recortado a 23 huesos) |
| Arreglos dispersos del ECS | 4096 entidades × 4 B × 32 tipos de componente | 512 KiB |
| Tocar al iniciar lo que usa el frame | Arenas (16 MiB) + pedazos (~9 a 32 MiB) + *ring buffer* (2 × 8 MiB) + pools chicos, a 0,4–0,7 ms por MiB | De ~15 a ~45 ms |

Los temporales de un corte son una estimación (el prototipo del ADR-0007 no midió su
memoria), y los índices se contaron en `uint32`. Con índices de 16 bits, la subida y el
pool de pedazos bajan casi a la mitad.

### Cómo lo hacen otros

- **Jason Gregory** (*Game Engine Architecture*, sección de gestión de memoria) describe
  las piezas clásicas: el allocator de pila con marcador, el de doble pila, el pool de
  bloques de tamaño fijo, el allocator por frame y el de doble buffer. Su argumento: el
  allocator general es lento por ser general, y un engine conoce sus patrones de uso.
- **Naughty Dog** (Christian Gyrling, GDC 2015): para *The Last of Us Remastered* a 60 fps
  usaban allocators lineales que se reinician por frame. Con varios frames en proceso a
  la vez (juego, render y GPU), cada programador resolvía por su lado "cuándo libero
  esto". Su respuesta: un número de frame que viaja con los datos y una consulta
  `HasFrameCompleted(frame)`. Además, el *tagged heap*: bloques de 2 MiB (una página
  grande de PS4, una entrada del TLB) asignados a una etiqueta y liberados juntos.
  Medían de 100 a 200 MiB desperdiciados al dimensionar cada allocator lineal para su
  peor caso.
- **Ryan Fleury** (*Untangling Lifetimes: The Arena Allocator*, 2022): agrupar las
  asignaciones por tiempo de vida en arenas simplifica todo lo demás. Propone arenas
  *scratch* con marcador (guardar la posición, usarla, volver), y dos *scratch* por hilo
  para cuando una función recibe una arena y necesita otra temporal. También reservar un
  rango virtual enorme y entregar páginas a medida que la arena crece.
- **Niklas Frykholm** (Bitsquid, *Custom memory allocation in C++*, 2010): toda la
  memoria del engine pasa por allocators explícitos, que se pasan como parámetro y
  llevan la cuenta de lo que asignaron. Al cerrar, cada uno verifica que no le quedó nada
  asignado.
- **John Lakos** (*Local ("Arena") Memory Allocators*, CppCon 2017): con allocators
  locales, un sistema que corre mucho tiempo no se degrada como con un solo allocator
  global, y en sus mediciones llegan a ser un orden de magnitud más rápidos. Es la
  motivación de `std::pmr`.
- **EASTL** (Electronic Arts, N2271, 2007), citado en el ADR-0009: los contenedores
  estándar de la época hacían difícil controlar de dónde salía la memoria. EASTL los
  rehízo con allocators más simples.

### Allocators de tamaño variable

- **TLSF** (Masmano, Ripoll, Crespo y Real, ECRTS 2004): un allocator de "buen ajuste"
  para sistemas de tiempo real. Pedir y devolver cuestan O(1) (menos de 200
  instrucciones en x86, según el paper), con poca fragmentación. Usa dos niveles de
  listas segregadas por tamaño y mapas de bits que se recorren con instrucciones de
  "contar ceros".
- **El *OffsetAllocator* de Sebastian Aaltonen** (MIT) aplica la idea de TLSF a offsets,
  para subasignar memoria de la GPU: 256 rangos de tamaño con una distribución en coma
  flotante de 8 bits y dos niveles de mapas de bits. Como no toca la memoria que
  administra, sirve para cualquier rango.
- **Buddy** (Knuth, *The Art of Computer Programming*, vol. 1): simple y sin
  fragmentación externa entre compañeros, pero redondea cada pedido a una potencia de 2.

### Linux y la memoria

- **Overcommit:** con `vm.overcommit_memory = 0`, `mmap` casi nunca falla, aunque no haya
  RAM. Si falta, el problema aparece al tocar la memoria, y lo resuelve el *OOM killer*.
  Tocar la memoria al iniciar adelanta ese problema al arranque.
- **`MAP_POPULATE`** pide al kernel que entregue las páginas al reservar.
- **`mlock`** fija las páginas en RAM, pero está limitado por `RLIMIT_MEMLOCK` (8 MiB en
  esta máquina).

### ASan y los allocators propios

- La documentación de ASan (*AddressSanitizerManualPoisoning*) describe la API para los
  allocators propios: envenenar la arena entera al crearla, desenvenenar cada asignación
  al entregarla y dejar zonas rojas entre asignaciones. ASan trabaja de a 8 bytes: un
  rango mal alineado deja huecos sin revisar.
- **Valgrind (Memcheck)** tiene *client requests* para lo mismo
  (`VALGRIND_MALLOCLIKE_BLOCK`, `VALGRIND_CREATE_MEMPOOL`).

### `std::pmr`

- `std::pmr::memory_resource` (C++17) es una interfaz con funciones virtuales
  (`do_allocate`, `do_deallocate`). `monotonic_buffer_resource` es una arena: libera todo
  junto. Cuando se acaba, pide al recurso de respaldo (por defecto, el heap). Con
  `null_memory_resource` de respaldo, lanza `bad_alloc`, que con `-fno-exceptions`
  aborta.

## Decisión

Elijo reservar al iniciar toda la memoria que usa el frame, repartida en regiones con un
dueño y una vida, y tocarla entera antes de jugar. El prototipo me mostró que lo caro no es pedir
memoria, sino tocarla por primera vez: un corte en memoria nueva cuesta ~100 µs, contra
~6 µs en memoria ya tocada. Así, en el frame nunca se le pide nada al sistema operativo,
y si falta memoria, el juego falla al arrancar y no en medio de una horda.

Lo temporal vive en arenas que se reinician solas (por paso, por frame y *scratch* para
cada corte), lo de tamaño fijo en pools con handles, y los pedazos, que cambian de
tamaño con cada corte, en un allocator tipo TLSF que también sirve para la GPU. Escribo
mis propios allocators y contenedores porque quiero entender y controlar cada byte, y
como ASan no ve dentro de ellos, los hago avisarle a ASan de lo que liberan.

| # | Dimensión | Elección |
|---|---|---|
| 1 | Organización | **Por sistema y por tiempo de vida.** Cada región tiene un sistema dueño y una sola vida (permanente, paso, frame, o la de lo que guarda), y cae en una fila de `presupuestos.md` |
| 2 | De dónde sale y cuándo se toca | **Cada región que usa el frame** (arenas, pools, pedazos, *ring buffer*) **se reserva al iniciar y se toca entera en ese momento.** En la CPU, con `mmap` y `MAP_POPULATE`, a través de `platform::`. En la GPU, con los bloques grandes de `gpu::Allocator` (dimensión 6). En el frame no se pide memoria al sistema operativo ni al driver. La memoria que vive un nivel la decide el ADR-0028 |
| 3 | Memoria temporal | **Una arena por paso de simulación**, que se reinicia al terminar cada paso; **una arena por frame para el render**, que se reinicia al terminar el frame; y **dos arenas *scratch* con marcador** para el trabajo interno de una función, como los temporales de cada corte |
| 4 | Objetos de tamaño fijo | **Pools con lista libre y handles con generación** (ADR-0009) |
| 5 | Pedazos | **Un allocator de offsets propio, tipo TLSF:** O(1), une los bloques vecinos libres y no toca la memoria que administra |
| 6 | Memoria de la GPU | **Bloques grandes por tiempo de vida** (lineal para lo que vive un nivel, el *ring buffer* para lo que vive un frame, el allocator de offsets para los pedazos) y una **cola de liberación por número de frame**. El *ring buffer* se dimensiona para el peor caso (ADR-0005) |
| 7 | Contenedores | **Propios, de capacidad fija, sin `std::pmr`.** Reciben su memoria al crearse y nunca crecen. La arena se pasa como parámetro explícito |
| 8 | Cuando se acaba | **Según el tipo de memoria.** Las arenas y las tablas del engine abortan con el nombre de la región y su pico. Los pools que llena el juego degradan con su regla, y el juego pregunta antes si hay lugar |
| 9 | Detección de errores | **En Debug: envenenamiento manual para ASan, patrones de relleno, guardas y la comprobación al cerrar.** RelWithDebInfo y Release no los llevan, para que las mediciones del ADR-0020 no paguen su costo. En todos los builds: la generación de los handles y el uso y el pico de cada región |
| 10 | Capacidad del ECS | **Fija, reservada al iniciar** |

## Consecuencias

- **El mapa de la memoria.** Cada región cae en una fila de `presupuestos.md`:

  | Región | Dueño | Vida | Allocator |
  |---|---|---|---|
  | Arena del paso | La simulación | Un paso | Lineal |
  | Arena del frame | El render | Un frame | Lineal |
  | Dos arenas *scratch* | Quien las usa | Una llamada | Lineal con marcador |
  | Pedidos de corte pendientes | El corte | Hasta que se procesan | Pool de 20 (ADR-0007) |
  | Metadatos de los pedazos | El corte | La del pedazo | Pool con handles |
  | Geometría de los pedazos (CPU y GPU) | El corte y `engine/gpu` | La del pedazo, más los frames en vuelo | Offsets, tipo TLSF |
  | Cuerpos y ragdolls | La física | La del cuerpo | Pools con handles, con el tope del ADR-0016 |
  | ECS | `engine/core` | Todo el programa | Capacidad fija |
  | *Ring buffer* | `engine/gpu` | Un frame en vuelo | Anillo |
  | Recursos de la GPU que viven un nivel | `engine/gpu` | Un nivel | Lineal, por bloques. Cómo se cargan lo decide el ADR-0028 |

- **El frame,** con las arenas en su lugar. Es una fe de erratas del ADR-0012, que
  reiniciaba una sola arena al terminar el frame:

  ```
  1. platform: vaciar la cola de eventos → estado de input
  2. mientras el acumulador ≥ paso fijo:
       simulación: gameplay → combate → corte → física → animación
       reiniciar la arena del paso
  3. render: interpolar → grabar comandos → enviar a la GPU
  4. fin de frame: reiniciar la arena del frame;
                   liberar lo que la GPU ya terminó de usar (cola por número de frame)
  ```

- **Las colas de eventos entre sistemas** (`slice_request` y las demás) viven en la arena
  del paso, porque se escriben y se vacían dentro del mismo paso. Lo que un paso le deja
  al render va en componentes, como el estado anterior que el ADR-0012 ya guarda para
  interpolar. **Fe de erratas del ADR-0012** (Decisión 4, el orden del frame y
  Consecuencias).
- **Los temporales de cada corte** van a una *scratch*: el corte guarda su marcador, usa
  ~240 KB y vuelve al marcador al terminar. El pico ya no depende de cuántos cortes hay
  en un frame. Las dos *scratch* son para la función que recibe una arena y además
  necesita otra temporal: usa la que no le pasaron (Fleury).
- **Al iniciar se toca lo que usa el frame:** las arenas, los pools, el pool de pedazos y
  el *ring buffer*. Medido: de 0,4 a 0,7 ms por MiB, del orden de 15 a 45 ms en total. Si falta memoria,
  falla al arrancar y no a mitad de una horda. La memoria que vive un nivel la decide el
  ADR-0028.
- **Reservar y tocar memoria es una función de `platform::`** (`mmap` y `MAP_POPULATE` en
  Linux; en Windows sería `VirtualAlloc`). `engine/core` la usa, como permiten las capas
  del ADR-0012, y no llama al sistema operativo directamente.
- **Liberar un pedazo** lo anota en una cola con el número del frame actual. Cuando la
  *fence* de ese frame indica que la GPU terminó (con 2 frames en vuelo, dos frames
  después), el allocator de offsets libera su lugar de verdad. El pool tiene margen para
  lo que espera liberarse (ver Cuentas).
- **Cuándo se descartan pedazos lo decide la simulación, no la GPU.** La cuenta es lógica:
  la cantidad de pedazos vivos (100, R5) y sus bytes, contra un tope igual al pool menos
  el margen para lo que espera liberarse (2 frames × 32 cortes, ver Cuentas). Si un corte
  la pasa, el corte descarta los más viejos (ADR-0020); la salpicadura que lo disimula es
  del ADR-0025. Lo que libera la GPU (las *fences*) depende del tiempo real, así que no
  entra en esa cuenta: si entrara, una partida grabada podría descartar otros pedazos
  (ADR-0007 y ADR-0016). Con el margen garantizado, el allocator de offsets no falla
  nunca. Si falla, es un error de dimensionamiento: aborta con el nombre de la región,
  como una arena.
- **La degradación de cada pool lleno:** los pedidos de corte, la del ADR-0007 (el combate
  no emite el pedido); los pedazos, descartar el más viejo (ADR-0020); los ragdolls, la
  del ADR-0016 (el más viejo se duerme o se congela).
- **Dos excepciones a la regla de abortar** (fe de erratas, ADR-0021), las dos solo en
  los builds de desarrollo: el dibujo de depuración descarta lo que no cabe y muestra en
  pantalla cuánto descartó, y Tracy con un servidor conectado pide memoria mientras corre.
  Por eso una medición del protocolo nunca se hace con el servidor conectado.
- **Lo que vive en `engine/core`:** la arena (con alineación por pedido, marcador y
  reinicio), las *scratch*, el pool con `handle<T>` (ADR-0009), el allocator de offsets y
  los contenedores de capacidad fija (`fixed_vector<T>` y los que hagan falta).
  `gpu::Allocator`, en `engine/gpu`, usa el mismo allocator de offsets sobre sus bloques
  de Vulkan. Ningún allocator se comparte entre hilos (ADR-0012): cuando haya más de un
  hilo, cada uno tendrá sus *scratch*.
- **Las revisiones de desarrollo:**
  - solo en Debug: cada arena y cada pool envenena lo que reinicia o devuelve, con zonas
    rojas entre asignaciones (alineadas a 8 bytes como mínimo, por ASan; fuera de ASan,
    las macros no generan código). Lo que se reinicia o se devuelve se llena con un
    patrón fijo, cada asignación de una arena lleva una guarda que se revisa al
    reiniciar, y al cerrar el programa cada pool tiene que estar vacío;
  - RelWithDebInfo no las lleva: es el build que mide el protocolo del ADR-0020, y tiene
    que costar lo mismo que Release, salvo las herramientas del ADR-0021 (de costo
    despreciable y medido) y su región de depuración, que cuenta en R4;
  - en todos los builds: el uso y el pico de cada región (los necesita el mensaje al
    abortar, y sirven para medir), la generación de los handles (ADR-0009) y abortar al
    quedarse sin memoria, que es el contrato de cada región.
- **Determinismo (ADR-0016):** las arenas no limpian la memoria al entregarla. Quien la
  pide la inicializa, y el patrón de desarrollo hace visible al que no lo hace. Ningún
  sistema recorre nada por dirección ni por offset: los offsets del allocator de los
  pedazos dependen de la historia de la partida.
- **Los contenedores no crecen:** su capacidad se fija al crearlos, con el peor caso a la
  vista. `std::vector` sigue permitido fuera del código de cada frame (ADR-0009).
- **Los tamaños siguen siendo hipótesis** (ADR-0020). Las cuentas sugieren que el *ring
  buffer* necesita ~2,3 MB por frame y el pool de pedazos ~9 MiB, contra 8 MiB y 32 MiB
  en `presupuestos.md`. Se corrigen con una medición hecha con el protocolo, no en este
  ADR. Los 16 MiB de la arena por frame pasan a cubrir la arena del paso, la del frame y
  las *scratch*. Cómo se reparten se fija midiendo.
- **Tests (ADR-0011, TDD estricto):**
  - la arena: alineación, marcador, reinicio, y que quedarse sin espacio aborta (*death
    test*);
  - el pool: reutiliza los slots libres, y un handle viejo se detecta por su generación;
  - el allocator de offsets: pedir y devolver, unir con el vecino de cada lado, fallar
    cuando no hay un bloque que alcance, y una prueba de propiedades con semilla fija:
    ningún bloque se superpone con otro, y lo libre más lo usado suma el total;
  - la cola de liberación: nada se libera antes de que termine su frame;
  - el *ring buffer*: da la vuelta sin pisar una región en vuelo;
  - en Debug con ASan, un *death test* por cada error de la Investigación: leer después
    de reiniciar, escribir pasado el final y usar un slot devuelto.
- **Lo que se sacrifica:**
  - memoria ocupada aunque no se use: el pico de R4 incluye las regiones enteras;
  - flexibilidad: una región que se queda chica se agranda cambiando su tamaño, no
    crece sola;
  - simplicidad: más código que `malloc` (el allocator de offsets, el envenenamiento, la
    cola de liberación);
  - un dato de un paso no se puede leer en el paso siguiente si no se guardó en un
    componente;
  - `std::pmr`, que es el estándar.
- **Abierto:**
  - los tamaños de cada región, con el protocolo del ADR-0020;
  - la fragmentación del allocator de offsets real: la simulación usó un *good fit*, que
    TLSF aproxima pero no iguala (busca por clases de tamaño). Se mide cuando exista;
  - índices de 16 o de 32 bits en los pedazos (ticket del cocinador y ADR-0022). Con 16
    bits, la subida y el pool bajan casi a la mitad;
  - la memoria que vive un nivel y la carga de los assets (ADR-0028);
  - cómo se ven los picos y el uso de cada región durante el juego (decidido en el
    ADR-0021: contadores en todos los builds y un panel en los de desarrollo);
  - el máximo de entidades del ECS y los valores de los patrones (sus tickets);
  - el swap: sin `mlock`, el sistema operativo puede sacar páginas de la RAM. Con el
    presupuesto de 1 GiB en una máquina de 8 GB, se acepta.

## Cómo lo defiendo

**¿Por qué no usar `malloc`, si en la mediana un corte gasta solo 2 µs en sus llamadas?**
Porque lo que importa es el peor caso, no la mediana. Las mismas llamadas llegaron a 161 µs,
según cómo había dejado el heap el resto del programa. Con una arena, pedir memoria es
sumar un número, y siempre cuesta lo mismo.

**Reservas al iniciar la memoria que usa el frame. ¿Por qué además la tocas entera, si nadie la está
usando todavía?**
Porque Linux solo promete la memoria: la entrega la primera vez que se escribe, y eso
cuesta ~0,7 ms por MiB. Si no la toco al iniciar, ese costo cae en el primer corte, en
medio del juego. Tocarla al arrancar cuesta unas decenas de milisegundos, una sola vez.

**¿Por qué una arena por paso, además de la del frame?**
Porque un frame puede tener hasta 8 pasos. Si todo esperara al final del frame, la arena
tendría que aguantar 8 veces lo de un paso. Además, así un dato de un paso no se puede
leer por error en el siguiente. Lo que tiene que durar más se guarda en un componente.

**Descartas un pedazo que la GPU todavía puede estar dibujando. ¿Qué pasa con su
memoria?**
No se libera en ese momento. Se anota con el número del frame, y recién cuando la GPU
avisa que terminó ese frame, su lugar vuelve a estar libre. Con 2 frames en vuelo, son
dos frames de espera, y el pool tiene margen para eso.

**¿Por qué un allocator tipo TLSF para los pedazos, y no slots fijos o buddy?**
Porque los pedazos cambian de tamaño con cada corte. Con slots fijos, un pedazo que se
vuelve a cortar puede dejar de caber, y buddy desperdicia cerca de un 40 % al redondear a
potencias de 2. En la simulación de un millón de cortes, un allocator de buen ajuste, que
es lo que TLSF aproxima, no falló con un 23 % de margen. Además, TLSF pide y libera en
tiempo constante. Su fragmentación real la voy a medir cuando exista.

**Usas ASan. ¿Cómo encuentra un error dentro de tu propia arena?**
Solo, no puede: para ASan, la arena es un único bloque válido. Lo comprobé: sin ayuda no
detectó ninguno de los tres errores típicos. Por eso la arena y los pools le avisan a ASan
qué partes están libres (las "envenenan"), y así sí los detecta.

**¿Por qué no `std::pmr`, si es parte del estándar?**
Porque cada pedido es una llamada virtual, los vectores siguen creciendo y, si la arena se
acaba, pide memoria al heap sin avisar. Yo quiero contenedores que no crezcan y que fallen
de forma visible si el tamaño estaba mal pensado.

**¿Qué pasa si el juego se queda sin memoria en medio de una horda?**
Depende de qué memoria se acaba. Si es una arena, es un error mío de dimensionamiento, y el
juego se detiene diciendo qué región se llenó. Si es algo que llena el juego, tiene una
regla: el pedazo más viejo desaparece, el ragdoll más viejo se duerme, o el tajo solo
empuja. El jugador no ve un error.

## Referencias

- Jason Gregory, *Game Engine Architecture*, 4.ª ed., CRC Press, capítulo de los sistemas
  de soporte del engine, sección de gestión de memoria.
- Christian Gyrling, *Parallelizing the Naughty Dog Engine Using Fibers*, GDC 2015:
  <https://www.gdcvault.com/play/1022186/Parallelizing-the-Naughty-Dog-Engine>
- Ryan Fleury, *Untangling Lifetimes: The Arena Allocator* (2022):
  <https://www.dgtlgrove.com/p/untangling-lifetimes-the-arena-allocator>
- Niklas Frykholm, *Custom memory allocation in C++*, Bitsquid (2010):
  <https://bitsquid.blogspot.com/2010/09/custom-memory-allocation-in-c.html>
- John Lakos, *Local ("Arena") Memory Allocators*, CppCon 2017.
- Electronic Arts, *EASTL*, N2271 (2007):
  <https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2271.html>
- Miguel Masmano, Ismael Ripoll, Alfons Crespo y Jorge Real, *TLSF: a New Dynamic
  Memory Allocator for Real-Time Systems*, ECRTS 2004, pp. 79-88,
  doi:10.1109/EMRTS.2004.1311009.
- Sebastian Aaltonen, *OffsetAllocator* (MIT): <https://github.com/sebbbi/OffsetAllocator>
- Donald E. Knuth, *The Art of Computer Programming*, vol. 1, sección 2.5 (asignación
  dinámica de memoria y el sistema buddy).
- Google, *AddressSanitizerManualPoisoning*:
  <https://github.com/google/sanitizers/wiki/AddressSanitizerManualPoisoning>
- Valgrind, *Memcheck: Memory Pools*:
  <https://valgrind.org/docs/manual/mc-manual.html#mc-manual.mempools>
- Linux, `mmap(2)` (`MAP_POPULATE`): <https://man7.org/linux/man-pages/man2/mmap.2.html>
- Linux, *Overcommit Accounting*:
  <https://docs.kernel.org/mm/overcommit-accounting.html>
- cppreference, `std::pmr::memory_resource`:
  <https://en.cppreference.com/w/cpp/memory/memory_resource>
- Khronos, especificación de Vulkan, `maxMemoryAllocationCount` y
  `bufferImageGranularity`: <https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html>
- Adam Sawicki, *Vulkan Memory Types on PC and How to Use Them* (2021):
  <https://asawicki.info/news_1740_vulkan_memory_types_on_pc_and_how_to_use_them>
- EnTT (*sparse sets* paginados): <https://github.com/skypjack/entt>
