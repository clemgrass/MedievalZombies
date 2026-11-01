# Prototipo de la IA y la navegación

Nota de investigación del [ADR-0023](../adr/0023-ia-y-navegacion.md). Guarda las
mediciones que sostienen la decisión, para poder repetirlas.

Fecha: 2026-11-01, en la máquina de referencia del ADR-0020 (Dell XPS 13 9360,
i7-8550U, 8 GB de RAM). Son mediciones de laboratorio para decidir, no del protocolo del
ADR-0020. El prototipo es código descartable, fuera del repo.

| Qué | Versión |
|---|---|
| Linux / glibc | 7.2.8 / 2.44 |
| Clang | 23.1.3, con `-O2` (tiempos) y de `-O0` a `-O3` (determinismo) |
| GCC | 16.2.1, de `-O0` a `-O3` (determinismo) |
| ffmpeg | para el video comparativo |

## El prototipo

Un programa en C++20, de un solo hilo, que simula **una arena de 40×40 m vista desde
arriba** a 60 Hz, con paso fijo (ADR-0012). Todo es 2D sobre el suelo: alcanza para
medir la navegación y las decisiones de la horda en la arena plana del corte vertical.
Dentro del paso no asigna memoria: todo se reserva al iniciar.

**La escena:**

- **La arena:** muros exteriores, 8 columnas en anillo y 7 restos de muros y escombros,
  todos cajas, como el escenario del ADR-0016. La escena **puerta** cruza la arena con
  un muro que tiene una sola puerta de 2 m.
- **50 zombies** con la mezcla de estados del ADR-0018, repartida por un hash del índice
  (sin azar): 70 % enteros, 10 % cojos (velocidad × 0,55), 15 % que se arrastran
  (× 0,4, y una cápsula más ancha, de 0,45 m de radio) y 5 % sin brazos (no agarran).
  Caminan entre 1,0 y 1,4 m/s, con un radio de 0,35 m.
- **La caballera** sigue un recorrido fijo a 1,6 m/s, con pausas de 3 s en 6 puntos. No
  pelea: el agarre se resuelve solo a los 2 s, empujando a todos 1 m hacia atrás (una
  simplificación del ADR-0018).
- **El mago y su guardaespaldas** (opcional): el mago se aleja si la caballera está a
  menos de 8 m e invoca un zombie cada 6 s en la esquina más lejana. El guardaespaldas
  se pone entre el mago y la caballera, a 1,5 m del mago.
- **Los controladores** se mueven y se sacan de lo que penetran, como en el ADR-0016:
  dos iteraciones de zombie contra zombie (mitad y mitad), contra la caballera (que no
  se mueve) y contra las cajas.

**Lo que se compara:**

| Capa | Variantes |
|---|---|
| Navegación | A* en grilla (8 vecinos, sin cortar esquinas, suavizado por línea de vista); A* en una navmesh de rectángulos con el *funnel* de Mononen; campo de flujo (Dijkstra desde la caballera, con montículo o con la cola de Dial) |
| Evasión local | Ninguna (solo el empuje de los controladores); separación de Reynolds; ORCA |
| Decisión | FSM con `switch`; BT plano (nodos en un arreglo, hojas por `switch`); *utility* con bono a la acción actual |
| Coordinación | Todos atacan cuando quieren; 2, 3 o 4 *tokens* de ataque |
| Reparto | Pensar cada 1, 2, 4 u 8 pasos; recalcular el camino cada 15 a 120 pasos |

**Detalles que importan para leer los números:**

- **La grilla** marca como bloqueada toda celda cuyo centro esté a menos de 0,40 m de
  una caja. Así el camino ya deja espacio para la cápsula del zombie. La que se arrastra
  (0,45 m) puede rozar las esquinas, y el controlador lo corrige.
- **La navmesh** sale de fusionar celdas libres de esa grilla en rectángulos (convexos).
  Es una versión mínima de lo que hace Recast con vóxeles, regiones y contornos: sirve
  para medir A* sobre polígonos y el *funnel*, no para el castillo.
- **La línea de vista** (un recorrido DDA de Amanatides y Woo sobre la grilla) es un
  atajo común a todas las navegaciones: si el zombie ve a su objetivo, camina directo y
  no pide camino.
- **El camino se recalcula** cada 30 pasos (repartido por índice), o antes si el
  objetivo se alejó más de 1,5 m de su final.
- **ORCA** sigue a RVO2 (van den Berg y otros): hasta 10 vecinos a menos de 3 m,
  horizonte de 1 s y responsabilidad de la mitad. Contra la caballera y contra un zombie
  que no se mueve (preparando el ataque o agarrando), toda la responsabilidad.
- **La separación de Reynolds** suma un empuje desde cada vecino más cerca que la suma de
  los radios más 0,3 m.
- **El mago** es una FSM propia en los tres modelos de decisión: los jefes se escriben
  aparte en todos.

**Cómo se mide:** `steady_clock` alrededor de cada sección del paso; 3600 pasos (60 s de
juego) sin contar los primeros 60; 5 corridas fijadas al núcleo 3 con `taskset`. Las
tablas dan la **mediana de las 5 corridas** de la media, el p99 y el máximo por paso, en
µs. El reloj de la CPU quedó libre, así que hay ruido: sirven para comparar órdenes de
magnitud, no décimas.

## Resultados

### P2: la navegación

Arena, ORCA, FSM y todos atacan. "Navegar" es todo lo que da la velocidad deseada:
caminos, línea de vista, el campo de flujo y seguirlos.

| Variante | Navegar (µs: media / p99 / máx.) | Total IA (µs) | Caminos o campos en 60 s |
|---|---|---|---|
| A* en grilla de 0,25 m | 28 / 694 / 1156 | 83 / 721 / 1194 | 714 caminos |
| A* en grilla de 0,5 m | 10 / 177 / 307 | 66 / 208 / 348 | 742 caminos |
| **A* en navmesh (de 0,25 m)** | **9 / 36 / 112** | **63 / 114 / 211** | 762 caminos |
| A* en navmesh (de 0,5 m) | 5 / 15 / 80 | 60 / 112 / 177 | 716 caminos |
| Flujo de 0,25 m, montículo | 84 / 3437 / 4459 | 137 / 3499 / 4492 | 78 campos |
| Flujo de 0,25 m, Dial | 35 / 1142 / 2094 | 90 / 1184 / 2137 | 78 campos |
| Flujo de 0,5 m, Dial | 10 / 299 / 498 | 66 / 346 / 566 | 46 campos |

| Datos de la arena | Grilla de 0,25 m | Grilla de 0,5 m |
|---|---|---|
| Celdas (caminables) | 25 600 (22 956) | 6400 (5727) |
| Grilla de bloqueo | 25 KB | 6 KB |
| Campo de flujo (`float`) | 100 KB | 25 KB |
| Navmesh: rectángulos y portales | 80 y 236 (~6 KB) | 37 y 122 (~3 KB) |
| Construir la navmesh | 0,12–0,17 ms | 0,03 ms |

La navmesh guarda además una tabla de celda a rectángulo (100 KB a 0,25 m) para ubicar
un punto. Una navmesh de verdad lo resuelve con una grilla o un árbol de polígonos.

**Escala** (arena, ORCA, FSM; µs por paso):

| Zombies | Navmesh: navegar | Navmesh: evasión | Navmesh: total | Flujo 0,5 m: total |
|---|---|---|---|---|
| 50 | 9 / 36 / 112 | 36 / 71 / 160 | 63 / 114 / 211 | 66 / 346 / 566 |
| 100 | 22 / 70 / 172 | 129 / 214 / 334 | 207 / 347 / 528 | 206 / 418 / 759 |
| 200 | 48 / 138 / 241 | 368 / 609 / 747 | 602 / 1020 / 1275 | 609 / 1046 / 1541 |

### P3: la evasión local

Navmesh y FSM. En la escena puerta, 50 zombies cruzan hacia la caballera, que espera
detrás del muro. "Solapamiento" es la suma de lo que se penetran los pares de zombies
**antes** de que los controladores los separen, promediada por paso: mide cuánto
trabajo le deja la evasión al empuje. "Giro" es cuánto cambia la dirección de cada
zombie por paso.

| Variante | Evasión (µs) | Solapamiento | Giro por paso | Cruzan 10 / 25 / 40 |
|---|---|---|---|---|
| Ninguna | 0,2 / 0,8 / 6,8 | 0,88 m | 0,08° | 9,7 / 13 / 18 s |
| Reynolds | 6 / 12 / 61 | 0,36 m | 1,01° | 9,9 / 14 / 21 s |
| ORCA | 36 / 66 / 150 | 0,022 m | 1,32° | 9,8 / 15 / 21 s |

En la arena, lo mismo: solapamiento 0,82 m, 0,36 m y 0,022 m; giro 0,40°, 1,22° y
1,44°.

### P4: el modelo de decisión

Navmesh, 3 *tokens*, con el mago, el guardaespaldas y percepción por vista (15 m y línea
de vista; un zombie alertado alerta a los que están a menos de 4 m). Al final hay 62
agentes, con los invocados.

| Modelo | Decidir (µs: media / p99 / máx.) | Hash del estado |
|---|---|---|
| FSM | 1,0 / 2,5 / 15 | `395a2ca226a966f2` |
| BT plano | 5,4 / 9,3 / 36 | `395a2ca226a966f2` |
| *Utility* | 1,7 / 3,2 / 27 | `395a2ca226a966f2` |

**Los tres dan el mismo hash:** toman exactamente las mismas decisiones, así que la
diferencia de costo es solo del modelo. "Decidir" incluye la percepción y el reparto de
los *tokens*. Sin percepción (la horda sabe dónde está la caballera), la FSM baja de
1,7 a 1,1 µs en otra corrida.

**Lo que costó sumar el guardaespaldas** a cada modelo:

- **FSM:** un `if` al principio de la función, con su propia lógica (4 líneas).
- **BT:** un subárbol más en la lista de prioridades, una condición y una acción
  nuevas. El orden de prioridad se lee en el árbol.
- ***Utility*:** una acción nueva con su puntaje, y **un factor `(1 - guardaespaldas)`
  en los puntajes de las demás**, para que no ganen. Cada tipo nuevo toca los puntajes
  de todos.

### P5: la coordinación del ataque

Arena, navmesh, FSM y percepción total. 60 s.

| Variante | Ataques | Agarres | Máx. agarrando a la vez | Tiempo con 0 / 1 / 2 / 3 / 4+ agarrando |
|---|---|---|---|---|
| Todos atacan | 115 | 101 | 7 | 39 / 10 / 8 / 9 / **33 %** |
| 2 *tokens* | 24 | 24 | 2 | 62 / 9 / 29 / 0 / 0 % |
| 3 *tokens* | 34 | 29 | 3 | 63 / 15 / 12 / 10 / 0 % |
| 4 *tokens* | 42 | 31 | 4 | 60 / 15 / 16 / 9 / 0,6 % |

Sin *tokens*, la caballera pasa un tercio del tiempo con 4 o más zombies agarrándola. Los
que no tienen *token* esperan en un anillo a 1,6 m y giran despacio alrededor.

### P6: el reparto en el tiempo

Arena, navmesh, FSM y 3 *tokens*.

| Pensar cada | Decidir (µs) | Navegar (µs) | Ataques |
|---|---|---|---|
| 1 paso | 1,0 / 2,4 / 24 | 9,0 / 35 / 84 | 34 |
| 2 pasos | 0,8 / 1,8 / 37 | 5,6 / 28 / 76 | 34 |
| 4 pasos | 0,7 / 2,1 / 27 | 4,5 / 26 / 54 | 32 |
| 8 pasos | 0,7 / 2,1 / 14 | 4,2 / 26 / 85 | 35 |

Pensar menos seguido ahorra sobre todo en navegar, porque la línea de vista se guarda
entre decisiones. Recalcular el camino cada 15, 30, 60 o 120 pasos deja el p99 de
navegar en 56, 36, 27 y 23 µs, sin cambio visible en los ataques.

### P7: el determinismo

El mismo hash del estado al final de 60 s, en tres configuraciones (navmesh con BT,
*tokens*, vista y el mago; flujo con Dial y pensar cada 4; puerta con grilla y Reynolds):

| Compilador | Resultado |
|---|---|
| Clang `-O0`, `-O3`; GCC `-O0`, `-O2`, `-O3` | **El mismo hash en las tres** |
| Clang `-O3 -march=native` | Otro hash en las tres |

Las colas de prioridad desempatan por índice de celda, y los *tokens* por distancia y
después por índice: así el orden no depende de la implementación de la cola.
`-march=native` activa FMA y cambia los redondeos, como ya midió el ADR-0021.

## El video

`comparativas_adr0023.mp4` (90 s, fuera del repo, en el scratchpad de la sesión),
vista desde arriba en tiempo real. Cada zombie tiene el color de su estado: verde
persigue, amarillo espera su turno, naranja prepara el ataque, rojo agarra, azul se
recupera y gris no vio a nadie. La caballera es blanca, el mago violeta y el
guardaespaldas celeste. Las líneas grises son los caminos.

1. **La puerta:** sin evasión, con Reynolds y con ORCA.
2. **La navegación:** A* en grilla, en navmesh con *funnel* y el campo de flujo.
3. **El ataque:** todos a la vez contra 3 *tokens*, con el mago.

## Lo que mostró

- **Con 50 zombies, navegar es barato si la estructura es la adecuada.** La navmesh
  cuesta 9 µs por paso de media. Lo que se pasó del presupuesto fue el campo de flujo
  ingenuo: **3,4 ms en p99** cada vez que se recalcula, por recorrer 25 600 celdas con
  un montículo. Lo que pesaba era la cola de prioridad, no el algoritmo: la cola de
  Dial (costos enteros y 15 baldes) lo baja 3 veces, y una grilla de 0,5 m otras 4.
- **El campo de flujo escala con el área, y A* en la navmesh con la cantidad de
  polígonos.** La arena de 40×40 m tiene 80 rectángulos y 25 600 celdas. El castillo y
  sus zonas son más grandes y tienen pisos y escaleras, que una grilla 2D no representa.
  El flujo empieza a convenir con cientos o miles de agentes que van al mismo lugar (el
  caso de un RTS). Con 200 zombies todavía no gana.
- **Los picos salen de juntar trabajo en un paso,** no de la media: varios caminos
  recalculados a la vez, o un campo de flujo entero. El reparto por índice y una
  estructura con menos nodos los bajan.
- **La mayoría de los zombies no necesita camino:** en la arena casi todos ven a la
  caballera, y la línea de vista directa los resuelve. Los caminos son para rodear
  muros.
- **ORCA casi elimina el solapamiento** (40 veces menos que no hacer nada) por ~35 µs.
  Sin evasión, la horda llega igual de rápido a la puerta, pero se encima y los
  controladores hacen todo el trabajo de separarla. Reynolds queda en el medio. Desde
  arriba y a esta escala, las tres se ven parecidas: la columna sin evasión es algo
  más ancha y apretada en la puerta. La diferencia real se juzgará en 3D, con
  animación y de cerca, donde dos cuerpos encimados se notan.
- **El costo de decidir es despreciable** en los tres modelos (de 1 a 5 µs por paso con
  62 agentes). Elegir el modelo es una cuestión de cómo se escribe, se lee y se depura,
  no de rendimiento. Con *utility*, cada tipo nuevo obliga a retocar los puntajes de los
  demás.
- **Hace falta coordinar el ataque:** sin límite, hasta 7 zombies agarran a la caballera
  a la vez, que es exactamente lo injusto que el ADR-0018 quiere evitar.
- **A 200 zombies manda la evasión** (368 µs de media), por la búsqueda de vecinos de
  fuerza bruta. Con una grilla de vecinos bajaría, pero a 50 no hace falta.
- **El determinismo se sostiene** con los flags del ADR-0008, siempre que las colas
  desempaten por índice.

## Lo que toca a otros ADRs

- **ADR-0016:** dejó abierto "cómo se separan los zombies de la horda entre sí". La
  evasión le pasa al controlador una velocidad; el controlador sigue sacándolos de lo que
  penetran como red de seguridad.
- **ADR-0017:** si la navmesh se genera en el cocinador a partir de la colisión del
  escenario, es un paso más del cocinador.
- **ADR-0018:** la escala de tiempo por personaje también la aplica la IA a sus
  temporizadores, y el zombie ensartado no decide (su raíz la mueve el combate).
- **ADR-0020:** "Gameplay e IA" tiene 1,3 ms por paso. El prototipo usa ~0,1 ms en IA y
  navegación con 50 zombies, pero es 2D, sin el gameplay real y fuera del protocolo: el
  número sigue siendo hipótesis.
- **ADR-0027:** dónde aparecen los zombies, las oleadas y el director quedan para ese
  ADR (decisión del autor). Este prototipo los pone a mano.

## Cómo repetirlo

El prototipo es un solo archivo, `proto.cpp`, con un *script* que corre cada
configuración 5 veces:

```sh
clang++ -std=c++20 -O2 -o proto proto.cpp
taskset -c 3 ./proto --nav mesh --avoid orca --brain fsm --tokens 3
taskset -c 3 ./proto --scene door --nav mesh --avoid none
taskset -c 3 ./proto --nav flow --cs 0.5 --dial --n 200
./proto --nav mesh --tokens 3 --mage --sight --video tokens.mp4
```

Opciones: `--scene arena|door`, `--nav grid|mesh|flow`, `--cs` (tamaño de celda),
`--dial`, `--avoid none|sep|orca`, `--brain fsm|bt|util`, `--tokens N` (0 = todos
atacan), `--think N`, `--repath N`, `--n N`, `--sight`, `--mage`, `--steps N`,
`--video archivo.mp4` e `--info` (tamaños de la grilla y la navmesh).
