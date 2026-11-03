# ADR-0027: Niveles y escenas

- **Estado:** Aceptado
- **Fecha:** 2026-11-02
- **Ticket:** #41
- **Reemplaza / Reemplazado por:** —
- **Fe de erratas:** 2026-11-03 (#42): el ADR-0028 decide la carga y el guardado. **Una
  zona está cargada cuando llegó del disco y su subida a la GPU terminó**: la lee un
  hilo de I/O a un slot fijo de la CPU, y el frame la sube al slot de la GPU con un
  presupuesto de 2 MB por frame. Los slots son tantos como el tope de zonas, así que la
  garantía del tope cubre también la memoria. **El checkpoint se guarda en una tarjeta
  de memoria** (un archivo binario de 8 KB con dos bloques, A y B) cada vez que la
  caballera llega a uno; morir y volver no escribe. **El texto del mundo suma el tamaño
  del slot** (CPU y GPU), **el presupuesto del global y la lista de fases** con su
  índice estable (a la que solo se le agregan filas; una fase borrada queda como
  retirada, porque la tarjeta guarda las fases terminadas como bits y el cocinador no
  recuerda las borradas). Cada fila nombra la fase por el id de su zona y un nombre de
  fase, que el texto de la zona le da a cada fase. Todo eso se copia al binario del mundo, junto con el peor caso
  de polígonos de navmesh para A* (ADR-0023). **El botón del panel del mundo** escribe
  el texto del mundo por el hilo de I/O. Se corrige en las Decisiones 3, 9 y 10 y en
  Abierto.

## Contexto y problema

Hay que decidir **cómo se arma un nivel y qué es una escena**: con qué herramienta se
colocan las piezas del escenario, los zombies, las luces y los triggers; en qué formato
queda; cómo entra al ECS; cómo se describe lo que pasa en el nivel (las oleadas, la
horda, el jefe); y cómo se pasa de un nivel a otro y se vuelve a un checkpoint.

El juego es lineal: la caballera atraviesa el castillo y varias zonas a su alrededor, en
orden, hasta la torre donde está la princesa (visión). El corte vertical es una sola
arena de ruinas, tipo coliseo, con tres fases (una oleada, una horda que termina al
matar una cantidad de zombies y la pelea contra el mago) y checkpoints entre las fases.

**La idea del autor** para este ADR:

- **Elegir exactamente dónde aparecen los zombies** (del ADR-0023). Por eso los puntos de
  aparición, las oleadas y el director quedaron aquí y no en la IA.
- **Un mundo caminable y explorable:** que se sienta que la caballera sale de A y camina
  hasta Z, recorriendo un castillo completo. Las zonas se cargan por secciones sin que el
  jugador lo note, y solo se dibuja lo que el jugador ve.

Varios ADRs ya aceptados dejaron preguntas para este:

| ADR | Qué dejó abierto |
|---|---|
| ADR-0012 | Cómo entra una escena al ECS. El módulo `game/` tiene los niveles |
| ADR-0017 | Cómo se arma un nivel con las piezas del escenario (con sus prefijos `UBX_`, `UCP_` y `USP_`) y qué formato tiene una escena |
| ADR-0021 | El editor de niveles. El encabezado del *replay* guarda el escenario o nivel de partida |
| ADR-0022 | Cómo se colocan las luces y los objetos. El frame lleva un arreglo global de hasta 16 antorchas |
| ADR-0023 | Dónde y cuándo aparecen los zombies (oleadas, director, puntos de aparición) y dónde aparecen los invocados del mago. Que el cocinador vea la colisión del nivel armado, dónde queda la navmesh dentro de lo cocinado, el radio de inflado como parámetro del nivel y las alturas del castillo |
| ADR-0024 | Qué música tiene cada nivel y dónde se colocan los sonidos de ambiente |
| ADR-0025 | Dónde se colocan las antorchas y las fuentes de ambiente |
| ADR-0026 | Los niveles, los checkpoints y qué pantalla lleva a cuál nivel (junto con el ADR-0028) |

Hay que decidir:

1. **la herramienta para armar el nivel;**
2. **la unidad de contenido:** un archivo por nivel, piezas reutilizables, zonas;
3. **el formato,** el de la fuente y el cocinado;
4. **las entidades y el ECS:** qué describe la escena, los ids y las referencias;
5. **el gameplay del nivel:** spawns, oleadas, fases y el jefe;
6. **los triggers y los eventos;**
7. **las luces y el ambiente;**
8. **la colisión y la navmesh del nivel armado;**
9. **la estructura del mundo y las transiciones,** con qué se reinicia en un checkpoint;
10. **la iteración, la validación, las pruebas, la memoria y la depuración.**

Lo que ya está decidido y limita este ADR:

- **ADR-0009:** sin excepciones ni RTTI. En el código de cada frame no hay contenedores
  que asignen memoria.
- **ADR-0010:** regiones por sistema y por tiempo de vida, una de ellas la del nivel.
- **ADR-0011:** TDD estricto en el núcleo y en el cocinador; tests después en el
  gameplay.
- **ADR-0012:** ECS propio con *sparse sets*, la entidad es un *handle* con generación.
  Un hilo, paso fijo de 60 Hz. Capas: `platform` → `core` → `gpu` → sistemas → `game`.
- **ADR-0016 y ADR-0021:** la misma partida grabada da el mismo hash del estado.
- **ADR-0017:** Blender para modelar, glTF 2.0 como intercambio, leído con cgltf solo en
  el cocinador, y un binario propio con secciones planas cocinado en el build, sin
  compatibilidad hacia atrás. El escenario lleva su colisión con prefijos, y un prefijo
  desconocido falla el build. El cocinador genera la navmesh (ADR-0023).
- **ADR-0020:** 60 fps en la Intel UHD 620. No hay requisito de tiempo de carga.
- **ADR-0021:** Dear ImGui para las herramientas, solo en los builds de desarrollo. El
  dibujo de depuración y las CVars.
- **ADR-0022:** hasta 16 antorchas por frame, con Lambert y atenuación cuadrática hasta
  el radio de cada luz.
- **ADR-0023:** la navmesh crece por partes: una grilla simple para la arena plana, el
  resto de Recast para las alturas del castillo. Recast/Detour es una referencia, no una
  dependencia.
- **ADR-0029:** Blender en la CI, con la versión de parche fijada.

**Fuera de alcance:**

| Qué | Lo decide |
|---|---|
| Cómo se cargan los recursos sin cortar el frame (este ADR exige que caminar no lo corte), cuánto viven, el *hot reload* y qué muestra la pantalla de carga | ADR-0028 |
| Dónde y cómo se guarda un checkpoint | ADR-0028 |
| Quién controla la cámara (si el nivel coloca cámaras por zona, es una consecuencia de ese ADR) | ADR-0031 |
| Las acciones del jugador (interactuar con una puerta o una palanca) | ADR-0032 |
| Las cinemáticas | ADR-0034 |
| El comportamiento de los zombies y del mago (cada cuánto invoca, por ejemplo) | ADR-0023 |
| El algoritmo de la navmesh | ADR-0023 |

## Criterios

Los cuatro del ticket, más cinco que salen de cruzar lo ya decidido:

1. **Un flujo simple para una sola persona.**
2. **Encaje con el pipeline de assets (ADR-0017) y el ECS (ADR-0012).**
3. **Tiempos de carga.**
4. **Iteración rápida:** cuánto pasa desde un cambio hasta verlo en el juego.
5. **Colocar exactamente** dónde están los zombies, los triggers y las luces (la idea del
   autor).
6. **No romper el determinismo:** el mismo nivel da el mismo estado y el mismo hash, sin
   depender del orden en que Blender guarda los objetos (ADR-0016, ADR-0021).
7. **Memoria con presupuesto** (ADR-0010, ADR-0020).
8. **Aprendizaje.**
9. **Estándar de la industria.**

## Opciones

Los números son de un prototipo medido en la máquina de referencia (ver Investigación).
Nada de este prototipo usa la GPU.

### 1. La herramienta para armar el nivel

| Opción | Cómo es | Medido | Problema |
|---|---|---|---|
| **Blender como editor** | Las piezas se colocan como *collection instances*; los zombies, los triggers y los checkpoints, como *empties* con propiedades; las antorchas, como luces | P1: todo llega al glTF (propiedades como `extras`, el volumen de un trigger como la escala, las luces como `KHR_lights_punctual`). Exportar la arena: 0,11 s más ~0,9 s de abrir Blender | Las propiedades se escriben a mano, como texto libre, y no se validan hasta el cocinador. El `.blend` es binario: git solo dice "Binary files differ", y cambia en cada guardado |
| **Blender con un complemento propio** | Lo mismo, más un complemento en Python (en `tools/`) con listas de tipos (zombie, evento), el dibujo de los radios y exportar al guardar | No medido. Exportar sin abrir Blender bajaría el ciclo a ~0,1 s | Código Python atado a la API de Blender. La versión fijada (ADR-0029) controla los cambios de la API |
| **Un editor propio dentro del engine** | Con Dear ImGui (ADR-0021): se coloca, se ve con el renderer del juego y se prueba sin salir | No medido | El más caro: selección, *gizmos*, deshacer y guardar son un proyecto entero. El ADR-0017 ya lo descartó para los modelos |
| **TrenchBroom** (GPLv3) | Editor de niveles maduro para *brushes* de Quake, con entidades definidas en archivos FGD | No medido | Pensado para geometría de planos, no para piezas low poly hechas en Blender. Es un segundo formato de geometría y otra herramienta |
| **Texto a mano** | Las posiciones como números en un archivo | P2: el diff se lee | Colocar a ciegas. El castillo de 32 zonas tiene 608 marcadores |

### 2. La unidad de contenido

| Opción | Cómo es | Medido | Problema |
|---|---|---|---|
| **Todo suelto en un archivo** | Cada muro y cada columna es su propio objeto | — | Corregir una pieza (su colisión, por ejemplo) es corregirla en cada copia |
| **Piezas reutilizables** (*prefabs*) | Una pieza (muro, columna, antorcha) con su malla y su colisión, colocada muchas veces | P1: cada instancia llega como un nodo con hijos que **comparten la malla**: 8 mallas para 157 nodos en la arena, 70 para 5 024 en el castillo de 32 zonas | Los hijos de cada instancia **repiten el nombre** (`muro` ×24): el nombre solo no identifica un objeto |
| **Piezas y zonas** | Lo anterior, y el recorrido lineal partido en zonas, cada una su archivo | P3: el castillo de 32 zonas se cocina en 18 ms | Lo que cruza de una zona a otra (un trigger, una referencia) necesita un nombre que no dependa del archivo |

### 3. El formato

**La fuente:**

| Opción | Cómo es | Problema |
|---|---|---|
| **Todo en el glTF** | Las posiciones y también el gameplay (fases, condiciones) como propiedades de los objetos | Las fases son listas con orden y condiciones. Escribirlas como propiedades sueltas en Blender es incómodo y no se lee en un diff |
| **El glTF para lo que se coloca, un texto para lo que pasa** | P2: el glTF trae dónde (posiciones, volúmenes, luces); un texto por nivel trae cuándo y cuántos, y nombra los marcadores | Dos archivos que tienen que coincidir. El cocinador lo valida (P3) |
| **Leer el `.blend`** | Sin exportar | El ADR-0017 eligió glTF. El formato del `.blend` es interno de Blender |

**Lo cocinado:**

| Opción | Cómo es | Medido | Problema |
|---|---|---|---|
| **Secciones del binario del ADR-0017** | Instancias, colisión, luces, marcadores, navmesh, fases y triggers, cada uno una sección plana con su cantidad | P3: arena 52 KB en 0,66 ms; castillo de 32 zonas 1,7 MB en 18 ms. **Mismos bytes** con Clang, con GCC y recorriendo los nodos al revés | Cambiar el formato obliga a recocinar todo (como ya decidió el ADR-0017) |
| **Texto leído en el juego** | El juego interpreta el texto al cargar | — | Un intérprete de texto en el juego, y los errores aparecen al jugar, no en el build |

### 4. Las entidades y el ECS

**Qué describe la escena:**

| Opción | Cómo es | Problema |
|---|---|---|
| **Componentes sueltos por objeto** | Cada objeto lista sus componentes y sus valores (como una escena de Godot o de Unity) | Expone el ECS a la herramienta: un cambio en un componente obliga a tocar todos los niveles |
| **Tipo y valores que cambian** | Cada marcador dice su tipo (`spawn`, `antorcha`) y solo los valores propios; el resto sale de la tabla del tipo. Como las plantillas de *Dungeon Siege* (Bilas, 2002) o los *spawners* de Naughty Dog, que son un diccionario de claves y valores (Gregory) | El cocinador tiene que conocer cada tipo |

**Los ids y el orden:**

| Opción | Cómo es | Medido | Problema |
|---|---|---|---|
| **El camino completo** (`arena.muro.01/UBX_muro`), con un hash de 64 bits | Legible. Dos caminos iguales fallan el build | P3: ordenar por id da los mismos bytes con cualquier orden de los nodos. P4: el mismo hash en 300 cargas y con los dos compiladores | **Renombrar** un objeto cambia su id: lo que lo nombraba se rompe (y el cocinador lo atrapa) |
| **Un id guardado en cada objeto** | Una propiedad con un número único, que sobrevive a renombrar (como el `unique_id` de Godot 4.6) | — | Alguien tiene que asignarlo y cuidar que no se repita (al duplicar un objeto en Blender, la propiedad se copia) |
| **El orden del glTF** | Las entidades se crean en el orden en que aparecen | — | **Cambiar el orden del *outliner* de Blender cambia los *handles* y el hash del *replay*** |

**Las referencias** entre entidades (la palanca y su puerta, la fase y sus spawns) van
por nombre, y el cocinador falla si el nombre no existe.

### 5. El gameplay del nivel

| Opción | Cómo es | Medido | Problema |
|---|---|---|---|
| **Datos: un texto de fases** | Cada fase lista sus comandos (aparecer, gotear por las puertas, subir el máximo, el jefe, las invocaciones) y cómo termina. Un sistema genérico lo ejecuta | P5: 23 líneas para la arena; el intérprete, 34, una vez. Cambiar un número: recocinar en ~1 ms, sin Blender | Algo que el texto no sabe decir necesita una palabra nueva en el cocinador y en el intérprete. El riesgo es que crezca hasta ser un lenguaje |
| **C++ por nivel** | Una función de `game/` por nivel, con las fases escritas a mano | P5: 38 líneas para la arena, el mismo hash | Recompilar para cambiar un número. Un nombre de marcador mal escrito falla recién al jugar esa fase |
| **Un lenguaje de *script*** (Lua 5.4, MIT, por ejemplo) | Las fases y los eventos en un lenguaje embebido | No medido | Una dependencia con su ADR: una máquina virtual, un recolector de basura que asigna memoria y un manejo de errores con `longjmp` que hay que encajar en el ADR-0009 y el ADR-0010 |
| **Un director adaptativo** | Como el de *Left 4 Dead* (Booth, 2009): decide cuándo y dónde aparecen los zombies según cómo va el jugador | No medido | Contradice "colocar exactamente" |

P5 mostró además que **dos implementaciones de la misma regla se separan en los bordes:**
la primera vez, el hash final no coincidió porque un camino reiniciaba un contador al
terminar la última fase y el otro no.

### 6. Los triggers y los eventos

| Opción | Cómo es | Problema |
|---|---|---|
| **Un volumen y un evento de una lista cerrada** | El trigger es un *empty* con forma de caja (P1: el volumen es la escala); el texto dice qué evento dispara (empezar una fase, un checkpoint, abrir una puerta) | Un evento nuevo necesita sumarlo a la lista |
| **Un volumen que llama a C++** | El trigger tiene un nombre, y `game/` registra qué hacer con él | Las reglas quedan repartidas entre el nivel y el código |
| **Con *script*** | El trigger llama a una función del *script* | Lo mismo que la opción de *script* del punto 5 |

En las tres, el trigger se revisa en el paso fijo, contra la posición de la caballera.
Así es determinista.

### 7. Las luces y el ambiente

**Cómo se colocan:**

| Opción | Cómo es | Problema |
|---|---|---|
| **Luces de Blender** | Se ven en el *viewport* | P1: la intensidad llega convertida a unidades físicas (200 W → 10 870 cd). El ADR-0022 usa un modelo estilizado con un radio, así que **el radio tiene que ser explícito**: la distancia de corte de Blender o una propiedad |
| **Una pieza "antorcha"** | La luz, el fuego (ADR-0025) y el sonido (ADR-0024) juntos, en una pieza reutilizable | Las tres cosas se mueven juntas; cambiar el fuego de todas es cambiar la pieza |

**Qué 16 antorchas entran al frame** (el castillo de 32 zonas tiene 512):

| Opción | Cómo es | Problema |
|---|---|---|
| **Las 16 más cercanas, en cada frame** | Recorrer las antorchas y quedarse con las más cercanas a la cámara | No medido; es lineal en las antorchas. Una luz que entra o sale de las 16 cambia de golpe, a menos que se desvanezca |
| **Una lista fija por zona, del cocinador** | El cocinador asigna a cada zona sus antorchas (como mucho 16) y falla si son más | Una zona con muchas antorchas tiene que partirse |

**La música y el ambiente:** la música de cada nivel y los sonidos de ambiente son datos
del nivel (P2: `musica arena_tema`; un sonido de ambiente sería otro marcador).

### 8. La colisión y la navmesh del nivel armado

El cocinador **aplana las instancias** y ve la colisión del nivel entero. Para eso
aplica a cada pieza la transformación de su instancia, con la rotación de 180° sobre Y
del ADR-0017 como `M' = R·M·R`.

| Opción | Dónde queda la navmesh | Medido | Problema |
|---|---|---|---|
| **Una sección del binario del nivel** | Junto con lo demás del nivel | P3: arena, 88 × 88 celdas → 201 rectángulos en 0,12 ms; castillo de 32 zonas, 6 432 rectángulos en 5,1 ms | Se recocina con cualquier cambio del nivel (son milisegundos) |
| **Un archivo aparte por nivel** | Junto al nivel | — | Dos archivos que tienen que coincidir |

**Validar contra la navmesh:** P3 atrapó un error real. El punto de invocación 5 quedaba
dentro de una columna, y el cocinador falló con su nombre.

### 9. La estructura del mundo y las transiciones

| Opción | Cómo es | Medido | Problema |
|---|---|---|---|
| **Un nivel por archivo, con pantalla de carga entre niveles** | El recorrido lineal partido en niveles: al terminar uno, se carga el siguiente | P4: crear las entidades del castillo de 32 zonas, 0,05 ms | Una pantalla de carga entre zonas |
| **Zonas que se cargan mientras se juega** (*streaming*) | El mundo partido en zonas; se cargan las cercanas (los *world chunks* de Naughty Dog, el *World Partition* de Unreal) | P8: caminando las 32 zonas, con 25 m de precarga, hay como mucho 5 zonas cargadas (205 instancias) | El sistema más complejo: lo que cruza el borde de una zona, la memoria de varias zonas a la vez, cargar sin cortar el frame y **no romper el determinismo** (abajo) |
| **Todo el mundo cargado** | Un archivo con todo | P4: 364 KB de ECS para 32 zonas, con mallas de juguete | Lo que pesa son los assets, no el nivel |

Cómo se cargan los recursos lo decide el ADR-0028. Este ADR decide qué es un nivel y
cuándo se pasa al siguiente.

**Con zonas que se cargan mientras se juega, cuándo entra una zona a la simulación**
(sus zombies, sus triggers, su colisión). P8: la caballera camina las 32 zonas del
castillo sintético, y cada zona tarda en llegar un tiempo al azar, como un disco real.
Cinco discos distintos por caso:

| Opción | Cómo es | Medido | Problema |
|---|---|---|---|
| **Cuando termina de cargar** | Los zombies de la zona aparecen al llegar sus datos | **5 hashes distintos con 5 discos**, en todos los casos: el disco decide la partida, y un *replay* no se repite | — |
| **Cuando la caballera entra en la zona, y si no llegó, la simulación espera** | La entrada es un paso de la simulación. Si los datos no están, el paso no corre (como la pausa) | **1 hash con los 5 discos,** en todos los casos. Con 25 m de precarga, **0 frames de espera**, incluso con tirones de 0,8 a 3 s. Con 2 m de precarga y esos tirones, hasta 2 284 frames de espera en todo el recorrido | Si el disco no llega a tiempo, el jugador ve una espera. La partida no cambia |

**Qué se dibuja** (P8, el castillo de 32 zonas, con 25 m de precarga, contando
instancias cada 10 pasos):

| Qué | Media | Pico |
|---|---|---|
| Todo lo cargado | 129 | 205 |
| Lo que está en el frustum (ADR-0022) | 35 | 76 |
| Lo anterior, hasta el final de la niebla (40 m, supuesto) | 30 | 41 |
| Lo anterior, solo la zona actual y sus vecinas | 30 | 41 |

En este castillo sintético las zonas son patios abiertos a 50 m entre sí, así que la
niebla ya deja fuera las zonas lejanas. **Dentro del castillo** (salas, pasillos), las
paredes tapan lo que el frustum deja pasar: ahí sirve saber qué zonas se ven desde cada
zona, o los portales entre salas (Quake, Teller). Eso no se midió.

**Cargada no es lo mismo que simulando** (P8b, después de la auditoría). Con una sola
regla ("entra al cruzar el borde"), lo que está cerca de la caballera pero en la zona de
al lado no tiene colisión ni navmesh. P8b separa dos cosas, cada una con dos
distancias:

- **la carga**, que decide el disco: se pide una zona a menos de 35 m de su borde y se
  descarga a más de 65 m;
- **la simulación**, que decide el paso fijo: una zona entra a menos de 10 m (si no
  llegó, 0 pasos) y sale a más de una distancia de salida. Al salir, sus entidades se
  destruyen. Los zombies no entran a una zona que no simula.

| Tope de zonas cargadas | Regla del tope | Medido (5 discos × 3 rangos, salida 40 m y 20 m) |
|---|---|---|
| Sin tope | — | 1 hash, 0 esperas; hasta **9 zonas cargadas** |
| 5 o 4 | Pedir en orden de índice; con el tope lleno, descargar la más lejana que no simula | **Bloqueo en las 15 corridas:** descargaba una zona más cercana que la que pedía, y la que la simulación necesitaba nunca llegaba |
| 5 o 4 | **Pedir primero la más cercana, y descargar solo una zona más lejana que la que se pide** | 1 hash, 0 esperas, 0 bloqueos |

Que no haya bloqueos en un recorrido no lo garantiza. **La garantía** es que, en
cualquier punto de la navmesh, las zonas a menos de la distancia de salida quepan en el
tope menos uno (las que pueden estar simulando a la vez, más la que se carga). En el
castillo sintético, ese peor caso es:

| Distancia de salida | Peor caso (zonas) | Tope mínimo |
|---|---|---|
| 15, 20 o 25 m | 4 | 5 |
| 30 m | 6 | 7 |
| 40 m | 9 | 10 |

**Qué se reinicia en un checkpoint:**

| Opción | Cómo es | Medido | Problema |
|---|---|---|---|
| **Recargar el nivel y empezar la fase** | Volver es recargar el nivel y entrar en la fase del checkpoint, desde cero | P5: 0,012 ms, el mismo hash por los dos caminos. Lo que hay que recordar: el nivel y la fase | Lo que el jugador cambió en el nivel antes del checkpoint (una puerta abierta) se pierde, salvo que la fase lo diga |
| **Una foto del estado** | Guardar todo el estado del mundo en el checkpoint | — | Mucho más que guardar, y su formato es del ADR-0028 |

### 10. La iteración, la validación, las pruebas, la memoria y la depuración

| Qué | Medido |
|---|---|
| Ciclo con un cambio en Blender | P6: ~1 s en la arena (abrir Blender es casi todo), ~2,4 s en el castillo de 32 zonas |
| Ciclo con un cambio en el texto | P6: ~1 ms, sin Blender |
| Lo que el cocinador de juguete hace fallar | Un tipo de zombie, marcador o evento desconocido; un marcador que no existe; un patrón sin marcadores; una fase sin final; dos caminos con el mismo id; un spawn fuera de la navmesh; una cápsula o esfera con escala no uniforme |
| Memoria del nivel en el ECS | P4: 11 KB la arena, 364 KB el castillo de 32 zonas |
| Pruebas | El cocinador lleva TDD estricto (ADR-0011, ADR-0017). El *replay* de una fase da el mismo hash (P5) |
| Depuración | P7: la vista cenital dibuja la navmesh, la colisión, los radios de las antorchas, los triggers y los marcadores por fase |

## Investigación

### Prototipo medido

Un script de Blender, un texto de nivel y un programa en C++20 con cgltf, medidos en la
máquina de referencia el 2026-11-02. El detalle y cómo repetirlo están en
[`../research/prototipo-de-niveles.md`](../research/prototipo-de-niveles.md). Un video
de 78 s muestra la arena cocinada y sus tres fases.

| # | Qué | Resultado |
|---|---|---|
| P1 | Blender → glTF | Propiedades → `extras`; *collection instances* → nodos que comparten la malla; volumen del trigger = escala; luces con la intensidad convertida. El `.glb` sale igual dos veces; el `.blend`, no |
| P2 | Texto y git | Mover un spawn en el `.blend`: "Binary files differ". En el texto: una línea |
| P3 | Cocinador | Arena 0,66 ms, 52 KB; castillo de 32 zonas 18 ms, 1,7 MB. Mismos bytes con Clang, GCC y los nodos al revés. Atrapó un punto de invocación dentro de una columna |
| P4 | Carga | 117 entidades en 1,5 µs; 3 744 en ~50 µs. El mismo hash en 300 cargas y con los dos compiladores |
| P5 | Fases | Datos y C++ dan el mismo hash. Checkpoint: recargar y entrar en la fase, 0,012 ms |
| P6 | Iteración | Blender: ~1 s (arena) y ~2,4 s (32 zonas). Texto: ~1 ms |
| P7 | Video | La vista cenital de la arena y sus fases, con una muerte y la vuelta al checkpoint |
| P8 | Zonas que se cargan al caminar | Si los zombies aparecen al cargar la zona: 5 hashes con 5 discos. Si aparecen al entrar la caballera, esperando a la zona: 1 hash, y 0 esperas con 25 m de precarga. Pico de 5 zonas cargadas; se dibujan 30 instancias de 129 |
| P8b | Cargada ≠ simulando | Carga a 35/65 m, simulación a 10/20 m: 1 hash, 0 esperas. Con tope, pedir en orden de índice bloqueó las 15 corridas; pedir primero la más cercana, ninguna. Con 20 m de salida, el peor caso son 4 zonas: tope 5 |

Lo que mostró, además de los números:

- **Cargar el nivel en sí no cuesta nada.** Crear 3 744 entidades lleva microsegundos. El
  tiempo de carga va a estar en los assets, que son del ADR-0028.
- **El orden es lo que rompe el determinismo,** no el formato. Ordenar por id hace que ni
  el cocinador ni los *handles* dependan de Blender.
- **El cocinador es el lugar para atrapar errores de colocación,** porque ve el nivel
  armado entero, con la navmesh.
- **Abrir Blender es casi todo el ciclo de iteración.** Lo que se ajusta mucho (cuántos
  zombies, cada cuánto) conviene que no pase por Blender.
- **Con zonas que se cargan al caminar, el disco no puede decidir nada de la
  simulación.** La zona entra a la simulación en un paso que decide la simulación, y si
  sus datos no llegaron, la simulación espera. Así el disco solo decide cuándo se ve, y
  el *replay* se repite con cualquier disco.
- **Cargada no es lo mismo que simulando,** y un tope de memoria necesita prioridad por
  distancia: sin ella, el sistema se bloqueó. El tope se puede garantizar en el
  cocinador, que conoce la navmesh y los límites de cada zona.

**Lo que toca a otros ADRs:** el prototipo mostró que la decisión cambia los ADR-0010,
0012, 0017, 0021, 0022, 0023, 0024 y 0026, y la visión. Lo que cambia en cada uno está
en la lista de fe de erratas de Consecuencias. El ADR-0025 no cambia: el fuego de las
antorchas entra en su pool de partículas. Al ADR-0028 le deja cuánto hay que guardar
para un checkpoint y cómo cargar una zona sin cortar el frame.

### Cómo lo hacen otros

- **Naughty Dog** (Gregory, "Game Object Models"): separa el modelo de objetos del
  editor (*tool-time*) del modelo del juego (*run-time*). Sus *spawners* son un
  diccionario de claves y valores. **Lo que se itera mucho (*spawners*, regiones,
  *splines*) va en texto o en un binario liviano;** la geometría, en archivos
  empaquetados. El mundo se carga por *chunks*.
- ***Dungeon Siege*** (Bilas, GDC 2002): objetos armados con componentes y plantillas de
  datos. Más de 7 300 tipos y más de 100 000 objetos colocados en dos mapas, sin tocar
  C++.
- **Godot:** al importar una escena, los sufijos del nombre (`-col`, `-colonly`,
  `-navmesh`) convierten un objeto en colisión o en navmesh, la misma idea que los
  prefijos del ADR-0017. Sus escenas son texto (TSCN), pensado para el control de
  versiones, y desde la versión 4.6 cada nodo guarda un `unique_id`.
- **Unity:** el modo *Force Text* guarda las escenas y los *prefabs* como YAML, para el
  control de versiones.
- **Unreal Engine 5:** *One File Per Actor* guarda cada actor de un nivel en su propio
  archivo, para no bloquear el nivel entero al editar. Viene activado con *World
  Partition*, su sistema de *streaming*.
- **Quake y TrenchBroom:** un mapa es una lista de entidades con pares de clave y valor.
  TrenchBroom (GPLv3) es su editor actual.
- ***Left 4 Dead*** (Booth, 2009): un director decide cuándo y dónde aparecen los
  zombies, según cómo va el jugador.

## Decisión

Quiero un mundo caminable y explorable: la caballera sale de A y camina hasta Z,
recorriendo un castillo completo, sin pantallas de carga en el camino. El mundo se parte
en zonas que se cargan por secciones mientras se camina, sin que el jugador lo note, y
solo se dibuja lo que el jugador ve. Lo armo en Blender, porque ya es mi herramienta y
porque coloco exactamente cada zombie, cada trigger y cada antorcha. Lo que pasa en cada
zona (las oleadas, la horda, el jefe) lo escribo en un texto, que se ajusta sin abrir
Blender y se lee en un diff. El disco nunca decide qué pasa en la partida: solo decide
cuándo se ve.

| # | Dimensión | Elección |
|---|---|---|
| 1 | Herramienta | **Blender como editor.** Las piezas se colocan como ***collection instances***; los spawns, los puntos de invocación, los triggers, los checkpoints, el inicio y los sonidos de ambiente, como ***empties*** con la propiedad `tipo` y sus valores propios. **El cocinador valida todo** y falla con el nombre del objeto. Un **complemento de Python** (listas de tipos, dibujo de los radios, exportar al guardar) queda para un ticket aparte, si escribir las propiedades a mano molesta |
| 2 | Unidad | **El mundo, partido en zonas.** Cada zona es **un `.blend`** en coordenadas del mundo, más **un texto de la zona** con lo que pasa en ella. **Un texto del mundo** dice qué zonas hay, cuáles son **vecinas** de cada una (las que se ven o a las que se camina desde ella), dónde empieza la caballera, el **tope de zonas cargadas a la vez** y las distancias de carga y de simulación. **Las piezas son reutilizables** (muro, columna, antorcha) y llevan su colisión. **El corte vertical es un mundo de una sola zona,** la arena |
| 3 | Formato | **La fuente:** el glTF de cada zona dice *dónde* (posiciones, volúmenes, luces); el texto de la zona dice *cuándo y cuántos*, y nombra los marcadores; el texto del mundo, cómo se unen las zonas, y además (ADR-0028) el tamaño del slot, el presupuesto del global y **la lista de fases con su índice estable**, a la que solo se le agregan filas; cada fila nombra la fase por el id de su zona y **un nombre de fase**, que el texto de la zona le da a cada fase (nunca por su número de orden, que se corre al insertar una). **Lo cocinado:** **un binario por zona** con las secciones del ADR-0017 (instancias, colisión, luces, marcadores, navmesh, fases, triggers y, en los builds de desarrollo, los nombres de depuración) y **un binario del mundo** (las zonas, sus límites, sus vecinas, el inicio, **el tope y las cuatro distancias** de carga y de simulación, y lo que hace falta para validarlos en vivo: **la curva del peor caso**, es decir cuántas zonas hay como máximo a menos de cada distancia de salida, de metro en metro, y **la entrada máxima** sin una vecina olvidada; con el ADR-0028, también el tamaño del slot, el presupuesto del global, el índice de cada fase y el peor caso de polígonos de navmesh entre las zonas cargadas a la vez). Sin compatibilidad hacia atrás, como el resto de lo cocinado. **Cocinar dos veces da los mismos bytes** |
| 4 | Entidades | **Tipo y valores propios:** cada marcador dice su tipo, y lo demás sale de la tabla del tipo (ADR-0018, ADR-0023). **El id es el FNV-1a de 64 bits del camino completo**, que empieza con el nombre de la zona, así es **único en todo el mundo**; dos caminos con el mismo id fallan el build. **Cada sección se ordena por id, y las entidades se crean en ese orden:** los *handles* y el hash no dependen del orden de Blender. **Las referencias van por nombre** y el cocinador falla si no existe. **Un texto de zona solo nombra objetos de su zona:** una vecina puede estar cargada sin estar simulando, así que nombrarla no sería seguro |
| 5 | Gameplay | **Datos: un texto de fases por zona,** con una **lista cerrada de comandos** (aparecer en un spawn, gotear por un grupo, subir el máximo de vivos, el jefe y sus puntos de invocación) y de **finales** (todos muertos, N muertes, el jefe muerto). Un sistema genérico de `game/` las ejecuta. **La primera fase empieza cuando la caballera cruza el borde de la zona** (revisado en el paso fijo), o con un trigger que la fase nombra. La zona ya está simulando desde los 10 m. Una zona sin fases es para caminar. **Un grupo (`puerta.*`) se ordena por nombre.** Algo que el texto no sabe decir suma una palabra a la lista, en el cocinador y en el sistema, con su test; **no hay C++ por zona**. **Sin lenguaje de *script* y sin director adaptativo.** El mago sigue con su FSM (ADR-0023) |
| 6 | Triggers | **Un volumen y un evento de una lista cerrada:** el volumen es la escala del *empty* (una caja); los eventos, para empezar, son empezar una fase, activar un checkpoint, abrir una puerta y cambiar el *track* de la música. Los estados de la música (exploración, combate, jefe) siguen saliendo del ADR-0024: el combate, de los zombies alertados; el jefe, de su fase. **Se revisan en el paso fijo** contra la posición de la caballera, así son deterministas |
| 7 | Luces y ambiente | **Una pieza "antorcha"** con la luz, el fuego (ADR-0025) y el sonido (ADR-0024). **El radio es la propiedad `radio`**; del glTF se usa el color, no la intensidad. **Las 16 antorchas más cercanas a la cámara**, entre las zonas cargadas, entran al frame; **una antorcha que entra o sale se desvanece**. **La música de cada zona** va en su texto, y cambia con el *crossfade* del ADR-0024 cuando la caballera cruza el borde de otra zona. Los sonidos de ambiente son marcadores |
| 8 | Colisión y navmesh | **El cocinador ve el mundo armado entero:** aplana las instancias de todas las zonas (con `M' = R·M·R`, la rotación del ADR-0017), genera la navmesh del mundo y **la parte por zona**: cada zona guarda sus polígonos y los portales hacia los de sus vecinas, nombrados por zona y polígono. **Un portal hacia una zona que no simula está cerrado.** **El radio de inflado es un parámetro del mundo,** el mismo en todas las zonas, para que los bordes coincidan. **Falla** si un spawn, un punto de invocación, un checkpoint o el inicio caen fuera de la navmesh, y si desde un punto de la navmesh de una zona hay **otra zona a menos de la distancia de entrada que no está en su lista de vecinas** (una vecina olvidada) |
| 9 | Mundo y transiciones | **Un mundo continuo, sin pantallas de carga al caminar.** **Cargada no es lo mismo que simulando,** y cada cosa tiene dos distancias, medidas desde la caballera hasta el borde de la zona. **La carga** (la decide el disco, y no cambia la partida): **una vecina de la zona actual se pide a menos de 35 m y se descarga a más de 65 m**. **La simulación** (la decide el paso fijo): **una zona entra a la simulación** (su colisión, su navmesh, sus triggers y sus entidades) **a menos de 10 m**, y si sus datos no llegaron, **la simulación espera: el frame corre 0 pasos**, como en la pausa, y un contador lo registra. **Sale a más de 20 m**: las entidades que están en la zona en ese paso (según su polígono de la navmesh, o su posición si no tienen uno) se destruyen en orden de id; un zombie que siguió a la caballera a otra zona simulada sigue vivo, y una fase que no terminó empieza de cero si la caballera vuelve. **Los portales de la navmesh hacia una zona que no simula están cerrados**, y un pedazo que cae fuera de la zona simulada se borra. **Las fases empiezan al cruzar el borde** de la zona (o con su trigger). **El tope de zonas cargadas** (5, de partida): se pide **primero la zona más cercana**, y con el tope lleno se descarga la cargada más lejana que no simula, **solo si está más lejos que la que se pide**; si no, no se pide todavía y, si la simulación la necesita, espera. **Nunca se aborta.** El cocinador **garantiza que no haya bloqueo**: falla si en algún punto de la navmesh hay más zonas a menos de la distancia de salida que el tope menos uno. Cómo se carga sin cortar el frame lo decide el ADR-0028 (un hilo de I/O, slots fijos y una subida de 2 MB por frame; **una zona está cargada cuando llegó del disco y terminó su subida**). **Lo que se dibuja:** solo lo cargado; de eso, lo que pasa el culling por esfera contra el frustum (ADR-0022), **hasta el final de la niebla**, y **solo la zona actual y sus vecinas**. **Un checkpoint** es un marcador de una zona, ligado a una fase: volver es cargar la zona del checkpoint, detrás de la pantalla de carga, aplicar desde ahí la misma regla de carga (la más cercana primero, con el tope) y entrar en la fase desde cero. Las fases ya terminadas siguen terminadas. Lo que hay que recordar es **la zona, la fase y qué fases terminaron**; cómo se guarda lo decide el ADR-0028 (una tarjeta de memoria que se escribe al llegar a cada checkpoint). **Reintentar una fase la repite igual:** su semilla sale del id de la zona y del número de la fase |
| 10 | Iteración, pruebas y depuración | **El build cocina el mundo** como los demás assets: un cambio en el texto no abre Blender (~1 ms), y uno en Blender reexporta solo esa zona. **El cocinador lleva TDD estricto** (ADR-0011, ADR-0017) y falla por: un tipo de zombie, marcador o evento desconocido; un marcador que no existe o que es de otra zona; un patrón sin marcadores; una fase sin final; dos caminos con el mismo id; un marcador fuera de la navmesh; una vecina olvidada; un punto de la navmesh con más zonas a menos de la distancia de salida que el tope menos uno; y una cápsula o esfera con escala no uniforme. **El *replay*** guarda el mundo y el checkpoint de partida (zona, fase y fases terminadas). **Un test inyecta tiempos de carga al azar** y exige el mismo hash, como P8 y P8b. **La depuración** (ADR-0021): el dibujo de la navmesh, la colisión, los radios de las antorchas, los triggers, los marcadores por fase y los bordes de las zonas con su estado (pedida, cargada, simulando), y un panel de ImGui con las zonas cargadas, las esperas y el tiempo de cada carga. **Ajustar las distancias y el tope por sensación:** tienen una sola fuente, el texto del mundo, y el juego usa los del binario del mundo, así que la garantía del cocinador vale. En los builds de desarrollo, **el panel del mundo los cambia en vivo** para probarlos jugando: **cada cambio se valida en el momento** contra la curva del peor caso y la entrada máxima del binario, y uno que rompa la garantía se rechaza con el motivo (por ejemplo, "salida 25,5 m: el peor caso pasa el tope"). El panel también exige **entrada < salida** y **carga < descarga**, y para una salida no entera usa el valor de la curva en el metro siguiente. **El tope en vivo nunca pasa el del binario,** que es con el que se reservó la memoria al iniciar (ADR-0010): se puede bajar y volver a subir hasta ese valor; para subirlo se cambia el texto del mundo, se recocina y se reinicia. Estos valores **no son CVars** (fe de erratas del ADR-0021): son datos de los assets, así que en Release vienen del binario y no hay panel. Un cambio de la entrada o la salida mientras se graba se graba como evento del paso (ADR-0021), porque cambia la simulación, y el encabezado del *replay* guarda los valores del mundo en uso al empezar. **Un botón escribe los valores en el texto del mundo** (por el hilo de I/O, ADR-0028), y el próximo cocinado los fija |

## Consecuencias

- **El camino de una zona:**

  ```
  zona.blend ──blender -b──→ zona.glb ─┐
  zona (texto) ─────────────────────────┼─→ cocinador ─→ zona.bin (una por zona)
  mundo (texto) ────────────────────────┘      │      └→ mundo.bin
                                               └ ve todas las zonas: ids únicos,
                                                 navmesh del mundo partida por zona,
                                                 referencias, vecinas olvidadas,
                                                 tope sin bloqueo
  ```

  Y en el juego, por zona, con las distancias desde la caballera hasta su borde:

  ```
  carga (el disco):        fuera ─(< 35 m)→ pedida ─(llegó)→ cargada ─(> 65 m, sin simular)→ fuera
  simulación (paso fijo):  no simula ─(< 10 m y cargada)→ simulando ─(> 20 m)→ no simula
                           (< 10 m y no cargada: el frame corre 0 pasos)
  ```
- **Lo que se gana:** un mundo continuo que se arma con la herramienta que ya uso, sin
  un editor propio; los errores de colocación aparecen en el build, con el nombre del
  objeto; ajustar una oleada no pasa por Blender; y el *replay* da el mismo hash con
  cualquier disco, porque la simulación nunca depende de cuándo llega una zona.
- **Lo que se sacrifica:**
  - **dos archivos por zona** (el `.blend` y el texto) que tienen que coincidir. El
    cocinador lo valida, pero el error aparece al cocinar, no al escribir;
  - **renombrar un objeto rompe lo que lo nombra** (el cocinador lo atrapa);
  - **las vecinas se escriben a mano** en el texto del mundo. El cocinador atrapa la que
    falta a menos de la distancia de entrada, pero no una zona que solo se ve de lejos:
    esa no se dibuja;
  - **si el disco no llega a tiempo, el jugador ve una espera** (un tirón). La partida
    no cambia, pero se nota. Lo evita el margen entre la carga y la entrada (P8b:
    25 m y 0 esperas, incluso con tirones de 3 s);
  - **lo que está en una zona cuando sale de la simulación se destruye:** un zombie que
    no siguió a la caballera no la espera si ella vuelve; uno que la siguió a otra zona
    simulada sigue vivo;
  - **el *streaming* es el sistema más complejo de este ADR,** y el corte vertical no lo
    necesita: entra cuando haya una segunda zona;
  - **lo que el jugador cambió en una fase antes de morir** (una puerta abierta) vuelve
    a su estado inicial, salvo que la fase lo diga.
- **La cámara por zona:** si el ADR-0031 elige una cámara colocada por el nivel, sus
  zonas o rieles serían marcadores de la zona, con las mismas reglas.
- **Fe de erratas en otros ADRs.** Al aceptar este ADR se corrigieron, como fe de
  erratas (ADR-0014, regla 11):
  - **ADR-0017:** el cocinador suma el mundo: aplana las instancias, cocina un binario
    por zona y uno del mundo, lee los textos de zona y de mundo, y suma sus
    validaciones;
  - **ADR-0023:** la navmesh se genera para el mundo armado y se parte por zona, con
    portales entre zonas que se cierran cuando la vecina no simula; el radio de inflado es
    un parámetro del mundo; los spawns y los puntos de invocación los da este ADR;
  - **ADR-0022:** el culling suma la distancia de la niebla y las zonas visibles; las 16
    antorchas son las más cercanas, con fundido;
  - **ADR-0021:** el encabezado del *replay* guarda el mundo, el checkpoint de partida y
    los valores del mundo en uso; el registro por frame suma el contador de esperas, que
    no cambia cómo se calcula R1; y los valores del mundo (distancias y tope) son una
    excepción a la Decisión 7: datos de los assets, no CVars, con su propio panel;
  - **ADR-0012:** entrar y salir de la simulación son pasos de la simulación; esperar a
    una zona deja el frame en 0 pasos;
  - **ADR-0010:** la vida "un nivel" pasa a ser "una zona", dimensionada por el tope de
    zonas cargadas. Como las zonas no se liberan en el orden en que se cargaron, un solo
    bloque lineal por nivel no alcanza; cómo se reparte lo decide el ADR-0028. El mapa de
    la memoria suma los recursos de la GPU y los datos en la CPU de cada zona. La
    capacidad del ECS cubre las entidades de las zonas cargadas;
  - **ADR-0024:** la música va por zona y un trigger puede cambiarla; los sonidos de
    ambiente son marcadores de la zona;
  - **ADR-0026:** la pantalla de carga aparece al empezar y al volver a un checkpoint, no
    al caminar; si una espera por una zona muestra un indicador queda abierto;
  - **la visión:** el recorrido es continuo, sin pantallas de carga entre zonas;
  - **`docs/performance/presupuestos.md`:** la memoria que vivía un nivel vive una zona,
    con el tope de zonas cargadas.
- **Tickets, en orden** (cada uno crece desde el anterior):
  1. el cocinador de una zona: aplanar instancias, ids, marcadores, luces y colisión, en
     TDD estricto;
  2. los textos de zona y de mundo: fases, triggers, vecinas y sus validaciones, en TDD
     estricto;
  3. la navmesh de la zona en su binario (junto al ticket del ADR-0023);
  4. cargar una zona en el ECS, con las entidades en orden de id y el hash, en TDD
     estricto (el ECS es núcleo, ADR-0011);
  5. el sistema de fases y de triggers, con la arena del corte vertical;
  6. los checkpoints (junto al ADR-0028);
  7. la antorcha como pieza y las 16 más cercanas con fundido (junto a los ADR-0022,
     ADR-0024 y ADR-0025);
  8. el dibujo de depuración de las zonas, los triggers y los marcadores (junto al
     ADR-0021);
  9. varias zonas: vecinas, carga y descarga, entrada y salida de la simulación con
     espera, el tope con prioridad por distancia, las validaciones de vecinas olvidadas y
     del tope sin bloqueo, y el test con tiempos de carga al azar (junto al ADR-0028);
  10. la navmesh partida por zona, con portales entre zonas que se cierran cuando la
      vecina no simula (junto al ADR-0023);
  11. el culling por la niebla y por zonas visibles (junto al ADR-0022);
  12. el complemento de Blender, si hace falta.
- **Abierto:**
  - **los valores de partida,** que se ajustan jugando: la carga (35 m) y la descarga
    (65 m), la entrada (10 m) y la salida (20 m) de la simulación, el tope de zonas (5),
    el final de la niebla y el tiempo del fundido de las antorchas. Las distancias y el
    tope se prueban en vivo con el panel del mundo, que rechaza un valor que rompa la
    garantía y no deja que el tope pase el del binario, y se fijan en el texto del mundo. Conviene
    empezar con un tope generoso e ir bajándolo. Con zonas de otro tamaño que las del
    castillo sintético, el cocinador dice si el tope alcanza;
  - **las piezas en su propio `.blend`:** si cada zona las enlaza desde una biblioteca de
    piezas (*Link* de Blender), hay que verificar que el exportador las exporte igual que
    las piezas del mismo archivo (P1 probó solo esas);
  - **el culling dentro del castillo:** si las paredes de las salas no alcanzan con las
    vecinas y la niebla, portales entre salas, cuando el profiler lo pida;
  - **las alturas del castillo** (pisos, escaleras): el resto de Recast (ADR-0023);
  - si reintentar una fase igual se siente bien, o si conviene variar la semilla en cada
    intento;
  - cómo se carga una zona sin cortar el frame, cuánto vive cada recurso y cómo se
    guarda el checkpoint (decidido en el ADR-0028: un hilo de I/O, slots fijos por zona,
    lo compartido en lo global, y una tarjeta de memoria).

## Cómo lo defiendo

**¿Por qué Blender como editor de niveles y no un editor propio?**
Porque un editor propio es un proyecto entero (selección, *gizmos*, deshacer, guardar)
antes de tener el primer nivel, y Blender ya es mi herramienta para los modelos. En el
prototipo, todo lo que coloco llega al glTF: las piezas como nodos que comparten la
malla, las propiedades como `extras`, el volumen de un trigger como la escala y las
luces. Lo que Blender no hace, validar, lo hace el cocinador, que ve el mundo entero. Es
la división que describe Gregory: el modelo de objetos del editor y el del juego son
dos cosas distintas.

**¿Por qué el gameplay en un texto aparte y no como propiedades en Blender?**
Porque son cosas distintas: dónde está algo se decide mirando, y cuándo y cuántos se
decide ajustando números. Las fases son listas con orden y condiciones, que en el panel
de propiedades de Blender son incómodas de escribir y no se leen en un diff: mover un
spawn en el `.blend` solo dice "Binary files differ". Además, cambiar el texto se
recocina en ~1 ms sin abrir Blender, y abrir Blender es casi todo el segundo que cuesta
iterar. Naughty Dog hace lo mismo: lo que se itera mucho (sus *spawners*) va en texto o
en un binario liviano.

**¿Por qué el id es el hash del camino y no el orden del glTF?**
Porque el orden es lo que rompe el determinismo, no el formato. Si las entidades se
crearan en el orden del glTF, reordenar dos objetos en el *outliner* de Blender
cambiaría los *handles* y el hash de todos los *replays*. Ordenando por id, el
cocinador dio los mismos bytes con Clang, con GCC y recorriendo los nodos al revés. El
costo es que renombrar un objeto rompe lo que lo nombra, pero el cocinador lo atrapa en
el build.

**¿Por qué las fases son datos y no código, ni un lenguaje de *script*?**
Porque escribí las tres fases de la arena de las dos formas, y dieron el mismo hash: 23
líneas de texto contra 38 de C++ que habría que repetir y recompilar en cada nivel. La
primera vez el hash no coincidió, porque una regla de borde estaba escrita distinto en
los dos caminos: con un solo intérprete, ese error no existe. Un lenguaje de *script*
sería una dependencia con su máquina virtual, su recolector de basura y su manejo de
errores, que choca con los ADR-0009 y 0010. Si algún día el texto no alcanza, se suma
una palabra a la lista, con su test.

**¿Por qué no un director como el de *Left 4 Dead*?**
Porque quiero elegir exactamente dónde y cuándo aparece cada zombie. Un director
adaptativo decide por mí según cómo va el jugador. La horda de la fase 2 ya tiene lo
que necesito de él: gotea por las puertas con un máximo de vivos que sube, pero con
números que escribo yo.

**¿Cómo cargas zonas mientras se camina sin romper el *replay*?**
Separando lo que se carga de lo que se simula. Que una zona esté cargada lo decide el
disco, y no cambia nada de la partida. Que una zona simule lo decide el paso fijo: entra
cuando la caballera está a menos de 10 m de su borde, y si los datos no llegaron, el
frame corre 0 pasos, como en la pausa; sale a más de 20 m. Lo medí: si los zombies
aparecían al terminar de cargar, cinco discos distintos daban cinco partidas distintas,
incluso con un SSD. Con la regla del paso, siempre el mismo hash. El disco decide cuándo
se ve una zona, nunca qué pasa en ella.

**¿Y si el disco no llega a tiempo?**
El jugador ve una espera: el frame queda quieto hasta que la zona llega. La partida no
cambia, pero se nota, así que la evito con el margen: una zona se pide a 35 m y entra a
la simulación a 10 m, y esos 25 m son 10 segundos a los 2,5 m/s del prototipo. No hubo
ninguna espera, ni con tirones de disco de 3 segundos. El registro por frame cuenta las
esperas, así que si aparecen, las veo.

**¿Cómo evitas que el tope de memoria bloquee el juego?**
Con dos reglas, y las dos salieron de un error del prototipo. En el juego, se pide
primero la zona más cercana, y con el tope lleno solo se descarga una zona más lejana
que la que se pide. La primera versión del prototipo pedía en orden de índice y se
bloqueó en todas las corridas: descargaba algo más cercano de lo que pedía. En el
cocinador, la garantía: en cualquier punto de la navmesh, las zonas a menos de la
distancia de salida tienen que caber en el tope menos uno. Con 40 m de salida, el
castillo sintético pedía un tope de 10; con 20 m, de 5. Esos números los voy a ajustar
jugando, por sensación: el panel del mundo los cambia en vivo, pero rechaza cualquier
valor que rompa la garantía, porque el cocinador guarda la curva del peor caso en el
binario del mundo. El tope en vivo nunca pasa el del binario: la memoria de las zonas se
reservó al iniciar con ese valor, así que superarlo pide recocinar y reiniciar.

**¿Por qué no hay pantallas de carga entre zonas?**
Porque quiero que se sienta que la caballera sale de A y camina hasta Z, recorriendo un
castillo completo. Cargar el nivel en sí no cuesta nada: crear las 3 744 entidades del
castillo sintético llevó unos 50 µs. Lo que pesa son los assets, y cargarlos sin cortar
el frame le toca al ADR-0028. La pantalla de carga queda para el inicio y para la vuelta
a un checkpoint.

**¿Cómo decides qué se dibuja?**
Solo lo cargado, y de eso, la esfera contra el frustum del ADR-0022, hasta el final de
la niebla y solo en la zona actual y sus vecinas. La niebla del estilo PSX no es solo
estética: lo que está más lejos no se ve, así que no se dibuja. En el castillo
sintético, de 129 piezas cargadas en promedio se dibujaban 30. Dentro de las salas, si
las paredes tapan más de lo que esto recorta, el siguiente paso son los portales entre
salas, cuando el profiler lo pida.

**¿Por qué la navmesh del mundo entero y no una por zona?**
Porque los zombies caminan de una zona a otra, y los bordes tienen que coincidir. El
cocinador ve todas las zonas a la vez, genera una sola navmesh y la parte por zona, con
portales hacia las vecinas. Por eso el radio de inflado es uno para todo el mundo. Que
el cocinador vea el mundo armado también le permite atrapar errores de colocación: en
el prototipo encontró un punto de invocación que quedaba dentro de una columna.

**¿Qué guarda un checkpoint?**
Muy poco: la zona, la fase y qué fases ya terminaron. Volver es cargar la zona del
checkpoint detrás de la pantalla de carga, dejar que la regla de carga traiga lo demás
con el tope, y entrar en la fase desde cero; en el prototipo, eso último tardó 0,012 ms. Reintentar
una fase la repite igual, porque su semilla sale de la zona y del número de la fase:
quien muere puede aprender el encuentro. Cómo se escribe a disco es del ADR-0028.

**¿Qué sacrificas?**
Dos archivos por zona que tienen que coincidir, aunque el cocinador lo valida. Una lista
de vecinas escrita a mano: el cocinador atrapa la que falta cerca, pero no una que solo
se ve de lejos. Una espera visible si el disco es muy lento. Lo que queda en una zona
que sale de la simulación se destruye. Y el sistema más complejo del ADR, el de las
zonas que se cargan al caminar, que el corte vertical no necesita: entra cuando exista
una segunda zona.

## Referencias

- Jason Gregory, *Game Engine Architecture*, 3.ª edición, CRC Press, 2018: los
  capítulos del sistema de gameplay (el modelo de objetos, los *spawners*, los *chunks*
  del mundo).
- Jason Gregory (Naughty Dog), "Game Object Models", presentación:
  <https://www.slideshare.net/ShawnPresser/game-object-models-game-engine-architecture>
- Scott Bilas, "A Data-Driven Game Object System", GDC 2002:
  <https://www.slideshare.net/maa77/a-data-driven-game-object-system>
- Seth J. Teller y Carlo H. Séquin, "Visibility Preprocessing for Interactive
  Walkthroughs", SIGGRAPH 1991.
- Michael Abrash, *Graphics Programming Black Book*, Coriolis, 1997: los capítulos sobre
  el *potentially visible set* de Quake.
- Michael Booth, "The AI Systems of Left 4 Dead", Artificial Intelligence and
  Interactive Digital Entertainment (AIIDE), 2009.
- Godot Engine, "Node type customization using name suffixes":
  <https://docs.godotengine.org/en/stable/tutorials/assets_pipeline/importing_3d_scenes/node_type_customization.html>
- Godot Engine, "TSCN file format":
  <https://docs.godotengine.org/en/stable/engine_details/file_formats/tscn.html>
- Epic Games, "One File Per Actor in Unreal Engine":
  <https://dev.epicgames.com/documentation/unreal-engine/one-file-per-actor-in-unreal-engine>
- JetBrains, "Asset serialization mode" (el modo *Force Text* de Unity):
  <https://www.jetbrains.com/help/rider/Asset-serialization-mode.html>
- TrenchBroom, manual: <https://trenchbroom.github.io/manual/latest>
- Khronos, *KHR_lights_punctual*:
  <https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_lights_punctual>
- Johannes Kuhlmann y colaboradores, *cgltf* (MIT): <https://github.com/jkuhlmann/cgltf>
