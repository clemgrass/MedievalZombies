# Prototipo de niveles y escenas

Nota de investigación del [ADR-0027](../adr/0027-niveles-y-escenas.md). Guarda las
mediciones que sostienen la decisión, para poder repetirlas.

Fecha: 2026-11-02, en la máquina de referencia del ADR-0020 (Dell XPS 13 9360,
i7-8550U, Intel UHD 620). Son mediciones de laboratorio para decidir, no del protocolo
del ADR-0020. Nada de este prototipo usa la GPU: los tiempos son de la CPU, con el reloj
libre. El prototipo es código descartable, fuera del repo.

| Qué | Versión |
|---|---|
| Linux | 7.2.8 |
| Blender | 5.2.2 LTS (la versión fijada por el ADR-0029) |
| cgltf | 1.15 (commit `85cd623`, 2026-02-02) |
| Clang | 23.1.3, con `-O2`, sin `-march` (ADR-0008) |
| GCC | 16.2.1, con `-O2`, sin `-march` |
| FFmpeg | 9.0.2 (solo para el video) |

## El prototipo

Tres piezas, en el orden en que trabajaría el autor:

- **`arena.py`, un script de Blender** (`blender -b -P`) que arma la arena del corte
  vertical como la armaría el autor a mano:
  - **tres piezas reutilizables** (colecciones de Blender fuera de la escena): un tramo de
    muro de 4 m con su caja `UBX_`, una columna con su cápsula `UCP_` y un escombro con
    su esfera `USP_`;
  - **la arena:** un piso de 44 × 44 m con su caja, un anillo de 24 muros de radio 18 m
    con 4 huecos (las puertas de la horda), 12 columnas y 4 escombros, todos como
    *collection instances* de las piezas;
  - **16 antorchas** como luces puntuales de Blender, con la propiedad `radio`;
  - **marcadores de gameplay:** *empties* con propiedades (*custom properties*): 4 spawns
    de la oleada (`tipo=spawn`, `fase=1`, `zombie`, `cantidad`), 4 puertas de la horda,
    el mago, 6 puntos de invocación, el inicio de la caballera, 2 checkpoints y un
    trigger (un *empty* con forma de caja, cuya escala es el volumen);
  - **los parámetros del nivel** como propiedades de la escena: `nivel`, `radio_inflado`
    y `musica`;
  - **variantes:** `base`; `moved`, con un spawn movido 1 m (para el diff de git); y
    `castle N`, con N copias de la arena separadas 50 m, como castillo sintético grande
    (8 y 32 zonas).

  Guarda el `.blend` y exporta un `.glb` con *Custom Properties* (`export_extras`) y
  *Punctual Lights* activados.
- **`arena.nivel`, un archivo de texto** con el gameplay de la arena: las tres fases
  (oleada, horda que termina con 80 muertes, mago), qué spawns usa cada una, el
  checkpoint de cada fase, el goteo de la horda por las puertas y el trigger de la
  música. Nombra los marcadores del glTF por su nombre, con `*` para un grupo
  (`arena.spawn.puerta.*`). 23 líneas con contenido.
- **`l27.cpp`, un programa en C++20** (1010 líneas) con seis modos:
  - `cook`: lee el `.glb` con cgltf y el `.nivel`, y escribe un binario de nivel con
    cabecera y secciones planas alineadas a 16 bytes (como el del ADR-0017): mallas,
    instancias, colisión, luces, marcadores, navmesh (cabecera, grilla y rectángulos),
    fases, comandos, grupos, triggers y nombres de depuración;
  - `load`: valida la cabecera y las secciones, crea las entidades en un ECS de juguete
    con *sparse sets* (ADR-0012) y calcula un hash de todos los componentes;
  - `phases`: corre las fases del corte vertical a 60 Hz por dos caminos, **desde los
    datos cocinados** y **desde C++ escrito a mano para la arena**, con un modelo falso
    de combate (los zombies caminan derecho hacia la caballera y ella mata al más cercano
    a menos de 2 m cada 0,6 s). Escribe un hash cada 600 pasos. Puede simular una muerte
    en un paso dado y volver al checkpoint;
  - `video`: la vista cenital de la arena cocinada y de las fases, en RGB crudo para
    FFmpeg;
  - `stream` (P8, sumado cuando el autor pidió un mundo continuo): la caballera camina
    las 32 zonas del castillo sintético y las zonas se cargan mientras camina;
  - `stream2` (P8b, sumado después de la auditoría): lo mismo, separando la carga de la
    simulación, con el tope de zonas cargadas y el peor caso que validaría el cocinador.

**Lo que el cocinador de juguete hace, y lo que no:**

- recorre todos los nodos del glTF y les da un **id: el FNV-1a de 64 bits del camino
  completo** (`arena.muro.01/UBX_muro`). Si dos caminos dan el mismo id, falla;
- aplica la rotación de 180° sobre Y del ADR-0017 a cada transformación del mundo como
  `M' = R·M·R`, con `R = diag(−1, 1, −1)`, porque la malla de la pieza ya quedó rotada
  por `R` en el cocinador: es la misma conjugación que el ADR-0017 aplica a las matrices
  de *bind*;
- ajusta la caja, la cápsula o la esfera de cada colisionador desde el `min` y `max` del
  accessor `POSITION` (el ajuste real es el del ADR-0017) y falla si una cápsula o una
  esfera tienen escala no uniforme;
- **ordena cada sección por id**, así el orden de los nodos en el glTF no importa;
- genera **la navmesh simple** que el ADR-0023 pide para la arena plana: una grilla de
  0,5 m sobre el piso (cajas con la cara de arriba en y ≈ 0), sin las celdas a menos
  del radio de inflado de un obstáculo (lo que sube de 0,3 m), y las celdas fusionadas
  en rectángulos de forma golosa. **No** calcula portales ni alturas: eso es del
  ADR-0023;
- **valida:** que cada spawn, punto de invocación e inicio caiga en la navmesh; que cada
  marcador que nombra el texto exista; que cada patrón encuentre al menos un marcador;
  que cada fase diga cómo termina; y que cada tipo de zombie, de marcador y de evento sea
  conocido. Cada error falla con el nombre del objeto;
- dentro de un grupo (`puerta.*`), **ordena por nombre**: es el orden en que se reparte
  la horda.

## Resultados

### P1: qué sobrevive de Blender al glTF

| Qué se armó en Blender | Qué llegó al glTF |
|---|---|
| Propiedades de un *empty* (`tipo`, `fase`, `zombie`, `cantidad`) | `extras` del nodo, con los enteros como enteros y los textos como textos |
| Propiedades de la escena | `extras` de la escena |
| *Collection instance* | Un nodo con la transformación de la instancia y los objetos de la pieza como hijos. **Los hijos comparten la malla**: 8 mallas para 157 nodos en la arena, 70 para 5024 en el castillo de 32 zonas |
| Nombres de los hijos | **Se repiten**: los 24 muros tienen un hijo `muro` y otro `UBX_muro`. El nombre solo no identifica; el camino sí |
| Escala de un *empty* | `scale` del nodo, con los ejes ya pasados a Y arriba (`[15, 1,5, 15]`). La forma con que Blender dibuja el *empty* (caja, flecha) **no** se exporta: el volumen de un trigger es su escala |
| Luz puntual | `KHR_lights_punctual`. **La intensidad se convierte a unidades físicas** (200 W → 10 870 cd, con el modo *Standard*), y el `range` sale de la distancia de corte (*custom distance*). La propiedad `radio` llega aparte, en `extras` |
| Nodo de la luz | Lleva una rotación de −90° sobre X (la luz apunta hacia −Z en glTF) |

**Exportar dos veces el mismo `.blend` da los mismos bytes** (`sha256` igual).
**Guardar dos veces el mismo `.blend` no:** los `.blend` difieren aunque la escena sea
la misma.

### P2: el texto y git

| Cambio | Lo que muestra `git diff` |
|---|---|
| Mover un spawn 1 m en el `.blend` (751 KB) | `Binary files differ`. Mismo tamaño en bytes, ningún dato legible |
| El mismo cambio en un texto | Una línea |

En el prototipo, las posiciones viven en Blender y el texto solo nombra los marcadores,
así que mover un spawn no toca el texto. Lo que sí cambia en el texto (cuántos zombies,
cada cuánto gotea la horda, cuántas muertes terminan la fase) se lee en el diff.

### P3: el cocinador

| Nivel | Nodos | Instancias | Colisionadores | Luces | Marcadores | Navmesh | Binario | Tiempo (leer glTF / nodos / navmesh / total) |
|---|---|---|---|---|---|---|---|---|
| Arena | 157 | 41 | 41 | 16 | 19 | 88 × 88 celdas → 201 rectángulos | 52 KB | 0,30 / 0,05 / 0,12 / **0,66 ms** |
| Castillo, 8 zonas | 1 256 | 328 | 328 | 128 | 152 | 388 × 188 → 1 608 | 426 KB | 3,0 / 0,4 / 1,2 / **5,4 ms** |
| Castillo, 32 zonas | 5 024 | 1 312 | 1 312 | 512 | 608 | 388 × 788 → 6 432 | 1,7 MB | 8,7 / 1,3 / 5,1 / **17,9 ms** |

- **Mismos bytes** al cocinar la arena con Clang, con GCC y **recorriendo los nodos al
  revés**: ordenar por id hace que el binario no dependa del orden del glTF.
- **La validación atrapó un error real** en la primera versión de la arena: el punto de
  invocación 5, en un anillo de 8 × 4 m alrededor del mago, quedaba dentro de una
  columna (a 0,45 m de su centro, con 0,5 m de radio y 0,45 m de inflado).
  El cocinador falló con `el marcador arena.invocacion.5 cae fuera de la navmesh`. Se
  corrigió achicando el anillo a 6 × 3 m.
- Las mallas del prototipo son cajas y cilindros: el tamaño del binario y el tiempo de
  leer el glTF crecen con las mallas reales, que son del cocinador de assets (ADR-0017).

### P4: la carga

300 cargas de cada nivel desde la memoria, después de leer el archivo una vez:

| Nivel | Archivo | Entidades | Memoria del ECS | Leer el archivo | Crear las entidades (mediana / p99) | Hash, Clang = GCC |
|---|---|---|---|---|---|---|
| Arena | 52 KB | 117 | 11 KB | 0,04 ms | 0,0015 / 0,003 ms | sí |
| Castillo, 8 zonas | 426 KB | 936 | 91 KB | 0,3 ms | 0,013 / 0,044 ms | sí |
| Castillo, 32 zonas | 1,7 MB | 3 744 | 364 KB | 1,5–1,7 ms | 0,04–0,05 / 0,17 ms | sí |

- **El hash es el mismo en las 300 cargas y entre Clang y GCC.** Las entidades se crean
  en el orden de las secciones, que ya vienen ordenadas por id: los *handles* no
  dependen de cómo estaba ordenado Blender.
- Leer el archivo se midió con el archivo ya en la caché del sistema: es el mejor caso.
- **Crear las entidades del nivel cuesta microsegundos.** El costo de cargar un nivel va
  a estar en los assets (mallas, texturas, sonidos) y en subirlos a la GPU, que son del
  ADR-0028.
- Un detalle del prototipo: un componente con relleno implícito (`u32` seguido de `u64`)
  metía bytes sin inicializar en el hash. Con un campo de relleno explícito, el hash es
  estable. Es la misma regla que el hash de estado del ADR-0016 y del ADR-0021.

### P5: las fases, desde datos o desde C++

| Camino | Líneas | Qué hace falta para un nivel nuevo |
|---|---|---|
| **Datos:** el texto de la arena | 23 | Otro texto. El intérprete (34 líneas) y la lectura del texto en el cocinador se escriben una vez |
| **C++:** las fases de la arena escritas a mano | 38 | Otra función de C++ por nivel, y recompilar para cambiar un número |

- **Los dos caminos dan el mismo hash** cada 600 pasos durante toda la partida (12 115
  pasos, 106 muertes), con Clang y con GCC.
- **La primera vez no coincidió el hash final:** al terminar la última fase, el camino
  de datos no reiniciaba el contador de muertes y el de C++ sí. Es una regla de borde
  escrita dos veces de dos formas distintas. Con una sola implementación, el problema
  no existe.
- **El camino en C++ busca los marcadores por nombre en runtime** (el id del nombre). Un
  nombre mal escrito falla recién al jugar esa fase; en el camino de datos falla el
  build.
- **Checkpoint:** la caballera muere en el paso 7000 (fase 2). Volver al checkpoint es
  **recargar el nivel y entrar en la fase 2 desde cero**: 0,012 ms. Los dos caminos
  terminan con el mismo hash. Lo único que hace falta recordar para volver es el nivel y
  la fase.

### P6: el ciclo de iteración

| Cambio | Pasos | Tiempo |
|---|---|---|
| Mover un spawn en Blender (arena) | Exportar con `blender -b` (0,11 s de exportación + ~0,9 s de abrir Blender) → cocinar (0,7 ms) → recargar (µs) | **~1 s** |
| Lo mismo en el castillo de 32 zonas | Exportar (1,36 s + ~1 s) → cocinar (18 ms) → recargar | **~2,4 s** |
| Cambiar cuántos zombies tiene una fase (texto) | Cocinar (~1 ms) → recargar | **~1 ms**, sin Blender |

Abrir Blender es la mayor parte del ciclo. Un complemento de Blender que exporte al
guardar, con Blender ya abierto, lo bajaría a la exportación sola (0,1 s en la arena).

### P7: el video

`comparativas_adr0027.mp4`, 78 s a 1280×720: la vista cenital de la arena cocinada (la
navmesh en rectángulos, la colisión, las antorchas con su radio, el trigger y los
marcadores por fase) y las tres fases corriendo desde el texto, a ×4. En el segundo 32
la caballera muere en la fase 2 y vuelve a su checkpoint. Un panel a la derecha explica
los colores y la fase actual.

### P8: zonas que se cargan al caminar

Se sumó cuando el autor pidió un mundo continuo, sin pantallas de carga entre zonas.

**El montaje:** el castillo de 32 zonas (4 columnas × 8 filas, 50 m entre centros). La
caballera camina de centro a centro en serpentina, a 2,5 m/s, y se detiene a pelear si
hay un zombie a menos de 8 m. Cada zona tiene los 10 zombies de la oleada de la arena.
Una zona **se pide** cuando la caballera está a menos de la distancia de precarga de su
borde, y **se descarga** cuando se aleja 30 m más (después de haber entrado a la
simulación). **El tiempo que tarda en llegar** se sortea con una semilla que imita al
disco, en tres rangos: SSD (5 a 30 ms), lento (50 a 400 ms) y con tirones (800 a
3000 ms). Cada caso corre con 5 semillas de disco. **El hash es de la trayectoria:** el
estado cada 60 pasos, con el número de paso.

Dos reglas para que aparezcan los zombies de una zona:

- **al cargar:** cuando la zona termina de llegar;
- **al entrar:** en el paso en que la caballera entra en la zona. Si la zona no llegó,
  el paso no corre (un frame de espera).

| Precarga | Disco | Regla | Hashes distintos en 5 discos | Frames de espera (peor) | Zonas cargadas (pico) | Instancias cargadas (pico) |
|---|---|---|---|---|---|---|
| 2 m | SSD | al cargar | **5** | 0 | 3 | 123 |
| 2 m | SSD | al entrar | **1** | 0 | 3 | 123 |
| 2 m | lento | al cargar | **5** | 0 | 3 | 123 |
| 2 m | lento | al entrar | **1** | 0 | 3 | 123 |
| 2 m | tirones | al cargar | **5** | 0 | 3 | 123 |
| 2 m | tirones | al entrar | **1** | **2 284** | 3 | 123 |
| 25 m | SSD | al cargar | **5** | 0 | 5 | 205 |
| 25 m | SSD | al entrar | **1** | 0 | 5 | 205 |
| 25 m | lento | al cargar | **5** | 0 | 5 | 205 |
| 25 m | lento | al entrar | **1** | 0 | 5 | 205 |
| 25 m | tirones | al cargar | **5** | 0 | 5 | 205 |
| 25 m | tirones | al entrar | **1** | 0 | 5 | 205 |

- **"Al cargar" da una partida distinta con cada disco,** incluso con un SSD: unos pocos
  frames de diferencia en la llegada cambian cuándo empiezan a caminar los zombies.
- **"Al entrar" da siempre el mismo hash.** La espera solo aparece con una precarga
  corta y un disco con tirones (2 284 frames, ~38 s sumados en todo el recorrido). Con
  25 m de precarga, que a 2,5 m/s son 10 s de margen, no hubo ninguna espera.
- **La primera versión de P8 tenía un error:** el hash era solo el del estado final, y
  como al final ya no quedan zombies, todas las corridas daban lo mismo. Un hash de
  determinismo tiene que cubrir la trayectoria, como el hash cada 60 pasos del ADR-0021.

**Qué se dibuja** (con 25 m de precarga, SSD y la regla "al entrar"; instancias contadas
cada 10 pasos). La cámara es la de la visión (2,3 m detrás de la caballera, mirando hacia
donde camina) con 82° de campo horizontal (51° vertical a 16:9). Cada pieza es una
esfera de 3 m:

| Qué | Media | Pico |
|---|---|---|
| Todo lo cargado | 129 | 205 |
| En el frustum | 35 | 76 |
| En el frustum, hasta 40 m (el final de la niebla, supuesto) | 30 | 41 |
| Lo anterior, solo la zona actual y sus vecinas | 30 | 41 |

Las zonas del castillo sintético son patios abiertos separados 50 m, así que la niebla
ya deja fuera a las que no son vecinas. Lo que tapan las paredes de un interior no se
midió.

### P8b: cargada no es lo mismo que simulando

Se sumó después de la auditoría del PR. Con una sola regla ("la zona entra al cruzar el
borde"), lo que estaba cerca de la caballera pero en la zona de al lado no tenía
colisión ni navmesh, y no estaba dicho cuándo sale una zona ni qué pasa con el tope.

**El montaje:** el mismo recorrido de P8, con dos estados por zona, cada uno con dos
distancias medidas hasta el borde de la zona:

- **la carga** (reloj de pared, el disco): se pide una zona a menos de 35 m y se
  descarga a más de 65 m, si no simula;
- **la simulación** (el paso fijo): una zona entra a menos de 10 m, y si no llegó, el
  frame no corre pasos; sale a más de la distancia de salida, y sus zombies se destruyen
  en orden. Las fases empiezan al cruzar el borde. Un zombie no se mueve a una zona que
  no simula (portales cerrados).

**El tope de zonas cargadas, primera versión:** se recorren las zonas en orden de índice;
con el tope lleno, se descarga la cargada más lejana que no simula. **Se bloqueó en las
15 corridas** con tope 5 y con tope 4: descargaba una zona más cercana que la que pedía,
y la que la simulación necesitaba nunca llegaba (se cortó cada corrida al minuto de
espera).

**Segunda versión:** se piden primero las zonas más cercanas, y con el tope lleno solo se
descarga una zona más lejana que la que se pide.

Con 40 m de salida:

| Tope | Hashes distintos en 5 discos | Esperas (peor) | Bloqueos | Zonas cargadas (pico) | Simulando (pico) |
|---|---|---|---|---|---|
| Sin tope | 1 | 0 | 0 | 9 | 4 |
| 5 | 1 | 0 | 0 | 5 | 4 |
| 4 | 1 | 0 | 0 | 4 | 4 |

Con 20 m de salida (el valor elegido), las mismas cifras salvo **2 zonas simulando como
máximo**, en los tres discos (SSD, lento y con tirones) y con Clang y GCC.

**La garantía del cocinador.** Que un recorrido no se bloquee no prueba que ninguno lo
haga. La condición es que, en cualquier punto de la navmesh, las zonas a menos de la
distancia de salida quepan en el tope menos uno: las que pueden estar simulando a la vez,
más una para cargar la siguiente. Calculado sobre todas las celdas caminables del
castillo sintético:

| Distancia de salida | Peor caso (zonas) | Tope mínimo |
|---|---|---|
| 15, 20 o 25 m | 4 | 5 |
| 30 m | 6 | 7 |
| 40 m | 9 (cerca de una esquina) | 10 |

Por eso la salida quedó en 20 m (10 m más que la entrada, para que una zona no entre y
salga muchas veces en el borde) con tope 5.

## Cómo repetirlo

```sh
# en una carpeta vacía, con cgltf clonado en ../dl/cgltf
blender -b --factory-startup -P arena.py -- out base
blender -b --factory-startup -P arena.py -- out moved
blender -b --factory-startup -P arena.py -- out castle 8
blender -b --factory-startup -P arena.py -- out32 castle 32
python3 -I inspect_glb.py out/base.glb                # P1: qué llegó al glTF
clang++ -std=c++20 -O2 -I../dl/cgltf l27.cpp -o l27_clang
g++     -std=c++20 -O2 -I../dl/cgltf l27.cpp -o l27_gcc
./l27_clang cook out/base.glb arena.nivel out/arena.lvl           # P3
./l27_clang cook out/base.glb arena.nivel out/arena_rev.lvl reverse
./l27_gcc   cook out/base.glb arena.nivel out/arena_gcc.lvl
sha256sum out/*.lvl
./l27_clang load out/arena.lvl 300                                 # P4
./l27_clang phases out/arena.lvl data > p_data.txt                 # P5
./l27_clang phases out/arena.lvl cpp  > p_cpp.txt
diff p_data.txt p_cpp.txt
./l27_clang phases out/arena.lvl data 7000                         # checkpoint
bash make_video.sh                                                 # P7
./l27_clang cook out32/castle.glb castle.nivel out32/castle.lvl
./l27_clang stream out32/castle.lvl                                # P8
./l27_clang stream2 out32/castle.lvl 40                            # P8b, salida 40 m
./l27_clang stream2 out32/castle.lvl 20                            # P8b, salida 20 m
```
