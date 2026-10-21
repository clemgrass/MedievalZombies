# Mediciones de la estrategia de memoria

Nota de investigación del [ADR-0010](../adr/0010-estrategia-de-memoria.md). Guarda las
mediciones que sostienen la decisión, para poder repetirlas.

Fecha: 2026-10-21, en la máquina de referencia del ADR-0020 (Dell XPS 13 9360,
i7-8550U, 8 GB de RAM), con Linux 7.2, glibc, Clang 22 y `-O2`. Son mediciones de
laboratorio para decidir, no del protocolo del ADR-0020: no hubo calentamiento térmico y
había otras aplicaciones abiertas. El prototipo es código descartable, fuera del repo.

Configuración del sistema que importa aquí:

| Qué | Valor |
|---|---|
| `vm.overcommit_memory` | 0 (heurístico: `mmap` casi nunca falla, aunque no haya memoria) |
| *Transparent huge pages* | `enabled = always`, `defrag = madvise` |
| `ulimit -l` (memoria que se puede bloquear con `mlock`) | 8 MiB |

## 1. Los temporales de un corte: `malloc` o arena

**Qué se midió:** el trabajo de memoria de un corte, repetido 5000 veces. Cada corte pide
16 bloques que suman ~240 KB: es una estimación de los temporales de un corte en un
zombie de ~3000 vértices y ~4800 triángulos (posiciones deformadas, clasificación, tabla
de aristas, contornos, listas de triángulos de cada pedazo, vértices nuevos, tapas). Entre
corte y corte, el "resto del juego" libera y vuelve a pedir 200 bloques de 16 B a 16 KiB,
sobre 20 000 bloques vivos, para que el heap no esté limpio como en un benchmark aislado.

Se compararon cinco variantes, todas en µs por corte:

- **solo llamadas:** pedir y liberar los 16 bloques, sin escribirlos;
- **+ escribir:** pedirlos, escribirlos enteros (como el corte real) y liberarlos;
- **arena precalentada:** una arena cuya memoria ya se escribió antes;
- **arena fría:** una arena sobre páginas que nadie tocó todavía.

| Variante | p50 | p99 | p99,9 | Máximo |
|---|---|---|---|---|
| `malloc` + `free`, solo llamadas | 1,8 | 37,1 | 87,4 | 161,1 |
| Arena, solo llamadas | **0,04** | **0,17** | **0,51** | 15,8 |
| `malloc` + escribir + `free` | 9,3 | 89,2 | 178,1 | 235,4 |
| Arena precalentada + escribir | 5,9 | 41,4 | 76,4 | 125,4 |
| **Arena fría + escribir** | **102,8** | **317,6** | **445,7** | **553,5** |

Con un bloque más, de 300 KB (sobre el umbral de 128 KiB en que glibc pasa a `mmap`):

| Variante | p50 | p99 | p99,9 | Máximo |
|---|---|---|---|---|
| `malloc` + `free`, solo llamadas | 3,0 | 24,1 | 41,5 | 130,6 |
| Arena, solo llamadas | 0,06 | 0,49 | 1,41 | 20,4 |
| `malloc` + escribir + `free` | 21,4 | 128,6 | 220,2 | 241,1 |
| Arena precalentada + escribir | 18,0 | 129,1 | 224,2 | 252,2 |
| **Arena fría + escribir** | **300,3** | **799,4** | **1160,4** | **1283,2** |

Lo que mostró:

- **Pedir memoria a una arena no cuesta nada medible:** sumar un desplazamiento. Las
  llamadas a `malloc` y `free` del mismo corte llegan a 37 µs en el p99 y a 161 µs en el
  peor caso, porque dependen del estado del heap que dejó el resto del programa.
- **El costo grande no es el allocator: es tocar memoria nueva.** Escribir 240 KB de
  páginas que nadie tocó cuesta ~100 µs por corte en la mediana, 17 veces más que
  escribirlos en memoria ya tocada. Con 4 cortes por paso son ~0,4 ms de los 2,0 ms de
  "combate y corte" (ADR-0020), sin que el algoritmo haga nada.
- **glibc se adapta:** con el bloque de 300 KB, `malloc` dejó de usar `mmap` después del
  primer `free` (su umbral es dinámico), y quedó casi igual que la arena precalentada.
  Es un comportamiento correcto, pero es una decisión de glibc, no del autor.
- **El ruido de la máquina:** la arena "solo llamadas" tuvo un máximo de 15,8 µs por una
  interrupción o un cambio de contexto. Ninguna estrategia evita eso; las colas de las
  otras filas lo incluyen.

## 2. El primer toque de una página

**Qué se midió:** reservar 256 MiB con `mmap` y escribir un byte en cada página de 4 KiB,
midiendo cada escritura. Después, escribir otra vez. Tres repeticiones.

| Variante | `mmap` | Primer toque | Por MiB | Peor página | Segundo toque |
|---|---|---|---|---|---|
| Sin nada (THP del sistema) | 10–32 µs | 178–190 ms | **0,69–0,74 ms** | 0,28–0,63 ms | 0,8–1,2 ms |
| `MADV_NOHUGEPAGE` | 20–35 µs | 182–216 ms | 0,71–0,84 ms | 0,39–**2,41 ms** | 0,9–1,6 ms |
| `MAP_POPULATE` | **93–115 ms** | 3,8–6,6 ms | 0,015–0,026 ms | 0,02–0,04 ms | 0,8–1,0 ms |

Lo que mostró:

- **`mmap` solo promete la memoria.** Linux entrega la página (la busca y la llena de
  ceros) la primera vez que se escribe: ~0,7 ms por MiB. Una sola página llegó a costar
  2,4 ms, un 14 % del frame.
- **`MAP_POPULATE` adelanta ese costo a la reserva** (~0,4 ms por MiB), y después el
  primer toque cuesta lo mismo que el segundo. Tocar la memoria a mano al iniciar
  (escribir un byte por página) logra lo mismo.
- **Las *huge pages* transparentes no cambiaron nada medible** con esta configuración
  (`defrag = madvise`).
- **Bloquear la memoria** (`mlock`) para que el sistema no la mande a swap no es
  práctico: el límite del usuario es de 8 MiB.

## 3. ASan dentro de una arena y de un pool

**Qué se midió:** tres errores clásicos, compilados con `-fsanitize=address`, sin y con
envenenamiento manual (`ASAN_POISON_MEMORY_REGION` al reiniciar la arena o al devolver
el slot, `ASAN_UNPOISON_MEMORY_REGION` al entregarlo, y 8 bytes envenenados entre
asignaciones de la arena).

| Error | Sin envenenar | Envenenando |
|---|---|---|
| Leer un contorno después de reiniciar la arena (dato del frame anterior) | **Nada:** devuelve el valor viejo | `use-after-poison` |
| Escribir un byte después del final de una asignación de la arena | **Nada:** pisa la asignación vecina | `use-after-poison` |
| Leer un slot del pool después de devolverlo | **Nada** | `use-after-poison` |

Lo que mostró:

- **Para ASan, una arena es un solo bloque válido.** Lo que pasa adentro no lo ve. Sin
  envenenamiento, el Debug con ASan del ADR-0006 no detecta ninguno de los errores
  típicos de un allocator propio.
- **El envenenamiento manual los detecta los tres,** con la línea exacta del error.
- **Un detalle del prototipo:** con el pool en la pila y `-O1`, el compilador reutilizó
  el valor ya escrito y no volvió a leer la memoria, así que ASan no tuvo nada que
  revisar. Los pools reales viven en memoria propia del engine, no en la pila; el caso se
  repitió así.
- **ASan trabaja de a 8 bytes:** envenenar un rango que no empieza o no termina en un
  múltiplo de 8 deja huecos sin envenenar. Las asignaciones y las zonas rojas tienen que
  respetar esa alineación.

## 4. La fragmentación del pool de pedazos

**Qué se simuló:** solo la contabilidad de offsets, como la de un allocator de la GPU, sin
tocar memoria. Un millón de cortes (~35 horas de combate a 8 cortes por segundo, R5):

- un pedazo ocupa sus índices (`uint32`, 12 B por triángulo) más sus vértices nuevos
  (32 B cada uno, ADR-0020);
- un zombie entero tiene ~4800 triángulos;
- un corte da 2 pedazos (70 %), 3 (20 %) o 4 (10 %), con proporciones al azar, y crea de
  63 a 200 vértices nuevos (los 180 a 570 del maniquí del ADR-0007, escalados a 3000
  vértices). Los triángulos de las tapas y de los bordes se reparten entre los pedazos;
- el 70 % de los cortes es a un zombie entero, y el 30 % vuelve a cortar un pedazo
  existente, que se libera;
- con 100 pedazos (R5), el más viejo se descarta;
- lo que se libera espera un corte antes de volver a estar disponible, como espera la
  GPU (frames en vuelo).

Tres estrategias:

- **slots fijos:** cada pedazo ocupa un slot del tamaño máximo;
- **buddy:** bloques de potencias de 2 que se parten y se vuelven a unir;
- ***good fit* con unión de vecinos:** el bloque libre más chico que alcanza. Es el
  comportamiento de TLSF, que lo hace en O(1); la simulación usa un `std::map` porque
  solo mide la fragmentación, no el tiempo.

"Fallos" son los pedazos que no entraron.

| Pool | Slots de 64 KiB | Buddy | *Good fit* |
|---|---|---|---|
| 32 MiB | 2 (pedazos de más de 64 KiB) | 0 | 0 |
| 8 MiB | 2 | 0 | 0 |
| 4 MiB | 2,1 millones (caben 64 slots) | 69 | 0 |
| 3,5 MiB | 2,1 millones | 693 744 (el pool cae a 2 MiB) | 0 |
| 3,25 MiB | 2,1 millones | 693 744 | 1 |
| 3 MiB | 2,1 millones | 693 744 | 258 |

| Qué | Slots de 64 KiB | Buddy | *Good fit* |
|---|---|---|---|
| Lo reservado en el pico (con su desperdicio interno) | 6,75 MiB | 4,03 MiB | **2,85 MiB** |

Lo que mostró:

- **Los pedazos vivos ocupan poco:** ~2,85 MiB en el pico, con el tope de 100. El pedazo
  más grande midió ~66 KB.
- **Los slots fijos fallan por dos lados:** un pedazo que se volvió a cortar varias veces
  acumula vértices nuevos y pasa del tamaño del slot (2 fallos en un millón), y el pool
  tiene que tener 100 slots más los que esperan liberarse.
- **Buddy desperdicia ~40 %** al redondear a potencias de 2, y su pool tiene que ser una
  potencia de 2: con 3,5 MiB solo usa 2 MiB.
- ***Good fit*** no falló nunca con un 23 % de margen sobre lo vivo (3,5 MiB), y falló
  1 vez con un 14 % (3,25 MiB). La fragmentación externa existe, pero es chica.
- Lo que la simulación no cubre: un pico de 32 cortes en un frame (después de un tirón),
  en que lo liberado espera 2 frames enteros. Esa cuenta va en el ADR.

## Cómo repetirlo

1. Escribir los tres programas en C++20, sin dependencias:
   - `lat.cpp`: los dos escenarios del punto 1, con `std::chrono::steady_clock` alrededor
     de cada corte y percentiles sobre las 5000 muestras, más el punto 2 con `mmap`,
     `madvise` y `MAP_POPULATE`. La arena fría reutiliza una región de 64 MiB y la
     devuelve al sistema con `MADV_DONTNEED` (fuera de la medición) cuando se llena:
     así cada corte toca páginas nuevas sin consumir gigas;
   - `asan_demo.cpp`: una arena lineal y un pool de 4 slots con y sin envenenamiento, y
     los tres errores de la tabla;
   - `frag.cpp`: la sesión del punto 4 con semilla fija (1), las tres estrategias detrás
     de la misma interfaz, y los tamaños de pool de la tabla.
2. Compilar `lat.cpp` y `frag.cpp` con `clang++ -std=c++20 -O2`, y `asan_demo.cpp` con
   `clang++ -std=c++20 -g -O1 -fsanitize=address`.
3. Correr `lat` sin otras cargas pesadas, y comparar las medianas antes que los máximos:
   los máximos dependen del ruido de la máquina.
