# Prototipo de recursos en runtime y guardado

Nota de investigación del [ADR-0028](../adr/0028-recursos-en-runtime-y-guardado.md).
Guarda las mediciones que sostienen la decisión, para poder repetirlas.

Fecha: 2026-11-03, en la máquina de referencia del ADR-0020 (Dell XPS 13 9360,
i7-8550U, Intel UHD 620, NVMe Toshiba XG5 de 256 GB). Son mediciones de laboratorio para
decidir, no del protocolo del ADR-0020. La CPU y la GPU corrieron con el reloj libre. El
prototipo es código descartable, fuera del repo.

| Qué | Versión |
|---|---|
| Linux | 7.2.8 |
| Sistema de archivos | btrfs, `/home` con `compress=zstd:3` |
| GCC | 16.2.1, con `-O2`, sin `-march` (ADR-0008) |
| liburing | 2.15 |
| LZ4 | 1.10.0 |
| zstd | 1.5.7 |
| SDL3 | 3.4.18 |
| Mesa (ANV) | 26.2.5, Vulkan 1.4.354 |
| glslc | shaderc 2026.4 |
| FFmpeg | 9.0.2 (solo para el video) |

## El prototipo

- **`io.cpp`:** genera las 32 zonas sintéticas y mide P1 (cómo se lee), P2 (en el lugar
  o parseado) y P3 (compresión).
- **`world.h`:** el castillo de 32 zonas y las reglas de carga y simulación del
  ADR-0027.
- **`walk.cpp`:** P8, el recorrido del castillo con lecturas reales del disco.
- **`alloc.cpp`:** P4, los allocators de la memoria de las zonas.
- **`save.cpp`:** P7, la partida guardada ante un cierre forzado.
- **`vk/`:** una copia del renderer del prototipo del ADR-0025 (Vulkan, la escena de 50
  zombies), con tres opciones nuevas: subir N MB por frame (`upload`, `upmode`), la
  recarga en caliente con inotify (`reload`) y el `VkPipelineCache` en disco
  (`pcache`). `hot.sh` cambia un shader, una textura y una malla mientras corre.
- **`render.cpp` y `make_video.sh`:** P9, el video.

**Los datos** van en `/home` (btrfs), no en `/tmp`, que en esta máquina es tmpfs (RAM).
Hay dos carpetas: una con la compresión de btrfs apagada (`chattr +m`) y otra con la
compresión de `/home` (zstd:3).

### Las zonas sintéticas

Cada zona es un binario con el formato del ADR-0017: una cabecera con firma y versión,
una tabla de secciones y las secciones alineadas a 256 bytes. Pesa de 3 a 12 MB (227 MB
las 32), y son en su mayoría texturas, como en un juego con estilo PSX:

- **las instancias del nivel:** de 200 a 600 registros de 64 bytes;
- **la geometría:** cilindros de baja poligonización, vértices de 32 bytes (posición,
  normal, UV), alrededor del 20 % de la zona;
- **las texturas:** RGBA8 de 64², 128² y 256², con un patrón (ladrillo, madera, piedra
  o tela), una paleta de 12 a 32 colores y *dithering* de Bayer.

Además, un archivo de 48 MB con 192 texturas de 256², para ver la cola.

**Limitación:** las texturas sintéticas comprimen más que las reales (la paleta es chica
y el alfa es constante). Las razones de compresión de P3 son optimistas.

### Leer en frío

`posix_fadvise(POSIX_FADV_DONTNEED)` saca el archivo de la caché de páginas sin
privilegios. `mincore` confirmó que no quedaba nada residente antes de cada corrida en
frío.

## P1: cómo se lee sin cortar el frame

Se cargan 5 zonas y el archivo de 48 MB (91 MB) mientras corre un bucle de frames de
16,7 ms. Se mide cuánto tiempo pasa el hilo principal dentro de las llamadas de la
carga, en cada frame. Cinco repeticiones; el p99 y el máximo son de todas.

Resultados en frío, en la carpeta sin compresión de btrfs:

| Método | Frames hasta terminar | Bloqueo p99 (ms) | Bloqueo máximo (ms) |
|---|---|---|---|
| Todo de una vez | 1 | 63 | 63 |
| Por partes, 256 KB por frame | 308 | 1,7 | 2,0 |
| Por partes, 1 MB por frame | 77 | 2,8 | 2,9 |
| Por partes, 4 MB por frame | 20 | 4,8 | 5,4 |
| `mmap`, copiando 1 MB por frame | 77 | 5,1 | 5,1 |
| `mmap` con `MADV_WILLNEED`, 4 MB por frame | 20 | 6,5 | 7,0 |
| Hilo de I/O | 5 | 0,001 | 0,06 |
| io_uring, 16 en vuelo | 6 | 3,0 | 3,3 |
| io_uring, 16 en vuelo, `IOSQE_ASYNC` | 6 | 0,03 | 0,05 |
| io_uring, 16 en vuelo, `O_DIRECT` | 6 | 0,04 | 0,18 |

En la carpeta con compresión de btrfs, lo mismo, salvo picos de 30 ms leyendo por partes
de 4 MB e io_uring común con hasta 7 ms. Con caché tibia (el archivo ya en memoria), io_uring común
sigue costando ~2 ms por frame: copia desde la caché dentro del `submit`.

**Tres corridas, una descartada.** El archivo de la tabla de arriba se perdió: una
segunda corrida lo sobrescribió, y esa corrida quedó contaminada por otro proceso pesado
(tests de otro proyecto, 8 procesos al 100 % de CPU y 4 GB en swap); se descartó. Los
números de arriba son de la primera, copiados antes de perderla. La tercera
(`p1_nocomp.jsonl`, `p1_fscomp.jsonl`, guardada) esperó a que la carga bajara de 1, e
incluye `SDL_AsyncIO`. En frío:

| Método | Sin compresión de btrfs: p99 / máx (ms) | Con compresión de btrfs: p99 / máx (ms) |
|---|---|---|
| Por partes, 256 KB por frame | 1,8 / 2,1 | 1,6 / 2,0 |
| Por partes, 1 MB por frame | 22,9 / 23,7 | 3,1 / 3,4 |
| Por partes, 4 MB por frame | 36,2 / 36,6 | 4,8 / 6,6 |
| `mmap`, 1 MB por frame | 15,2 / 28,1 | 4,9 / 5,0 |
| Hilo de I/O | 0,001 / 0,06 | 0,018 / 0,06 |
| io_uring común, 16 en vuelo | 2,8 / 3,0 | 3,6 / 9,1 |
| io_uring, `IOSQE_ASYNC` | 0,03 / 0,04 | 0,03 / 0,03 |
| io_uring, `O_DIRECT` | 0,04 / 0,06 | 0,03 / 0,04 |
| `SDL_AsyncIO` | 0,013 / **12,1** | 0,013 / **15,4** |

La carpeta sin compresión se midió primero, cuando la máquina recién salía de la carga
anterior (la carga de 5 minutos todavía era 4,6, con memoria en swap). **Leer en el hilo
principal quedó expuesto a ese estado:** de 3 a 36 ms por frame según la corrida. El
hilo de I/O e io_uring con `IOSQE_ASYNC` u `O_DIRECT` no pasaron de 0,06 ms en ninguna.

**`SDL_AsyncIO`:** el frame en que se piden las lecturas cuesta de 10 a 15 ms, en frío y
en tibio (con tibio: 9,9 y 12,4 ms). Se comporta como io_uring sin `IOSQE_ASYNC`: con
los datos en la caché, la copia ocurre dentro del pedido, en el hilo que pide. Pedir de
a poco por frame repartiría el costo, pero no lo saca del hilo principal.

## P2: el binario en el lugar o parseado

Ocho zonas ya en memoria, 20 repeticiones, la mediana:

| Zona (MB) | En el lugar (ms) | Parseado (ms) |
|---|---|---|
| 7,55 | 0,00027 | 3,67 |
| 5,63 | 0,00024 | 2,70 |
| 4,81 | 0,00015 | 1,79 |
| 8,16 | 0,00029 | 3,93 |

**En el lugar:** revisar la firma, la versión, el tamaño y que cada sección caiga dentro
del archivo, alineada. **Parseado:** un lector de campos con revisión de límites, los
vértices convertidos a arreglos separados y cada recurso en su propia asignación.

## P3: compresión

Por sección, para poder descomprimir directo al destino. En caliente (en memoria), la
mejor de tres:

| Codec | En disco (MB) | Razón | Descomprime (MB/s) |
|---|---|---|---|
| Sin comprimir | 226,8 | 1,0 | (copia) 11 087 |
| LZ4 | 84,6 | 2,7 | 2 234 |
| LZ4 HC 9 | 50,1 | 4,5 | 3 265 |
| zstd 1 | 43,7 | 5,2 | 795 |
| zstd 3 | 41,0 | 5,5 | 768 |
| zstd 19 | 26,5 | 8,6 | 1 173 |

En frío, leer y descomprimir las 32 zonas, tres repeticiones:

| Codec | Leer (ms) | Descomprimir (ms) | Total (ms) |
|---|---|---|---|
| Sin comprimir | 235–242 | 0 | 235–242 |
| LZ4 | 109–112 | 108–114 | 218–225 |
| zstd 3 | 62–75 | 319–327 | 381–396 |
| zstd 19 | 50–51 | 201–213 | 251–264 |
| Sin comprimir, con la compresión de btrfs | 219–226 | (en el kernel) | 219–226 |

Generar las zonas con zstd 19 tardó 144 s; con LZ4 HC, 21 s.

## P4: la memoria de las zonas

`alloc.cpp` genera 20 castillos (con `world.h`, cada uno con su tope según el peor caso
del cocinador del ADR-0027) y, en cada uno, 20 recorridos al azar de 300 salas, con
latencias de carga al azar de hasta 1,5 s. Las cargas y descargas que salen de las
reglas del ADR-0027 se repiten contra cada allocator, alineando cada zona a 64 KB. Para
el TLSF se usa un mejor ajuste con fusión de vecinos, que se fragmenta como un TLSF
(Masmano et al., 2004); el mínimo sin fallos se busca por bisección.

| Castillo | Tope | Zona más grande (MB) | Pico real (MB) | Slots (MB) | Mejor ajuste, mínimo (MB) | Buddy (MB) |
|---|---|---|---|---|---|---|
| 42 | 5 | 12,0 | 52,7 | 60,0 | 80,0 | 128 |
| 1 | 6 | 11,1 | 54,9 | 66,4 | 76,3 | 128 |
| 2 | 5 | 11,9 | 39,8 | 59,7 | 54,2 | 64 |
| 4 | 7 | 11,9 | 68,7 | 83,1 | 91,5 | 128 |
| 6 | 6 | 11,9 | 52,7 | 71,6 | 69,8 | 128 |
| 16 | 7 | 12,0 | 64,9 | 84,0 | 84,9 | 128 |

(Los 20 castillos están en `p4.jsonl`.) El mejor ajuste necesitó más que los slots en 18
de 20. Con el montón del tamaño del pico real, el primer fallo de cada castillo fue por
fragmentación: había lugar libre suficiente, pero partido.

## P5: la subida a la GPU

La demo del prototipo del ADR-0025, 600 frames medidos después de 120 de calentamiento.
Cada frame sube N MB antes de dibujar; *timestamps* alrededor de la copia. **Reloj de la
GPU libre** (300 a 520 MHz de media): el total del frame varía mucho con la frecuencia;
el costo de la copia misma es estable.

| MB por frame | Copia a buffer, GPU (ms, mediana) | Copia a texturas 128², GPU (ms) | `memcpy` al *staging*, CPU (ms) | Escritura directa, CPU (ms) |
|---|---|---|---|---|
| 1 | 0,31 | 0,30 | 0,15 | 0,19 |
| 2 | 0,62 | 0,61 | 0,27 | 0,34 |
| 4 | 1,06 | 1,09 | 0,45 | 0,61 |
| 8 | 1,89 | 2,08 | 0,87 | 1,70 |
| 16 | 3,03 | 3,51 | 1,81 | 4,84 |

`vulkaninfo`: una sola familia de colas, con gráficos, cómputo y transferencia.

## P6: la recarga en caliente

`vk/hot.sh` arranca la demo con `reload=1` y, cada 3 s, cambia un archivo de `hot/`. El
juego mira la carpeta con inotify (`IN_CLOSE_WRITE`, `IN_MOVED_TO`) una vez por frame,
después de esperar la *fence* de ese frame:

- **shader:** una línea del fragment shader (el albedo, más verde), compilada con
  glslc y movida a `hot/`. Pipeline nuevo; el viejo a la lista de retiro, que destruye
  lo que tiene más de 2 frames (los frames en vuelo);
- **textura:** la piel de los zombies, 64×64, escrita por un script de Python. Imagen
  nueva con sus mips; cada set de descriptores se actualiza al empezar su próximo frame;
  la imagen vieja se retira 4 frames después;
- **malla:** el zombie inflado 4 cm por la normal, con el mismo esqueleto. Se copia a
  una región libre del buffer de vértices, y el dibujo usa el rango nuevo.

Con `validate=1` (capas de validación de Khronos), ningún mensaje. Tres repeticiones
con el caché de Mesa, y dos sin él (`MESA_SHADER_CACHE_DISABLE=true`):

| Qué | Cocinar (ms) | Aplicar en el juego (ms) | Desde guardar hasta el frame (ms) |
|---|---|---|---|
| Shader, con el caché de Mesa | 248–294 (glslc) | 0,3–0,4 | 252–297 |
| Shader, sin el caché de Mesa | 220–222 | **116–135** | 353–367 |
| Textura | 31–49 | 1,1–1,3 | 41–67 |
| Malla | 35–63 | 0,1–0,3 | 45–81 |

"Aplicar" corre en el hilo principal. Con el caché de Mesa, el shader ya estaba
compilado de la corrida anterior: el número real de un shader nuevo es el de la fila
sin caché.

**Un error encontrado al escribirlo:** la primera versión destruía la textura vieja en
cuanto los dos sets apuntaban a la nueva. Con dos recargas seguidas, eso podía destruir
una imagen que un frame en vuelo todavía leía. La regla que quedó: lo viejo se retira
con el número de frame en que deja de estar referenciado, más los frames en vuelo.

### P6b: los pipelines al arrancar

La demo crea sus pipelines al arrancar (ADR-0022). Diez frames por corrida:

| Configuración | Crear los pipelines (ms) |
|---|---|
| Sin el caché de Mesa y sin `VkPipelineCache` | 1 288–1 315 |
| Sin el caché de Mesa, `VkPipelineCache` vacío (primera vez) | 1 271 |
| Sin el caché de Mesa, `VkPipelineCache` de 205 KB leído del disco | 0,53–0,58 |
| Con el caché de Mesa, sin `VkPipelineCache` | 2,9–3,0 |

## P7: la partida guardada

`save.cpp` guarda en texto `clave = valor` (versión, secuencia, hash del mundo, zona,
fase, fases terminadas como 256 bits, muertes), con un CRC-32 al final: 180 bytes. El
escritor escribe una línea por llamada, como uno ingenuo.

**Cierre forzado:** en cada intento hay una partida buena; un proceso hijo guarda sin
parar y avisa por un *pipe* cada guardado completo; el padre lo mata con `SIGKILL` entre
0,2 y 20 ms después y carga como el juego. 1 000 intentos por estrategia, en `/home`:

| Estrategia | Carga buena | Sin partida | Corrupción detectada |
|---|---|---|---|
| Escribir encima (truncar y escribir) | 478 | **522** | 339 |
| Temporal y `rename` | 1 000 | 0 | 0 |
| Temporal, `fsync`, `rename` y `fsync` de la carpeta | 1 000 | 0 | 0 |
| A y B, con `fsync`; gana la secuencia más alta válida | 1 000 | 0 | 5 (cargó la otra) |

**Costo por guardado,** 300 veces, con 20 ms entre guardados:

| Estrategia | p50 (ms) | p99 (ms) | Máximo (ms) |
|---|---|---|---|
| Escribir encima | 0,19 | 2,28 | 4,10 |
| Temporal y `rename` | 0,35 | 2,44 | 3,41 |
| Temporal, `fsync`, `rename` y `fsync` de la carpeta | 21,0 | 31,4 | **655,5** |
| A y B con `fsync` | 11,8 | 14,6 | 15,7 |

**Versiones:** una partida v1 (sin `muertes`) la carga el lector v2 con `muertes = 0`;
una v2 la carga el lector v1, que ignora la clave nueva; un byte cambiado da "corrupta".

**Limitación:** `SIGKILL` simula un cierre forzado del proceso, no un corte de luz. Con
un cierre forzado, el kernel termina de escribir lo que ya recibió; con un corte de luz,
no. Probar un corte de luz pide una máquina virtual o `dm-flakey` (con privilegios);
para eso vale la literatura (Pillai et al., 2014).

## P8: el recorrido del castillo

El castillo de `world.h` (semilla 42): 32 salas rectangulares de 20 a 45 m pegadas por
un lado, casi siempre a la anterior y a veces a una vieja (ramas). La caballera recorre
el árbol en profundidad (entra a cada rama y vuelve) a 5 m/s: 27 959 pasos, unos 2,3 km.
Las reglas son las del ADR-0027 (carga 35/65 m, simulación 10/20 m, la más cercana
primero, tope 5 garantizado por el peor caso del recorrido). A un 60 % del camino muere y
vuelve al último checkpoint, con todo descargado y en frío. Cada zona se lee de su
archivo real al slot que le toca.

**El tiempo va acelerado ×16** (un frame cada 1,04 ms), para que una corrida dure 30 s.
La latencia inyectada está en tiempo de juego (se divide por 16); el disco real no, así
que en tiempo de juego el disco parece 16 veces más lento. Es el caso pesimista.

| Método | Latencia inyectada | Corridas | Frames de espera | Carga p99 (ms) | Carga máx (ms) | Hash |
|---|---|---|---|---|---|---|
| Hilo de I/O | 0 | 1 | 0 | 0,0008 | 0,19 | `bbf1808e…` |
| Hilo de I/O | hasta 300 ms (picos ×3) | 5 | 0 | 0,0006–0,0025 | 0,02–0,75 | `bbf1808e…` |
| Hilo de I/O | hasta 1 500 ms (picos ×3) | 3 | 107, 0, 59 | 0,0007–0,0029 | 0,02–0,04 | `bbf1808e…` |
| io_uring con `IOSQE_ASYNC` | 0 y 300 ms | 2 | 0 | 0,0011–0,0034 | 0,05–0,06 | `bbf1808e…` |
| Síncrono, 1 MB por frame | 0 y 300 ms | 2 | 0 | 0,14 | 5,6–7,1 | `bbf1808e…` |
| **Sin esperar** (entra a la zona cuando llega) | hasta 300 ms | 3 | — | — | — | `bbf1808e…` ×3 |
| **Sin esperar** | hasta 1 500 ms | 3 | — | — | — | `f9631a3f…`, `bbf1808e…`, `7d9e70a9…` |

"Carga" es el tiempo del hilo principal en mirar los resultados, decidir los pedidos y
pedirlos, sin `open` y `close` (de 0,02 a 0,07 ms, medidos aparte). El pico de zonas
residentes fue 5 en todas; 24 desalojos por el tope.

Con 300 ms, la variante sin esperar dio el mismo hash: las zonas siempre llegaron antes
de hacer falta (25 m de margen a 5 m/s son 5 s). Recién con 1 500 ms se separa, y lo
hace justo en las semillas en que la variante que espera tuvo que esperar.

## P10: el ejecutable

`exe/guardar.cpp` reserva una sección `.partida` de 4 KB (alineada a 4096, con una firma)
y la imprime al arrancar.

- **Escribir el ejecutable que corre:** `open(O_WRONLY)` falla con `ETXTBSY` ("Text
  file busy"): el kernel lee el código desde ese archivo, a demanda.
- **Copiar y renombrar:** leer el ejecutable, cambiar la sección en la copia, `fsync` y
  `rename` encima del original funciona. El proceso que corre sigue con el inodo viejo,
  y el próximo arranque lee la partida nueva (bandera 1, zona 9).
- **Con los assets adentro:** las 32 zonas (227 MB) en una sección `.rodata.mundo` con
  `.incbin`. Enlazar: 3,7 s; el ejecutable mide 227 MB; arrancar: 3 ms, con 4 MB de RSS,
  porque el kernel mapea el ejecutable y no lo lee entero. Guardar por copia y `rename`:
  **de 7,3 a 8,7 s por guardado** (tres veces), porque reescribe los 227 MB.

`vulkaninfo` muestra, en la UHD 620, un tipo de memoria visible desde la CPU, coherente
y **con caché** (`HOST_CACHED`, el tipo 2), además del visible sin caché (el tipo 1, el
que usó el prototipo de P5).

## P9: el video

`make_video.sh` arma `comparativas_adr0028.mp4` (68 s, 1280×720): los clips 1 a 5 salen
de las trazas de P8 con `render.cpp` (el castillo desde arriba, el estado de cada zona,
los slots con su relleno real y el costo de la carga por frame), y el 6 es la demo de
Vulkan con la recarga en caliente.

1. el recorrido con el hilo de I/O (×4);
2. la lectura síncrona de 1 MB por frame, con las barras de más de 1 ms en rojo (×2);
3. el disco muy lento: la simulación espera;
4. la variante sin esperar, con la misma semilla;
5. la muerte y la vuelta al checkpoint;
6. la recarga del shader, de la textura y de la malla.

## Cómo repetirlo

```sh
g++ -std=c++20 -O2 -o io io.cpp -luring -llz4 -lzstd -lSDL3
mkdir -p $D/nocomp $D/fscomp && chattr +m $D/nocomp
for c in raw lz4 lz4hc zstd1 zstd3 zstd19; do mkdir -p $D/nocomp/$c; ./io gen $D/nocomp/$c $c; done
mkdir -p $D/fscomp/raw && ./io gen $D/fscomp/raw raw
./io p1 $D/nocomp/raw 5; ./io p2 $D/nocomp/raw; ./io p3 $D/nocomp
g++ -std=c++20 -O2 -o walk walk.cpp -luring && ./run_p8.sh
g++ -std=c++20 -O2 -o alloc alloc.cpp && ./alloc 20 20 300
g++ -std=c++20 -O2 -o save save.cpp && ./save crash $D/save tmp_rename 1000
(cd vk && ./build.sh && ./run_p5.sh && ./hot.sh "" validate=1)
./make_video.sh
```

Antes de medir: carga de la máquina por debajo de 1 (`/proc/loadavg`) y nada pesado
corriendo. P1 se arruinó una vez por no revisarlo.
