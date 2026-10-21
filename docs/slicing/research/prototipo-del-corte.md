# Prototipo del corte de una malla animada

Nota de investigación del [ADR-0007](../adr/0007-corte-de-mallas.md). Guarda lo que midió
y lo que encontró un prototipo del corte, para poder repetirlo.

Fecha: 2026-10-20, en la máquina de referencia del ADR-0020 (Dell XPS 13 9360,
i7-8550U). Son mediciones de laboratorio para decidir, no del protocolo del ADR-0020:
no hubo calentamiento térmico y había otras aplicaciones abiertas. Entre ejecuciones,
los tiempos variaron hasta un 30 %.

## Qué hace el prototipo

Un programa en C++20 de un solo archivo, compilado con `g++ -std=c++20 -O2`, sin
`-march` (ADR-0008), en un hilo. No es código del engine: quedó fuera del repo, igual
que los scripts del ADR-0017.

**El material:** el maniquí masculino de Quaternius (*Universal Animation Library 2*,
CC0, ADR-0019), con su esqueleto de 65 huesos, en el instante 0,5 s del clip
`Zombie_Walk_Fwd_Loop` (agachado, con las rodillas y los codos doblados) y en la pose
de referencia (*T-pose*), como control. Un script de Python lee el `.glb`, calcula las
matrices de skinning de esa pose y escribe un binario simple.

**Lo que hace el "cocinador" del prototipo,** una vez por malla:

1. un **índice soldado**: cada vértice apunta al primero que tiene la misma posición de
   referencia, bit a bit. Es el *shadow index buffer* de meshoptimizer;
2. las **cáscaras**: las componentes conexas de la malla soldada;
3. los **huesos dominantes** de cada cáscara, con los dedos fundidos en la mano, como
   pide el ADR-0017.

**Lo que hace cada corte:**

1. **Skinning en la CPU** (LBS, 4 huesos) de todos los vértices, en la pose del instante.
2. **Clasificar** cada vértice soldado **una sola vez**, por el signo de su distancia al
   plano. Una distancia exactamente 0 cuenta como positiva (desempate simbólico): así,
   toda arista que cruza tiene extremos con signos opuestos.
3. **Partir los triángulos** que cruzan: el vértice solitario queda en un triángulo, y
   los otros dos en dos triángulos, con el mismo sentido de giro.
4. **Un punto de corte por arista soldada,** calculado una sola vez desde el extremo de
   menor índice. Los dos triángulos de una arista, y los dos lados de una costura de UV,
   reciben exactamente el mismo punto.
5. **Pesos del punto nuevo:** la mezcla de los pesos de los dos extremos, los 4 de mayor
   peso, renormalizados. **Posición de referencia:** se des-deforma el punto,
   `b = (Σ wᵢ·Mᵢ)⁻¹·p`, con los pesos ya recortados. Para comparar, también se calcula la
   interpolación.
6. **Contornos:** cada triángulo partido aporta un segmento orientado entre dos puntos
   de corte. Los segmentos se encadenan por el índice del punto, sin comparar
   posiciones.
7. **Tapas:** los contornos se proyectan al plano. Los de área positiva son exteriores, y
   los de área negativa, agujeros. Un agujero se une al exterior más chico **de la misma
   cáscara** que lo contiene, con un puente (Eberly), y cada polígono se triangula
   cortando orejas (*ear clipping*). Cada lado recibe su tapa, con la normal hacia
   afuera.
8. **Pedazos:** dentro de las cáscaras que el plano cruzó, *union-find* sobre los
   triángulos de cada lado. Las cáscaras enteras quedan como están. Después, dos
   fragmentos del mismo lado se unen si comparten un hueso dominante.
9. **Salida:** cada pedazo es una lista de índices sobre el buffer de vértices original,
   que no se copia, más los vértices nuevos (los de las aristas, compartidos por los dos
   lados, y los de las tapas, uno por lado). Para comparar, se mide también copiar
   todos los vértices de cada pedazo a un formato de 32 B.

**Lo que verifica**, fuera del tiempo medido:

- **cerrado y orientado:** cada arista dirigida de un pedazo tiene su inversa en el
  mismo pedazo, una sola vez;
- **el volumen:** la suma de los volúmenes de los pedazos, en la pose, es la del
  original;
- **el error de cada método** de posición de referencia: se vuelve a deformar el vértice
  nuevo y se mide su distancia al plano.

## La malla

| Qué | Resultado |
|---|---|
| Vértices | 8546 (6994 soldados): glTF duplica 1552 en las costuras |
| Triángulos | 13 744 |
| **Cáscaras cerradas** | **61**: torso con cabeza, 2 brazos, 2 piernas y 56 piezas de dedos (falanges y articulaciones). Se superponen, no están unidas |
| Defectos | **Una arista del torso la comparten más de dos triángulos** (no es *manifold*) |
| Cocinado (índice soldado, cáscaras y huesos) | 2,4 ms |

**No es una malla cerrada:** es un conjunto de cáscaras cerradas encajadas unas en otras.
Es habitual en personajes de juegos: los ojos, los dientes, el pelo o una armadura
suelen ser piezas aparte.

## Cinco cortes, en la pose agachada

| Corte | Puntos de corte | Contornos | Pedazos (− / +) | Vértices nuevos | Tiempo |
|---|---|---|---|---|---|
| Cintura, horizontal | 107 | 3 | **3 / 1** | 329 | 0,54 a 0,70 ms |
| Cuello, inclinado | 126 | 3 | 1 / 1 | 382 | 0,65 a 0,74 ms |
| Rodillas, horizontal | 58 | 2 | **2 / 1** | 182 | 0,53 a 0,61 ms |
| Diagonal, del hombro a la cadera | 124 | 3 | 2 / 2 | 392 | 0,63 a 0,77 ms |
| Vertical, de la cabeza a la entrepierna | 183 | 1 | 1 / 1 | 569 | 0,73 a 0,83 ms |

- **Un corte da N pedazos, no 2.** En la cintura, el plano corta también los dos
  antebrazos, que cuelgan a esa altura: abajo quedan las piernas con la pelvis y las dos
  manos. En las rodillas, las dos pantorrillas caen por separado. Con planos al azar
  aparecieron cortes de hasta 5 pedazos.
- **La conectividad sola no alcanza:** en la cintura, el lado de abajo tiene 58 cáscaras
  enteras (las piernas y los dedos). Agrupadas por los huesos que comparten, quedan 3
  pedazos.

### En qué se va el tiempo

Peor caso medido (el corte vertical), el mejor de 300 repeticiones:

| Etapa | Tiempo |
|---|---|
| Skinning de toda la malla en la CPU | 0,12 a 0,13 ms |
| Clasificar y partir los triángulos | 0,25 a 0,26 ms |
| Contornos y tapas | 0,12 a 0,13 ms |
| Pedazos (conectividad, huesos e índices de cada pedazo) | 0,29 a 0,30 ms |
| **Total** | **0,79 a 0,83 ms** |
| Además, copiar los vértices de cada pedazo | +0,23 ms y ~290 KB |

- **La malla del maniquí tiene el triple que un zombie del juego** (~3000 vértices,
  ADR-0020). El costo crece con los triángulos: para un zombie del juego, del orden de
  **0,2 a 0,3 ms por corte**, y 4 cortes en un paso (R5) caben en los 2,0 ms de
  "combate y corte" (`docs/performance/presupuestos.md`).
- **El prototipo no está optimizado:** usa `std::vector` que crecen, y recorre la malla
  entera en varias pasadas. Es una cota superior.
- **Las tapas** cuestan poco con contornos chicos: el corte vertical tiene un solo
  contorno de 183 puntos, y cortar orejas es O(n²).

## Interpolar o des-deformar la posición de referencia

Distancia al plano de los vértices nuevos, vueltos a deformar en la pose del corte:

| Corte | Interpolar: máximo | Interpolar: promedio | Des-deformar: máximo |
|---|---|---|---|
| Cintura | 0,71 mm | 0,07 mm | 0,0002 mm |
| **Cuello** | **10,07 mm** | 0,51 mm | 0,0002 mm |
| Rodillas | 3,09 mm | 0,43 mm | 0,0001 mm |
| Diagonal | 0,79 mm | 0,10 mm | 0,0002 mm |
| Vertical | 0,70 mm | 0,04 mm | 0,0001 mm |
| **Control: pose de referencia** | **0,00 mm** | 0,00 mm | 0,0001 mm |

- **Confirma la cuenta del ADR-0015:** el error de interpolar se anula en la pose de
  referencia y crece con la rotación relativa entre huesos. En el cuello agachado llega
  a 1 cm: el borde de la piel no coincide con el borde de la tapa, y se ve una grieta o
  un escalón.
- **Des-deformar es exacto** hasta el redondeo de `float`. El determinante más chico de
  la matriz mezclada fue 0,655: ninguna estuvo cerca de ser singular.
- **Más de 4 huesos:** hasta 44 de los 183 puntos del corte vertical mezclaron más de 4
  huesos. Se conservaron los 4 de mayor peso, y como se des-deforma después de
  recortar, el resultado sigue siendo exacto.

## Robustez

2000 planos al azar que cruzan el cuerpo, y 1000 planos que pasan **exactamente** por
uno o dos vértices de la pose (el caso degenerado), en las dos poses:

| Qué | Pose agachada | Pose de referencia |
|---|---|---|
| Planos al azar con todos los pedazos cerrados | 1975 / 2000 | 1979 / 2000 |
| Planos por vértices con todos los pedazos cerrados | 981 / 1000 | 977 / 1000 |
| **Fallos que cruzan la arista defectuosa del torso** | **44** | **44** |
| **Otros fallos** | **0** | **0** |
| Error de volumen | ~10⁻⁸ (relativo) | ~10⁻⁸ |

**Todos los fallos vienen del defecto del asset.** Si el plano pasa por la arista que
comparten más de dos triángulos, el contorno queda abierto y la tapa no se puede cerrar.
(En la pose de referencia, además, uno de los cinco cortes de prueba con nombre también
cruzó esa arista. No está en la tabla, que cuenta solo los planos al azar).

Tres problemas aparecieron durante la investigación y quedaron resueltos en el
prototipo:

1. **Agujeros de otra cáscara.** Los contornos de cáscaras superpuestas se encimaban,
   y un contorno de una se tomaba como agujero de otra. Los agujeros solo se buscan en
   la misma cáscara.
2. **Contornos invertidos.** En la pose agachada, LBS hace que la malla se cruce consigo
   misma en las articulaciones dobladas, y aparecen contornos con la orientación al
   revés y sin contenedor. Se triangulan solos, respetando su orientación, para que el
   pedazo quede cerrado (aunque esa parte de la tapa mire hacia adentro).
3. **Contornos de área cero.** Cuando el plano roza una cáscara justo en un vértice,
   queda un contorno de 3 puntos con área exactamente 0 en `float`. No era ni exterior
   ni agujero, y se descartaba. Ahora se triangula como exterior.

En ~26 % de los cortes al azar, cortar orejas tuvo que forzar alguna (no encontró una
oreja válida y cortó igual). El pedazo queda cerrado, pero esos triángulos de la tapa
pueden quedar degenerados o encimados. Pasa con puntos repetidos o alineados, en
contornos degenerados. Ninguno de los cinco cortes de la tabla forzó una oreja.

## La hoja detenida: hendidura

Una segunda versión del prototipo corta solo la **región barrida**: la parte del plano
por donde pasó la hoja hasta detenerse. Para la topología basta un semiplano: la hoja
entra desde un lado y se detiene en una recta.

**Cómo funciona:**

1. Primero se arman los contornos, como en un corte completo, pero sin partir nada.
2. Cada segmento del contorno está dentro de la región si su punto medio lo está. Un
   contorno con todos sus segmentos dentro se corta completo. Uno sin ninguno, no se
   toca. Uno con algunos tiene **tramos** de segmentos seguidos dentro.
3. Cada punto de corte queda en uno de tres estados:
   - **partido:** está dentro de un tramo, y tiene un vértice por lado;
   - **unido:** es el extremo de un tramo, donde se juntan las dos caras de la ranura, y
     tiene un solo vértice para los dos lados;
   - **sin usar:** la hoja no llegó.
4. Se parten los triángulos con sus dos puntos usados. Un triángulo que tiene un solo
   punto usado, el extremo de un tramo, se parte en abanico desde ese punto, sin
   separar nada. Así no quedan vértices en T.
5. Cada tramo se cierra con una tapa por lado: el tramo más la cuerda que va de su último
   punto al primero, que es el fondo de la ranura. Las dos tapas comparten esa cuerda.
6. Los pedazos salen de la conectividad, sobre identificadores que distinguen los dos
   lados de cada punto partido, y de los huesos compartidos **del mismo lado**.

**Resultados, en la pose agachada:**

| Caso | Contornos completos / parciales | Pedazos | Tiempo |
|---|---|---|---|
| Cintura, la hoja cruza todo | 3 / 0 | 4 (igual que la primera versión) | 0,72 ms |
| Cintura, la hoja entra por la izquierda y se detiene en el centro del torso | 1 / 1 | **2**: cae el antebrazo izquierdo, y el cuerpo queda entero, con la ranura | 0,76 ms |
| Cintura, la hoja solo alcanza el antebrazo izquierdo | 1 / 0 | 2 | 0,72 ms |
| Cuello, la hoja se detiene a mitad del cuello | 1 / 1 | **1**, con la ranura | 0,75 ms |
| Rodillas, corta la pierna izquierda y se detiene en la derecha | 1 / 1 | 2 | 0,66 ms |

Todos los pedazos quedaron cerrados, con el volumen conservado (error de ~10⁻⁸).

**Robustez:** 3000 planos al azar con rectas de detención al azar, en las dos poses:

| Qué | Pose agachada | Pose de referencia |
|---|---|---|
| Con todos los pedazos cerrados | 2971 / 3000 | 2958 / 3000 |
| Fallos que cruzan la arista defectuosa del torso | 29 | 42 |
| **Otros fallos** | **0** | **0** |

El prototipo usa una recta de detención sin el borde del radio de la punta: ese roce lo
resuelve el combate con las cápsulas (ADR-0007), así que el corte solo ve la recta.

Un caso apareció y quedó resuelto: **a un contorno le falta un solo segmento** para estar
entero dentro. Entonces la cuerda que cierra la ranura coincide con ese segmento, y esa
arista queda con 4 caras. Como la hoja cruzó todo menos una astilla, ese contorno se
corta completo.

### ¿Se abre sola la ranura?

Si cada cara de la ranura se des-deforma con los pesos del vértice de su lado, las dos
caras coinciden en la pose del corte (0,0002 mm del plano) y se separan cuando el zombie
se mueve. Medido con poses posteriores del mismo clip:

| Caso | 0,1 s después | 0,4 s después |
|---|---|---|
| Torso | máximo 2,0 mm, promedio 0,7 mm | máximo 0,6 mm, promedio 0,1 mm |
| Cuello | máximo 0,5 mm, promedio 0,3 mm | máximo 4,3 mm, promedio 1,7 mm |

**Casi no se abre:** a los dos lados del corte manda casi el mismo hueso. A 640×360 y
con la cámara de la visión, un píxel a 3 m cubre ~0,8 cm (51° de campo vertical) o
~0,45 cm al apuntar (30°): una ranura de milímetros no se ve. Para
que se vea el interior, la ranura se tiene que abrir a propósito.

## Cómo repetirlo

1. Descargar *Universal Animation Library 2* [Standard] de Quaternius (CC0) desde
   OpenGameArt, como en el ADR-0017.
2. Con Python y numpy, leer `UAL2_Standard.glb` (la cabecera, el JSON y el bloque
   binario), evaluar la jerarquía de nodos con el clip `Zombie_Walk_Fwd_Loop` en
   t = 0,5 s y escribir: posiciones, normales, UV, `JOINTS_0`, `WEIGHTS_0`, índices,
   las matrices de skinning (`inverse(M_malla) · M_hueso · bind_inversa`), los padres y
   los nombres de los huesos.
3. Compilar el prototipo con `g++ -std=c++20 -O2` y correrlo con ese archivo y el
   número de repeticiones.
4. Para la hendidura, exportar además las poses de t = 0,6 s y t = 0,9 s del mismo clip,
   que solo aportan sus matrices de skinning.
