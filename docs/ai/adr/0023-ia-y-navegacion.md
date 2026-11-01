# ADR-0023: IA de los zombies y navegación

- **Estado:** Aceptado
- **Fecha:** 2026-11-01
- **Ticket:** #37
- **Reemplaza / Reemplazado por:** —

## Contexto y problema

Hay que decidir **cómo piensan y cómo se mueven los zombies**: qué deciden hacer en cada
momento, cómo llegan hasta la caballera por el escenario sin atravesarse entre ellos, y
cómo cambia todo eso cuando un tajo les quita una pierna o un brazo. Es lo que sostiene
el pilar 3 de la visión: **"la horda es un rompecabezas"**. El jugador se posiciona para
alinear zombies en un tajo cargado, y eso solo funciona si la horda se mueve de forma
legible.

Varios ADRs ya aceptados dejaron preguntas para este:

| ADR | Qué dejó abierto |
|---|---|
| ADR-0016 | Cómo se separan los zombies de la horda entre sí. Las líneas de visión de la IA usan sus rayos |
| ADR-0018 | Cómo se mueve y decide la horda, y a quién ataca. Los estados del zombie vivo (caminar, cojear, arrastrarse, retorcerse) salen de sus extremidades útiles; sus decisiones, de aquí |
| ADR-0020 | Que 50 zombies con IA quepan en "Gameplay e IA": 1,3 ms por paso, todavía hipótesis |
| Visión | "50 zombies con comportamientos distintos a la vez", el mago que invoca zombies y se defiende con un guardaespaldas, y los tipos de zombie del corte vertical |

Hay que decidir:

1. el **modelo de decisión**: máquina de estados, árbol de comportamiento, *utility* u
   otro;
2. **dónde vive el comportamiento de cada tipo**: en código o en datos;
3. la **percepción**: cómo se entera un zombie de que la caballera está ahí;
4. la **coordinación del ataque**: cuántos atacan a la vez y quién;
5. la **representación del espacio navegable**;
6. **de dónde salen esos datos**: a mano, en el cocinador o al cargar;
7. la **búsqueda de camino**;
8. el **movimiento local**: cómo se esquivan los zombies entre sí;
9. el **zombie mutilado y los estados especiales**;
10. la **escala y el costo**: cada cuánto piensa cada zombie y cuánto se gasta.

Lo que ya está decidido y limita este ADR:

- **ADR-0008:** sin `-ffast-math` ni `-march`, para que la simulación sea determinista.
- **ADR-0009:** en el código de cada frame no se usan `virtual` por entidad ni
  contenedores que asignen memoria. La recursión tiene que estar acotada.
- **ADR-0010:** regiones de memoria por sistema, con límites duros. Lo que vive un paso
  va en la arena del paso.
- **ADR-0012:** ECS con *sparse sets*, un hilo y paso fijo de 60 Hz. El orden del paso
  es gameplay → combate → corte → física → animación. Las órdenes van por colas y las
  consultas (un rayo, un barrido) son llamadas directas hacia abajo.
- **ADR-0015:** animación por capas: clips para la base (caminar, arrastrarse) y
  procedural encima. La IA dice qué hacer; la animación, cómo se ve.
- **ADR-0016:** los zombies son **controladores cinemáticos** que se mueven y se sacan de
  lo que penetran. Hay rayo y barrido de esfera. El escenario se describe con cajas,
  cápsulas y esferas.
- **ADR-0017:** el escenario sale de Blender y lo cocina el build. La colisión se marca
  con prefijos (`UBX_`, `UCP_`, `USP_`).
- **ADR-0018:** el agarre es una consulta al final del ataque del zombie. Sin brazos
  útiles, no agarra. Cada personaje tiene **su escala de tiempo** (la pausa de impacto,
  la espada clavada). El zombie ensartado no decide: avanza por la hoja. **Los personajes
  empujan los pedazos sin frenarse**, así que los pedazos no son obstáculos para
  navegar.
- **ADR-0020:** 50 zombies en el pico de la horda (R5) y partidas grabadas deterministas
  para medir.
- **ADR-0021:** Dear ImGui, el dibujo de depuración y el *replay* por paso.

**Fuera de este ADR** (decisión del autor, 2026-11-01): **dónde y cuándo aparecen los
zombies** (las oleadas, la horda que crece, los puntos de aparición) lo decide el
ADR-0027, junto con los niveles. El autor quiere elegir exactamente dónde están. Las
invocaciones del mago son una acción del mago, así que su comportamiento sí está aquí;
dónde aparecen los invocados, no.

## Criterios

Los cinco del ticket, más tres que salen de cruzar lo ya decidido:

1. **Rendimiento con muchos zombies** (ADR-0020): 50 zombies en 1,3 ms por paso, junto
   con el resto del gameplay.
2. **Encaje con el ECS** (ADR-0012), dentro de las reglas del ADR-0009.
3. **Comportamiento creíble y divertido.**
4. **Depurable:** se puede ver qué decidió cada zombie y por qué (ADR-0021).
5. **Aprendizaje.**
6. **Determinismo:** la misma partida grabada da el mismo hash del estado (ADR-0020 y
   ADR-0021).
7. **Justo y legible para el jugador:** el pilar 3. El ADR-0018 ya evita que otro zombie
   agarre a la caballera mientras tiene la espada clavada, porque sería injusto.
8. **Se escribe entre una persona:** sumar un tipo de zombie, o al mago, no exige
   construir un editor.

## Opciones

Los tiempos son de un prototipo medido en la máquina de referencia (ver Investigación):
una arena 2D de 40×40 m con 50 zombies a 60 Hz. Son µs por paso de simulación, con la
media, el p99 y el máximo.

### 1. El modelo de decisión

| Opción | Cómo funciona | Problema |
|---|---|---|
| **FSM** | Un estado por zombie (persigue, espera, ataca, agarra) y transiciones escritas a mano, con un `switch` | Con muchos estados, las transiciones crecen de a pares y se repiten |
| **FSM jerárquica** (HFSM, Harel, 1987) | Estados que contienen estados: "en combate" contiene "ataca" y "espera". Una transición del padre vale para todos sus hijos | Más estructura que mantener. *Left 4 Dead* usa una variante con una pila de acciones que se suspenden y se retoman |
| **Árbol de comportamiento** (BT) | Un árbol de selectores (prueba hijos hasta que uno funcione) y secuencias (todos en orden), con condiciones y acciones en las hojas. Se evalúa desde la raíz, así que reacciona solo | El BT de los libros y de muchos engines es un árbol de objetos con métodos virtuales, que el ADR-0009 no permite. Aquí sería **plano**: los nodos de cada tipo en un arreglo compartido, las hojas por `switch` y el estado de cada zombie en su componente |
| ***Utility AI*** | Cada acción calcula un puntaje con curvas sobre el estado (distancia, energía, cuántos atacan), y gana la de mayor puntaje | Difícil de predecir y de afinar: el comportamiento sale de cómo se cruzan las curvas |
| **GOAP** (*F.E.A.R.*) | Cada zombie tiene metas y un planificador busca, con A*, la secuencia de acciones que las cumple | Pensado para enemigos que planifican (cubrirse, flanquear). Un zombie no planifica. Caro por agente |

**Medido:** el mismo comportamiento escrito en los tres primeros modelos (perseguir,
esperar turno, atacar y agarrar, el que se arrastra, y un guardaespaldas). **Los tres dan
exactamente el mismo hash del estado**, así que deciden igual. Cuesta de 1 a 5 µs por
paso con 62 agentes: FSM 1,0, *utility* 1,7 y BT plano 5,4 µs de media. **El costo no
decide.** Lo que decide es cómo se escribe: al sumar el guardaespaldas, la FSM sumó un
`if`; el BT, un subárbol; y la *utility*, un factor en los puntajes de **todas** las
demás acciones, para que no le ganaran.

### 2. Dónde vive el comportamiento de cada tipo

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Código C++ por tipo** | Cada tipo de zombie tiene su función de decisión | Cambiar un número exige recompilar |
| **Código común + tabla de datos por tipo** | Un solo comportamiento en C++, con los números en la tabla de cada tipo (velocidad, alcance, espera entre ataques, si bloquea). El ADR-0018 ya tiene esa tabla para los materiales | Un comportamiento de verdad distinto (el que bloquea con el escudo) sigue siendo código |
| **Árbol cocinado desde un archivo** | La estructura del BT se escribe en un archivo de texto, el build lo cocina (ADR-0017) y se puede recargar en caliente | Es un formato propio más, con su cocinador, sus errores y sus tests |
| **Un lenguaje de *scripting*** (Lua y similares) | El comportamiento se escribe en un lenguaje interpretado | Una dependencia externa con su propio ADR. Esconde costo por frame y asigna memoria |

### 3. La percepción

| Opción | Cómo funciona | Problema |
|---|---|---|
| **La horda sabe** | Todo zombie activo conoce la posición de la caballera | En la arena alcanza. En el castillo, un zombie detrás de una puerta cerrada corre hacia ella |
| **Vista** | Distancia, ángulo de visión y un rayo de la física (ADR-0016) | Un rayo por zombie y por decisión. Medido: con 50 zombies, la vista no se nota en el costo |
| **Vista y oído** | Además, eventos de ruido con un radio (el tajo, un grito, una puerta) | Hay que decidir qué hace ruido |
| **Contagio** | Un zombie alertado alerta a los que tiene cerca | Barato. Hace que la horda reaccione en ola, y se puede leer |

### 4. La coordinación del ataque

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Todos atacan cuando quieren** | Cada zombie ataca apenas llega | **Medido: hasta 7 agarrando a la vez**, y un tercio del tiempo con 4 o más. Es lo injusto que el ADR-0018 quiere evitar |
| ***Tokens* de ataque** (*DOOM*, 2016) | Hay N fichas. Un zombie necesita una para atacar y la devuelve al terminar. Los demás esperan cerca | Hay que decidir quién recibe la ficha: el más cercano, el que está frente a la cámara, el que espera hace más tiempo. **Medido:** con 3 fichas, nunca más de 3 agarrando, y 34 ataques por minuto en vez de 115 |
| **Puestos alrededor de la caballera** | Un anillo de lugares; cada zombie toma uno libre y ataca desde ahí | Ordenado, pero rígido: con 50 zombies y 8 puestos, el resto igual necesita una regla |
| **Fichas y espera en anillo** | Fichas para atacar, y los que no tienen esperan a una distancia, girando despacio | Se ve como "esperar turno". En exceso se nota artificial |

### 5. La representación del espacio navegable

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Grilla** | Celdas libres u ocupadas sobre el suelo | Muchas celdas: la arena de 40×40 m tiene 25 600 a 0,25 m. Es 2D: los pisos y escaleras del castillo necesitan capas |
| ***Waypoints*** | Un grafo de puntos puestos a mano | El zombie solo sabe caminar por las líneas: no sabe qué hay entre ellas |
| **Navmesh** | Polígonos convexos que cubren el suelo caminable; dentro de uno se camina recto | Construirla es lo difícil (ver la dimensión 6). Es el estándar de la industria y representa pisos y rampas |
| **Campo de flujo** | Sobre una grilla, la distancia de cada celda al objetivo; cada zombie baja por la pendiente | Un campo por objetivo. Con un solo objetivo (la caballera) sirve a todos a la vez |

**Medido:** la misma arena es **80 rectángulos** de navmesh (~6 KB) contra 25 600 celdas
(25 KB de grilla y 100 KB de campo de flujo).

### 6. De dónde salen los datos de navegación

| Opción | Cómo funciona | Problema |
|---|---|---|
| **A mano en Blender** | Se modela la navmesh como una malla aparte, con un prefijo, igual que la colisión del ADR-0017 | Trabajo manual en cada nivel, y se desincroniza si cambia el escenario |
| **Generada en el cocinador** | El build la calcula desde la colisión del escenario, al estilo de Recast: vóxeles, regiones, contornos y polígonos | Es el algoritmo más largo de este ADR. Medido: la versión mínima (rectángulos sobre una grilla) se construye en 0,12–0,17 ms |
| **Generada al cargar el nivel** | El mismo algoritmo, pero en el juego | Tiempo de carga, y el código de construcción viaja en el ejecutable |
| **Generada y retocada** | El cocinador la genera, y un prefijo en Blender marca zonas para quitar o forzar | Más reglas, pero es como trabajan las herramientas de la industria |

**Recast/Detour** (Mononen), la librería que usan Unreal Engine y Godot, queda **como
referencia para estudiar y comparar, no como dependencia** (decisión del autor,
2026-11-01).

### 7. La búsqueda de camino

| Opción | Cómo funciona | Medido (navegar, µs) |
|---|---|---|
| **A* en grilla** | Por zombie, con suavizado por línea de vista | 0,25 m: 28 / 694 / 1156. 0,5 m: 10 / 177 / 307 |
| **A* en navmesh + *funnel*** | Por zombie, sobre los polígonos; el *funnel* (Mononen, 2010) tensa el camino por los portales | **9 / 36 / 112** |
| **Campo de flujo compartido** | Un Dijkstra desde la caballera cada vez que cambia de celda | 0,25 m con montículo: **84 / 3437 / 4459**. Con la cola de Dial: 35 / 1142 / 2094. 0,5 m con Dial: 10 / 299 / 498 |
| **A* jerárquico** (HPA*) | Un grafo de regiones encima del de celdas o polígonos | No medido: sirve para mapas mucho más grandes |

En todas, **si el zombie ve a su objetivo, camina directo** y no pide camino: en la
arena es el caso más común. **El campo de flujo escala con el área; A* en la navmesh,
con los polígonos.** Con 200 zombies en la arena, el campo de flujo todavía no le gana a
la navmesh.

### 8. El movimiento local

| Opción | Cómo funciona | Medido |
|---|---|---|
| **Solo el empuje** | Cada zombie va derecho; los controladores se separan al chocar (ADR-0016) | 0,2 µs. Solapamiento antes de separarlos: 0,88 m por paso |
| **Separación de Reynolds** (1999) | Una fuerza que aleja de los vecinos, sumada a la dirección deseada | 6 µs. Solapamiento 0,36 m |
| **ORCA** (van den Berg y otros, 2011) | Cada vecino recorta las velocidades que llevarían a chocar en el próximo segundo, y se elige la permitida más cercana a la deseada. Cada uno asume la mitad del esquive | 36 µs. **Solapamiento 0,022 m: 40 veces menos.** Es lo que usa Godot (RVO2) |

En la puerta de 2 m, las tres cruzan a 40 zombies en 18 a 21 s: la evasión no cambia el
ritmo, cambia cuánto se enciman. **A 200 zombies, ORCA pasa a ser lo más caro** (368 µs
de media) por buscar vecinos a fuerza bruta.

### 9. El zombie mutilado y los estados especiales

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Mismo comportamiento, otros números** | El estado que da el ADR-0018 (caminar, cojear, arrastrarse, retorcerse) cambia la velocidad, la cápsula, el alcance y si puede agarrar | Un zombie que se arrastra no persigue igual: va a los tobillos, se mete bajo un tajo alto |
| **Un comportamiento por estado** | Cada forma de moverse tiene su árbol o su FSM | Más comportamientos que escribir y probar |
| **Mismo comportamiento con ramas** | Uno solo, con ramas para lo que de verdad cambia (sin brazos no agarra; el que se retuerce no se mueve) | Las ramas crecen con cada tipo nuevo |

Y para navegar:

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Una navmesh para el más ancho** | Se infla por el radio mayor (el que se arrastra, 0,45 m) | Los demás no pasan por huecos donde cabrían |
| **Una navmesh por tamaño** | Como Recast y Godot: una por radio | El doble de datos y de construcción. El caballo (fuera del corte vertical) necesitará la suya |

Además, la IA **respeta la escala de tiempo** de cada personaje (ADR-0018): un zombie
congelado por la pausa no piensa ni cuenta tiempo. **El zombie ensartado no decide**:
su raíz la mueve el combate.

### 10. La escala y el costo

| Opción | Cómo funciona | Medido |
|---|---|---|
| **Todos piensan en cada paso** | Simple, y reacciona en 16,7 ms | Decidir: 1,0 µs; navegar: 9 µs |
| **Pensar cada N pasos, repartido por índice** | El zombie `i` piensa cuando `(paso + i) % N == 0`. Determinista | N = 4: navegar baja a 4,5 µs. Reacciona hasta 50 ms más tarde |
| **Recalcular el camino cada M pasos, repartido** | Igual que el anterior, solo para los caminos | M = 15 a 120: el p99 de navegar va de 56 a 23 µs |
| **Nivel de detalle por distancia** | Los lejanos o fuera de cámara piensan menos (como el *Active Area Set* de *Left 4 Dead*) | Con 50 zombies en una arena, no hace falta |

**Medido:** con 50 zombies, la IA y la navegación completas (decidir, navegar, ORCA y
mover) cuestan **63 / 114 / 211 µs** con la navmesh: menos del 10 % de 1,3 ms. Con 200
zombies, 602 / 1020 / 1275 µs. El prototipo es 2D y no tiene el gameplay real: el
presupuesto sigue siendo hipótesis.

## Investigación

### Prototipo medido

Un programa en C++20, de un hilo, medido en la máquina de referencia (ADR-0020) el
2026-11-01, fuera del protocolo. Simula una arena de 40×40 m vista desde arriba a 60 Hz,
con 50 zombies (enteros, cojos, que se arrastran y sin brazos), la caballera en un
recorrido fijo, y el mago con su guardaespaldas. El detalle y cómo repetirlo están en
[`../research/prototipo-de-ia-y-navegacion.md`](../research/prototipo-de-ia-y-navegacion.md).

| # | Qué | Resultado |
|---|---|---|
| P2 | Navegación | Navmesh 9 / 36 / 112 µs. A* en grilla de 0,25 m: p99 694 µs. Campo de flujo ingenuo: p99 **3,4 ms**; con Dial y 0,5 m, 0,3 ms |
| P3 | Evasión | ORCA: 36 µs y 40 veces menos solapamiento. Reynolds: 6 µs, 2,4 veces menos. Igual ritmo por la puerta |
| P4 | Decisión | FSM 1,0, *utility* 1,7, BT plano 5,4 µs; **el mismo hash en los tres** |
| P5 | Coordinación | Sin fichas: hasta 7 agarrando. Con 3: nunca más de 3 |
| P6 | Reparto | Pensar cada 4 pasos: navegar a la mitad. Los picos bajan repartiendo los caminos |
| P7 | Determinismo | El mismo hash con Clang y GCC, de `-O0` a `-O3`. `-march=native` lo cambia |
| — | Escala | 200 zombies: 1,0 ms en p99. A esa cantidad, manda la búsqueda de vecinos de ORCA |

Un video de 90 s, con la vista desde arriba en tiempo real, compara la evasión en la
puerta, las tres navegaciones y el ataque con y sin fichas.

Lo que mostró, además de los números:

- **Lo que pasó del presupuesto fue la estructura de datos, no el algoritmo.** El campo
  de flujo de 0,25 m recorría 25 600 celdas con un montículo. La cola de Dial (costos
  enteros y un anillo de 15 baldes) lo bajó 3 veces sin cambiar ni una dirección (el
  mismo hash).
- **Los picos salen de juntar trabajo en un paso**: varios caminos recalculados a la vez,
  o un campo de flujo entero. Repartir por índice los baja sin perder determinismo.
- **La mayoría de los zombies no necesita camino:** en una arena abierta, la línea de
  vista directa resuelve casi todo.
- **El determinismo exige desempatar por índice** en las colas de prioridad y al repartir
  las fichas, para que el orden no dependa de la implementación de la cola.

**Lo que toca a otros ADRs:**

- **ADR-0016:** la evasión le pasa al controlador una velocidad deseada. El controlador
  sigue sacando a los zombies de lo que penetran, como red de seguridad. Responde lo que
  el ADR-0016 dejó abierto.
- **ADR-0017:** si la navmesh se genera en el cocinador, es un paso más del cocinador, y
  puede necesitar un prefijo nuevo para marcar zonas.
- **ADR-0018:** la IA aplica la escala de tiempo de cada personaje a sus temporizadores.
- **ADR-0020:** el reparto de "Gameplay e IA" sigue como hipótesis.
- **ADR-0027:** si la navmesh se genera en el cocinador, el cocinador tiene que ver la
  colisión del nivel armado. Cómo se arma un nivel lo decide ese ADR.

### Cómo lo hacen otros

- ***Left 4 Dead*** (Booth, 2009): A* sobre una navmesh, sin optimizar el camino.
  Sigue el camino de forma reactiva: camina hacia un punto más adelante y deja que la
  evasión local resuelva los detalles; "(re)calcular el camino es barato". Cada actor se
  divide en *Locomotion*, *Body* (la animación), *Vision* e *Intention*, con
  comportamientos concurrentes hechos de acciones en una pila (una FSM jerárquica). El
  *Active Area Set* solo puebla las zonas cercanas a los jugadores.
- ***Halo 2*** (Isla, GDC 2005): el árbol de comportamiento, en realidad un grafo
  acíclico de ~50 comportamientos. Es el origen del BT en los videojuegos.
- ***F.E.A.R.*** (Orkin, GDC 2006): GOAP, con una FSM de solo tres estados (ir, animar,
  usar un objeto) y un planificador encima.
- ***Utility AI*** (Mark y Dill, GDC 2010; Mark, *Behavioral Mathematics for Game AI*,
  2009): decidir con curvas de respuesta en vez de reglas.
- ***DOOM*** (2016), según la charla de Loudy y Campbell en la GDC 2018: fichas de ataque
  compartidas; un demonio pide una para atacar y la devuelve al terminar. En dificultades
  más altas hay más fichas, y la prioridad depende, entre otras cosas, de si está en
  pantalla. (Fuentes secundarias: la charla no se revisó directamente.)
- ***Supreme Commander 2*** (Emerson, *Game AI Pro*, 2013): campos de flujo por *tiles*
  para mover cientos o miles de unidades sin un camino por unidad.
- **Recast y Detour** (Mononen): la navmesh generada desde la geometría por vóxeles, y
  el *funnel*. Unreal Engine y Godot la usan; Godot evita choques con RVO2 (ORCA).
- ***The Behavior Tree Starter Kit*** (Champandard y Dunstan, *Game AI Pro*, 2013): un BT
  con los nodos en memoria contigua y el estado por agente aparte. Es la forma de BT que
  encaja con el ADR-0009.

## Decisión

Elijo un árbol de comportamiento plano para los zombies y una navmesh propia, generada
por el cocinador. El prototipo mostró que el costo no decide el modelo: los tres
deciden igual por unos pocos µs. Elijo el BT porque crece mejor con los tipos de zombie
de la visión y porque el orden de prioridad se lee en el árbol. Además es el estándar de
la industria, y escrito como datos contiguos encaja con las reglas del ADR-0009. Los
jefes son pocos y únicos, así que cada uno lleva su propia máquina de estados.

La navmesh es lo que usa la industria, es chica y representa los pisos y escaleras que
va a tener el castillo. Generarla yo, en vez de usar Recast, es el trabajo más largo de
este ADR, pero es justamente lo que quiero entender. ORCA hace que la horda no se encime
casi sin costo. Las fichas de ataque hacen que la horda sea peligrosa sin ser injusta:
sin ellas, siete zombies agarran a la caballera a la vez.

Ya decidido sobre el alcance (2026-11-01): **dónde y cuándo aparecen los zombies**
(oleadas, director, puntos de aparición) va al ADR-0027, porque quiero elegir
exactamente dónde están. **Recast/Detour es una referencia**, no una dependencia.

| # | Dimensión | Elección |
|---|---|---|
| 1 | Modelo de decisión | **BT plano para los zombies.** Los nodos de cada tipo viven en un arreglo compartido, con los hijos contiguos: selector, secuencia, negación, condición y acción. Las hojas se despachan con un `switch` sobre su id. Se evalúa **desde la raíz en cada decisión**, así que reacciona solo. **Las acciones comprometidas** (preparar el ataque, agarrar, recuperarse) avanzan en cada paso fuera del árbol, y el árbol las respeta con una condición de "ocupado". **Los jefes** (el mago; después, el rey y la reina) llevan **una FSM propia** con `switch` |
| 2 | Dónde vive el comportamiento | **El árbol de cada tipo se arma en C++ al iniciar,** con un constructor, fuera del paso. **Los números de cada tipo** (velocidad, alcance, tiempos del ataque, radios de percepción) van en la **tabla de datos del tipo**, la misma del ADR-0018, y se ajustan sin recompilar |
| 3 | Percepción | **Vista:** distancia, ángulo de visión y un rayo de la física (ADR-0016). **Oído:** eventos de ruido con posición y radio, que emite el combate (el tajo, el rebote en el metal). **Contagio:** un zombie alertado alerta a los que tiene cerca |
| 4 | Coordinación del ataque | **Fichas de ataque, 3 para empezar** (configurable), y **espera en anillo** para los que no tienen ficha. **La ficha va al más cercano, con ventaja para los que están en cámara.** El desempate es por entidad. Se toma al empezar a preparar el ataque y se devuelve al terminar de recuperarse, o antes si su dueño deja de poder atacar |
| 5 | Espacio navegable | **Navmesh:** polígonos convexos con sus portales |
| 6 | Origen de los datos | **La genera el cocinador desde la colisión del escenario** (ADR-0017), con un algoritmo propio al estilo de Recast. **Un prefijo nuevo en Blender** permite retocarla: quitar o forzar zonas |
| 7 | Búsqueda de camino | **A* sobre los polígonos y *funnel*.** Si el zombie ve a su objetivo sobre la navmesh, camina directo. **Seguimiento reactivo**, como *Left 4 Dead*: camina hacia un punto más adelante en el camino, y la evasión resuelve los detalles |
| 8 | Movimiento local | **ORCA entre los agentes.** Los muros los resuelven la navmesh y el controlador (ADR-0016). ORCA le pasa al controlador una velocidad deseada, y el controlador sigue sacando a los zombies de lo que penetran, como red de seguridad |
| 9 | Zombie mutilado | **Un árbol por tipo, con ramas para lo que de verdad cambia,** y **los números de cada estado de locomoción** del ADR-0018 en la tabla del tipo. **Una navmesh inflada por el radio mayor del corte vertical** (0,45 m, el que se arrastra). **La IA respeta la escala de tiempo** de cada personaje, y **el zombie ensartado no decide** |
| 10 | Escala y costo | **Cada zombie decide en cada paso.** **El camino se recalcula repartido por entidad,** cada 30 pasos (configurable), o antes si el objetivo se aleja del final del camino. **Sin nivel de detalle** con 50 zombies |

## Consecuencias

- **La IA corre al principio del paso, dentro del gameplay** (ADR-0012), en este orden:

  ```
  percepción → fichas → decisión (BT o FSM) → camino → ORCA → velocidad deseada
  ```

  La física mueve después a los controladores con esa velocidad, en su turno del mismo
  paso. La IA no mueve a nadie: escribe la velocidad deseada en un componente.
- **Los componentes**, con nombres de partida (ADR-0013):
  - `ai_brain`: el tipo (que elige el árbol o la FSM), la acción en curso, sus
    temporizadores y si tiene ficha;
  - `ai_perception`: si está alertado y dónde vio a la caballera por última vez;
  - `nav_agent`: el camino como **un arreglo fijo de 32 esquinas**, la esquina actual, el
    paso del último cálculo y la velocidad deseada. Si el camino tiene más esquinas, el
    zombie sigue las primeras 32 y lo recalcula al llegar a la última.

  Son ~0,5 KB por zombie: ~25 KB con 50, dentro de "Mundo y ECS".
- **El BT plano respeta el ADR-0009:** no tiene `virtual` por entidad ni memoria por
  decisión, y la recursión está acotada por la profundidad del árbol. El constructor
  verifica al iniciar que ningún árbol pase de un máximo, por ejemplo 16 niveles. El
  costo es que **un tipo de nodo nuevo se suma editando el `switch`**, no heredando de
  una clase.
- **En el corte vertical, todos los zombies comparten un árbol:** el campesino y los
  soldados con cota de malla o con armadura completa se distinguen por los números de
  su tabla (velocidad, umbrales del ADR-0018). **El guardaespaldas tiene su propio
  árbol**, cuya primera rama es interponerse entre el mago y la caballera. **El mago es
  una FSM:** se aleja de la caballera, invoca zombies y llama al guardaespaldas. Dónde
  aparecen los invocados lo decide el ADR-0027.
- **El ruido sobrevive de un paso al siguiente.** El combate corre después del gameplay,
  así que un tajo del paso N se oye en el paso N+1. Las colas del ADR-0012 viven en la
  arena del paso y desaparecen al terminarlo, así que **el ruido va en un arreglo fijo
  del mundo** (por ejemplo, hasta 16 ruidos). El combate lo escribe y la IA lo lee y lo
  vacía en el paso siguiente: es la regla del ADR-0010, que pide guardar en un
  componente lo que un paso le deja al siguiente. **Si el arreglo está lleno,** el ruido
  nuevo reemplaza al de radio más chico, si es más grande que él; si no, se descarta. Un
  contador del registro por frame cuenta los descartes (ADR-0010: los pools que llena el
  juego degradan con su regla). En el corte vertical, **un zombie alertado no se olvida**
  de la caballera.
- **La ventaja de la cámara** usa la cámara del paso anterior: un zombie dentro de su
  ángulo de visión cuenta como más cercano, con un descuento configurable en metros. La
  cámara ya tiene que avanzar con la simulación, porque de ella sale el punto apuntado
  del tajo (visión y ADR-0018): una partida grabada solo se repite si la cámara se
  repite. Así la ventaja tampoco rompe el determinismo. Es lo que hace *DOOM*: el que
  está en pantalla tiene prioridad para atacar.
- **Las fichas y el ADR-0018:** durante la secuencia de la espada clavada, los demás
  zombies siguen con sus fichas, pero con la escala de tiempo baja. **No usan ficha:** el
  guardaespaldas, los jefes y **el agarre que sale de la espada clavada** (el zombie
  ensartado que alcanza a la caballera, o la energía que llega a 0), porque ese zombie no
  decide: lo lleva el combate. En el corte vertical todos los ataques de la horda común
  son agarres, así que **las fichas limitan los agarres de la horda común**; con el
  ensartado y el guardaespaldas puede haber hasta dos más.
- **La ficha vuelve antes si su dueño deja de poder atacar:** si muere, si pierde los
  brazos útiles, si queda ensartado o si pasa a escala 0 por un pedido de corte en la
  cola. Sin esta regla, una ficha quedaría tomada por un zombie que ya no termina de
  recuperarse, y la horda se quedaría sin fichas.
- **El punto del agarre lo marca el temporizador de la tabla del tipo,** no el clip. La
  IA hace avanzar el ataque con ese temporizador, que es determinista y vive en la
  simulación; la animación (ADR-0015) reproduce el clip del agarre escalado a esa
  duración. Así "la animación llega a su punto" (ADR-0018) en el mismo paso en que el
  temporizador termina, y la consulta del agarre tiene una sola fuente.
- **La navmesh, en el cocinador:**
  - lee las cajas, cápsulas y esferas de la colisión (ADR-0017) y el piso, y **la
    escribe en el binario cocinado** (ADR-0017): polígonos convexos con vértices en 3D,
    portales entre vecinos y una grilla para encontrar en qué polígono está un punto.
    **Pide al ADR-0027** que la colisión del nivel armado quede al alcance del
    cocinador; dónde queda la navmesh dentro de lo cocinado lo fija ese ADR;
  - es **determinista:** cocinar dos veces la misma escena da los mismos bytes;
  - **el radio de inflado tiene una sola fuente:** el cocinador lo recibe como parámetro
    del nivel y lo guarda en la sección de la navmesh. Al cargar el nivel, el juego
    verifica que ningún radio de las tablas de tipos lo supere, y si alguno lo supera,
    falla con el nombre del tipo y los dos radios, nunca en silencio. Así subir el radio
    de una tabla sin recocinar no deja zombies pasando por huecos donde no caben;
  - **crece por partes.** Para la arena plana del corte vertical basta una versión simple
    (el suelo en una grilla, inflada por el radio, y celdas fusionadas en polígonos).
    Las alturas, pisos y escaleras del castillo piden el resto del algoritmo de Recast
    (campo de alturas, regiones y contornos), cuando el ADR-0027 defina los niveles;
  - el nombre del prefijo nuevo lo fija el ticket del cocinador. Como el ADR-0017 hace
    fallar el build con cualquier prefijo desconocido, el prefijo se suma a su lista.
- **La línea de vista para caminar directo es un rayo sobre la navmesh,** que recorre los
  polígonos a lo largo del segmento, como el `raycast` de Detour. No es el rayo de la
  física, porque lo que importa es si se puede caminar, no si hay algo en medio. **La
  vista de la percepción sí usa el rayo de la física.**
- **La memoria de A*** (costos, padres y la cola abierta) tiene el tamaño de la cantidad
  de polígonos del nivel y se reserva al cargarlo, en la memoria del nivel (ADR-0010 y
  ADR-0028). Una búsqueda no asigna nada.
- **ORCA**, con los valores del prototipo para empezar: hasta 10 vecinos a menos de 3 m y
  un horizonte de 1 s. Cada uno asume la mitad del esquive; contra la caballera y contra
  un zombie quieto (preparando el ataque, agarrando, congelado o ensartado), todo el
  esquive. **Los vecinos se buscan por fuerza bruta;** con 200 zombies esa búsqueda
  domina, y una grilla de vecinos llega cuando el profiler la pida.
- **El zombie mutilado:** después de un corte, el pedazo vivo (ADR-0018) sigue con el
  mismo árbol y su nuevo estado de locomoción. Su camino se descarta, porque cambió su
  cápsula. **Las ramas del corte vertical:** sin brazos útiles, se acerca pero no ataca
  ni pide ficha; el que se retuerce no se mueve ni decide. **El intacto no pasa por
  huecos más angostos que el que se arrastra.** Una navmesh por tamaño llega con el
  caballo.
- **El reparto de los recálculos usa el índice estable de la entidad** (el del *handle*,
  ADR-0012), no su posición en el arreglo denso del *sparse set*. Las dos son
  deterministas: con un hilo y destrucción diferida, las muertes ocurren en el mismo
  orden en cada repetición. Pero la posición densa cambia cuando muere otro zombie (el
  último ocupa su lugar, ADR-0010), y con ella el turno de recálculo: un camino podría
  quedar viejo hasta ~59 pasos, en vez de los 30 prometidos, y los recálculos se
  juntarían en un paso. El índice del *handle* no cambia mientras el zombie vive.
- **La escala de tiempo:** la IA multiplica sus temporizadores por la escala de cada
  personaje (ADR-0018). Con escala 0 (la pausa de impacto, un pedido de corte en la
  cola), el zombie no decide ni pide camino, y ORCA lo trata como quieto.
- **Determinismo:**
  - no se usa azar ni el reloj;
  - la cola abierta de A*, el reparto de fichas y la lista de vecinos de ORCA desempatan
    por índice;
  - las fichas, el anillo y los demás valores son CVars que afectan la simulación
    (ADR-0021): un cambio durante una grabación queda grabado.
- **Depuración (ADR-0021):**
  - **dibujo de depuración:** la navmesh y sus portales, los caminos, la velocidad
    deseada contra la elegida por ORCA, el cono de la vista, los radios de ruido y quién
    tiene ficha;
  - **un panel de ImGui** para el zombie elegido: su estado, su ficha, su camino y **el
    recorrido de su árbol en la última decisión** (cada nodo visitado y su resultado).
    Ese recorrido se guarda solo para el zombie elegido;
  - **contadores en el registro por frame:** búsquedas de A*, polígonos expandidos,
    fichas en uso y vecinos de ORCA.
- **Tests (ADR-0011):**
  - **la generación de la navmesh vive en el cocinador, así que lleva su TDD estricto**
    (ADR-0017). Sobre unas cajas conocidas, con casos calculados a mano: los polígonos
    convexos, conectados por sus portales, que dejan afuera lo inflado por el radio, y
    los mismos bytes al cocinar dos veces. Un prefijo de retoque mal escrito falla el
    build;
  - **la IA y la navegación en el juego llevan tests después,** como el resto del
    gameplay. Son funciones puras con casos calculados a mano: A* y el *funnel* sobre una
    navmesh armada a mano, con las esquinas esperadas; el programa lineal de ORCA, con dos
    agentes de frente que no chocan; el reparto de fichas (el más cercano, la ventaja de
    la cámara, el desempate); y la evaluación de un árbol chico;
  - **los tests de *replay*** cubren la horda entera con un hash del estado.
- **Rendimiento:** "Gameplay e IA" sigue con 1,3 ms por paso, como hipótesis. El
  prototipo usó ~0,1 ms en IA y navegación con 50 zombies, pero es 2D y no tiene el
  gameplay real.
- **Lo que se gana:** una IA barata (menos del 10 % de su presupuesto en el prototipo),
  determinista, depurable zombie por zombie y justa con el jugador. Está hecha con las
  piezas estándar de la industria (BT, navmesh, A*, *funnel*, ORCA, fichas), todas
  escritas y entendidas desde cero.
- **Lo que se sacrifica:**
  - escribir el generador de navmesh, en vez de usar Recast, que ya lo resuelve;
  - la flexibilidad del BT con objetos: un tipo de nodo nuevo pasa por el `switch`;
  - los huecos por los que solo pasaría un zombie entero, con una sola navmesh;
  - el matiz de la *utility* y la escala de los campos de flujo, que servirían con miles
    de agentes;
  - hasta 30 pasos (0,5 s) de un camino viejo, salvo que el objetivo se aleje del final.
- **Tickets, en orden** (cada uno crece desde el anterior):
  1. el formato de la navmesh, A* y el *funnel*, sobre una navmesh armada a mano;
  2. el cocinador genera la navmesh de la arena desde su colisión, con el prefijo nuevo;
  3. seguir el camino, el rayo sobre la navmesh y el reparto de los recálculos;
  4. ORCA entre agentes, con la velocidad deseada hacia el controlador (ADR-0016);
  5. el BT plano, su constructor y el árbol del zombie común (perseguir, esperar,
     atacar, agarrar), con su tabla de datos;
  6. la percepción: vista, ruido y contagio;
  7. las fichas y la espera en anillo;
  8. los estados de locomoción, la escala de tiempo y el zombie ensartado;
  9. el árbol del guardaespaldas y la FSM del mago;
  10. el dibujo de depuración, el panel y los contadores (junto al ADR-0021);
  11. los tests de *replay* de la horda.
- **Abierto:**
  - todos los valores de partida, que se ajustan jugando: las fichas, el radio del
    anillo, la distancia y el ángulo de la vista, el radio del contagio, los radios de
    ruido, el descuento de la cámara, el intervalo de recálculo, el punto adelantado del
    seguimiento y los parámetros de ORCA;
  - el resto del algoritmo de Recast (alturas, pisos, escaleras), cuando el ADR-0027
    defina el castillo;
  - cómo llega la colisión del nivel armado al cocinador y dónde queda la navmesh dentro
    de lo cocinado (ADR-0027);
  - una navmesh por tamaño, con el caballo;
  - si los zombies del castillo pueden olvidar a la caballera;
  - los comportamientos fuera del corte vertical: el escudo, el soldado con espada que
    bloquea o contraataca, los perros, el caballero y su caballo, el rey y la reina;
  - una grilla de vecinos para ORCA, si el profiler la pide.

**Fe de erratas en otros ADRs.** Al aceptar este ADR se corrigieron, como fe de erratas
(ADR-0014, regla 11):

| ADR | Qué se corrige |
|---|---|
| ADR-0016 | Cómo se separan los zombies de la horda: ORCA le pasa al controlador una velocidad deseada. Se cierra en Abierto |
| ADR-0017 | El cocinador genera la navmesh desde la colisión del escenario, de forma determinista, y acepta el prefijo nuevo para retocarla. Guarda el radio de inflado, que recibe como parámetro del nivel, y necesita ver la colisión del nivel armado (ADR-0027) |
| `docs/performance/presupuestos.md` | La nota del prototipo en "Gameplay e IA", que sigue siendo hipótesis |

## Cómo lo defiendo

**¿Por qué un árbol de comportamiento, si la FSM midió más rápido?**
Porque la diferencia no importa: la FSM decidió en 1,0 µs por paso y el BT en 5,4 µs,
con 62 agentes. Es menos del 0,5 % del presupuesto de 1,3 ms. Los tres modelos tomaron
exactamente las mismas decisiones (el mismo hash), así que lo único que cambia es cómo
se escriben. El BT crece mejor con los tipos de zombie de la visión, y en el árbol se lee
el orden de prioridad: primero lo comprometido, después atacar, después esperar turno,
después perseguir. En una FSM, ese orden queda repartido entre las transiciones.

**¿Qué es un BT "plano" y por qué no el clásico?**
El BT clásico, el de los libros y de muchos engines, es un árbol de objetos: cada nodo es
una clase con un método virtual. *Halo 2* aportó la estructura del árbol, no esa
implementación. El ADR-0009 no permite `virtual` por entidad en el código de cada frame.
Mi BT guarda los nodos de cada tipo en un arreglo compartido, con los hijos contiguos, y
despacha las hojas con un `switch`. Cada zombie solo guarda su estado en un componente.
Es la forma orientada a datos que describen Champandard y Dunstan en *Game AI Pro*. Lo
que pierdo: un tipo de nodo nuevo se agrega editando el `switch`, no heredando de una
clase.

**¿Por qué no *utility AI* o GOAP?**
GOAP planifica secuencias de acciones con A*, y sirve para enemigos que se cubren y
flanquean, como en *F.E.A.R.*. Un zombie no planifica: persigue y ataca. *Utility* decide
con curvas, y en el prototipo, al sumar el guardaespaldas, tuve que tocar el puntaje de
**todas** las demás acciones para que no le ganaran. Con muchos tipos, eso se vuelve
difícil de predecir y de afinar.

**¿Por qué una navmesh y no un campo de flujo, que es lo típico para hordas?**
El campo de flujo sirve a todos los zombies con un solo cálculo, pero su costo escala con
el área. En la arena de 40×40 m son 25 600 celdas, y recalcularlo costó 3,4 ms en p99,
casi tres veces el presupuesto. Con una cola mejor y celdas más grandes bajó a 0,3 ms,
pero el castillo es más grande y tiene pisos y escaleras, que una grilla 2D no
representa. La misma arena es 80 polígonos de navmesh, y un camino cuesta 9 µs de media.
El flujo conviene con miles de unidades, como en *Supreme Commander 2*. Con 200 zombies
todavía no gana.

**¿Por qué escribir tu propio generador de navmesh, si Recast es el estándar?**
Porque es justamente lo que quiero entender: cómo se pasa de la geometría del escenario
a un espacio caminable. Recast es la referencia que estudio y con la que comparo. Además,
para la arena plana del corte vertical alcanza una versión simple, y el resto del
algoritmo (alturas, regiones y contornos) llega cuando los niveles lo pidan. La genera el
cocinador, así que el juego no paga ese costo y el resultado es determinista.

**¿Qué es ORCA y por qué no alcanza con que los controladores se empujen?**
ORCA mira la velocidad de cada vecino y descarta las velocidades que llevarían a chocar
en el próximo segundo. Después elige la permitida más cercana a la que el zombie quería,
y cada uno asume la mitad del esquive. Sin evasión, los zombies se enciman y el
controlador los separa después: medí 40 veces más solapamiento. Llegan igual de rápido,
pero los cuerpos se meten uno dentro del otro, y en 3D eso se nota. ORCA cuesta 36 µs con
50 zombies, y el controlador sigue sacándolos de lo que penetran, como red de seguridad.

**¿Para qué las fichas de ataque? ¿No le quitan amenaza a la horda?**
Sin ellas, en el prototipo hasta 7 zombies agarraban a la caballera a la vez, y un
tercio del tiempo eran 4 o más. Eso no es difícil, es injusto: el jugador no puede
leerlo ni responder. Con 3 fichas, la horda sigue rodeándola y presionando, pero ataca
por turnos, y el jugador puede pensar dónde posicionarse para el tajo. Es el pilar 3, "la
horda es un rompecabezas". Es lo que hace *DOOM* (2016), que además da prioridad al que
está en pantalla. La cantidad de fichas es configurable, y puede subir con la
dificultad.

**¿Cómo garantizas que la IA sea determinista?**
No uso azar ni el reloj, y compilo con los flags del ADR-0008: sin `-ffast-math` ni
`-march`. Medí el mismo hash con Clang y GCC, de `-O0` a `-O3`; con `-march=native`
cambia, por FMA. Además, todo lo que ordena desempata por índice: la cola de A*, las
fichas y los vecinos de ORCA. Con un solo hilo y la destrucción diferida, las muertes
ocurren siempre en el mismo orden, así que la partida grabada se repite igual. Aparte, el
reparto de los recálculos usa el índice estable del *handle*, no la posición en el
arreglo denso del *sparse set*. No es por el determinismo: es porque esa posición cambia
cuando muere otro zombie, y un camino podría quedar viejo el doble de lo prometido.

**¿Cómo cambia la IA de un zombie al que le cortaste una parte?**
El ADR-0018 decide si el pedazo sigue vivo y su forma de moverse: caminar, cojear,
arrastrarse o retorcerse. La IA no tiene un árbol nuevo para cada caso. Usa el mismo
árbol con los números de ese estado (velocidad, cápsula, alcance) y ramas para lo que de
verdad cambia: sin brazos no ataca ni pide ficha, y el que se retuerce no se mueve. El
camino se descarta, porque cambió su cápsula. La navmesh se infla por el radio del que se
arrastra, así que todos pasan por donde él pasa.

**¿Cuántos zombies con IA aguanta, y qué se rompe primero?**
Con 50, toda la IA (decidir, buscar caminos, ORCA y mover) costó ~0,1 ms por paso en el
prototipo: menos del 10 % de su presupuesto. Con 200 zombies, 1,0 ms en p99. Ahí lo
primero que crece es ORCA, porque busca los vecinos a fuerza bruta: 50 zombies son 2500
pares, y 200 son 40 000. Si alguna vez hace falta, la solución es una grilla de vecinos,
pero no la agrego hasta que el profiler la pida. El prototipo es 2D y no tiene el
gameplay real, así que el presupuesto sigue siendo hipótesis hasta medir con el
protocolo.

## Referencias

- Michael Booth, "The AI Systems of Left 4 Dead", Artificial Intelligence and
  Interactive Digital Entertainment Conference (AIIDE), 2009,
  <https://cdn.fastly.steamstatic.com/apps/valve/2009/ai_systems_of_l4d_mike_booth.pdf>.
- Damian Isla, "Handling Complexity in the *Halo 2* AI", GDC 2005.
- Jeff Orkin, "Three States and a Plan: The A.I. of F.E.A.R.", GDC 2006.
- Dave Mark y Kevin Dill, "Improving AI Decision Modeling Through Utility Theory", GDC
  2010 (AI Summit).
- Dave Mark, *Behavioral Mathematics for Game AI*, Course Technology, 2009.
- Kurt Loudy y Jake Campbell, "Embracing Push Forward Combat in DOOM", GDC 2018.
- Elijah Emerson, "Crowd Pathfinding and Steering Using Flow Field Tiles", en Steve
  Rabin (ed.), *Game AI Pro*, CRC Press, 2013.
- Alex J. Champandard y Philip Dunstan, "The Behavior Tree Starter Kit", en Steve Rabin
  (ed.), *Game AI Pro*, CRC Press, 2013.
- Craig W. Reynolds, "Steering Behaviors For Autonomous Characters", GDC 1999.
- Jur van den Berg, Stephen J. Guy, Ming Lin y Dinesh Manocha, "Reciprocal n-Body
  Collision Avoidance", en *Robotics Research*, Springer Tracts in Advanced Robotics 70,
  2011.
- Mikko Mononen, *Recast & Detour*, <https://github.com/recastnavigation/recastnavigation>,
  y "Simple Stupid Funnel Algorithm", 2010.
- Peter E. Hart, Nils J. Nilsson y Bertram Raphael, "A Formal Basis for the Heuristic
  Determination of Minimum Cost Paths", *IEEE Transactions on Systems Science and
  Cybernetics*, 1968.
- Robert B. Dial, "Algorithm 360: Shortest-Path Forest with Topological Ordering",
  *Communications of the ACM* 12(11), 1969.
- John Amanatides y Andrew Woo, "A Fast Voxel Traversal Algorithm for Ray Tracing",
  Eurographics 1987.
- David Harel, "Statecharts: A Visual Formalism for Complex Systems", *Science of
  Computer Programming* 8(3), 1987.
- Ian Millington, *AI for Games*, 3.ª ed., CRC Press, 2019.
- Godot Engine, documentación: *Using NavigationAgents* y *Using agent avoidance*.
