# Prototipo de VFX y partículas

Nota de investigación del [ADR-0025](../adr/0025-vfx-y-particulas.md). Guarda las
mediciones que sostienen la decisión, para poder repetirlas.

Fecha: 2026-11-02, en la máquina de referencia del ADR-0020 (Dell XPS 13 9360,
i7-8550U, Intel UHD 620). Son mediciones de laboratorio para decidir, no del protocolo
del ADR-0020. **La GPU se fijó en 700 MHz** (`gt_min_freq_mhz` y `gt_max_freq_mhz`),
como en el ADR-0022, para poder comparar: la frecuencia pedida quedó en 700 MHz. La
frecuencia actual leída cada 15 frames promedió de 300 a 450 MHz porque la GPU duerme
entre frames (un frame mide ~4 ms de 16,7). La CPU quedó libre. El prototipo es código
descartable, fuera del repo.

| Qué | Versión |
|---|---|
| Linux | 7.2.8 |
| Mesa (ANV) | 26.2.5, Vulkan 1.4.354 |
| GCC | 16.2.1, con `-O2`, sin `-march` (ADR-0008) |
| glslc | shaderc 2026.4, `--target-env=vulkan1.3 -O` |
| Encabezados de Vulkan | Khronos Vulkan-Headers v1.4.363 |

## El prototipo

Es el prototipo del ADR-0022 con lo de este ADR encima: la misma base de Vulkan
(`dynamic rendering`, *synchronization2*, bindless con el índice del objeto en un *push
constant*), la escena de 50 zombies (el maniquí de Quaternius diezmado a 3269 vértices,
CC0) con 8 antorchas, niebla, *dithering* de 15 bits y vértices que tiemblan, a 640×360
con blit *nearest* a 3200×1800. Sin pedazos, el frame de GPU mide 4,2 ms: 2,67 la
pasada de color y 1,45 el blit.

**Las partículas** son registros de 32 B: posición y vida (`vec4`), velocidad y un
`uint` con el color (RGB8), el tamaño (6 bits, `(c/63)² × 1,5 m`) y dos banderas
(emisiva, estirada). El mismo formato sirve para dibujar desde la CPU y como estado en la
GPU.

- **En la CPU:** arreglos SoA con capacidad fija. El banco integra, choca con el piso
  (`y < 0`) y, al morir, la partícula renace en un emisor con un *hash* `pcg` del índice
  y del frame. Después escribe los registros, agrupados por mezcla, en un buffer mapeado
  por frame en vuelo.
- **En la GPU:** `sim.comp`, una invocación por partícula (grupos de 64), con la misma
  regla de renacer. Sin escritura en la CPU. Con `COLLIDE_DEPTH`, proyecta la partícula,
  lee la profundidad de este frame y la refleja con una normal reconstruida por
  diferencias.

**El dibujo:** `particle.vert` arma un quad que mira a la cámara, estirado con la
velocidad proyectada si la partícula lo pide, y lo ilumina en el vértice con la
atenuación de las antorchas (sin normal). Tres variantes: `INST` (4 vértices por
instancia), `PULL` (6 vértices leídos del SSBO) y `POINTS` (`gl_PointSize`).
`particle.frag` recorta un disco y mezcla en tres modos: *screen-door* con la matriz de
Bayer de 4×4, *alpha* premultiplicado y aditivo.

**Las pasadas:** la de color dibuja lo opaco y, si nada necesita la profundidad, las
partículas y las manchas de malla en la misma pasada. Si algo la necesita (los *decals*
proyectados o el choque de la GPU), la pasada se corta, la profundidad pasa a
`DEPTH_READ_ONLY_OPTIMAL` y sigue otra pasada con el color cargado. Cada tramo lleva un
*timestamp*.

**Las marcas:**

- proyectadas: una caja de 36 vértices generados en el shader, una instancia por marca,
  caras traseras (`cull FRONT`); el fragment shader reconstruye la posición del mundo con
  la inversa de la vista y la proyección, la lleva al espacio de la caja y descarta lo
  de afuera;
- de malla: Sutherland–Hodgman contra los 6 planos de la caja, en el espacio de la caja,
  sobre los triángulos del piso y de las columnas que miran hacia arriba; cada marca tiene
  96 vértices reservados, con los que sobran degenerados, y se dibujan todas en una
  llamada con *depth bias*;
- pintura: una máscara `R8` de 512² sobre los 40 × 40 m del piso; la CPU pinta la
  mancha y sube la máscara entera cuando cambia; el shader del piso la lee con el filtro
  *nearest*.

La forma de la mancha es la misma en las tres: un disco con el borde ondulado y cinco
gotas satélite, con una semilla por marca.

**La demo** (`demo=1`) repite cada 3,5 s: un tajo de 220° en 0,26 s que corta en el
contacto (la pausa de impacto de 66 ms congela la hoja y retrasa la salpicadura), tres
chorros desde el muñón, un charco, un rebote contra la placa con 45 chispas, polvo y el
fuego de las 8 antorchas. La estela guarda la empuñadura y la punta en el tiempo propio
de la caballera y se suaviza con Catmull-Rom (4 subdivisiones).

**Los *frames* de impacto:** una pasada a pantalla completa a 640×360 que lee el color y
la profundidad y escribe otra imagen, que es la que va al blit. El shader de las mallas
escribe alfa 0 en los personajes y 1 en el resto, y las mezclas de las partículas
conservan el alfa del destino.

**Las capas de la tapa** usan el corte oblicuo de la cintura del prototipo del ADR-0022
(las dos mitades, vistas de cerca). La profundidad desde la piel sale de:

- la cápsula: el radio de la cápsula menos la distancia al eje. El radio se estimó como
  la distancia media al eje de los vértices más cercanos a cada hueso;
- el contorno: las aristas del borde de la tapa (las que usa un solo triángulo de la
  tapa), 203 en este corte, recorridas por cada píxel;
- un campo de distancia: la CPU llena una textura `R8` en la base `(u, v)` del plano, con
  la distancia a la arista más cercana (0 a 10 cm), por fuerza bruta, y la tapa la lee
  con filtrado lineal.

Grosores: piel 4 mm y grasa 8 mm; con armadura, metal 8 mm, gambesón 8 mm y piel 4 mm.
Una CVar los multiplica.

## Resultados

Medianas de 600 frames, en ms. Cuando las dos vueltas coinciden, se da una; si no, las
dos. Tres corridas de la primera vuelta (CPU con 5000, 20 000 y 50 000) midieron la
pasada de color entre 3,2 y 3,9 ms, contra 2,7 en todas las demás: se toma la segunda
vuelta.

### P1: CPU o GPU (instancing, aditivo)

| Partículas | CPU: integrar | CPU: escribir | Subida por frame | GPU: simular | GPU: dibujar |
|---|---|---|---|---|---|
| 1000 | 0,01 | 0,004 | 32 KB | 0,02 | 0,02 |
| 5000 | 0,04 | 0,02 | 160 KB | 0,03 | 0,11 |
| 20 000 | 0,15 | 0,08 | 640 KB | 0,09 | 0,48 |
| 50 000 | 0,41 | 0,24 | 1,6 MB | 0,20 | 1,2 |
| 100 000 | 0,80 | 0,49 | 3,2 MB | 0,34 | 2,4 |

### P2: dibujo, mezcla, orden y *overdraw*

| Configuración | Dibujo (GPU) | CPU |
|---|---|---|
| 20 000, *instancing* | 0,48 | — |
| 20 000, *vertex pulling* | 0,70 | — |
| 20 000, *point sprites* | 0,12 | — |
| 100 000 en la GPU: *instancing* / *pulling* / *points* | 2,40 / 3,55 / 0,64 | — |
| 20 000: *screen-door* / *alpha* / aditivo | 0,49 / 0,48 / 0,48 | — |
| Ordenar 20 000: `std::sort` / *radix* | — | 1,11 / 0,33 |
| Ordenar 50 000: `std::sort` / *radix* | — | 2,95 / 0,86 |
| 200 humos (~0,8 m), *alpha* | 0,21 | — |
| 1000 humos, *alpha* / *screen-door* | 1,0 / 1,15 | — |

### P3: choque y pasadas

| Configuración | GPU |
|---|---|
| Simular 20 000 en la GPU: piso / profundidad | 0,09 / 0,10 |
| Simular 100 000: piso / profundidad | 0,33 / 0,40–0,45 |
| Frame sin efectos / con la pasada de color partida | 4,23 / 4,16 |

### P4: marcas

| Marcas | Proyectadas | De malla | Pintura |
|---|---|---|---|
| 100 | 0,25 | 0,06 | — |
| 500 | 1,16 | 0,23 | 0 medible |
| 2000 | 4,7 | 0,95 | — |

De malla: 2 µs de CPU por marca (mediana), de 7 a 19 µs en el p99; 30 vértices como
máximo. Pintura: 16 µs de CPU por marca. La subida de la máscara (256 KB) va antes del
primer *timestamp*: no está medida.

### P5 y P6: la demo

| Configuración | Partículas (GPU) | Manchas (GPU) | CPU |
|---|---|---|---|
| Manchas de malla, *screen-door* | 0,03 | 0,20 | 0,005 |
| Manchas de malla, *alpha* | 0,03 | 0,20 | 0,006 |
| Pintura, *screen-door* | 0,03 | 0 | 0,005 |

En promedio hay ~200 partículas vivas (6,5 KB de subida por frame), con picos de ~600.

### P7: *frames* de impacto

| Modo | GPU |
|---|---|
| Sin pasada (solo las barreras) | 0,025 |
| Invertir | 0,10 |
| Siluetas | 0,14 |

### P8: capas de la tapa

| Modo | Pasada de color, de cerca | De lejos (distancia de juego) |
|---|---|---|
| Sin capas (ADR-0022) | 0,64 | 0,26 |
| Cápsula | 1,00 | — |
| Contorno (203 aristas) | 1,79 | 0,60 |
| Campo de distancia | 0,64 | — |

El campo de distancia por fuerza bruta en la CPU: 0,97 ms con 32², 3,6 ms con 64² y
14,5 ms con 128², por corte.

## El video

`comparativas_adr0025.mp4`, 107 s, 23 clips a 1280×720 (la imagen de 640×360 escalada
×2 con *nearest*):

1. sangre con *screen-door* y manchas de malla;
2. sangre con *alpha blending*;
3. sangre aditiva;
4. *point sprites*;
5. sin pausa de impacto;
6. cámara lenta ×0,25;
7. *frames* de impacto: invertir;
8. *frames* de impacto: siluetas;
9. siluetas en cámara lenta;
10. 300 marcas proyectadas;
11. 300 marcas de malla;
12. 300 marcas pintadas;
13. 1000 humos con *alpha*;
14. 1000 humos con *screen-door*;
15. 100 000 partículas en la GPU;
16. las mismas, chocando contra la profundidad;
17. la tapa sin capas;
18. capas desde la cápsula;
19. capas desde el contorno;
20. capas desde el campo de distancia;
21. capas con armadura;
22. capas a la distancia de juego;
23. capas a la distancia de juego, con el triple de grosor.

## Cómo repetirlo

```
./build.sh                                   # shaders con glslc y el programa con g++
./v25 sim=cpu n=20000 frames=600 warm=120    # una medición: una línea JSON
./run_matrix.sh results.jsonl                # la matriz completa, dos vueltas (~20 min)
./make_video.sh                              # los 23 clips y el video
```

Claves principales: `sim=none|cpu|gpu`, `n`, `draw=inst|pull|points`,
`blend=0|1|2`, `sort=none|std|radix`, `collide=plane|depth`, `smoke=1`,
`decals=none|proj|mesh|paint`, `ndecals`, `demo=1`, `impact=0|1|2`, `hitstop`, `ts`,
`scene=horde|cut`, `layers=0|1|2|3`, `armor`, `far`, `layerScale`, `sdfRes`,
`validate=1` (las *validation layers*, sin errores en todas las configuraciones).
