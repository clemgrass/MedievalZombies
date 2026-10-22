# ADR-0015: Sistema de animación

- **Estado:** Aceptado
- **Fecha:** 2026-10-19
- **Ticket:** #28
- **Reemplaza / Reemplazado por:** —
- **Fe de erratas:** 2026-10-22 (#31): el ADR-0018 cambia el plano del tajo, que pasa
  por el pecho de la caballera y por el punto apuntado, y ya no contiene la dirección de
  la cámara. Por eso, el 90° del arco pasa a ser la dirección del pecho al punto apuntado,
  sin su componente sobre la línea. La duración del barrido baja con la carga, partiendo de 0,26 s, y el
  tajo rápido tiene la suya. La pausa de impacto congela solo a la caballera y al zombie
  golpeado, así que congela el reloj de la animación de esos dos, no el de todo el juego.
  Se corrige en la Decisión 7 y en los valores de partida del tajo.

## Contexto y problema

La animación tiene que resolver tres cosas que en otros juegos casi no se cruzan:

- **La espada no tiene un set fijo de golpes** (visión): el tajo sigue el ángulo que
  elige el jugador, con cualquier inclinación.
- **El zombie se anima mientras se corta,** y su malla es una malla animada (*skinned
  mesh*). El corte (ADR-0007) necesita la geometría **en la pose de ese instante**.
- **El zombie sigue moviéndose después de perder partes:** con la cabeza y al menos un
  brazo o una pierna, se arrastra, cojea o se defiende con lo que le queda (visión). Su
  esqueleto cambia en pleno juego.

Hay que decidir:

1. el **enfoque**: clips grabados, animación procedural, física o una combinación;
2. el **tipo de skinning**: cómo los huesos deforman la malla;
3. **dónde corre el skinning**: en la CPU o en la GPU;
4. qué le **entrega la animación al corte** y qué pasa con la animación de los pedazos;
5. cómo se anima **el tajo** sin un set fijo de golpes;
6. cómo se anima **un zombie al que le faltan partes**.

Lo que ya está decidido y limita este ADR:

- **ADR-0008:** cuaterniones para las rotaciones, `float` escalar, sin SIMD hasta que el
  profiler lo pida.
- **ADR-0012:** ECS, paso fijo de 60 Hz con interpolación en el render, un solo hilo.
- **ADR-0020:** la animación tiene **1,2 ms por paso de simulación** para 50 esqueletos
  (`docs/performance/presupuestos.md`, hipótesis). La UHD 620 tiene sus propios
  13,3 ms.
- **ADR-0019:** los assets de terceros solo pueden ser CC0 o CC BY.

**Fuera de alcance:** el algoritmo del corte (ADR-0007), la física de los ragdolls y de
los pedazos (ADR-0016), la herramienta y el formato con que se crean las animaciones
(ADR-0017) y las reglas de combate y desmembramiento (ADR-0018). Este ADR decide **qué
garantiza la animación** a cada uno de ellos.

## Criterios

Los del ticket, más el presupuesto del ADR-0020:

1. **Compatibilidad con el corte de mallas animadas:** el criterio que más pesa.
2. **Rendimiento en GPU integrada,** dentro de los presupuestos del ADR-0020.
3. **Aprendizaje:** entender cada pieza (pose, jerarquía, skinning, IK).
4. **Calidad visual,** dentro del estilo PSX de la visión.
5. **Costo de producción para una sola persona:** cuántas animaciones hay que crear a
   mano.

## Opciones

### 1. Enfoque

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Solo clips** | Animaciones grabadas (caminar, atacar, morir), mezcladas con una máquina de estados | El tajo libre necesitaría un clip por cada ángulo posible. Un zombie mutilado necesita un clip por cada combinación de partes |
| **Solo procedural** | Todo se calcula en el momento: IK, osciladores, reglas | Caminar y arrastrarse de forma creíble solo con reglas es difícil y lleva mucho ajuste |
| **Física activa** (*active ragdoll*) | El cuerpo es un ragdoll con motores en las articulaciones que intentan seguir una pose, como *Half Sword* o Euphoria (*GTA IV*) | Es lo más vivo, pero también lo más difícil de controlar y de ajustar. Depende de una física de articulaciones robusta (ADR-0016), y es caro con 50 cuerpos |
| **Por capas** | Clips para la base (caminar, trotar, arrastrarse), procedural encima (el brazo de la espada con IK, el torso girando, los agarres) y física para lo que ya no tiene control (pedazos, muertes) | Hay que definir bien cómo se mezclan las capas y en qué orden |

### 2. Tipo de skinning

| Opción | Cómo deforma | Para el corte | Problema |
|---|---|---|---|
| **Rígido** (1 hueso por vértice, estilo PS1) | Cada vértice sigue a un solo hueso. *Final Fantasy VII* y *Resident Evil* lo usaban | Lo más simple: cada vértice pertenece a un hueso | Las articulaciones se ven quebradas, a menos que el modelo esté separado en piezas. Un vértice nuevo en una arista entre dos huesos igual necesita dos pesos |
| **LBS** (*linear blend skinning*, hasta 4 huesos por vértice) | Cada vértice mezcla las matrices de hasta 4 huesos según sus pesos. Es el estándar de la industria, y el de glTF | Un vértice nuevo en una arista recibe los pesos interpolados de sus dos extremos | En giros fuertes, el volumen se colapsa (el efecto "envoltorio de caramelo"). **El rígido es un caso particular:** pesos de 1,0 |
| **DQS** (*dual quaternion skinning*, Kavan y otros, 2008) | Mezcla cuaterniones duales en vez de matrices, y conserva el volumen | Igual que LBS | Más complejo, y en algunas poses infla las articulaciones. En low poly casi no se nota la diferencia |

### 3. Dónde corre el skinning

| Opción | Cómo funciona | Problema |
|---|---|---|
| **CPU** | La CPU deforma los vértices cada frame y los sube a la GPU | Medido: no cabe en el presupuesto (ver Investigación) |
| **GPU, en el vertex shader** | Se suben solo las matrices de los huesos, y cada vértice se deforma al dibujarse | El corte no ve la geometría deformada: está solo en la GPU |
| **GPU, en un compute shader** (como el *skin cache* de Unreal) | Se deforma una vez por frame a un buffer, y todas las pasadas lo reutilizan | Solo conviene con varias pasadas (sombras, reflejos). Con sombras simples o ninguna (visión), hace el mismo trabajo y gasta más memoria |
| **Híbrido** | La GPU deforma para dibujar. La CPU deforma solo el zombie que se corta, en el paso del corte | Hay dos implementaciones de la misma cuenta que tienen que coincidir |

### 4. Qué le entrega la animación al corte, y qué pasa con los pedazos

| Opción | Qué pasa con los pedazos | Problema |
|---|---|---|
| **Pedazos estáticos** | Al cortar, la pose se "congela": cada pedazo pasa a ser una malla rígida | La mitad con cabeza no puede arrastrarse, y la visión lo pide |
| **Pedazos animados** | Cada pedazo conserva el esqueleto, con los huesos que ya no tienen vértices enmascarados, y los vértices nuevos reciben pesos | Más complejo. Cada pedazo es un personaje animado más |
| **Mixto** | El pedazo que sigue vivo (con la cabeza) sigue animado. Los pedazos muertos quedan rígidos o pasan a ragdoll | Hay que decidir en el momento qué pedazo es cuál, con las reglas del ADR-0018 |

En las tres opciones, la animación garantiza lo mismo: **la pose del paso de simulación
del corte está en la CPU** (las matrices de los huesos), para que el corte pueda deformar
la malla en esa pose.

### 5. El tajo sin set fijo de golpes

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Espacio de mezcla** (*blend space*) | Unos 8 clips, uno por dirección del tajo, y se mezclan los dos más cercanos al ángulo elegido | Las mezclas entre clips muy distintos se ven blandas, y el filo no sigue exactamente la línea dibujada |
| **Arco procedural con IK** | La punta de la espada recorre un arco en el plano del tajo (la línea del jugador más la dirección de la cámara). Los dos brazos alcanzan la empuñadura con IK de dos huesos, y el torso gira con el arco | Hay que lograr que se vea con peso, sin clips de referencia |
| **Espada física** (*Die by the Sword*, *Half Sword*) | La espada es un cuerpo físico que se empuja con fuerzas | El filo no pasa exactamente por donde se apuntó, y la visión quiere precisión |
| **Mixto** | Arco procedural con IK para la espada y los brazos, y un clip para el cuerpo (el paso adelante, la carga, el peso) | Hay que mezclar bien el clip del cuerpo con la IK de los brazos |

### 6. El zombie al que le faltan partes

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Un clip por estado** | Clips de "cojear", "arrastrarse con los brazos", "arrastrarse con un brazo"… La máquina de estados elige según las partes que quedan, y los huesos perdidos se ignoran (se enmascaran) | Hay que producir esos clips |
| **Locomoción procedural** | Las reglas mueven lo que queda: por ejemplo, el brazo que queda tira del cuerpo | Difícil de lograr creíble |
| **Física activa** | El ragdoll con motores se las arregla solo con lo que le queda | Lo mismo que en el enfoque: caro y difícil de controlar |

## Investigación

### Cuánto cuesta el skinning en la CPU, medido

Medido en la máquina de referencia (ADR-0020) el 2026-10-19. El código y cómo repetirlo
están en [`../research/skinning-y-prototipo-del-tajo.md`](../research/skinning-y-prototipo-del-tajo.md):

- **Carga:** 50 zombies de 3000 vértices con 20 huesos, en C escalar y un hilo.
- **Compilación:** `-O2` sin `-march`, como pide el ADR-0008.
- **Resultado:** el mejor de 200 repeticiones, en 5 ejecuciones.

| Qué | Resultado |
|---|---|
| LBS, 4 pesos, 50 zombies | **2,5 a 2,9 ms** |
| Rígido, 1 hueso, 50 zombies | 1,2 a 1,4 ms |
| LBS, **un solo zombie** | **~50 µs** |

Es una medición de laboratorio, no del protocolo del ADR-0020: no hubo calentamiento
térmico, y los datos eran sintéticos. Aun así, alcanza para decidir:

- **El skinning de todos los zombies en la CPU no cabe.** La animación tiene 1,2 ms por
  paso, y el LBS solo ya ocupa más del doble.
- **Deformar en la CPU solo el zombie que se corta sí cabe:** 50 µs, dentro de los
  2,0 ms del corte.
- **En la GPU, la cuenta es chica:** 150 000 vértices por frame. La UHD 620 tiene que
  medirse cuando exista el renderer, pero el vertex shader de skinning es el caso común
  en la industria desde hace dos décadas.
- **Subir las matrices de los huesos** cuesta 50 × 20 × 64 B (`mat4`, ADR-0008) = 64 KB
  por frame, nada frente al ancho de banda medido en el ADR-0020.

### Skinning

- **LBS** es la opción número uno de la industria por simplicidad y eficiencia, a pesar
  de sus artefactos (Kavan y otros, 2008).
- **DQS** (Kavan, Collins, Žára y O'Sullivan, *ACM TOG* 2008) corrige el colapso de
  volumen con un costo parecido en la GPU, pero en algunas poses infla las
  articulaciones.
- **Los juegos de PS1** usaban skinning rígido: cada vértice pesaba 100 % en un hueso.
  *Final Fantasy VII*, *Resident Evil* y *Tekken* además separaban el modelo en piezas
  por miembro, mientras que *Dino Crisis* usaba una malla continua con pesos de 0 y 1.
  Es parte del aspecto PSX que la visión busca.
- **glTF 2.0** define hasta 4 huesos por vértice (`JOINTS_0` y `WEIGHTS_0`), que es lo
  que exportan las herramientas de modelado.
- **El *skin cache* de Unreal** deforma en un compute shader para reutilizar el
  resultado en varias pasadas (profundidad, base, sombras, velocidad). Hace el mismo
  cálculo que el vertex shader, y la ganancia solo aparece con varias pasadas.

### Cortar una malla animada

- **Los vértices nuevos** que crea el corte en una arista pueden recibir los pesos
  interpolados de los dos extremos de la arista. Así el pedazo sigue siendo una malla
  animada. Es lo que hacen los plugins comerciales de corte de *skeletal meshes*, en los
  que un corte produce dos mallas animadas completas.
- **El corte se calcula en la pose del instante,** pero los vértices nuevos se guardan
  en la pose de referencia (*bind pose*), para que el pedazo se siga animando con los
  mismos huesos. Hay dos formas, y el ADR-0007 elige:
  - **interpolar** la posición de referencia con el mismo `t` de la arista. Es simple,
    pero **no es exacto con LBS**. El skinning es lineal en la posición si los pesos
    están fijos, y lineal en los pesos si la posición está fija, pero no en las dos
    cosas a la vez. Queda un término cruzado `−t(1−t)·Σᵢ(w₁ᵢ − w₀ᵢ)·Mᵢ·(b₁ − b₀)`. Se
    anula si los dos extremos tienen los mismos pesos, o si los huesos involucrados
    tienen la misma rotación (por ejemplo, en la pose de referencia), y crece con la
    rotación relativa entre ellos. Con una arista de 10 cm entre dos huesos girados 30°,
    el vértice se sale del plano del corte hasta ~1,3 cm;
  - **des-deformar** el punto del corte: `b = (Σᵢ wᵢ·Mᵢ)⁻¹·p`, con `p` el punto de
    intersección en la pose del instante. Deformado de nuevo en esa pose, cae justo en el
    plano. Es exacto con dos condiciones: `w` tienen que ser los pesos finales (ya
    recortados a 4 y renormalizados), y la matriz mezclada tiene que ser invertible. En
    torsiones extremas puede ser casi singular: es el mismo colapso del "envoltorio de
    caramelo".
- **Pueden aparecer más de 4 huesos:** si cada extremo de la arista tiene 4 huesos
  distintos, la mezcla tiene hasta 8. Lo habitual es conservar los 4 de mayor peso y
  renormalizar para que sumen 1. Con la interpolación, eso mueve un poco el vértice. Si
  se des-deforma después de recortar, el resultado sigue siendo exacto. El detalle lo
  decide el ADR-0007.
- **Transferir pesos** a geometría nueva es un problema estudiado. Abdrashitov y otros
  (Epic Games, SIGGRAPH Asia 2023) lo resuelven en dos etapas: primero transfieren los
  pesos donde es confiable, y después interpolan el resto.

### Animación procedural y física

- **IK de dos huesos analítica:** con un hombro, un codo y una muñeca, la posición del
  codo sale de la ley de los cosenos. Es exacta, barata y estándar para brazos y
  piernas (Jason Gregory, *Game Engine Architecture*).
- **FABRIK** (Aristidou y Lasenby, *Graphical Models*, 2011): IK iterativa para cadenas
  largas. Busca cada articulación sobre una línea, sin ángulos ni matrices. Converge en
  pocas iteraciones, y está integrada en Unreal y Unity.
- **Euphoria** (NaturalMotion, *GTA IV*, 2008): *Dynamic Motion Synthesis*, una
  simulación del cuerpo y de su control motor, sin clips. Es el referente de la física
  activa.
- ***Half Sword*** simula todo el combate con física, con ragdolls activos y una espada
  física. Es exacto y exagerado a la vez, pero el filo no va donde se apunta, sino donde
  lo llevan las fuerzas.
- ***Motion matching*** (Simon Clavet, Ubisoft, GDC 2016) busca en una base grande de
  captura de movimiento el fragmento que mejor sigue al personaje. Es el estándar actual
  de la industria, pero necesita horas de captura: no aplica a una sola persona.

### Costo de producción

- **El ADR-0019 descarta** las bibliotecas de animaciones con licencias propias.
  Mixamo, por ejemplo, no es CC0 ni CC BY.
- **Quaternius publica bajo CC0** dos bibliotecas de animaciones para un esqueleto
  humano común: la *Universal Animation Library* 1 y 2, con más de 250 animaciones en
  total. La segunda incluye **locomoción de zombie**, combate cuerpo a cuerpo y
  arrastrarse. Si se usan, cómo se usan y cómo se adaptan lo decide el ADR-0017.
- **Las capas reducen los clips:** la IK de la espada reemplaza todos los clips de
  ataque, y enmascarar los huesos perdidos permite reutilizar un clip de arrastrarse con
  una o dos manos.

### Prototipo del tajo

Un prototipo en el navegador (three.js, a 640×360 y con la cámara de la visión) comparó
dos formas de animar el tajo. Está descrito en la nota de investigación:

- **arco procedural con IK:** la punta sigue exactamente el arco del plano elegido;
- **mezcla de 8 clips,** uno por dirección cada 45°. El prototipo mezcla linealmente las
  posiciones de los dos clips más cercanos. Entre dos direcciones, eso acerca la punta
  al cuerpo hasta un 7,6 % (1 − cos 22,5°, a mitad de camino) y la saca del arco. Una
  mezcla por articulaciones con cuaterniones, como la haría el engine (ADR-0008), no
  acortaría así el radio, pero la punta tampoco recorrería el arco exacto: cada clip
  tiene su propio recorrido, y solo la IK pone el filo justo sobre la línea.

También mostró un problema de alcance: el plano del tajo contiene el rayo central de la
cámara, que no pasa por el cuerpo de la caballera.

### Dónde corre en el frame

- **La pose se evalúa en cada paso de simulación** (ADR-0012), así que es determinista.
  En el orden del ADR-0012 (gameplay → combate → corte → física → animación), la
  animación corre **después** del corte. Entonces el combate y el corte del paso N usan
  la pose con que terminó el paso N−1, que además es la última que vio el jugador.
- **El render interpola** entre la pose del paso anterior y la del actual, igual que
  interpola las posiciones (ADR-0012). Para las rotaciones de los huesos basta *nlerp*
  (interpolar y normalizar el cuaternión), porque dos pasos seguidos están muy cerca.
  Como `q` y `−q` son la misma rotación, si `dot(q₀, q₁) < 0` se invierte `q₁` antes de
  interpolar: si no, *nlerp* toma el camino largo.
- **La pausa de impacto** (visión) congela el reloj de la animación sin tocar el paso
  fijo.

## Decisión

Elijo combinar las tres técnicas, cada una donde mejor funciona, y probé el tajo en un
prototipo antes de decidir. El arco procedural con IK sigue la línea que dibujo, y la
mezcla de clips se sale del arco: eso define la sensación del juego.

| # | Dimensión | Elección |
|---|---|---|
| 1 | Enfoque | **Por capas.** Clips para la base (trotar, caminar, arrastrarse), procedural encima (la espada, el torso, los agarres) y física para lo que ya no tiene control (pedazos y muertes, ADR-0016) |
| 2 | Skinning | **LBS, con hasta 4 huesos por vértice.** El skinning rígido (pesos de 1,0) es un caso particular: el aspecto segmentado de PS1 lo decide cada asset, no el engine |
| 3 | Dónde | **Híbrido.** La GPU deforma en el vertex shader para dibujar. La CPU deforma **solo el zombie que se corta**, en el paso del corte |
| 4 | Corte y pedazos | **Mixto.** El pedazo que sigue vivo (con la cabeza, según el ADR-0018) conserva **el esqueleto completo**, con los huesos que perdió enmascarados, y sigue animado con los mismos clips. Los pedazos muertos quedan rígidos o pasan a ragdoll (ADR-0016) |
| 5 | El tajo | **Mixto.** La punta recorre un arco en el plano del tajo, los dos brazos alcanzan la empuñadura con IK analítica de dos huesos, el torso gira siguiendo la punta, y un clip da el peso del cuerpo (el paso adelante y la carga) |
| 6 | Zombie mutilado | **Un clip por estado** (cojear, arrastrarse con dos brazos o con uno…), con los huesos perdidos enmascarados |
| 7 | En el frame | La pose se evalúa **en cada paso de simulación** (ADR-0012), después del corte: el combate y el corte usan la pose con que terminó el paso anterior. El render interpola con *nlerp* (revisando el hemisferio), y la pausa de impacto congela el reloj de la animación de los personajes que congela (ADR-0018) |

### Lo que la animación le garantiza al corte (ADR-0007)

- Las **matrices de los huesos** con que terminó el paso anterior, que son la pose con
  que el combate detectó el contacto, están en la CPU cuando corre el corte.
- La CPU puede deformar con LBS la malla del zombie cortado en esa pose, con la misma
  cuenta que el vertex shader.
- Los **pesos de los vértices** están en la CPU, para que el corte les asigne pesos a
  los vértices nuevos y los guarde en la pose de referencia. Así el pedazo vivo se sigue
  animando con los mismos huesos. El método (interpolar o des-deformar, y qué hacer si
  pasan de 4 huesos) lo decide el ADR-0007.
- **Los pedazos heredan la pose anterior** (y la transformación anterior) del zombie
  original, para que el render tenga contra qué interpolar en su primer frame.

### Valores de partida del tajo

Salen del prototipo. Son el punto de partida, no definitivos, y se pueden cambiar sin
recompilar (ADR-0021):

| Parámetro | Valor |
|---|---|
| Duración del barrido | 0,26 s de partida; baja a medida que sube la carga, y el tajo rápido tiene la suya (ADR-0018) |
| Paso adelante | 0,35 m |
| Giro del torso hacia la punta | 60 % |
| Giro de la línea mientras se carga | 120 °/s |
| Recorrido del arco | De ~195° a −25°, más 40° hacia atrás con la carga máxima |

Los ángulos del arco se miden dentro del plano del tajo, desde el pecho de la caballera:
**0° apunta al extremo de la línea hacia donde va el filo** (la flecha), **90° apunta
del pecho al punto apuntado, sin su componente sobre la línea** (ADR-0018), y el ángulo
crece de la flecha hacia adelante. Así, 195°
queda detrás y del lado opuesto a la flecha, y −25° pasa un poco más allá de la flecha.
Los valores se escriben en grados para ajustarlos a mano, y se convierten a radianes al
cargarlos (ADR-0008).

## Consecuencias

- **La CPU no deforma mallas en el caso normal.** Los 1,2 ms de animación por paso
  (ADR-0020) quedan para muestrear clips, mezclarlos, resolver la IK y recorrer la
  jerarquía. Deformar un zombie para el corte cuesta ~50 µs y entra en los 2,0 ms del
  corte.
- **Lo que se paga por frame,** no por paso, va dentro de "Render en la CPU" (2,5 ms,
  `docs/performance/presupuestos.md`): interpolar las poses, armar las matrices de
  skinning y subirlas. Las matrices de los pedazos en ragdoll las cuenta el ADR-0016.
- **El skinning de la CPU es una función pura** (entran la malla, los pesos y las
  matrices, y sale la malla deformada) y vive en una capa de abajo, junto a la
  matemática de `core` (ADR-0012). Así `slicing` y `animation` la usan sin depender uno
  del otro, y el corte lee las matrices de los huesos como un componente del ECS.
- **Hay dos implementaciones del mismo skinning,** la del shader y la de la CPU, y
  tienen que coincidir. Cuando exista el renderer, un test deforma una malla de prueba
  en la GPU (con lavapipe, ADR-0011), lee el resultado y lo compara vértice a vértice
  con el de la CPU, con tolerancia.
- **Tests de la animación (ADR-0011):**
  - **TDD y cobertura del 100 %** en lo que tiene especificación cerrada: el skinning de
    la CPU (va en la ruta del corte), la IK de dos huesos, el recorrido de la jerarquía y
    el *nlerp*. Varias propiedades sirven para tests con semilla fija:
    - el codo queda a distancia `a` del hombro;
    - la mano llega al objetivo (con tolerancia) si `|a − b| + ε ≤ d ≤ a + b − ε`, y
      fuera de ese rango queda en el punto alcanzable más cercano, sobre la línea
      hombro-objetivo;
    - con todas las matrices de skinning en la identidad y pesos que suman 1, el
      resultado es la pose de referencia;
  - **tests después** en lo que es sensación: la máquina de estados y la mezcla de
    capas.
- **El formato de vértice de las mallas animadas** suma 4 índices de hueso y 4 pesos por
  vértice. Cómo se importan (por ejemplo, desde `JOINTS_0` y `WEIGHTS_0` de glTF) y el
  número máximo de huesos por esqueleto los decide el ADR-0017.
- **El pedazo vivo es un personaje más:** cuenta como uno de los 50 zombies de R5
  (ADR-0020). Los pedazos muertos cuentan como pedazos.
- **Clips mínimos para el corte vertical:**
  - zombie: caminar, cojear, arrastrarse con dos brazos y con uno, agarrar;
  - caballera: trotar, el salto de esquive, el paso del tajo, la carga y sacudirse de un
    agarre.

  De dónde salen lo decide el ADR-0017. Las bibliotecas CC0 de Quaternius son una
  candidata compatible con el ADR-0019.
- **Determinismo:** como la pose sale del paso de simulación, una partida grabada
  reproduce los mismos cortes (ADR-0020).
- **Lo que se sacrifica:**
  - la física activa, más viva pero cara y difícil de controlar;
  - el *motion matching*, que necesita horas de captura;
  - DQS, que conserva mejor el volumen en giros fuertes.
- **Abierto:**
  - **el alcance:** el plano del tajo contiene el rayo central de la cámara, que pasa a
    0,5 m a la izquierda de la caballera y más arriba. En algunos ángulos, el arco queda
    lejos del pecho y los brazos no llegan (el prototipo lo mide). Cómo se resuelve
    (mover el centro del arco, un paso lateral o limitar los ángulos) lo deciden el
    ADR-0018 y el ajuste del combate (decidido en el ADR-0018: el plano pasa por el
    pecho, que es siempre el centro del arco);
  - el traspaso de la pose al ragdoll y los cuerpos físicos de los pedazos (ADR-0016);
  - qué partes definen cada estado del zombie mutilado (decidido en el ADR-0018);
  - cómo se asignan los pesos de los vértices nuevos: interpolar (simple, con un error
    fuera del plano del corte) o des-deformar (exacto), y qué hacer cuando pasan de 4
    huesos (ADR-0007);
  - las matrices de los huesos cambian en cada frame, así que van en el ring buffer por
    frame en vuelo del ADR-0005, igual que la geometría del corte. Su tamaño (~64 KB por
    frame) se suma a esa fila de `presupuestos.md` cuando se fije el número de frames en
    vuelo (ADR-0007 y ADR-0010).

## Cómo lo defiendo

**Si el corte necesita la malla deformada en la CPU, ¿por qué no deformas todo en la
CPU?**
Porque sería demasiado trabajo solo para la CPU, y rompería el límite que nos
propusimos. Deformar los 50 zombies con LBS, medido, cuesta de 2,5 a 2,9 ms, más del
doble de los 1,2 ms que tiene la animación. En cambio, deformar solo el zombie que se
corta cuesta unos 50 µs.

**¿Qué es el efecto "envoltorio de caramelo" de LBS, y por qué no te preocupa?**
Con una torsión extrema, la geometría se aplasta: cada vértice se mueve con un promedio
de matrices, y el promedio de dos rotaciones muy distintas colapsa el volumen hacia el
eje, como el centro retorcido de un envoltorio de caramelo. Sí me preocupa, y hay que
vigilarlo, aunque en personajes low poly hay pocas torsiones extremas. Si aparece, se
agrega un hueso intermedio de torsión en el antebrazo. Si eso no alcanza, se evaluaría
DQS en un ADR nuevo, porque cambia la Decisión 2.

**Cortas un zombie por la cintura. ¿Cómo sigue animándose la mitad de arriba? ¿Qué pesos
tienen los vértices nuevos del corte?**
Cada vértice tiene pesos: hasta 4 huesos y cuánto lo mueve cada uno. Los vértices nuevos
que crea el corte nacen en medio de una arista, sin pesos. El método candidato les da la
mezcla de los pesos de sus dos extremos: si el corte pasa al 30 % del camino entre un
vértice de la pelvis y uno de la columna, el vértice nuevo se mueve 70 % como el primero
y 30 % como el segundo. Por eso la mitad de arriba sigue siendo una malla animada, con el
mismo esqueleto y los huesos perdidos enmascarados, y la máquina de estados le pone el
clip de arrastrarse. Sin esos pesos quedaría como una estatua. El método exacto lo decide
el ADR-0007.

**¿Por qué IK para la espada y no clips de ataque, como hace casi todo el mundo?**
Por la flexibilidad en los ángulos, que hace que se vea mejor: con IK, el filo pasa
exactamente por la línea que apunté, y en el prototipo la mezcla de clips se salía del
arco. Además, quiero aprender IK.

***Half Sword* simula todo con física. ¿Por qué no la física activa, si es lo más
realista?**
Porque es mucho cálculo para las limitaciones de hardware que nos propusimos. Además,
con física el filo va donde lo llevan las fuerzas, no donde apunté, y la visión pide
precisión.

**¿Cómo calcula la IK de dos huesos la posición del codo?**
Con la ley de los cosenos. Conozco el largo del brazo (`a`), el del antebrazo (`b`) y la
distancia del hombro a la mano (`d`), que forman un triángulo. El ángulo en el hombro
sale de `cos(α) = (a² + d² − b²) / (2·a·d)`, y el codo queda a una distancia `a` del
hombro, girado α desde la línea hombro-mano. Hacia qué lado se dobla lo decide un vector
polo (por ejemplo, hacia abajo y afuera). No itera: es una fórmula directa, y por eso es
barata. Antes de calcular, `d` se acota a `[|a − b| + ε, a + b − ε]`: si la mano queda
fuera de alcance, el argumento del `acos` sale de [−1, 1] y el resultado sería NaN.

**¿Por qué la pose se calcula en el paso de simulación y no en cada frame de render?**
Por tres motivos:

- **Determinismo:** el paso es fijo, y si la pose dependiera del frame de render, la
  misma partida grabada daría poses y cortes distintos.
- **El corte** ocurre dentro del paso de simulación. Como la animación corre después
  del corte, el combate y el corte usan la pose con que terminó el paso anterior, que es
  la última que vio el jugador: el corte pasa por donde él vio la espada.
- **La suavidad no se pierde:** el render interpola entre la pose del paso anterior y
  la del actual, igual que las posiciones.

## Referencias

- Ladislav Kavan, Steven Collins, Jiří Žára y Carol O'Sullivan, *Geometric Skinning
  with Approximate Dual Quaternion Blending*, ACM Transactions on Graphics 27(4), 2008:
  <https://users.cs.utah.edu/~ladislav/kavan08geometric/kavan08geometric.html>
- Andreas Aristidou y Joan Lasenby, *FABRIK: A fast, iterative solver for the Inverse
  Kinematics problem*, Graphical Models 73(5), 2011:
  <https://www.andreasaristidou.com/FABRIK>
- Rinat Abdrashitov y otros, *Robust Skin Weights Transfer via Weight Inpainting*,
  SIGGRAPH Asia 2023: <https://dl.acm.org/doi/fullHtml/10.1145/3610543.3626180>
- Jason Gregory, *Game Engine Architecture*, 4.ª ed., capítulo de sistemas de animación.
- Simon Clavet, *Motion Matching and The Road to Next-Gen Animation*, GDC 2016.
- NaturalMotion, Euphoria: <https://en.wikipedia.org/wiki/NaturalMotion>
- Epic Games, *How Half Sword makes bloody, brutal medieval combat feel real*:
  <https://store.epicgames.com/news/how-half-sword-makes-bloody-brutal-medieval-combat-feel-real>
- Unreal Engine, `FGPUSkinCache`:
  <https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/FGPUSkinCache>
- Wicked Engine, *Skinning in a Compute Shader*:
  <https://wickedengine.net/2017/09/skinning-in-compute-shader>
- Polycount, sobre el skinning rígido en PS1:
  <https://polycount.com/discussion/comment/2795099>
- Khronos, *glTF 2.0 Specification*, sección *Skins*:
  <https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#skins>
- Quaternius, *Universal Animation Library* (CC0):
  <https://quaternius.com/packs/universalanimationlibrary.html>
