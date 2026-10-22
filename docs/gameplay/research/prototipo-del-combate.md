# Prototipo del combate: modelos de fuerza y regla de vivo o muerto

- **Ticket:** #31 (ADR-0018)
- **Fecha:** 2026-10-22

Esta nota mide las preguntas del ADR-0018 que se pueden contestar sin engine: **cómo se
gasta la fuerza del tajo** al cruzar el cuerpo, **cuánto ayudan las juntas** de una
armadura, **cuánto cuesta calcularlo**, **cuántos estados de zombie mutilado** salen de
cada regla de vivo o muerto, y **cuánto se aleja el plano del tajo del pecho** de la
caballera. **La sensación no se puede medir aquí:** la duración de la pausa de impacto o
lo que cuesta sacar la espada se ajustan jugando.

## Qué hace el prototipo

Un exportador en Python y un programa en C++20 de un solo archivo, compilado con
`g++ -std=c++20 -O2`, en un hilo. No es código del engine: queda fuera del repo, igual
que los prototipos del ADR-0007 y del ADR-0017.

**El material:** el maniquí masculino de Quaternius (*Universal Animation Library 2*,
CC0, ADR-0019), en el instante 0,5 s del clip `Zombie_Walk_Fwd_Loop`: agachado, con los
brazos adelante. El exportador recorta el esqueleto a los 23 huesos del ADR-0017 (sin
dedos ni puntas), deforma los vértices en esa pose y **ajusta una cápsula por hueso** a
los vértices que domina: el eje principal de esos vértices y el radio en el percentil 90
de su distancia al eje. Salen 22 cápsulas (la raíz no mueve vértices).

**La escena:** el zombie mira a la caballera, a 1,3 m. La cámara de apuntar de la visión
está 2,0 m detrás de ella, 0,5 m a su izquierda y a 1,9 m de altura. Cada tajo:

1. elige un **punto apuntado** al azar sobre el cuerpo (de 0,15 a 1,5 m de altura) y un
   **ángulo de línea** al azar;
2. arma el **plano del tajo** con el rayo de la cámara y la línea, como pide la visión;
3. pone el **centro del arco** en el pecho de la caballera proyectado sobre el plano,
   con la hoja de 0,5 a 2,0 m del centro, y la barre 220° a través del punto apuntado;
4. en pasos de **dθ** (0,25° de referencia), lanza la hoja como una semirrecta desde el
   centro y mide qué tramo de cada cápsula cruza. Es la misma información que da el
   barrido en el plano del ADR-0016, con más resolución.

Los cinco modelos gastan una **fuerza** `F` contra esos tramos, en el orden en que la hoja
los alcanza. La escala: **`Fmax` (carga máxima) atraviesa 4 campesinos de costo
mediano**, como pide la visión ("un tajo cargado parte hasta 4 zombies").

| Modelo | Qué cuesta | Cuándo rebota |
|---|---|---|
| **A. Umbral por zombie** | Nada: si `F` supera el umbral del tipo de zombie, lo atraviesa entero | Si no lo supera |
| **B. Costo fijo por parte** | Cada cápsula tiene un costo fijo (el de su sección mediana), que se paga entero al entrar | Si no alcanza para la primera parte |
| **C. Costo por área** | El área cortada, por la resistencia del material (carne 1, cota 2,5, placa 6) | Nunca |
| **D. Área + umbral de entrada** | Como C, más un **umbral para entrar** en cada parte (carne 0, cota 0,10, placa 0,35, junta 0,03 `Fmax`) | Si no alcanza el umbral de la primera parte |
| **E. Carne por área + cáscara por largo** | Por dentro todo es carne, por área. La armadura es una **cáscara**: cuesta por metro de placa cortada (cota 0,25, placa 0,9 `Fmax`/m). Con los umbrales de D | Como D |

C se apoya en la mecánica de fractura: cortar es abrir superficie nueva, y la energía por
unidad de área (`Gc`, en J/m²) es una propiedad del material. E agrega lo que pasa con una
placa: es una lámina fina, así que lo que cuesta es su largo cortado, no el área de lo
que encierra.

**Los tres tipos de zombie:** campesino (todo carne); cota de malla (cota en la pelvis, la
columna, las clavículas, los brazos y los muslos); armadura completa (placa en todo, con
juntas en el cuello, las axilas, los codos, la cintura, las ingles y las rodillas). Los
números de cada material son **valores de partida a ojo**, no medidos: lo que compara el
prototipo es la forma de cada modelo, no su calibración.

**Los resultados de un tajo:**

| Resultado | Qué pasa |
|---|---|
| Falla | La hoja no toca ninguna cápsula |
| Rebota | No entra en la primera parte que toca |
| Clavada | Entra y se detiene sin atravesar ninguna parte |
| Corta y se clava | Atraviesa alguna parte (un brazo) y se detiene en otra |
| Atraviesa | Cruza todo el zombie |

## Resultados

### Lo que cuesta atravesar a cada tipo (modelo C, en `Fmax`)

2000 tajos al azar:

| Tipo | p10 | Mediana | p90 | Máximo |
|---|---|---|---|---|
| Campesino | 0,10 | 0,25 | 0,58 | 0,93 |
| Cota de malla | 0,11 | 0,53 | 1,30 | 1,81 |
| Armadura completa | 0,54 | 1,36 | 3,06 | 5,01 |

**El costo depende mucho de por dónde pasa el plano.** En el campesino, la mediana sube de
0,12 `Fmax` a la altura de las rodillas (0,15 a 0,42 m) a 0,40 a la altura del cuello y
la cabeza (1,23 a 1,50 m). En esta pose, un tajo alto cruza también los dos brazos, que
el zombie lleva adelante. **El modelo por área cobra lo que el jugador ve en el camino de
la hoja.**

### Resultados por carga

Con carga 0,15 (la del tajo rápido) y carga máxima, en % de 2000 tajos:

| Modelo | Tipo | Carga | Rebota | Clavada | Corta y se clava | Atraviesa |
|---|---|---|---|---|---|---|
| A | Campesino | 0,15 | 99,5 | 0 | 0 | 0 |
| A | Campesino | 0,25 | 0 | 0 | 0 | 99,5 |
| A | Armadura | 1,0 | 99,5 | 0 | 0 | 0 |
| B | Campesino | 0,15 | 0 | 27,2 | 51,5 | 20,9 |
| B | Armadura | 1,0 | 0 | 17,4 | 57,8 | 24,4 |
| C | Campesino | 0,15 | 0 | 11,0 | 65,8 | 22,8 |
| C | Armadura | 0,15 | 0 | 77,0 | 22,1 | 0,5 |
| C | Armadura | 1,0 | 0 | 8,1 | 61,7 | 29,8 |
| D | Campesino | 0,15 | 0 | 11,0 | 65,8 | 22,8 |
| D | Campesino | 0,5 | 0 | 0 | 13,2 | 86,3 |
| D | Armadura | 0,25 | 97,5 | 1,1 | 0,8 | 0 |
| D | Armadura | 0,5 | 0 | 31,6 | 63,9 | 4,0 |
| D | Armadura | 1,0 | 0 | 11,1 | 63,8 | 24,7 |
| E | Armadura | 1,0 | 0 | 12,8 | 71,3 | 15,3 |

(El 0,5 % que falta en cada fila son tajos que no tocan el cuerpo.)

Lo que muestran:

- **A no puede dejar la hoja clavada.** Es todo o nada: contradice la mecánica que ya
  decidió el ADR-0007.
- **C nunca rebota:** cualquier fuerza entra un poco. Con carga 0,15, el 77 % de los
  tajos contra una armadura completa quedan clavados en la placa. Para que el metal
  rechace un golpe débil hace falta un umbral de entrada (D o E).
- **Con poca fuerza, la hoja casi siempre queda clavada.** Con carga 0,15, un campesino
  deja la hoja clavada en el 77 % de los tajos (clavada más corta y se clava), en B, C y
  D por igual. Pasa con cualquier modelo que gaste la fuerza al avanzar: **si el tajo
  rápido usa las mismas reglas que el cargado, casi siempre deja la espada clavada**, y
  sacarla cuesta energía. El tajo rápido necesita su propia regla (dimensión 3 del ADR).
- **Con la armadura completa, la carga máxima atraviesa solo del 15 al 30 % de los
  tajos.** Es calibración (los números de la placa), no forma: se ajusta.

### Estabilidad: el mismo tajo con un error chico de puntería

El mismo tajo con el punto apuntado movido hasta ±1,5 cm y la línea girada hasta ±2°,
con carga 0,5: % de tajos cuyo resultado cambia de categoría.

| Tipo | B | C | D | E |
|---|---|---|---|---|
| Campesino | 4,2 % | 1,8 % | 2,0 % | 2,5 % |
| Cota de malla | 6,9 % | 1,3 % | 2,0 % | 2,0 % |
| Armadura completa | 5,9 % | 6,5 % | 10,4 % | 12,9 % |

**B es el más inestable en carne y cota:** rozar el borde de una parte cuesta lo mismo que
cruzarla entera, así que un centímetro cambia el resultado. **Los umbrales de entrada (D y
E) suman inestabilidad en la armadura:** cerca del umbral, un error chico decide entre
entrar y no entrar. Es el precio de que el metal rechace golpes. Cuando el resultado no
cambia, el punto donde se detiene la hoja se mueve de 1° a 2° en todos los modelos.

### Las juntas

La primera prueba modeló cada junta como una **esfera en el espacio** (de 6 cm, 10 en la
cintura) donde la placa no existe. Para comparar sin sesgo, se corta el mismo tajo con y
sin juntas:

| Cómo se define la junta | Costo con juntas / sin juntas (mediana) | Atraviesa con carga máxima | Entra con carga 0,25 |
|---|---|---|---|
| Esfera en el espacio, modelo D | 90 % | 13,8 % contra 11,9 % | — |
| Esfera en el espacio, modelo E | 97 % | 7,7 % contra 6,2 % | — |
| **Por eje, modelo E** | **84 %** (p10 67 %) | **8,7 % contra 1,2 %** | **23,9 % contra 0 %** |

**Una junta como esfera casi no cambia nada:** es una parte chica de todo lo que corta el
plano, y las cápsulas se superponen en las articulaciones (el borde de una queda dentro
de la otra). La tercera fila define la junta **por parte:** si el plano cruza el eje de
una cápsula a menos de 8 cm de una articulación, toda esa parte se trata como junta. Así,
**acertar una junta es lo que deja entrar a un tajo débil:** con carga 0,25, uno de cada
cuatro tajos que pasa por una junta entra, contra ninguno sin juntas. El costo total
baja menos, porque el resto de las partes que cruza el plano siguen blindadas. **Para que
la junta valga, el tajo tiene que cruzar pocas partes:** el cuello sin los hombros, el
codo sin el torso.

### El ángulo

Medido después de que el autor eligiera combinar el ángulo con las juntas por eje. **El
ángulo** es el que forma el plano del tajo con el hueso: su seno es
`|dirección del hueso · normal del plano|`. Vale 1 si el tajo cruza el hueso de forma
perpendicular (un brazo cortado de lado a lado), y 0 si el plano corre a lo largo del
hueso. **La regla:** el umbral de entrada del metal se divide por ese seno, con un piso
que limita cuánto puede crecer.

Con la puntería al azar del prototipo, el seno del primer contacto con la armadura tiene
mediana 0,53 (p10 0,13, p90 0,92): **el 47 % de los primeros contactos forman menos de
30° con el hueso.** Muchos son los brazos, que el zombie lleva adelante. Armadura
completa, modelo D, juntas por eje, en % de 2000 tajos:

| Piso del seno | Carga 0,25: rebota | Carga 0,5: rebota | Carga 0,75: rebota | Carga 1,0: rebota | Carga 1,0: atraviesa |
|---|---|---|---|---|---|
| Sin ángulo | 93,5 | 0 | 0 | 0 | 25,2 |
| 0,5 (umbral hasta 2×) | 93,5 | 59,4 | 0 | 0 | 20,4 |
| 0,3 (umbral hasta 3,3×) | 93,5 | 59,4 | 41,7 | 27,3 | 14,2 |

**Con un piso de 0,5, la carga máxima nunca rebota,** pero media carga rebota en más de la
mitad de los tajos mal orientados. Con un piso de 0,3, hasta la carga máxima rebota en uno
de cada cuatro. En la carne, el ángulo no necesita una regla propia: un corte oblicuo
cruza una sección más larga (una elipse en vez de un círculo), y el modelo por área ya lo
cobra.

### Costo del cálculo

Todo el barrido (220°) contra un zombie, modelo D, en la máquina de referencia del
ADR-0020 (Dell XPS 13 9360, i7-8550U):

| Resolución | Filtro | Tiempo por tajo y zombie | Resultado distinto del de 0,25° | Error de la detención |
|---|---|---|---|---|
| 0,25° | No | ~700 µs | 0 % | 0° |
| 0,25° | Sí | ~220 µs | 0 % | 0° |
| **1°** | **Sí** | **~58 µs** | **0 %** | **0,23° de media (máx. 0,5°)** |
| 2° | Sí | ~30 µs | 0,2 % | 0,55° de media (máx. 1,5°) |

El filtro descarta las cápsulas que no cruzan el plano, una prueba por cápsula y por
tajo. Con la cáscara del modelo E (borde por borde, mirando si cae dentro de otra
cápsula), el mismo caso de 1° sube a ~155 µs.

Son números de laboratorio, sin optimizar y fuera del protocolo del ADR-0020, con hasta un
30 % de variación entre ejecuciones. En el engine, el barrido del ADR-0016 ya dice qué
cápsulas toca la hoja en cada paso, y el tajo cruza un zombie en 2 o 3 pasos (ver abajo):
el costo se reparte y se limita a esas cápsulas. **Con 4 zombies, el modelo D a 1° usa del
orden de 0,25 ms de los 2,0 ms de "combate y corte",** que comparte con los 0,2 a 0,3 ms
de cada corte (ADR-0007).

### Vivo o muerto

Cortes completos (el plano entero, sin región barrida) sobre el maniquí, hasta 8
seguidos, cada uno sobre el pedazo que sigue vivo. Los pedazos se arman como en el
ADR-0007: cada hueso, de cada lado del plano, se une con su ancestro más cercano del mismo
lado. **La cabeza está completa** si ningún vértice del hueso `Head` queda separado del
resto. Tres reglas:

| Regla | El pedazo con la cabeza sigue vivo si… | Muere por corte | Cortes al azar hasta matarlo |
|---|---|---|---|
| **R1** (la de la visión) | la cabeza está completa y conserva al menos un brazo o una pierna (el brazo o el muslo, aunque sea un muñón) | 26 % | 3,0 |
| **R2** | la cabeza está completa y sigue unida al torso (`spine_03`) | 28 % | 3,0 |
| **R3** | la cabeza está completa y el torso entero: ningún corte en la pelvis ni en la columna | 57 % | 1,7 |

Lo que muestran:

- **Con R1 y R2, todas las muertes son por la cabeza.** Un solo plano casi nunca le quita
  al zombie las cuatro extremidades, así que la segunda condición no mata a nadie en el
  primer corte. **R1 y R2 dan casi lo mismo**, y la diferencia es de redacción.
- **En este rig, cortar el cuello es cortar la cabeza:** los vértices del hueso `Head`
  bajan hasta el cuello, así que todo plano que cruza el cuello separa alguno. Con estos
  pesos, "decapitar" y "cualquier corte en la cabeza" son la misma regla.
- **Con R1, un tajo bien cargado al torso no mata:** deja un zombie que se arrastra. Para
  matar de un golpe hay que acertar la cabeza (un 25 % de los tajos al azar), o hay que
  usar R3. Es una tensión con la visión, que dice a la vez que "si carga y se posiciona
  bien, la caballera mata casi cualquier cosa de un solo golpe" y que "si se corta por la
  cintura, la mitad de arriba se arrastra hacia la caballera".

**Los estados del zombie mutilado** (piernas y brazos útiles, contando cada extremidad
entera con su segmento de arriba y el de abajo), en % de los zombies después de cada corte,
con R1:

| Piernas, brazos | Después del 1.er corte | Del 2.º | Del 3.º |
|---|---|---|---|
| 2, 2 (entero) | 17,3 | 4,0 | 0,9 |
| 2, 1 | 3,9 | 2,0 | 1,1 |
| 1, 2 | 12,6 | 7,4 | 3,1 |
| 1, 1 | 8,9 | 7,4 | 4,7 |
| 0, 2 (se arrastra) | 21,3 | 22,9 | 18,2 |
| 0, 1 (se arrastra con un brazo) | 11,0 | 25,0 | 33,5 |
| 0, 0 (solo muñones) | 0 | 3,1 | 8,8 |
| 1, 0 o 2, 0 | 0 | 0,4 | 0,5 |

**El estado más común después de un corte es sin piernas:** en esta pose agachada, casi
todo plano por debajo de la cintura cruza las dos piernas. Arrastrarse con dos brazos o
con uno es lo que más se ve, y cojear (una pierna) es lo siguiente. Los estados con 0
brazos y alguna pierna casi no aparecen. Con espejo izquierda/derecha, los estados que
importan para moverse son cuatro: caminar, cojear, arrastrarse con dos brazos y con uno.

### El alcance del arco

La visión arma el plano del tajo con el rayo de la cámara, que pasa 0,5 m a la izquierda
de la caballera y más arriba. El ADR-0015 dejó abierto qué pasa cuando el plano queda
lejos de su pecho. Sobre 20 000 tajos al azar, la distancia del pecho al plano:

| p10 | Mediana | p90 | Máximo | Más de 0,25 m | Más de 0,35 m |
|---|---|---|---|---|---|
| 0,04 m | 0,18 m | 0,35 m | 0,53 m | 30 % | 10 % |

**El paso adelante del tajo (0,35 m, ADR-0015) alcanza para el 90 % de los tajos** si
tiene una componente lateral, hacia el plano.

La otra forma de resolverlo es **que el plano pase por el pecho de la caballera** y por el
punto apuntado, con la dirección de la línea dibujada. Así, el arco siempre nace en el
pecho, pero el corte ya no coincide exactamente con la línea de la pantalla: un punto del
plano 15 cm más cerca o más lejos que el punto apuntado se ve corrido 0,36° de mediana
(0,67° en el p90). A 640×360 y con 30° de campo vertical, son **unos 4 píxeles (8 en el
p90)**, y solo lejos del punto apuntado.

### Cuándo cruza la hoja cada zombie

Del ADR-0015: el barrido dura 0,26 s y recorre unos 220°, o sea unos 14° por paso de
simulación (60 Hz, ADR-0012). Un zombie a 1,3 m ocupa de 25° a 40° del arco: **la hoja lo
cruza en 2 o 3 pasos.** Cuatro zombies "lado a lado, a lo largo del barrido" (visión) se
cruzan en pasos distintos, así que en la práctica se emite **1 pedido de corte por paso, a
veces 2,** lejos del tope de 4 del ADR-0007. Si la pausa de impacto corta el barrido en
cada zombie, el tajo dura más: con 4 zombies y pausas de 50 a 80 ms, de 200 a 320 ms de
pausa sobre un barrido de 260 ms.

## Cómo repetirlo

1. Descargar *Universal Animation Library 2* [Standard] de Quaternius (CC0) desde
   OpenGameArt, como en el ADR-0017.
2. Con Python y numpy, leer `UAL2_Standard.glb`, evaluar el clip `Zombie_Walk_Fwd_Loop` en
   t = 0,5 s, deformar los vértices con LBS (4 huesos), fundir los dedos y las puntas en
   su hueso conservado, asignar cada vértice a su hueso dominante y ajustar las cápsulas
   (eje principal y radio en el percentil 90). Escribir los huesos, sus articulaciones,
   las cápsulas y los vértices en un archivo de texto.
3. Compilar el programa con `g++ -std=c++20 -O2` y correrlo con ese archivo. Usa una
   semilla fija (18): el resultado se repite, salvo los tiempos.
