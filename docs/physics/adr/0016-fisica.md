# ADR-0016: Física

- **Estado:** Aceptado
- **Fecha:** 2026-10-19
- **Ticket:** #29
- **Reemplaza / Reemplazado por:** —
- **Fe de erratas:** 2026-10-20 (#30): "una cápsula por hueso" significa "por hueso que
  mueve algún vértice". Un hueso que no mueve ninguno (como `root`) no tiene cápsula. Si
  queda en medio de una cadena, la rótula une a sus hijos con el ancestro más cercano
  que tenga cápsula, y ese hueso se mueve rígido junto a él (ADR-0017). Se aclara en
  Consecuencias.

## Contexto y problema

La física de este juego no es la de un simulador: casi todo lo que se ve moverse lo
mueve la animación (ADR-0015). La física aparece en momentos concretos:

- **la espada toca a un zombie:** hay que saberlo sin perder ningún contacto, aunque la
  punta vaya muy rápido;
- **un zombie se parte:** los pedazos caen, chocan con el suelo y entre sí, y quedan
  tirados (hasta ~100, ADR-0020);
- **un zombie muere entero:** se desploma (ADR-0015 dejó aquí el ragdoll);
- **los personajes se mueven por el escenario** sin atravesar paredes, y se empujan;
- **la cámara** se acerca a la caballera cuando una pared se interpone (visión).

Hay que decidir:

1. si la física es **propia o de una biblioteca**;
2. su **alcance**: qué tipos de cuerpos, formas, uniones y consultas existen;
3. el **solver**: cómo se resuelven los contactos y las uniones;
4. la **forma física de los pedazos** que crea el corte;
5. qué pasa con los **pedazos muertos y los zombies muertos** (rígidos o ragdoll);
6. cómo se mueven **los personajes**;
7. cómo se **detecta el golpe de la espada**;
8. la **fase amplia**: cómo se descartan los pares de cuerpos que no pueden tocarse.

Lo que ya está decidido y limita este ADR:

- **`CLAUDE.md`:** "el renderer, la física, la animación, el corte de mallas y el
  gameplay son propios". Cada dependencia externa necesita un ADR que la justifique.
- **ADR-0008:** matemática propia, `float`, sin `-ffast-math`.
- **ADR-0012:** paso fijo de 60 Hz, un hilo, ECS, y el orden del paso: gameplay →
  combate → corte → física → animación.
- **ADR-0015:** el tajo es un arco dentro del plano del tajo. Los pedazos muertos
  quedan rígidos o pasan a ragdoll, según decida este ADR.
- **ADR-0020:** la física tiene **1,5 ms por paso** para los zombies y ~100 pedazos
  (`docs/performance/presupuestos.md`, hipótesis). Una partida grabada tiene que
  reproducir los mismos resultados.

**Fuera de alcance:** el algoritmo del corte de la malla (ADR-0007), las reglas de qué
corta y qué empuja (ADR-0018), la navegación y la separación de la horda (ADR-0023) y
cómo se crea la geometría de colisión del escenario (ADR-0017).

## Criterios

Los del ticket:

1. **Restricción "desde cero con criterio"** (`CLAUDE.md`).
2. **Estabilidad:** pilas de pedazos que no tiemblan ni explotan.
3. **Rendimiento** dentro de los 1,5 ms por paso.
4. **Aprendizaje.**
5. **Integración con el corte y la animación.**

## Opciones

### 1. Propia o biblioteca

| Opción | Qué es | Problema |
|---|---|---|
| **Propia** | Se escribe en el engine, con el alcance justo | Mucho trabajo, y los errores de estabilidad son difíciles de encontrar |
| **Jolt Physics** (MIT) | El motor de *Horizon Forbidden West*, de código abierto. Guerrilla dejó un motor comercial por Jolt y duplicó la frecuencia de simulación con menos CPU | Contradice la restricción del proyecto. Además, el ECS, el paso y la memoria tendrían que adaptarse a su modelo |
| **Bullet** (zlib) o **PhysX 5** (BSD-3) | Motores maduros y muy usados | Lo mismo, y son más grandes |
| **Propia, con una biblioteca solo en los tests** | El motor es propio, y en los tests se compara contra Jolt o se usa como referencia | Una dependencia más, aunque solo de desarrollo |

### 2. Alcance

| Pieza | Para qué la necesita el juego |
|---|---|
| **Cuerpos rígidos dinámicos** | Los pedazos que caen |
| **Formas:** esfera, cápsula, caja, casco convexo | Las cápsulas calzan con los huesos. Las cajas, con el escenario. Una caja **dinámica** exige el par caja contra caja (SAT con recorte de caras), el más difícil de todos |
| **Escenario estático** | Suelo, muros y escombros de la arena |
| **Contactos con fricción** | Que los pedazos se detengan y no resbalen |
| **Uniones con límites** (rótula con cono y giro) | Los ragdolls |
| **Dormir** (*sleeping*) | Que los pedazos quietos no cuesten nada |
| **Consultas:** rayo y barrido de esfera | La cámara que se acerca, las líneas de visión de la IA |
| **Controlador de personaje** | La caballera y los zombies, sin atravesar paredes |
| **Detección del tajo** | Saber qué tocó la espada, sin perder contactos |

Quedan **fuera**: física activa (ADR-0015), cuerpos blandos, telas, fluidos y vehículos.

### 3. El solver

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Impulsos secuenciales** (PGS, Catto 2006) | Resuelve los contactos uno por uno con impulsos, varias iteraciones por paso, y guarda los impulsos del paso anterior para empezar mejor (*warm starting*) | Las pilas y las diferencias grandes de masa necesitan muchas iteraciones. La corrección de la penetración puede agregar energía |
| **Soft Step** (TGS Soft, el de Box2D v3) | Impulsos secuenciales con **subpasos** en vez de iteraciones, y restricciones "blandas" con resorte y amortiguación | Un poco más complejo que PGS |
| **XPBD** (Müller y otros, 2020) | Trabaja con posiciones en vez de velocidades, con subpasos. Simple de escribir y estable | En las pruebas de Catto tuvo problemas de precisión lejos del origen y con la fricción. La literatura de juegos lo usa menos que los impulsos |

### 4. La forma física de los pedazos

| Opción | Cómo se obtiene | Problema |
|---|---|---|
| **Casco convexo** (*Quickhull*) | Se calcula en el momento con los vértices del pedazo | Un pedazo cóncavo (medio torso con un brazo) queda como un bulto. Quickhull tiene casos numéricos difíciles (Gregorius, GDC 2014), y es otro algoritmo grande que escribir |
| **Caja o cápsula ajustada** | Ejes principales (PCA) de los vértices | Muy aproximado para formas largas o en L |
| **Las cápsulas de los huesos** | Cada hueso del zombie ya tiene una cápsula. El pedazo se queda con las de sus huesos, y la que atraviesa el plano se acorta | Aproximado en el corte oblicuo de un hueso grueso (el torso). No describe bien una armadura con formas planas. Hay que acortar con cuidado: la tapa esférica asoma un radio más allá del extremo del segmento |

### 5. Pedazos y zombies muertos

| Opción | Qué pasa | Problema |
|---|---|---|
| **Todo rígido** | Cada pedazo, o el zombie entero, es un solo cuerpo rígido | Un zombie entero cae como una tabla |
| **Todo ragdoll** | Cada pedazo con varios huesos es un ragdoll | Muchos cuerpos y uniones: con 100 pedazos, el costo crece rápido |
| **Mixto** | El zombie muerto entero es un ragdoll. Los pedazos son cuerpos rígidos (un pedazo con varios huesos es un cuerpo compuesto de varias cápsulas) | Hay que decidir cuándo un pedazo pasa a ser rígido o ragdoll |

### 6. Los personajes

| Opción | Cómo se mueve | Problema |
|---|---|---|
| **Cuerpo dinámico** | La física empuja la cápsula con fuerzas | Difícil de controlar: resbala, rebota y se cae en las escaleras. El movimiento ya lo da la animación |
| **Controlador cinemático** | La cápsula se mueve donde quiere el gameplay, y una consulta la frena o la desliza contra las paredes (*move and slide*) | No reacciona sola a los golpes: el empuje se programa |

### 7. La detección del tajo

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Discreta** | En cada paso, se prueba si la hoja se superpone con algún zombie | **Pierde golpes:** en el pico la punta avanza 0,68 m por paso, más que el ancho de un zombie (ver Investigación) |
| **Continua genérica** (barrido, tiempo de impacto) | Se barre la hoja entre dos pasos y se busca el primer contacto con cada forma | Es el caso general y difícil del CCD |
| **Barrido en el plano del tajo** | La hoja vive siempre en el plano del tajo (ADR-0015). Entre dos pasos, barre un **sector de anillo** en ese plano. Basta ver si cada cápsula toca ese sector | Solo sirve para el tajo. Otros golpes (los ataques de los enemigos) necesitan otra cosa |

### 8. La fase amplia

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Fuerza bruta** | Se prueban todos los pares de cajas (AABB) | O(n²): crece mal si los cuerpos se multiplican |
| **Grilla uniforme** | Cada cuerpo se anota en las celdas que toca, y solo se prueban los de la misma celda | Hay que elegir el tamaño de la celda |
| **Árbol AABB dinámico** (Box2D, Bullet) | Un árbol de cajas que se actualiza al moverse | Más código y más complejo |
| **Barrido y poda** (*sweep and prune*) | Se ordenan los extremos sobre un eje | Se degrada si muchos cuerpos se alinean |

## Investigación

### Datos medidos y calculados

Medido y calculado en la máquina de referencia (ADR-0020) el 2026-10-19. Es una
medición de laboratorio para decidir, fuera del protocolo del ADR-0020 (sin
calentamiento térmico y con otras aplicaciones abiertas). El código y cómo repetirlo
están en [`../research/mediciones-de-fisica.md`](../research/mediciones-de-fisica.md).

**La velocidad de la espada** (valores de partida del ADR-0015; el radio de la guarda y
el largo de la hoja salen de su nota,
[`skinning-y-prototipo-del-tajo.md`](../../animation/research/skinning-y-prototipo-del-tajo.md)):

| Qué | Sin carga | Con la carga máxima |
|---|---|---|
| Recorrido del arco | 220° (de 195° a −25°) = 3,84 rad | 260° = 4,54 rad |
| Radio de la punta | 0,6 + 1,25 = 1,85 m | 1,85 m |
| Recorrido de la punta | 7,1 m en 0,26 s | 8,4 m en 0,26 s |
| Velocidad media | 27 m/s | 32 m/s |
| Velocidad máxima (curva *smoothstep*: 1,5 × la media) | **41 m/s** | **48 m/s** |
| **Avance de la punta en un paso de 60 Hz, en el pico** | **0,68 m** | **0,81 m** |
| Ancho de un zombie | ~0,5 m | ~0,5 m |

**Un test discreto, una vez por paso, puede no ver a un zombie que la espada atravesó.**

**La fase amplia por fuerza bruta,** con 150 cuerpos (50 zombies y 100 pedazos) en una
arena de 20 × 20 m: 11 175 pares de AABB en **21 a 25 µs** (el mejor de 2000
repeticiones, en 5 ejecuciones). Es un 1,5 % de los 1,5 ms de la física. No incluye los
cuerpos de los ragdolls: cada uno suma una cápsula por hueso (~20, el esqueleto del
ADR-0015). Con 10 ragdolls se pasaría de ~150 a ~350 cuerpos (61 075 pares), y el costo
crecería unas 5,5 veces, a ~120 µs.

**El controlador de personaje,** con un trote de ~3 a 4 m/s, avanza unos 6 cm por paso,
mucho menos que el radio de su cápsula (~0,3 m). Para él, un test discreto no puede
atravesar nada. El *tunneling* depende de la relación entre la velocidad, el paso y el
tamaño, no del objeto: la espada lo sufre y la caballera no.

### Bibliotecas

- **Jolt Physics** (Jorrit Rouwe, Guerrilla Games; licencia MIT) es el motor de
  *Horizon Forbidden West*. En su charla de GDC 2022, Rouwe cuenta que el cambio a Jolt
  ahorró memoria y tamaño de ejecutable, y permitió duplicar la frecuencia de
  simulación con menos CPU.
- **Bullet** (zlib) y **PhysX 5** (BSD-3) son los otros motores abiertos y maduros.
- **qu3e** (Randy Gaul, zlib) es un motor 3D chico pensado para leerse: cajas,
  impulsos secuenciales y un árbol AABB dinámico. Es una buena referencia para aprender.

### Solvers

- **Impulsos secuenciales** (Erin Catto, *Fast and Simple Physics using Sequential
  Impulses*, GDC 2006): aceptar la penetración, recordar los impulsos del paso anterior,
  aplicar impulsos temprano y seguido, acumularlos y limitarlos, y actualizar la
  posición al final. Acumular y limitar el impulso equivale a un solver de Gauss-Seidel
  proyectado.
- **Solver2D** (Catto, 2024) comparó 8 solvers en pruebas difíciles (diferencias de
  masa grandes, a 30 km del origen, superposición grande y pocas iteraciones):
  - **TGS Soft**, rebautizado *Soft Step*, fue el mejor en casi todas, y es el que
    adoptó Box2D v3. La clave es que **los pasos más chicos rinden más que más
    iteraciones**;
  - **XPBD** sufrió lejos del origen: calcula la velocidad restando posiciones, y pierde
    dígitos significativos. La fricción también le costó;
  - **la relajación** (resolver de nuevo sin el término de corrección) mejora mucho la
    calidad.
- **XPBD** (Müller, Macklin, Chentanez, Jeschke y Kim, *Detailed Rigid Body Simulation
  with Extended Position Based Dynamics*, 2020) es incondicionalmente estable y simple
  de formular, y usa siempre las direcciones de restricción más recientes.

### Colisiones

- **Christer Ericson, *Real-Time Collision Detection*** (2004) es la referencia clásica
  de las pruebas entre formas, las fases amplias y la robustez numérica.
- **Dirk Gregorius** (Valve), en GDC:
  - *Implementing Quickhull* (2014): el casco convexo en producción, con mucho cuidado
    numérico, y la fusión de caras para que los contactos sean estables;
  - *Robust Contact Creation* (2015): cómo armar los puntos de contacto entre formas
    básicas.
- **Las cápsulas** tienen pruebas cerradas y baratas contra esferas, cápsulas y planos:
  se reducen a distancias entre puntos y segmentos. Contra una caja hace falta más:
  segmento contra caja orientada, con el teorema del eje separador (SAT) o con el punto
  más cercano. Y caja contra caja, con varios puntos por contacto, es el par más difícil
  (SAT con 15 ejes y recorte de caras, Gregorius 2015).
- **Acortar una cápsula en un plano.** Una cápsula es un segmento inflado por un radio
  `r`: si el extremo del segmento queda justo sobre el plano, la tapa esférica asoma `r`
  al otro lado, y los dos pedazos nacen superpuestos hasta `2r`. Para que la cápsula
  quede de su lado, **el segmento se recorta al semiespacio de los puntos con distancia
  con signo al plano mayor o igual que `r`**:
  - con las distancias con signo de los extremos, `s₀` y `s₁`, el corte cae en
    `t = (r − s₀) / (s₁ − s₀)`;
  - no divide por `|d·n|`, así que no tiene un caso especial cuando el plano corre casi
    paralelo al hueso;
  - cubre también la cápsula que cruza el plano solo con su radio (el segmento entero de
    un lado, pero a menos de `r`);
  - si el recorte queda vacío, sobra una esfera centrada en el extremo más lejano `P`,
    con radio `min(r, dist(P))`, o nada, si ese radio queda bajo un mínimo.

### Personajes y detección del tajo

- **Los controladores cinemáticos** (como `CharacterVirtual` de Jolt o el
  `CharacterController` de Unity) son el estándar para los personajes jugables: el
  gameplay decide el movimiento, y la física solo lo frena y lo desliza contra lo
  sólido.
- **El tajo en el plano.** Como la hoja está siempre en el plano del tajo (ADR-0015), la
  detección pasa a ser un problema 2D:
  - entre dos pasos, la hoja barre un sector de anillo, entre el radio de la guarda
    (0,6 m) y el de la punta (1,85 m), y entre dos ángulos de la espada. Como la
    animación corre después del combate (ADR-0012), el combate del paso N tiene la espada
    de los pasos N−2 y N−1, y las cápsulas de los zombies en la pose N−1 (ADR-0015);
  - hay golpe si la distancia del segmento de una cápsula al sector es menor o igual que
    su radio. No hace falta calcular la región que deja la cápsula en el plano (que es
    un estadio, un disco o, en general, una región convexa de borde elíptico);
  - el ángulo del primer contacto da el orden en que la espada atravesó a varios zombies.

  **Los supuestos:** el plano se congela al empezar el barrido, y el sector de cada paso
  se arma con el centro del arco de ese paso. El centro se mueve con el paso adelante
  (0,35 m en 0,26 s: ~2 cm por paso en promedio y ~3,4 cm en el pico). Para no perder
  ese margen, el radio de cada cápsula se infla con lo que se movió el centro en el paso.
  Dentro de esos supuestos, no hay túnel posible, porque se prueba todo lo barrido.

### Determinismo

Con el mismo ejecutable y la misma máquina, el resultado es determinista si:

- el paso es fijo (ADR-0012);
- hay un solo hilo (ADR-0012);
- los cuerpos y los contactos se recorren en un orden fijo: nunca por dirección de
  memoria ni por el orden de una tabla hash;
- toda la memoria se inicializa;
- nada depende del reloj real.

`-ffast-math` no afecta esto: un mismo binario da siempre lo mismo. Lo que aportan "sin
`-ffast-math`" y "sin `-march`" (ADR-0008) es otra cosa: que Debug y Release, o dos
compilaciones distintas, den los mismos números.

No hace falta determinismo entre máquinas distintas para medir: el protocolo mide en la
máquina de referencia (ADR-0020). Si un test de *replay* (ADR-0011) guardara en el repo un
hash del estado para compararlo en la CI, correría otro ejecutable en otra máquina, y
esta garantía no alcanzaría.

## Decisión

Elijo una física propia y acotada a lo que el juego necesita. Casi todo lo mueve la
animación, así que la física se concentra en los pedazos, los ragdolls, los personajes
contra el escenario y el golpe de la espada.

| # | Dimensión | Elección |
|---|---|---|
| 1 | Propia o biblioteca | **Propia.** Jolt queda como referencia para leer, no como dependencia |
| 2 | Alcance | Cuerpos rígidos **dinámicos: esferas y cápsulas**; **cajas solo estáticas** (el escenario); contactos con fricción; **uniones de rótula con límites** (ragdoll); dormir; **rayo y barrido de esfera**; controlador de personaje. Sin cajas dinámicas, casco convexo, física activa, cuerpos blandos, telas ni fluidos |
| 3 | Solver | **Soft Step:** impulsos secuenciales con subpasos y restricciones blandas, como Box2D v3. Se construye primero con impulsos secuenciales y después se agregan los subpasos y lo blando |
| 4 | Forma de los pedazos | **Las cápsulas de sus huesos.** Toda cápsula a menos de su radio del plano del corte (aunque su eje no lo cruce, como la tapa del hueso vecino en una articulación) se recorta en cada pedazo hacia su propio lado: el segmento se recorta al semiespacio con distancia al plano mayor o igual que el radio. Si no queda segmento, sobra una esfera en el extremo más lejano, de radio `min(r, dist)`, o nada si es muy chica. Un pedazo con varios huesos es un cuerpo compuesto |
| 5 | Pedazos y zombies muertos | **Mixto.** El zombie muerto entero es un ragdoll: las mismas cápsulas, unidas con rótulas. Los pedazos son cuerpos rígidos. Hay un **tope de ragdolls activos**, y los más viejos se duermen o se congelan |
| 6 | Personajes | **Controladores cinemáticos** que **se mueven y se sacan de lo que penetran:** el controlador avanza, y si su cápsula queda dentro de algo sólido, se empuja hacia afuera por la normal del contacto, lo que lo hace deslizar. Alcanza con pruebas de superposición, sin barridos. El empuje del tajo rápido se programa como un impulso sobre el controlador |
| 7 | Detección del tajo | **Barrido en el plano del tajo:** el plano se congela al empezar el barrido. Entre dos pasos, la hoja barre un sector de anillo, armado con el centro del arco de ese paso, y hay golpe si la distancia del segmento de una cápsula al sector es menor o igual que su radio más lo que se movió el centro en el paso |
| 8 | Fase amplia | **Fuerza bruta** (medida en ~22 µs para 150 cuerpos). Si el profiler lo pide, una grilla uniforme |

## Consecuencias

- **Las cápsulas por hueso son la única forma física del cuerpo del zombie:** detectan el
  tajo, forman el ragdoll y dan la forma de los pedazos. Para moverse por el escenario,
  el zombie vivo usa además la cápsula de su controlador. Cada esqueleto necesita una
  cápsula por cada hueso que mueve algún vértice (radio y segmento). Si un hueso sin
  cápsula queda en medio de una cadena, la rótula une a sus hijos con el ancestro más
  cercano con cápsula, y ese hueso se mueve rígido junto a él. Cómo se generan (a mano o
  ajustadas a los vértices de cada hueso) lo decide el ADR-0017.
- **Los huesos enmascarados no tienen cápsula activa** (ADR-0015): un segundo tajo no
  puede golpear el brazo que ya no está. En el pedazo vivo, las cápsulas que toca el
  plano se recortan igual que en los pedazos.
- **Los cuerpos unidos por una rótula no generan contactos entre sí.** Las cápsulas de
  dos huesos vecinos se superponen en la articulación, y si chocaran, el ragdoll se
  empujaría a sí mismo hasta explotar.
- **Los datos pasan por el ECS** (ADR-0012), como en el ADR-0015, porque la animación, la
  física y el corte están en la misma capa y no se enlazan entre sí:
  - las matrices de los huesos y las cápsulas por hueso son componentes, que la física
    lee sin depender de `animation`;
  - el corte deja los pedazos como entidades con sus componentes de forma, y la física
    crea los cuerpos en su turno;
  - el ragdoll escribe la pose en el mismo componente que lee el render.
- **Las consultas de solo lectura son llamadas directas hacia abajo.** El barrido del
  tajo, el rayo, el barrido de esfera y el *move and slide* devuelven un resultado en el
  momento. Las colas del ADR-0012 quedan para las órdenes que cambian el estado, como
  `slice_request`. **Esto precisa la Decisión 4 del ADR-0012**, que no distinguía las
  consultas de las órdenes, y quedó registrado allí como fe de erratas. El combate es
  gameplay y vive en `game/`, así que llamar a la física es una llamada legal hacia
  abajo.
- **El avance del controlador por paso tiene que quedar muy por debajo de su radio,**
  porque se mueve sin barridos. Al trote son ~6 cm contra ~0,3 m. Los movimientos más
  rápidos (el salto de esquive, un empuje) se recortan o se subdividen en varios
  movimientos chicos dentro del paso.
- **El escenario se describe con cajas, cápsulas y esferas,** porque no hay mallas de
  triángulos ni cascos convexos. Es un requisito para el ADR-0017: la arena de ruinas se
  aproxima con primitivas.
- **Los pedazos son aproximados:** un corte oblicuo de un hueso grueso o una pieza de
  armadura plana no quedan exactos. Al estilo PSX, y con los pedazos en el suelo, se
  acepta.
- **El traspaso desde la animación (ADR-0015):** los cuerpos del ragdoll y de los
  pedazos nacen con la transformación de la última pose, y con la velocidad que sale de
  la diferencia entre las dos últimas poses, más el impulso del tajo. Cuando la física
  del paso N crea los cuerpos, las dos últimas poses son la N−1 y la N−2, porque la
  animación corre después. La velocidad lineal es la del **centro de masa** del cuerpo,
  `v = (com_{N−1} − com_{N−2}) / dt`, con el centro calculado en las dos poses: en un
  pedazo compuesto, ese centro puede quedar lejos del origen del hueso. La velocidad
  angular sale del cuaternión relativo `q_{N−1} · q_{N−2}⁻¹`, después de llevar los dos
  al mismo hemisferio (si `dot < 0`, se invierte uno, como en el ADR-0015). Si no, el
  pedazo nace girando casi una vuelta completa por paso.
- **El camino de vuelta, hacia el render:** el zombie muerto sigue siendo una malla con
  LBS en la GPU (ADR-0015).
  - El ragdoll convierte las transformaciones de sus cápsulas en las matrices de los
    huesos, que lee el skinning.
  - Un pedazo rígido usa las matrices de huesos congeladas en la pose del corte,
    multiplicadas por la transformación de su cuerpo.
  - Ese trabajo se paga por frame, dentro de "Render en la CPU"
    (`docs/performance/presupuestos.md`), como pidió el ADR-0015. Lo acota el tope de
    ragdolls activos: un pedazo rígido cuesta una sola transformación.
- **La detección del tajo es una consulta de la física** que usa el combate, antes del
  corte (orden del ADR-0012). Usa la espada de los pasos N−2 y N−1 y las cápsulas en la
  pose N−1, como dice el ADR-0015: el corte pasa por donde el jugador vio la espada.
- **El orden de construcción** sigue el esqueleto andante (ADR-0014):
  1. consultas y controlador de personaje, para caminar por la arena;
  2. el barrido del tajo;
  3. cuerpos rígidos, contactos y dormir, para los pedazos;
  4. uniones y ragdoll.
- **Tests (ADR-0011):**
  - **TDD y 100 % de cobertura** en todo lo geométrico, que es matemática con
    especificación cerrada: cada forma que se mueve (esfera, cápsula y la cápsula del
    controlador) contra cada forma (esfera, cápsula y caja estática), el rayo, el
    barrido de esfera, recortar una cápsula en un plano (con sus casos límite) y el
    barrido del tajo. No hay caja contra caja, porque dos cajas estáticas no chocan;
  - **tests después** en el solver y el controlador, con escenarios conocidos: una pila
    de pedazos que se queda quieta, una esfera que rueda y se detiene, y un ragdoll que
    no explota;
  - **un test de determinismo** desde el principio, sin esperar al *replay*: correr dos
    veces el mismo escenario en el mismo proceso y comparar un hash del estado, bit a
    bit.
- **Determinismo:** se cumplen las condiciones de la Investigación (paso fijo, un hilo,
  orden fijo, memoria inicializada, nada del reloj). Con el mismo ejecutable y la misma
  máquina, una partida grabada da el mismo resultado (ADR-0020).
- **Presupuesto:** la física tiene 1,5 ms por paso (ADR-0020, hipótesis).
  - Los ragdolls tienen **su propio tope**, aparte de los 100 pedazos: cada uno suma una
    cápsula por hueso (~20) y sus uniones. La fase amplia medida (~22 µs) no los
    incluye; con 10 ragdolls sería ~120 µs, que sigue siendo poco.
  - El tope de ragdolls y el umbral para dormir se fijan midiendo, y se anotan en
    `docs/performance/presupuestos.md`.
- **Lo que se sacrifica:**
  - la robustez probada de Jolt, a cambio de aprender y controlar el costo;
  - el casco convexo, las mallas de triángulos y las cajas dinámicas;
  - la física activa (ADR-0015);
  - **los pedazos grandes caen rígidos:** un cuerpo sin cabeza o un tren inferior con las
    dos piernas es un cuerpo compuesto rígido, y cae como una tabla.
- **Abierto:**
  - si un pedazo con muchos huesos podría pasar a ragdoll (ADR-0018);
  - qué pasa entre los controladores y los pedazos del suelo: si los empujan, los
    ignoran o los frenan (ADR-0018);
  - si un test de *replay* necesita un hash estable entre máquinas, para compararlo en
    la CI (ADR-0021 y ADR-0029);
  - cómo se separan los zombies de la horda entre sí (ADR-0023);
  - qué decide que un zombie muera entero (ragdoll) o en pedazos (ADR-0018);
  - la mecánica de los agarres (ADR-0018);
  - qué otros golpes (los ataques de los enemigos) necesitan otra detección
    (ADR-0018).

## Cómo lo defiendo

**¿Por qué escribir tu propia física, si Jolt es gratis, abierto y lo usó Guerrilla en
*Horizon*?**
Es parte del aprendizaje, y así solo creamos lo realmente necesario para la física del
juego. Además, con una física propia, el costo depende de decisiones que entiendo y
controlo.

**¿Qué es el *tunneling*, por qué lo sufriría el tajo y cómo lo evitas?**
Es cuando algo va tan rápido que, entre dos pasos, atraviesa a otro objeto sin que la
física lo detecte. La punta de la espada llega a 0,68 m por paso, y un zombie mide
0,5 m. Se resuelve probando toda el área que barrió la hoja entre los dos pasos (un
sector de anillo en el plano del tajo), en vez de solo dónde quedó. Con la carga
máxima, la punta llega a 0,81 m por paso.

**¿Qué hace un solver de impulsos secuenciales, y qué le agrega Soft Step?**
Es el algoritmo encargado de resolver las restricciones de los cuerpos rígidos,
aplicando impulsos uno tras otro de forma iterativa. Soft Step le agrega dos cosas:
**subpasos**, porque dividir el paso en pasos más chicos rinde más que hacer más
iteraciones, y **restricciones suaves**, con resortes y amortiguación.

**¿Por qué los pedazos usan cápsulas y no la malla exacta del corte?**
Porque chocar mallas exactas es caro y complejo, y las cápsulas tienen pruebas simples y
baratas. Además, ya existen: son las mismas que detectan el tajo y arman el ragdoll. Al
estilo PSX, la aproximación no se nota.

**¿Por qué los personajes no son cuerpos dinámicos, como los pedazos?**
Por control: un cuerpo dinámico resbala, rebota y se cae, y el movimiento ya lo decide
la animación. Con un controlador cinemático, el personaje va justo donde quiere el
gameplay, y la física solo lo frena y lo desliza contra lo sólido. Además es más barato.

**Tienes 100 pedazos en el suelo. ¿Por qué no cuestan 100 veces la física de uno?**
Porque nadie está interactuando con ellos: están, por así decirlo, durmiendo. Un cuerpo
que se mueve más lento que un umbral durante un rato se duerme, el solver lo salta, y se
despierta si algo lo toca.

**La fuerza bruta es O(n²). ¿Por qué la usas en la fase amplia?**
Por ahora, mientras no hagamos nada demasiado intenso, funciona aunque sea O(n²): medida,
cuesta 22 µs para 150 cuerpos, un 1,5 % del presupuesto. Es medir antes de optimizar. Si
en el futuro se ve una limitación por culpa de esto, se cambia a algo más complejo, como
una grilla uniforme.

## Referencias

- Erin Catto, *Fast and Simple Physics using Sequential Impulses*, GDC 2006:
  <https://box2d.org/files/ErinCatto_SequentialImpulses_GDC2006.pdf>
- Erin Catto, *Solver2D* (2024): <https://box2d.org/posts/2024/02/solver2d/>
- Matthias Müller, Miles Macklin, Nuttapong Chentanez, Stefan Jeschke y Tae-Yong Kim,
  *Detailed Rigid Body Simulation with Extended Position Based Dynamics*, Computer
  Graphics Forum (SCA 2020): <https://diglib.eg.org/handle/10.1111/cgf14105>
- Jorrit Rouwe, *Architecting Jolt Physics for Horizon Forbidden West*, GDC 2022:
  <https://gdcvault.com/play/1027560/Architecting-Jolt-Physics-for-Horizon>
- Jolt Physics: <https://github.com/jrouwe/JoltPhysics>
- Dirk Gregorius, *Implementing Quickhull*, GDC 2014:
  <https://gdcvault.com/play/1020606/Physics-for-Game-Programmers>
- Dirk Gregorius, *Robust Contact Creation for Physics Simulations*, GDC 2015:
  <https://media.steampowered.com/apps/valve/2015/DirkGregorius_Contacts.pdf>
- Christer Ericson, *Real-Time Collision Detection*, Morgan Kaufmann (2004).
- Randy Gaul, qu3e: <https://github.com/RandyGaul/qu3e>
- Ian Millington, *Game Physics Engine Development*, 2.ª ed., CRC Press (2010).
