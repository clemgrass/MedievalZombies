# ADR-0028: Recursos en runtime y guardado

- **Estado:** Aceptado
- **Fecha:** 2026-11-03
- **Ticket:** #42
- **Reemplaza / Reemplazado por:** —
- **Fe de erratas:** 2026-11-06 (#76): el ADR-0034 decide las cinemáticas. **Cada una va
  en la parte de la CPU del binario de su zona,** en una sección propia (las cámaras y
  los actores horneados a 30 Hz, y los eventos), **o en el global si la usan varias
  zonas**, con el mismo reparto del cocinador. Una de un minuto con tres personajes pesa
  ~1,5 MB y entra en el slot. Se corrige en la Decisión 2.

## Contexto y problema

Hay que decidir **cómo vive un asset desde que está en el disco hasta que el juego lo
usa**: cómo se lee sin cortar el frame, en qué unidades, dónde queda en la memoria,
cómo sube a la GPU y cuándo se libera. También cómo se recarga en caliente mientras el
autor trabaja, y **cómo se guardan la partida y las opciones** sin perderlas nunca.

El mundo es continuo (ADR-0027): la caballera camina del principio al final del
castillo sin pantallas de carga, y las zonas se cargan y se descargan mientras camina.
Ese ADR ya decidió **qué** se carga y **cuándo** (a 35 m se pide, a 65 m se descarga,
con un tope de 5 zonas). Este decide **cómo**.

**La idea del autor** para este ADR:

- **Un sistema de guardado de verdad, con checkpoints.** El juego no va a ser largo,
  pero el guardado se hace igual, para aprender a hacerlo. **Una zona reservada** con
  datos muy concretos y **un bit que dice si hay partida.**
- **Todo dentro del ejecutable,** si no se escapa demasiado: analizarlo.
- **Recarga en caliente para todo lo visual:** cambiar un shader, una textura o una
  malla y verlo en el juego sin cerrarlo.

Varios ADRs ya aceptados dejaron preguntas para este:

| ADR | Qué dejó abierto |
|---|---|
| ADR-0010 | La memoria que vive una zona, en la CPU y en la GPU: las zonas no se liberan en orden, así que no es un solo bloque lineal. Cómo se reparte y se carga |
| ADR-0012 | Cómo se carga una zona sin cortar el frame. El engine es de un hilo, con dos excepciones (Tracy y el audio) |
| ADR-0017 | Cómo se cargan y se administran en runtime los binarios cocinados, y la recarga en caliente de los assets |
| ADR-0021 | Dónde se guardan las opciones del jugador. Empezar un *replay* en medio de una sesión, con el estado guardado |
| ADR-0022 | La carga de shaders y texturas, su recarga en caliente y dónde se guarda el `VkPipelineCache` |
| ADR-0024 | Qué sonidos carga cada zona y cuánto viven, y cómo se recarga la tabla de sonidos en desarrollo |
| ADR-0025 | La carga de las texturas de los VFX y su recarga en caliente |
| ADR-0026 | Dónde se guardan las opciones y su formato. Qué muestra la pantalla de carga y si una espera por una zona muestra algún indicador |
| ADR-0027 | Cómo se carga sin cortar el frame y cómo se escribe a disco un checkpoint (la zona, la fase y las fases terminadas) |

Hay que decidir:

1. **cómo se lee del disco** sin cortar el frame;
2. **la unidad de carga y el formato en disco:** archivos por zona o un paquete, si se
   comprime y si el binario se usa tal como llega;
3. **la vida de los recursos** que comparten varias zonas;
4. **la memoria de las zonas,** en la CPU y en la GPU;
5. **la subida a la GPU;**
6. **el arranque y la pantalla de carga:** qué es residente, los pipelines y qué muestra
   la pantalla;
7. **la recarga en caliente;**
8. **la partida guardada:** qué, cuándo, en qué formato y cómo no se pierde;
9. **las opciones del jugador:** dónde y en qué formato;
10. **el presupuesto, las pruebas y la depuración.**

Lo que ya está decidido y limita este ADR:

- **ADR-0004:** SDL3 detrás de una interfaz propia (`platform::`). Lo que se use de SDL3
  pasa por ahí.
- **ADR-0009:** sin excepciones ni RTTI; en el código de cada frame no hay contenedores
  que asignen memoria.
- **ADR-0010:** regiones por sistema y por tiempo de vida, **reservadas y tocadas al
  iniciar**: si falta memoria, falla al arrancar y no a mitad de una horda. Ya hay un
  allocator de offsets tipo TLSF (para los pedazos) y bloques grandes de
  `gpu::Allocator` con una cola de liberación por número de frame. Ningún allocator se
  comparte entre hilos.
- **ADR-0011:** TDD estricto en el núcleo y en el cocinador.
- **ADR-0012:** un hilo, paso fijo de 60 Hz. **Excepciones al hilo único:** Tracy (solo
  en desarrollo) y el hilo de audio, y ninguna toca el estado del juego.
- **ADR-0016 y ADR-0021:** la misma partida grabada da el mismo hash. Lo que cambia la
  simulación durante una grabación se graba como evento del paso.
- **ADR-0017:** un binario propio por asset, con **secciones planas y offsets, sin
  punteros**, y una cabecera con firma y versión. **Sin compatibilidad hacia atrás:** si
  el formato cambia, todo se recocina. Texturas RGBA8 sin comprimir.
- **ADR-0020:** 60 fps en la Intel UHD 620; R4 = 1 GiB de memoria total. **No hay
  requisito de tiempo de carga.** Reservas del frame: 0,2 ms de CPU y 2,6 ms de GPU.
- **ADR-0022:** todos los pipelines se compilan al arrancar.
- **ADR-0024:** el hilo de audio no asigna memoria, no toma locks y no hace I/O. Un
  sonido no se libera mientras una voz lo lee.
- **ADR-0026:** la pantalla de carga aparece **solo al empezar y al volver a un
  checkpoint**. Las opciones que cambian la simulación se graban como evento del paso.
- **ADR-0027:** el mundo son zonas, cada una con su binario, y un binario del mundo. Los
  ids son estables (hash del camino). **El disco nunca decide nada de la simulación:**
  si una zona no llegó cuando hace falta, la simulación espera con 0 pasos. **El tope de
  zonas cargadas** (5, de partida) lo garantiza el cocinador, sin bloqueo. **Volver a un
  checkpoint** carga su zona detrás de la pantalla de carga.

**Fuera de alcance:**

| Qué | Lo decide |
|---|---|
| Qué opciones existen y cómo se ven en el menú | ADR-0026 (y ADR-0031 y ADR-0032 las suyas) |
| El remapeo de los controles | ADR-0032 |
| Qué se carga y cuándo (las distancias, el tope, el orden) | ADR-0027 |
| El formato de cada asset cocinado | ADR-0017 |
| El orden del frame | ADR-0012 |

## Criterios

Los cuatro del ticket, más seis que salen de cruzar lo ya decidido:

1. **Memoria con presupuesto** (ADR-0010, ADR-0020).
2. **Tiempos de carga.**
3. **Iteración rápida en el desarrollo:** cuánto pasa desde guardar un archivo hasta
   verlo en el juego.
4. **Robustez de las partidas guardadas:** un corte de luz o un cierre forzado nunca
   deja al jugador sin partida.
5. **Cargar nunca corta el frame** (R1 y R3 del ADR-0020).
6. **No romper el determinismo:** ni la carga ni la recarga cambian la simulación sin
   quedar en el *replay* (ADR-0016, ADR-0021).
7. **Las menos excepciones posibles al hilo único** (ADR-0012).
8. **Aprendizaje:** poder explicar cada pieza.
9. **Estándar de la industria.**
10. **Simple para una sola persona.**

## Opciones

Los números son de un prototipo medido en la máquina de referencia (ver Investigación).
Las lecturas en frío sacan el archivo de la caché de páginas antes de cada corrida.

### 1. Cómo se lee del disco

Una zona pesa de 3 a 12 MB en el prototipo. La pregunta es cuánto le cuesta al hilo
principal, por frame, mientras se lee.

| Opción | Cómo funciona | Medido (P1, en frío) | Problema |
|---|---|---|---|
| **Leer todo de una vez** | `read` de la zona entera en el hilo principal | Bloquea el frame lo que tarda el disco: 62 ms para 91 MB | Un frame cortado por cada zona |
| **Leer por partes en el hilo principal** | Un presupuesto de bytes por frame (256 KB, 1 MB, 4 MB) | De 1,6 a 5,6 ms por frame, con picos de 30 ms. Con la máquina recién salida de otra carga pesada, de 23 a 36 ms. En el recorrido (P8), con 1 MB por frame: p99 0,14 ms y máximo 7 ms | El disco frío no tiene un tiempo fijo por byte: el presupuesto en bytes no acota el tiempo, y queda expuesto al estado de la máquina |
| **`mmap`** | El archivo se mapea y se lee al usarlo | Las faltas de página caen en el hilo principal: de 2,6 a 7,9 ms por frame. `MADV_WILLNEED` no lo arregló | La espera queda escondida dentro de un acceso a memoria cualquiera |
| **Un hilo de I/O propio** | Un hilo que solo lee archivos a memoria ya reservada. Recibe pedidos y devuelve avisos por colas de un productor y un consumidor sin locks (como el audio) | **~0,05 ms por frame** en el hilo principal (solo mirar la cola) | Una tercera excepción al hilo único. No toca el estado del juego ni usa allocators |
| **io_uring en el hilo principal** | Pedidos al kernel y una mirada a los resultados por frame, sin hilo propio | Con lecturas comunes: de 1 a 7 ms por frame, porque el kernel copia desde la caché **dentro** del `submit`. Con `IOSQE_ASYNC` u `O_DIRECT`: **~0,05 ms** | Linux solamente (Windows tiene su propia API). Hay que conocer esas banderas: sin ellas es tan malo como leer en el hilo principal |
| **`SDL_AsyncIO`** (SDL 3.2 o más nuevo) | La API de SDL3: io_uring si está, si no un pool de hilos de SDL. Una mirada por frame a una cola de resultados | 0,013 ms en el p99, pero **el frame que pide las lecturas cuesta de 10 a 15 ms**, en frío y en tibio: con los datos en la caché, copia dentro del pedido, como io_uring sin `IOSQE_ASYNC` | La implementación no se controla. Los hilos, si los usa, son de SDL |

### 2. La unidad de carga y el formato en disco

| Opción | Cómo es | Medido | Problema |
|---|---|---|---|
| **Un archivo por asset** | Lo que ya cocina el ADR-0017 | En el recorrido (P8), con un archivo por zona: `open` y `close` en el hilo principal, de 0,02 a 0,07 ms cada vez. Con un archivo por asset, eso se multiplica por los assets de la zona | Muchas aperturas por zona |
| **Un archivo por zona** | El cocinador junta en un binario todo lo que la zona necesita | P8 lo usa: una lectura por zona | Lo compartido entre zonas: se repite en cada archivo o va aparte (dimensión 3) |
| **Un paquete para todo** (*pak*) | Un solo archivo con una tabla de contenidos, abierto al arrancar | No medido. Sin `open` por zona | Un cambio recocina el paquete entero, salvo que el desarrollo use archivos sueltos |
| **El paquete dentro del ejecutable** | El paquete va en una sección propia del ejecutable (con `.incbin`), y se lee del propio ejecutable | P10: con las 32 zonas (227 MB), enlazar tarda 3,7 s; el juego arranca en 3 ms con 4 MB en RAM, porque el kernel mapea el ejecutable y no lo lee entero | Tocar los datos como memoria es usar `mmap` (faltas de página en el hilo principal): hay que leerlos con `pread`. En desarrollo, reenlazar por cada asset es lento: archivos sueltos. Godot ("embed PCK") y LÖVE (juegos "fusionados") lo hacen |

**La compresión** (P3, las 32 zonas, 227 MB sin comprimir):

| Opción | En disco | Leer y descomprimir, en frío | Nota |
|---|---|---|---|
| **Sin comprimir** | 227 MB | 237 ms | El disco de la máquina de referencia lee ~1 GB/s |
| **LZ4** | 85 MB (×2,7) | 220 ms | Descomprime a ~2,2 GB/s |
| **zstd nivel 3** | 41 MB (×5,5) | 390 ms | Descomprime a ~770 MB/s: más lento que el disco |
| **zstd nivel 19** | 27 MB (×8,6) | 255 ms | Comprimir tardó 144 s |
| **La del sistema de archivos** | btrfs con zstd:3 (la de `/home` en esta máquina) | 226 ms | Depende de la máquina del jugador |

Las texturas del prototipo son sintéticas, con paleta chica: comprimen más que las
reales. En un NVMe, **comprimir ahorra disco, pero no tiempo**. En un disco lento, sí lo
ahorraría.

**Usar el binario tal como llega** (P2): validar la cabecera y que cada offset caiga
dentro del archivo cuesta **0,0003 ms por zona**. Copiar y convertir cada recurso (un
lector de campos, vértices a arreglos separados) cuesta **de 2 a 4 ms por zona**. El
formato del ADR-0017 ya permite lo primero: secciones planas con offsets.

### 3. La vida de los recursos compartidos

El modelo de la caballera, los tipos de zombie, la espada, las piezas comunes del
castillo y los sonidos del corte se usan en muchas zonas.

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Conteo de uso en runtime** | Cada recurso tiene un contador; se libera cuando llega a 0 | Liberaciones en cualquier orden, de tamaños distintos: fragmentación. Un contador mal llevado es una fuga o un uso después de liberar |
| **Copiar lo compartido en cada zona** | El cocinador mete en el archivo de cada zona todo lo que usa | Disco repetido, y memoria repetida cuando dos zonas vecinas están cargadas a la vez. *Spider-Man* duplicó datos en el disco para leer seguido, aunque por el tiempo de búsqueda de un disco rígido |
| **Un conjunto global residente, y lo propio por zona** | El cocinador ve el mundo entero: lo que usan los personajes o varias zonas va a un conjunto **global**, cargado al arrancar; lo que usa una sola zona va en su archivo | Lo global ocupa memoria siempre. Un asset que usan dos zonas lejanas también queda residente |

No se midió. El tamaño del conjunto global depende del contenido; el cocinador lo puede
calcular y reportar.

### 4. La memoria de las zonas

El ADR-0010 dejó esta pregunta: las zonas no se liberan en orden. P4 repitió las cargas y
descargas de 20 recorridos al azar en 20 castillos distintos, con las reglas del
ADR-0027, y buscó cuánta memoria necesita cada allocator para no fallar nunca.

| Opción | Cómo funciona | Medido (P4) | Problema |
|---|---|---|---|
| **Slots fijos** | Tantos lugares como el tope de zonas, cada uno del tamaño de la zona más grande. Una zona ocupa un slot entero | **De 59 a 84 MB. No puede fallar:** hereda la garantía del tope que da el cocinador (ADR-0027) | Se desperdicia lo que le falta a cada zona para ser la más grande. Una zona enorme agranda todos los slots |
| **El allocator de offsets tipo TLSF** (el de los pedazos) | Un montón; cada zona pide exactamente su tamaño | El mínimo sin fallar: **de 54 a 92 MB**; en 18 castillos de 20, **más que los slots**, por la fragmentación | Ese mínimo es empírico: un recorrido nuevo puede necesitar más. Si falla, ¿la zona espera? Nada garantiza que se libere un hueco del tamaño justo |
| **Buddy** | Bloques de potencias de dos | 128 MB en 19 de 20 | Redondear a potencias de dos duplica casi todo |
| **Slots virtuales** (solo la CPU) | Se reserva el espacio de direcciones de los slots y se toca solo lo que usa la zona; al descargar, `MADV_DONTNEED` | La memoria real es el pico de lo cargado: de 40 a 69 MB | La página se toca al cargar, no al arrancar: rompe la regla del ADR-0010. En la GPU no existe sin *sparse binding* |

La memoria de la GPU (mallas y texturas de la zona) tiene la misma forma: un bloque
grande de `gpu::Allocator` partido en slots, o un montón de offsets.

### 5. La subida a la GPU

La UHD 620 es integrada: la GPU y la CPU comparten la RAM, y **tiene una sola familia de
colas** (gráficos, cómputo y transferencia juntos), así que no hay una cola de
transferencia que corra en paralelo.

| Opción | Cómo funciona | Medido (P5, reloj libre) | Problema |
|---|---|---|---|
| **Staging y copia, con presupuesto por frame** | Los bytes llegan a un buffer de *staging* mapeado; el frame graba una copia a la memoria final (`vkCmdCopyBuffer`, `vkCmdCopyBufferToImage`) | **~0,3 ms de GPU por MB**, igual para buffers que para texturas | Un MB por frame son 0,3 ms de la reserva de GPU |
| **Escribir directo** en memoria de la GPU mapeada | En la integrada, toda la memoria es visible desde la CPU | 0 ms de GPU; ~0,2 ms de CPU por MB (`memcpy`), con picos | **Las texturas no:** necesitan *tiling* óptimo, y eso pide una copia. En una GPU dedicada, la memoria rápida no es visible desde la CPU |
| **Leer directo al staging** | El hilo de I/O lee del disco al buffer de *staging* mapeado | No medido por separado: es la lectura de P1 con otro destino | El *staging* tiene que vivir hasta que la copia termine en la GPU (la cola por número de frame del ADR-0010) |

Una zona de 8 MB con 2 MB por frame sube en 4 frames (67 ms). La precarga de 25 m del
ADR-0027 deja varios segundos.

### 6. El arranque y la pantalla de carga

**Los pipelines** (ADR-0022: todos al arrancar). P6b, con los pipelines de la demo del
prototipo:

| Opción | Medido |
|---|---|
| Sin ningún caché | **1,3 s** |
| Con el caché propio del driver (Mesa lo guarda en `~/.cache`) | 2,9 ms |
| **Con un `VkPipelineCache` propio en disco** (205 KB) | **0,55 ms** |

El caché del driver existe en Mesa, pero no está garantizado en otros drivers. El
`VkPipelineCache` es estándar, pero el driver puede rechazar uno de otra versión o de
otra GPU, y algunos drivers no validan los datos: hay que guardar una cabecera propia
con el `vendorID`, el `deviceID`, el `pipelineCacheUUID` y un hash (Kapoulkine, 2019).

**Qué muestra la pantalla de carga** (ADR-0026: solo al empezar y al volver a un
checkpoint): en P8, leer en frío la zona del checkpoint y las que necesita la
simulación tardó de 8 a 15 ms de disco real. Con un disco lento inyectado (hasta 4,5 s
por zona), la pantalla duró hasta 2,4 s.

| Opción | Problema |
|---|---|
| **Una pantalla fija** (fondo y texto) | Ninguno técnico. Es lo mínimo |
| **Una barra de progreso** | Los bytes pedidos se conocen; el tiempo no |
| **Un consejo o una ilustración** | Contenido y traducción (ADR-0026) |

**La espera por una zona** (ADR-0027: la simulación corre 0 pasos): mostrar el frame
quieto, o un indicador chico después de un tiempo (por ejemplo, 0,5 s), como el ícono
de guardado de muchos juegos.

### 7. La recarga en caliente

P6 hizo la recarga en el renderer del prototipo del ADR-0025, con un guion que cambia
archivos mientras corre. **Limpio bajo las capas de validación.**

| Qué se recarga | Cómo se cambia sin romper un frame en vuelo | Medido, desde guardar hasta verlo |
|---|---|---|
| **Shader** | Pipeline nuevo; el viejo se destruye cuando ningún frame en vuelo lo usa | **~0,25 a 0,37 s:** glslc ~0,22 s; crear el pipeline, 115 a 135 ms en el hilo principal (0,3 ms si el driver lo tiene en caché) |
| **Textura** | Imagen nueva; cada set de descriptores se actualiza al empezar su próximo frame, y la vieja se destruye dos frames después | **~50 ms** |
| **Malla** | Una región nueva del buffer; la vieja se libera cuando ningún frame la lee | **~60 ms** (casi todo es la herramienta que la escribe) |

**Cómo se detecta un cambio:**

| Opción | Problema |
|---|---|
| **inotify** (Linux) sobre la carpeta de lo cocinado | Solo Linux. El cocinador tiene que escribir a un temporal y renombrar, o el juego lee un archivo a medias |
| **Mirar las fechas** cada N frames | Un `stat` por archivo, y una demora de N frames |
| **Un botón en el panel de ImGui** | Manual |

**Quién cocina:** el autor corre el build (como ahora), o una herramienta de `tools/`
mira las fuentes y cocina al guardar.

**El problema del determinismo:** un shader o una textura no cambian la simulación.
**Una malla sí:** el corte (ADR-0007) corta esa misma malla en la CPU, y las cápsulas de
la física salen de ella (ADR-0017). Las tablas (sonidos, VFX, textos) no la cambian; los
datos del mundo (colisión, navmesh, fases), sí. Lo que cambia la simulación, mientras se
graba un *replay*, se graba como evento del paso o se prohíbe recargarlo.

### 8. La partida guardada

**Qué se guarda** (ADR-0027): el mundo, la zona y la fase del checkpoint y qué fases
terminaron. En el prototipo, en texto: 180 bytes.

**El formato:**

| Opción | Cómo es | Problema |
|---|---|---|
| **Texto `clave = valor`** con versión y CRC | Una clave por línea, la versión al principio y un CRC-32 al final. Las claves que faltan toman su valor por defecto, y las desconocidas se ignoran | Más grande, aunque son 180 bytes. Se puede editar a mano (el CRC lo detecta) |
| **Binario** con cabecera, versión y CRC | Una struct | Migrar pide un caso por versión. No se lee con un editor |

P7 probó el texto: una partida v1 la carga el juego v2 (la clave nueva toma 0), una v2
la carga el v1 (ignora la clave nueva), y un byte cambiado se detecta.

**Cómo se escribe** (P7: 1 000 cierres con `kill -9` en un momento al azar, mientras se
guarda sin parar):

| Opción | Partidas perdidas | Costo por guardado | Problema |
|---|---|---|---|
| **Escribir encima** | **522 de 1 000:** el jugador se queda sin nada | 0,2 ms | Truncar y escribir deja un archivo vacío o a medias |
| **Temporal y renombrar** | 0 | 0,35 ms | `rename` es atómico, pero sin `fsync` un corte de luz puede dejar el archivo nuevo vacío en algunos sistemas de archivos |
| **Temporal, `fsync`, renombrar y `fsync` de la carpeta** | 0 | **21 ms de mediana, 655 ms de máximo** | Lo más seguro ante un corte de luz (Pillai et al., 2014). No puede ir en el hilo principal |
| **Dos archivos A y B** | 0 (5 corruptas detectadas, cargó la otra) | 12 ms (con `fsync`) | Se pisa el más viejo y gana el más nuevo válido. Dos archivos y un número de secuencia |

`kill -9` simula un cierre forzado, no un corte de luz: eso no se puede probar sin
privilegios. Para el corte de luz vale la literatura.

**Dónde vive** (la idea del autor: una zona reservada, con datos muy concretos y un bit
que dice si hay partida):

| Opción | Cómo es | Medido (P10) | Problema |
|---|---|---|---|
| **Dentro del ejecutable** | Una sección reservada del ejecutable guarda la partida | Linux no deja abrir para escribir el ejecutable que corre ("Text file busy"). Copiarlo, cambiar la sección, `fsync` y renombrar funciona: el próximo arranque lee la partida. Con los assets adentro (227 MB): **de 7,3 a 8,7 s por guardado** | Una actualización del juego reemplaza el ejecutable y borra la partida; "verificar archivos" de una tienda la restaura al original. Las instalaciones de solo lectura (Flatpak, AppImage, `/usr/bin`) no pueden guardar. Los antivirus desconfían de un ejecutable que se modifica |
| **Un archivo de texto** en la carpeta de datos | La partida de P7 | 180 bytes | Ninguno técnico |
| **Una tarjeta de memoria** en la carpeta de datos | Un archivo binario de **tamaño fijo**, con un diseño a nivel de bytes y de bits, como la SRAM de un cartucho o un bloque de la tarjeta de la PS1 (8 KB). Dos bloques A y B: se pisa el más viejo y gana el más nuevo válido | El esquema A/B de P7: 0 partidas perdidas, 12 ms por guardado | Migrar pide reglas para los campos nuevos (los bytes reservados) |

**Cuándo se guarda:** solo al llegar a un checkpoint (lo que se guarda es exactamente
eso), o también al salir.

**Cuántas partidas:** una sola, o varios espacios (*slots*).

**Si el mundo cambió** (una versión nueva del juego): los ids son estables (ADR-0027),
así que una partida de otro mundo se puede validar zona por zona; si la zona del
checkpoint ya no existe, se vuelve al checkpoint anterior que exista, o al principio.

**Dónde:** `SDL_GetPrefPath`, que en Linux sigue la especificación XDG
(`~/.local/share/<organización>/<juego>/`).

### 9. Las opciones del jugador

| Opción | Cómo es | Problema |
|---|---|---|
| **Texto `clave = valor`**, como la partida, en `~/.config/<juego>/` (XDG) | El jugador lo puede leer y editar. Un valor fuera de rango se lleva al rango válido, y uno desconocido se ignora | Hay que validar cada valor al leer |
| **Junto con la partida** | Un solo archivo | Borrar la partida borra las opciones, y al revés |
| **Binario** | — | No se edita a mano, que es lo que un jugador de PC espera poder hacer |

El caché de pipelines no es una opción ni una partida: va en `~/.cache/<juego>/` (XDG),
y se puede borrar sin perder nada.

### 10. Presupuesto, pruebas y depuración

| Qué | Lo que se puede decidir |
|---|---|
| Tiempo de carga | Una meta para el arranque y la vuelta al checkpoint. El ADR-0020 no tiene requisito |
| Memoria | Una fila de `presupuestos.md` para los slots de zona (CPU y GPU) y el conjunto global |
| CPU y GPU | La carga, en el hilo principal: menos de 0,01 ms por frame con un hilo o io_uring (P8). La subida: un presupuesto en MB por frame, de la reserva de GPU |
| Pruebas | El cargador valida la cabecera y los offsets (TDD, con archivos rotos a propósito); latencias al azar con el mismo hash (ya en el ADR-0027); el guardado con cierres inyectados; partidas viejas de cada versión como datos de prueba |
| Depuración | Un panel de ImGui: los pedidos (tamaño y cuánto tardaron), los slots, la subida por frame y las recargas |

## Investigación

### Prototipo medido

Código en C++20 con liburing, LZ4, zstd y SDL3, y la copia del renderer del prototipo
del ADR-0025 (Vulkan), medidos en la máquina de referencia el 2026-11-03. El detalle y
cómo repetirlo están en
[`../research/recursos-en-runtime-y-guardado.md`](../research/recursos-en-runtime-y-guardado.md).
Un video de 68 s muestra el recorrido del castillo y la recarga en caliente.

| # | Qué | Resultado |
|---|---|---|
| P1 | Cómo se lee, en frío | Costo por frame en el hilo principal: hilo de I/O ~0,05 ms; io_uring con `IOSQE_ASYNC` u `O_DIRECT` ~0,05 ms; io_uring común de 1 a 9 ms; por partes de 1,6 a 36 ms según el estado de la máquina; `mmap` de 2,6 a 28 ms; `SDL_AsyncIO`, de 10 a 15 ms en el frame que pide |
| P2 | En el lugar o parseado | 0,0003 ms contra 2 a 4 ms por zona |
| P3 | Compresión | Sin comprimir 237 ms, LZ4 220 ms, zstd-3 390 ms, para 227 MB en frío |
| P4 | Memoria de las zonas | Slots fijos de 59 a 84 MB, sin poder fallar; TLSF de 54 a 92 MB (más que los slots en 18 de 20); buddy 128 MB |
| P5 | Subida a la GPU | ~0,3 ms de GPU por MB. Una sola familia de colas |
| P6 | Recarga en caliente | Shader ~0,3 s, textura ~50 ms, malla ~60 ms. Sin errores de validación |
| P6b | Pipelines al arrancar | Sin caché 1,3 s; `VkPipelineCache` 0,55 ms; caché de Mesa 2,9 ms |
| P7 | Guardado | Escribir encima: 52 % de partidas perdidas. Temporal y renombrar: 0. `fsync`: 21 ms de mediana, 655 ms de máximo |
| P8 | Recorrido del castillo | 16 corridas con tres métodos y latencias al azar: **el mismo hash**. Con hasta 4,5 s por zona, la simulación esperó 107 y 59 frames. Sin esperar, el hash cambió |
| P9 | Video | El recorrido, la lectura en el hilo principal, la espera, la variante sin espera, la vuelta al checkpoint y la recarga en caliente |
| P10 | El ejecutable | Con 227 MB adentro: enlazar 3,7 s, arrancar 3 ms con 4 MB en RAM. Guardar dentro del ejecutable: "Text file busy" al abrirlo; copiar y renombrar funciona, de 7,3 a 8,7 s por guardado |

Lo que mostró, además de los números:

- **Leer no es el problema; esperar en el hilo principal, sí.** El disco lee ~1 GB/s:
  una zona tarda milisegundos. Lo que corta el frame es esperar a ese disco desde el
  hilo principal, y eso pasa con cualquier método que lea ahí, aunque sea por partes.
- **io_uring no es asíncrono por sí solo.** Con lecturas comunes, el kernel intenta la
  lectura dentro del `submit` y copia desde la caché ahí mismo.
- **Usar el binario tal como llega deja la carga en I/O puro.** El ADR-0017 ya lo
  permite.
- **Los slots fijos ganan porque el tope ya existe.** El cocinador garantiza que nunca
  hacen falta más zonas que el tope; con un slot por zona, esa garantía cubre también la
  memoria. Un montón general necesita un margen por la fragmentación, y ni así lo
  garantiza.
- **La recarga en caliente es la cola de liberación por frame que ya existe.** Lo viejo
  se retira cuando ningún frame en vuelo lo usa, igual que los pedazos (ADR-0010).
- **Guardar con `fsync` puede tardar más de medio segundo.** Aunque la partida pese 180
  bytes, no puede ir en el hilo principal.
- **Una malla no es solo visual en este juego:** la corta el corte y de ella salen las
  cápsulas.

### Cómo lo hacen otros

- ***Game Engine Architecture*** (Gregory, 3.ª ed., cap. 7): un *resource manager* con
  ids únicos, conteo de uso, archivos empaquetados, carga en el lugar con tablas de
  corrección de punteros, y memoria por tiempo de vida (pilas por nivel, *chunks*).
  Describe los recursos que se cargan y quedan residentes (*load-and-stay-resident*) en
  los juegos de Naughty Dog.
- **Unity:** sube texturas y mallas con un presupuesto de milisegundos por frame
  (`asyncUploadTimeSlice`) y un buffer circular (`asyncUploadBufferSize`), en el hilo de
  render.
- ***Marvel's Spider-Man*** (Ruskin, GDC 2019): el streaming de Manhattan y entrar en el
  Blu-ray. La torre de Fisk era demasiado grande para la memoria junto con la ciudad, y
  la cámara escondió el cambio.
- **SDL3** (`SDL_AsyncIO`, desde la 3.2): I/O asíncrona con una cola por frame, con
  io_uring en Linux o un pool de hilos.
- **io_uring** (Axboe, 2019): colas de pedidos y de resultados compartidas con el
  kernel. `IOSQE_ASYNC` manda el pedido directo a los workers del kernel.
- **El `VkPipelineCache` robusto** (Kapoulkine, 2019): una cabecera propia con el
  dispositivo, el driver y un hash, porque algunos drivers no validan los datos; escribir
  a un temporal y renombrar.
- **Guardar sin perder datos:** Pillai et al. (OSDI 2014) encontraron errores de
  consistencia ante cortes en aplicaciones muy usadas, por suponer cosas del sistema de
  archivos. El protocolo portable es temporal, `fsync`, `rename` y `fsync` de la
  carpeta (Luu, 2015; el caso de ext4 en 2009).
- **TLSF** (Masmano et al., 2004): el allocator O(1) de tiempo real que el ADR-0010 ya
  eligió para los pedazos.
- **El paquete dentro del ejecutable:** Godot puede exportar el paquete del juego dentro
  del ejecutable ("embed PCK"), y LÖVE arma un juego "fusionado" pegando el paquete al
  intérprete.
- **Los cartuchos y la PS1:** los cartuchos guardaban en una SRAM chica con batería,
  dentro del mismo cartucho. La tarjeta de memoria de la PS1 tenía 128 KB en 16 bloques
  de 8 KB, y el primero era el directorio: una partida ocupaba bloques de tamaño fijo.

## Decisión

Elijo que **el hilo principal nunca espere al disco**: un hilo propio lee y escribe, y el
juego solo mira una cola. Lo que se carga es un binario por zona que se usa tal como
llega, en lugares fijos de memoria que hereda la garantía del tope del ADR-0027: si el
cocinador dice que el tope alcanza, la memoria también alcanza. En Release, todo eso va
dentro del ejecutable, para que el juego sea un solo archivo. Mientras trabajo, todo lo
visual se recarga en caliente, y lo que cambia la simulación corta la grabación, porque
en ese momento no estoy buscando un bug. La partida es **una tarjeta de memoria**, como
la de la PS1: un archivo chico, de tamaño fijo, que diseño byte por byte, con un bit que
dice si hay partida. No vive dentro del ejecutable, porque una actualización la
borraría; vive donde cada sistema guarda los datos del usuario.

| # | Dimensión | Elección |
|---|---|---|
| 1 | Cómo se lee | **Un hilo de I/O propio: la tercera excepción al hilo único** (ADR-0012), con el contrato del hilo de audio: **no toca el estado del juego, no usa allocators y solo escribe en memoria reservada al iniciar.** Abre, lee (`pread` en bloques de 1 MB) y escribe (la partida y las opciones, con su `fsync`, y **los archivos de depuración del ADR-0021**: el log, el registro por frame y los bloques del *replay*, desde buffers reservados al iniciar; en los builds de desarrollo, también el archivo de CVars cambiadas del ADR-0021 y los valores que el panel del mundo escribe en el texto del mundo, ADR-0027, que son los únicos que escriben dentro del repo). **Dos excepciones a "nunca espera":** la etapa cero del arranque (Decisión 6) y un error fatal, en el que el hilo principal espera a que el hilo de I/O vacíe el log antes de abortar. Recibe pedidos y devuelve avisos por **dos colas de un productor y un consumidor sin locks**; sin pedidos, duerme. Atiende los pedidos en el orden en que llegan: el orden lo decide el hilo principal (la zona más cercana primero, ADR-0027). **El hilo principal mira los avisos una vez por frame**, al empezar el frame. **Dónde vive:** las primitivas de archivo (`open`, `pread`, `pwrite`, `fsync`, `rename`) y la de crear un hilo están en `platform::` (ADR-0004). **Son dos piezas:** el hilo de I/O y sus colas, encima de `engine/core` (no conocen la GPU), y **el cargador** (los slots, la validación y la subida), un sistema encima de `engine/gpu` |
| 2 | Unidad y formato | **Un binario por zona y uno global**, que arma el cocinador con los assets cocinados (ADR-0017). Cada uno tiene **dos partes: la de la CPU** (instancias, colisión, navmesh, marcadores, fases, triggers, sonidos y, por la fe de erratas del ADR-0034, cinemáticas) **y la de la GPU** (vértices, índices y texturas). Secciones planas con offsets, **usadas tal como llegan:** al llegar, se valida la cabecera y que cada offset caiga dentro del archivo. **Sin compresión;** la tabla de secciones guarda el codec (0 = crudo) para sumarla si el tamaño en disco lo pide. **En Release, un paquete** (el mundo, el global, las zonas, el binario de arranque y el de la música, con una tabla de contenidos) **va dentro del ejecutable**, en una sección propia; `platform::` sabe dónde empieza esa sección en el archivo, y **el hilo de I/O la lee del propio ejecutable con `pread`**, nunca como memoria. **En desarrollo, archivos sueltos** en la carpeta de lo cocinado |
| 3 | Vida de lo compartido | **El cocinador reparte, el runtime no cuenta.** Va al **global** lo que usan los personajes (la caballera, los tipos de zombie, la espada), la UI, los VFX y los sonidos comunes, y **todo asset que use más de una zona**; se carga al arrancar y vive todo el programa. Lo que usa una sola zona va en su binario. **La música es la excepción:** suena entre zonas con *crossfade*, así que va en **su propio binario**, que se carga al arrancar a la región de audio (ADR-0024). **Sin conteo de uso en runtime.** El cocinador reporta los tamaños y **falla** si una zona no entra, **revisando dos medidas**: el binario entero contra el slot de la CPU y la parte de la GPU contra el slot de la GPU. Falla también si la parte de la CPU del global **más la del binario de arranque** pasa el presupuesto de CPU del global, o si la de la GPU (también sumadas) pasa el de GPU. **Los tamaños se fijan en el texto del mundo** (ADR-0027), la misma fuente que el tope de zonas |
| 4 | Memoria de las zonas | **Slots fijos:** tantos como el tope del binario del mundo (ADR-0027), cada uno del **tamaño fijado en el texto del mundo**, uno para la CPU y otro para la GPU (de partida, 24 MiB y 24 MiB; el global, 64 MiB y 128 MiB; hipótesis de `presupuestos.md`). El cocinador los copia al binario del mundo, reporta la zona más grande y falla si una no entra: una zona enorme no agranda los slots sin que nadie lo note, sino que pide partirla o subir el tope a conciencia. **En la CPU**, una región de slots en **memoria de Vulkan visible desde la CPU y con caché** (`HOST_VISIBLE`, `HOST_COHERENT` y `HOST_CACHED`), mapeada siempre: la parte de la CPU se usa ahí mismo, y la parte de la GPU sirve de origen de la copia. **En la GPU**, un bloque grande de `gpu::Allocator` partido en slots iguales, para la parte de la GPU. Los dos se **reservan y se tocan al iniciar** (ADR-0010). Una zona ocupa un slot entero de cada uno, y se libera con la cola por número de frame. **El global no es igual:** su región de la CPU guarda **solo su parte de la CPU**, y su bloque de la GPU, su parte de la GPU. Esa parte de la GPU **sube al arrancar** detrás de la pantalla de carga, **a través de los slots de zona de la CPU**, que en ese momento todavía están libres: el hilo de I/O lee un trozo a un slot, la GPU lo copia, y se repite. Así el global no necesita un *staging* propio |
| 5 | Subida a la GPU | **El slot de la CPU es el *staging*:** el hilo de I/O lee la zona entera ahí, y el frame graba copias (`vkCmdCopyBuffer`, `vkCmdCopyBufferToImage`) del slot de la CPU al de la GPU, **con un presupuesto de 2 MB por frame** (~0,6 ms de GPU), antes de las pasadas del ADR-0022. Sin buffer de *staging* aparte y sin `memcpy` en el hilo principal. **Una zona está cargada** (ADR-0027) **cuando llegó del disco y su subida terminó** en la GPU |
| 6 | Arranque y pantalla de carga | **La etapa cero, antes del primer frame:** `opciones.txt` (para saber el idioma), un **binario de arranque** chico y su pipeline. **Es uno solo para todos los idiomas:** la fuente, "Cargando…" en cada idioma y el fondo de la pantalla de carga. La fuente queda repetida con la del global a propósito, para no esperar al global. **Vive dentro de la región y del bloque del global**, al principio, todo el programa (la pantalla de carga vuelve en cada checkpoint), comparte el rango de índices del global en el arreglo de texturas y cuenta dentro de su presupuesto; su parte de la GPU sube en la etapa cero por un slot de zona, que todavía está libre. Los lee el hilo de I/O; el hilo principal espera, porque todavía no hay simulación ni frame. **Después, detrás de la pantalla de carga:** el binario del mundo, el global (su parte de la GPU, a través de los slots de zona, Decisión 4), la música, los pipelines y la zona de la partida (o de la partida nueva) con la regla normal de carga. **El `VkPipelineCache`** vive en la carpeta de caché, con **una cabecera propia**: firma, versión, `vendorID`, `deviceID`, `driverVersion`, `pipelineCacheUUID`, tamaño y hash de los datos. Si algo no coincide, se empieza con uno vacío. Se escribe después de crear los pipelines, con temporal y `rename`, en el hilo de I/O. **La pantalla de carga es fija:** el fondo y "Cargando…" en el idioma elegido (ADR-0026), sin barra. **Si la simulación espera a una zona** (ADR-0027), el frame queda quieto; **pasados 0,5 s, aparece un indicador chico** en una esquina |
| 7 | Recarga en caliente | **Solo en los builds de desarrollo.** `platform::` mira la carpeta de lo cocinado con **inotify**, una vez por frame. **El cocinador escribe cada archivo a un temporal y lo renombra.** Cocina el build, como siempre. Se recargan **los shaders** (pipeline nuevo), **las texturas** (también las de los VFX y el atlas de las fuentes), **las mallas**, **la tabla de sonidos** (ADR-0024) y **las tablas de textos** (ADR-0026). Lo recargado va a una **región de recarga, solo de desarrollo**, porque puede no caber en el slot; vive hasta que su zona se descarga. **Lo viejo se retira con la cola por número de frame.** **Recargar algo que cambia la simulación** (una malla: el corte la corta y de ella salen las cápsulas) **corta la grabación del *replay*:** el archivo se cierra en ese paso, con una nota en el registro (ADR-0021) |
| 8 | La partida | **Una tarjeta de memoria:** `tarjeta.bin`, de **8 KB fijos**, en la carpeta de datos (`SDL_GetPrefPath`). Tiene **dos bloques de 4 KB, A y B.** Cada bloque: firma, versión del formato, número de secuencia, **banderas (el bit 0 dice si hay partida)**, el hash del mundo, la zona y la fase del checkpoint (**la fase, por su índice estable**, el mismo que usan los bits), **las fases terminadas como 256 bits**, bytes reservados en cero y **un CRC-32 al final**. **Cada bit es una fase, según un índice estable** que vive en **el texto del mundo**: una lista de fases a la que solo se le agregan filas. Una fase nueva toma el siguiente número, y una borrada queda en la lista como retirada, así que su número no se reusa y agregar una zona no corre los bits. El índice vive en la fuente versionada y no en lo cocinado, porque el cocinador no recuerda las fases borradas (cocinar dos veces da los mismos bytes). **Cada fila nombra la fase por el id de su zona y un nombre de fase** escrito en el texto de la zona, nunca por su número de orden: insertar una fase en medio de una zona no corre nada. El cocinador lo copia al binario del mundo y falla si una fase no tiene fila, si una fila no retirada no tiene fase, si un número se repite o si la lista pasa de 256. Todo en *little-endian*, con el diseño fijado por `static_assert`. **Guardar es pisar el bloque más viejo** (`pwrite` y `fsync`, en el hilo de I/O); **cargar es leer los dos y quedarse con el válido de secuencia más alta.** La primera vez se crea entera, con los dos bloques vacíos, con temporal y `rename`. **Una sola partida:** cada checkpoint pisa la anterior, y **no se guarda nada al salir.** Volver a un checkpoint al morir no escribe. **Versiones:** un campo nuevo solo ocupa bytes reservados, y una versión vieja lo lee como cero. **Si el mundo cambió** y la zona del checkpoint ya no existe, se vuelve al checkpoint anterior que exista en el orden del mundo, o al principio |
| 9 | Las opciones | **`opciones.txt` en la carpeta de configuración** (`$XDG_CONFIG_HOME/<juego>/`, por defecto `~/.config/<juego>/`), separado de la partida. **Texto `clave = valor`**, UTF-8, una por línea, con la versión primero y sin CRC, porque el jugador lo puede editar. Al leer: una clave desconocida se ignora, una que falta toma su valor por defecto, un valor fuera de rango se lleva al rango y una línea rota se ignora y se registra (ADR-0021). Se escribe al salir de la pantalla de opciones, con temporal, `fsync` y `rename`, en el hilo de I/O. **Las carpetas de configuración, de caché y de estado las arma `platform::`** con la especificación XDG, porque SDL3 solo da la de datos. **Los archivos de depuración** (el log, el registro por frame y los *replays*, ADR-0021) van en la de estado: `$XDG_STATE_HOME/<juego>/`, por defecto `~/.local/state/<juego>/` |
| 10 | Presupuesto, pruebas y depuración | **Metas (hipótesis):** del arranque al juego, ≤ 3 s; volver a un checkpoint, ≤ 1 s (en frío, en la máquina de referencia). **Memoria:** los slots y el global caben en las filas de assets y de recursos de la GPU de `presupuestos.md`; la región de recarga, en la de herramientas de depuración. **GPU:** la subida, 0,6 ms por frame, de la reserva. **Pruebas:** **TDD estricto** (ADR-0011) en el cargador (la validación, con archivos rotos a propósito), en la tarjeta de memoria (el formato, el CRC, los bloques A y B, y tarjetas de cada versión como datos de prueba), en los slots (memoria del núcleo) y en las dos colas sin locks del hilo de I/O (como la del audio, ADR-0024); **tests después** en la recarga en caliente; un test que **mata el proceso mientras guarda** y exige una partida válida; **un test del paquete de Release**, que enlaza un paquete chico en un ejecutable de prueba y lo lee por `platform::`, y corre en `debug`, donde la CI corre los tests; las latencias al azar con el mismo hash (ADR-0027). **Depuración:** un panel de ImGui de la carga (los pedidos en vuelo y lo que tardó cada uno, los slots y su zona, la subida por frame y las recargas), y zonas de Tracy en el hilo de I/O |

## Consecuencias

### El camino de una zona

1. **El cocinador** arma el binario de la zona, con su parte de la CPU y la de la GPU,
   y lo valida contra el tamaño del slot. En Release, lo mete en el paquete del
   ejecutable.
2. **El hilo principal**, con la regla del ADR-0027, decide pedirla: le asigna un slot
   de la CPU y otro de la GPU, y deja el pedido en la cola.
3. **El hilo de I/O** la lee al slot de la CPU (del archivo suelto o del ejecutable) y
   deja el aviso.
4. **El hilo principal**, al empezar el frame, ve el aviso y valida la cabecera y los
   offsets (~0,0003 ms).
5. **La subida**, 2 MB por frame, copia la parte de la GPU de un slot al otro. Cuando
   la GPU termina, la zona está cargada.
6. **La simulación** la hace entrar cuando la caballera cruza la distancia de entrada
   (ADR-0027).
7. **Al descargarla,** el sistema de audio para las voces que leen sus sonidos
   (ADR-0024). El slot de la GPU se libera con la cola por número de frame. **El de la
   CPU espera dos condiciones:** que pasen los frames en vuelo **y** que el reloj del
   audio pase el bloque en que se aplicó la orden de parar. La cola por frame sola (~33
   ms) no alcanza, porque el audio tiene más latencia que eso, y el hilo de I/O podría
   pisar un sonido que el callback todavía lee.

### Lo que se gana

- **Cargar no corta el frame:** el hilo principal paga ~0,001 ms por frame (P8), con
  cualquier disco y con la máquina cargada (P1).
- **La memoria no puede fallar en medio del juego:** está reservada al iniciar, y la
  garantía del tope cubre también los slots.
- **Cero copias en el hilo principal:** el disco escribe en el slot, y la GPU copia de
  ahí.
- **Un solo archivo en Release,** sin assets de otra versión al lado.
- **Iterar lo visual cuesta menos de medio segundo** (P6), sin cerrar el juego.
- **La partida no se pierde** ante un cierre forzado (P7) ni con una actualización del
  juego, y su formato se puede explicar byte por byte.

### Lo que se sacrifica

- **Una tercera excepción al hilo único.** Se acota con el mismo contrato del audio,
  pero hay que poder explicar la cola sin locks.
- **Memoria de más:** cada slot mide el tope fijado aunque la zona sea chica, y cada
  zona ocupa la parte de la GPU dos veces mientras está cargada (en el slot de la CPU y
  en el de la GPU). Una zona que no entra hace fallar el build: hay que partirla o subir
  el tope, y subirlo agranda todos los slots. Conviene que las zonas tengan tamaños
  parecidos.
- **Lo global ocupa siempre,** incluido un asset que usan dos zonas lejanas.
- **Sin compresión,** el juego ocupa más en disco.
- **Dos caminos de carga:** el paquete del ejecutable en Release y los archivos sueltos
  en desarrollo. Hay que probar los dos.
- **Recargar una malla corta la grabación,** y crear un pipeline nuevo frena el hilo
  principal ~120 ms (solo en desarrollo).
- **Una sola partida:** empezar de nuevo pisa la anterior.
- **El *replay* en medio de una sesión sigue abierto** (ADR-0021): la tarjeta guarda un
  checkpoint, no el estado completo.

### Tickets, en orden

1. el formato del paquete y el cargador, con su validación (TDD estricto);
2. el hilo de I/O y sus dos colas;
3. las regiones de slots en la CPU y los bloques de la GPU, y la cola de liberación de
   las zonas;
4. la subida con presupuesto por frame. Cada slot reserva **un rango fijo de índices**
   del arreglo de texturas *bindless* (ADR-0022), y hay otro para el global y otro para
   la región de recarga. Las imágenes de una zona se ligan dentro del bloque de su slot,
   respetando la alineación que pide el driver, sin una asignación de Vulkan por recurso;
5. el cocinador: los binarios por zona y el global, el de arranque y el de la música,
   el reparto, la lista de fases del texto del mundo, los reportes y sus fallas (TDD
   estricto, ADR-0017);
6. el paquete dentro del ejecutable en Release, su lectura desde `platform::` y su
   test con un paquete chico, en `debug`;
7. el arranque: el `VkPipelineCache` con su cabecera y la pantalla de carga;
8. la tarjeta de memoria (TDD estricto) y el test que mata el proceso;
9. las opciones y las carpetas XDG de `platform::`;
10. la recarga en caliente: inotify, la región de recarga y el corte de la grabación;
11. el panel de ImGui, las zonas de Tracy y las metas de carga medidas.

### Fe de erratas en otros ADRs

Al aceptar este ADR se corrigieron, como fe de erratas (ADR-0014, regla 11):

- **ADR-0012:** el hilo de I/O es la tercera excepción al hilo único (Decisión 5 y
  Consecuencias), encima de `engine/core`, y el cargador es un sistema encima de
  `engine/gpu`; el hilo también escribe los archivos de depuración. El orden del frame suma, al empezar, mirar los avisos de la
  carga, y antes de las pasadas, la subida.
- **ADR-0010:** las filas de las zonas pasan a ser slots fijos del tamaño fijado en el
  texto del mundo, en la CPU sobre memoria de Vulkan visible y con caché; las filas del
  global (solo su parte de la CPU; la de la GPU sube por los slots de zona) y de la
  región de recarga (solo de desarrollo). El hilo de I/O no usa allocators. La Decisión
  2 ya no deja la memoria de un nivel para después.
- **ADR-0017:** el cocinador arma los binarios por zona y el global, con sus dos partes,
  reparte los assets, lee del texto del mundo el slot y el presupuesto del global y
  falla si no entran (dos medidas por zona), copia el índice estable de cada fase desde
  la lista del texto del mundo (hasta 256) y calcula el peor caso de polígonos para A*.
  Arma también el binario de arranque y el de la música. Escribe con temporal y
  `rename`. El paquete de Release va dentro del ejecutable.
- **ADR-0022:** dónde vive el `VkPipelineCache` y su cabecera; la subida, antes de las
  pasadas; un rango fijo de índices del arreglo de texturas por slot (en Consecuencias).
- **ADR-0024:** los sonidos de una zona viven en su slot, y los comunes en el global,
  sin copiarse; la música queda en la región de audio, cargada al arrancar; la tabla se
  recarga con inotify (también en la Decisión 9).
- **ADR-0025:** las texturas de los VFX van en el global y se recargan como las demás.
- **ADR-0026:** la pantalla de carga es fija; el indicador de espera aparece a los
  0,5 s; "Continuar" solo aparece si la tarjeta tiene el bit de partida, y "Nueva
  partida" pide confirmar si lo tiene.
- **ADR-0027:** una zona está cargada cuando llegó del disco y terminó su subida; el
  checkpoint se guarda en la tarjeta de memoria; el texto del mundo suma el slot y el
  presupuesto del global, y la lista de fases con su índice estable (Decisiones 3 y
  10); el botón del panel del mundo escribe por el hilo de I/O.
- **ADR-0021:** recargar algo que cambia la simulación corta la grabación; dónde viven
  las opciones; el log, el registro por frame, el *replay* y el archivo de CVars los
  escribe el hilo de I/O; un error fatal espera a que se vacíe el log.
- **ADR-0004:** `platform::` suma las primitivas de archivo y de hilo, las carpetas XDG
  (configuración, caché y estado), inotify y la lectura del paquete del propio
  ejecutable.
- **ADR-0006:** el preset `release` enlaza el paquete (con el binario de arranque y el
  de la música) dentro del ejecutable.
- **ADR-0023:** la memoria de A* se reserva al iniciar, con el peor caso de polígonos
  entre las zonas cargadas a la vez; la navmesh de cada zona vive en su slot.
- **La visión:** las opciones y los checkpoints, en Abierto (sin cambio de versión: no
  cambia el diseño).
- **`presupuestos.md`:** las metas de carga (hipótesis), la subida en la GPU (de la
  reserva), los tamaños de partida del slot y del global (hipótesis), los sonidos
  comunes en el global, la región de recarga en la fila de las herramientas de
  depuración y el historial.

### Lo que queda abierto

- **El tamaño real de las zonas,** y con él los valores del slot y del global: el
  castillo de verdad todavía no existe. Los de partida (24 y 24 MiB por slot, 64 y
  128 MiB el global) son hipótesis.
- **De dónde sale el paquete en la CI y en la CD:** compilar `release` ahora exige el
  paquete cocinado, con Blender. Si el job de `release` del ADR-0029 y la CD cocinan
  ellos mismos o reciben el paquete como artefacto de otro job, contra la meta de 10
  minutos, lo deciden el ticket de la CI y el ADR-0035 (#82).
- **Los nombres** de la organización y del juego en las carpetas: elegidos una vez,
  nunca se cambian, o el juego deja de encontrar la partida.
- **Cómo encuentra `platform::` la sección del paquete** en cada formato de ejecutable
  (ELF hoy; PE y Mach-O si el ADR-0035 suma Windows y macOS), y las carpetas de cada
  sistema.
- **La compresión,** si el tamaño en disco lo pide.
- **Recargar los datos del mundo** (colisión, navmesh, fases): hoy se recocina y se
  vuelve al checkpoint.
- **El *replay* en medio de una sesión** (ADR-0021).

## Cómo lo defiendo

**Tu engine es de un hilo. ¿Por qué agregas otro para la carga?**
Porque el problema no es leer, es esperar. El disco lee una zona en milisegundos, pero
cualquier lectura en el hilo principal espera a un disco que no controlo: en el
prototipo, leer por partes le costó al frame de 2 a 36 ms según el estado de la máquina.
El hilo de I/O tiene el mismo contrato que el de audio: no toca el estado del juego, no
usa allocators y solo escribe en memoria reservada al iniciar. El hilo principal mira
una cola una vez por frame, y eso le cuesta ~0,001 ms.

**¿Por qué no io_uring o la API asíncrona de SDL3, que no necesitan un hilo tuyo?**
Los medí. io_uring es asíncrono solo con `IOSQE_ASYNC` u `O_DIRECT`; sin esas banderas,
el kernel copia los datos dentro del pedido y el frame paga de 1 a 9 ms. `SDL_AsyncIO`
se comportó igual: de 10 a 15 ms en el frame que pide. El hilo propio cuesta lo mismo
que io_uring bien usado, funciona en cualquier sistema, y además guarda la partida con
su `fsync`, que llegó a tardar 655 ms.

**Ya tienes un allocator tipo TLSF. ¿Por qué slots fijos para las zonas?**
Porque el tope de zonas ya existe y el cocinador lo garantiza. Con un slot por zona,
esa garantía cubre también la memoria: nunca falta lugar. Con un montón general, la
fragmentación pide un margen, y ni así está garantizado. En 20 castillos simulados, el
TLSF necesitó más memoria que los slots en 18, y el *buddy*, 128 MB en casi todos.

**¿Qué quiere decir que el binario se usa "tal como llega"?**
Que no hay deserialización. El cocinador escribe secciones planas con offsets, sin
punteros, así que el juego lee el archivo al slot, valida la cabecera y que cada offset
caiga dentro, y ya lo puede usar. Medido: 0,0003 ms por zona, contra 2 a 4 ms de
parsearlo. Todo el trabajo caro lo hace el cocinador, una vez.

**¿Por qué no comprimes, si casi todos los juegos lo hacen?**
Porque en un NVMe no ahorra tiempo: leer las 32 zonas sin comprimir tardó 237 ms, con
LZ4 220 y con zstd 390. Descomprimir cuesta lo mismo que lo que se ahorra de disco.
Ahorraría espacio en disco, y para eso la tabla de secciones ya guarda el codec: si el
tamaño lo pide, se suma sin cambiar el formato.

**¿Cómo subes a la GPU sin cortar el frame?**
Con un presupuesto de 2 MB por frame, unos 0,6 ms de GPU. El slot de la CPU es memoria
de Vulkan visible desde la CPU y con caché: el disco escribe ahí, la simulación lee la
navmesh ahí, y la GPU copia desde ahí. No hay un buffer de *staging* aparte ni un
`memcpy` en el hilo principal. La caché importa: sin ella, leer la navmesh sería lento.

**¿Por qué los assets dentro del ejecutable? ¿No tarda en arrancar?**
Porque así el juego es un solo archivo, y el código nunca corre con assets de otra
versión. No tarda: con 227 MB adentro arrancó en 3 ms, porque el kernel mapea el
ejecutable y no lo lee entero. Lo que no hago es usar esos datos como memoria, porque
sería `mmap` con las faltas de página en el hilo principal: el hilo de I/O los lee del
propio ejecutable con `pread`, como si fuera un archivo más.

**Tu idea era guardar dentro del ejecutable. ¿Por qué no lo hiciste?**
Lo probé y funciona: Linux no deja escribir el ejecutable que corre ("Text file busy"),
pero copiarlo, cambiar la sección y renombrarlo encima sí. El problema es otro: con los
assets adentro, cada guardado reescribe 227 MB (de 7 a 9 s), una actualización
reemplaza el ejecutable y borra la partida, y una instalación de solo lectura no puede
guardar. Me quedé con lo que me interesaba de la idea: una zona reservada, con un
diseño byte por byte y un bit que dice si hay partida. Es una tarjeta de memoria, como
la de la PS1.

**¿Cómo garantizas que un corte no deja al jugador sin partida?**
Con dos bloques. La tarjeta tiene A y B, cada uno con un número de secuencia y un CRC.
Guardar pisa siempre el más viejo, nunca el único válido; cargar se queda con el válido
de secuencia más alta. Si el corte pasa a mitad de una escritura, se rompe solo el
bloque que se escribía, y el CRC lo detecta. Lo probé matando el proceso 1 000 veces
mientras guardaba: ninguna partida perdida. Escribiendo encima, se perdió la mitad.

**¿Qué pasa con una partida vieja cuando actualizas el juego?**
El formato tiene versión y bytes reservados en cero: un campo nuevo ocupa bytes
reservados, y una versión vieja lo lee como cero. El contenido también puede cambiar:
la partida guarda el hash del mundo y el id de la zona, que es estable. Si la zona del
checkpoint ya no existe, vuelvo al checkpoint anterior que exista, o al principio.

**¿Cómo cambias un shader o una textura sin romper un frame que la GPU todavía dibuja?**
Lo viejo no se destruye en el momento: va a la misma cola por número de frame que ya
usan los pedazos, y se destruye cuando ningún frame en vuelo lo puede leer. Cada set de
descriptores se actualiza al empezar su propio frame. La primera versión del prototipo
tenía justo ese error con una textura, y lo encontré revisando el código antes de
correrlo; con la regla de la cola, las capas de validación no dieron ningún error. Una
malla, además, cambia la simulación (la
corta el corte y de ella salen las cápsulas), así que recargarla corta la grabación
del *replay*: si estoy recargando, no estoy cazando un bug.

**¿Qué sacrificas?**
Una tercera excepción al hilo único. Memoria: cada slot mide el tope fijado aunque la
zona sea chica, y la parte de la GPU de cada zona ocupa lugar dos veces mientras está
cargada. Lo
global queda residente siempre. Sin compresión, el juego ocupa más en disco. Dos
caminos de carga (el paquete en Release y los archivos sueltos en desarrollo) que hay
que probar. Y una sola partida: empezar de nuevo pisa la anterior.

## Referencias

- Jason Gregory, *Game Engine Architecture*, 3.ª ed., CRC Press, 2018. Cap. 7,
  "Resources and the File System".
- Jens Axboe, "Efficient IO with io_uring", 2019: <https://kernel.dk/io_uring.pdf>
- SDL3, *CategoryAsyncIO*: <https://wiki.libsdl.org/SDL3/CategoryAsyncIO>
- SDL3, `SDL_GetPrefPath`: <https://wiki.libsdl.org/SDL3/SDL_GetPrefPath>
- XDG Base Directory Specification:
  <https://specifications.freedesktop.org/basedir-spec/latest/>
- Unity, `QualitySettings.asyncUploadTimeSlice`:
  <https://docs.unity3d.com/ScriptReference/QualitySettings-asyncUploadTimeSlice.html>
- Unity, `QualitySettings.asyncUploadBufferSize`:
  <https://docs.unity3d.com/ScriptReference/QualitySettings-asyncUploadBufferSize.html>
- Elan Ruskin, "Marvel's Spider-Man: A Technical Postmortem", GDC 2019:
  <https://gdcvault.com/play/1026496>
- Arseny Kapoulkine, "Robust pipeline cache serialization", 2019:
  <https://zeux.io/2019/07/17/serializing-pipeline-cache/>
- Vulkan 1.3, *Pipeline Cache*:
  <https://registry.khronos.org/vulkan/specs/1.3/html/vkspec.html#pipelines-cache>
- T. S. Pillai et al., "All File Systems Are Not Created Equal: On the Complexity of
  Crafting Crash-Consistent Applications", OSDI 2014:
  <https://www.usenix.org/conference/osdi14/technical-sessions/presentation/pillai>
- Dan Luu, "Files are hard", 2015: <https://danluu.com/file-consistency/>
- Jonathan Corbet, "ext4 and data loss", LWN, 2009: <https://lwn.net/Articles/322823/>
- M. Masmano, I. Ripoll, A. Crespo y J. Real, "TLSF: a New Dynamic Memory Allocator for
  Real-Time Systems", ECRTS 2004.
- LZ4: <https://github.com/lz4/lz4>
- Zstandard, RFC 8878: <https://www.rfc-editor.org/rfc/rfc8878>
