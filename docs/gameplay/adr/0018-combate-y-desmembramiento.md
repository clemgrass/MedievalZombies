# ADR-0018: Reglas de combate y desmembramiento

- **Estado:** Aceptado
- **Fecha:** 2026-10-22
- **Ticket:** #31
- **Reemplaza / Reemplazado por:** —

## Contexto y problema

El corte de mallas (ADR-0007) sabe **cómo** partir un zombie por un plano y hasta una
recta. Este ADR decide **cuándo, cuánto y con qué consecuencias:** qué fuerza hace falta,
dónde se detiene la hoja, qué pedazo sigue vivo y qué pasa con la caballera en cada caso.
Son las reglas del pilar 1 de la visión ("el tajo es el protagonista") y del 3 ("la
horda es un rompecabezas").

Hay que decidir:

**El tajo**

1. el **modelo de fuerza:** cómo la carga se vuelve fuerza y cómo se gasta esa fuerza al
   cruzar el cuerpo;
2. la **resistencia de cada parte:** carne, cota y placa, y cómo se reconoce que el tajo
   acertó una junta;
3. qué pasa **cuando la fuerza no alcanza** para entrar (empuja o rebota) y cómo funciona
   el **tajo rápido;**
4. la **hoja clavada:** cómo se saca, qué hace el zombie mientras tanto y si se puede
   soltar la espada;
5. el **momento del golpe:** cuándo se emite el pedido de corte, cuándo empieza la pausa
   de impacto y a quién congela, y qué cuenta como corte especial;
6. el **alcance:** qué pasa cuando el plano del tajo queda lejos del pecho de la caballera
   (lo dejó abierto el ADR-0015);

**El desmembramiento**

7. **vivo o muerto:** qué pedazo sigue vivo y qué estados de zombie mutilado existen;
8. **cuántos cortes:** qué se puede volver a cortar;
9. **entero o en pedazos:** cuándo un pedazo muerto pasa a ragdoll y qué pasa entre los
   personajes y los pedazos del suelo;

**Fuera del tajo**

10. los **agarres:** cómo se detectan, cómo se sacude la caballera, qué pasa con varios
    zombies y si puede cargar mientras la agarran.

Lo que ya está decidido y limita este ADR:

- **La visión:** la precisión decide *por dónde* corta y la fuerza *hasta dónde* llega. La
  hoja puede rebotar, cortar solo un brazo, o llegar a la mitad del torso y quedar
  clavada; sacarla gasta energía. La armadura va por partes y las juntas bajan el umbral.
  Un tajo cargado parte hasta 4 zombies, lado a lado a lo largo del barrido. La energía
  es el único recurso: cargar, corregir el ángulo, trotar y soltarse de un agarre la
  gastan, y la caballera muere si llega a 0 mientras la agarran. Al mago solo lo aturde
  un tajo de máxima fuerza. La visión propone una regla de vivo o muerto ("la cabeza
  completa y al menos un brazo o una pierna") y deja las reglas exactas para este ADR.
- **ADR-0007:** el combate decide dónde se detiene la hoja **con las cápsulas, sin mirar
  la malla,** gastando la fuerza en los golpes en el orden en que la hoja los alcanza. El
  `slice_request` lleva el plano, la recta donde se detuvo la hoja y la pose del paso que
  lo emite. Un solo pedido por zombie y por tajo. Caben 4 cortes por paso y 20
  pendientes; antes de emitir, el combate pregunta si queda lugar, y si no queda, el
  zombie solo recibe el empuje. Los cortes que sobran se reparten en los pasos de la
  pausa de impacto. Una hendidura no cambia la máscara de huesos. La salida de un corte
  se puede volver a cortar.
- **ADR-0012:** paso fijo de 60 Hz, un hilo, ECS, y el orden gameplay → combate → corte →
  física → animación. Las consultas hacia abajo son llamadas directas.
- **ADR-0015:** el tajo es un arco procedural en su plano, de 0,26 s y unos 220°, con un
  paso adelante de 0,35 m. El pedazo vivo conserva el esqueleto completo y sigue animado;
  los muertos quedan rígidos o pasan a ragdoll.
- **ADR-0016:** el tajo se detecta con un barrido en su plano contra las cápsulas por
  hueso. Los personajes son controladores cinemáticos, y el empuje del tajo rápido es un
  impulso programado. Los ragdolls tienen su propio tope. Un pedazo grande sin ragdoll
  cae rígido, "como una tabla".
- **ADR-0017:** cápsulas ajustadas por el cocinador, con correcciones a mano. Los rigs
  tienen hasta 256 huesos (23 en el de Quaternius recortado).
- **ADR-0020:** "combate y corte" tienen 2,0 ms por paso (hipótesis). Una partida grabada
  reproduce los mismos resultados.

**Fuera de alcance:**

| Qué | Lo decide |
|---|---|
| Cómo se parte la malla | ADR-0007 |
| Las animaciones del tajo, de la espada clavada, del zombie mutilado y del agarre | ADR-0015 (técnica) y los tickets de contenido |
| Cómo se ve la cámara del tajo, la salpicadura y el sonido | ADR-0022, ADR-0025 y ADR-0024 |
| Cómo se mueve y decide la horda (y a quién ataca) | ADR-0023 |
| Cómo se ajustan los números sin recompilar | ADR-0021 |
| Los ataques propios de los enemigos especiales (soldado con espada, perros, mago) | Fuera del corte vertical. De ellos, solo el aturdimiento del mago es una regla de fuerza |

## Criterios

Los cuatro del ticket, más dos que salen de lo ya decidido:

1. **Diversión y claridad para el jugador:** el resultado de un tajo se puede anticipar y
   se lee en la pantalla (pilares 1 y 4 de la visión).
2. **Coherencia con la física:** las cápsulas, el barrido y los ragdolls del ADR-0016.
3. **Costo de rendimiento** dentro de los 2,0 ms de "combate y corte", con hasta 4 cortes
   por paso.
4. **Defendible como diseño:** cada regla tiene un porqué, y se puede explicar con una
   referencia o con una medición.
5. **Determinismo:** las reglas solo leen el estado del paso, sin azar ni reloj, para que
   una partida grabada dé los mismos cortes (ADR-0020).
6. **Ajustable sin recompilar:** la sensación se afina con números, no con código.

## Opciones

Las mediciones citadas salen del prototipo de la nota
[`../research/prototipo-del-combate.md`](../research/prototipo-del-combate.md). En esa
nota, `Fmax` es la fuerza de la carga máxima: atraviesa 4 campesinos de costo mediano.

### 1. El modelo de fuerza

**Qué representa la fuerza y cómo se gasta:**

| Opción | Cómo funciona | Problema |
|---|---|---|
| **A. Umbral por zombie** | Cada tipo de zombie tiene un umbral. Si la fuerza lo supera, se lo atraviesa entero; si no, rebota | Todo o nada: **la hoja nunca queda clavada,** lo que contradice la mecánica del ADR-0007 |
| **B. Costo fijo por parte** | Cada parte (cápsula) cuesta un número fijo, que se paga al entrar en ella | Rozar el borde de una parte cuesta lo mismo que cruzarla entera. Es el más inestable: un error de puntería de ±1,5 cm y ±2° cambia el resultado del 4 al 7 % de los tajos en carne y cota, contra 1 a 2 % de C |
| **C. Costo por área** | Cada parte tiene una resistencia por área. La hoja gasta la fuerza en el área que corta, en el orden en que la alcanza, y se detiene donde se acaba. Es la mecánica de fractura: cortar es abrir superficie nueva, y la energía por unidad de área es una propiedad del material | **Nunca rebota:** cualquier fuerza entra un poco. Con carga 0,15, el 77 % de los tajos contra una armadura completa quedan clavados en la placa |
| **D. Área + umbral de entrada** | Como C, más un **umbral para entrar** en cada parte, que depende de su material. La carne no tiene umbral; el metal sí | El umbral suma inestabilidad cerca de él: con armadura completa, un error chico cambia el resultado en el 10 % de los tajos (6,5 % en C) |
| **E. Carne por área + cáscara por largo** | Como D, pero la armadura es una **cáscara:** cuesta por metro de placa cortada, y por dentro todo es carne | Más fiel a cómo es una placa, pero en el prototipo da resultados parecidos a D, y cuesta ~2,5 veces más calcularla (~155 µs contra ~58 µs por tajo y zombie) |

**Lo que comparten B a E:** la fuerza que sobra después de un zombie sigue al siguiente,
en el orden en que la hoja los alcanza. Así salen solos los "4 zombies en línea" de la
visión, sin una regla aparte. **Y todos dejan la hoja clavada con poca fuerza:** con carga
0,15, un campesino la deja clavada en el 77 % de los tajos. Si el tajo rápido usa las
mismas reglas, casi siempre deja la espada clavada (ver dimensión 3).

**Lo que cobra cada modelo es lo que se ve.** En C, D y E, un tajo a la altura del cuello
del maniquí, que también cruza los dos brazos que el zombie lleva adelante, cuesta más del
triple que uno a la altura de las rodillas (0,40 contra 0,12 `Fmax`). El jugador ve qué
hay en el camino de la hoja.

**La curva de la carga** (cómo crece la fuerza mientras se mantiene el botón):

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Lineal** | La fuerza crece igual todo el tiempo, hasta `Fmax` | Ninguno en particular: es la base para ajustar |
| **Rápida al principio** | Una carga corta ya da bastante fuerza, y los últimos tramos cuestan más | Premia las cargas cortas contra campesinos. La carga máxima se siente lejana |
| **Lenta al principio** | Cargar poco casi no sirve | Obliga a comprometerse, y deja más tiempo expuesta a la caballera |

El tiempo hasta la carga máxima es un número para ajustar, no una opción.

**El cansancio** (la energía llega a 0 mientras se carga, y "el golpe pierde fuerza"):

| Opción | Cómo funciona | Problema |
|---|---|---|
| **La carga se detiene y el golpe sale reducido** | La fuerza queda donde estaba y se multiplica por un factor (por ejemplo, 0,5) | Hay que mostrar que la carga se detuvo |
| **La carga cae mientras se mantiene** | Sin energía, la fuerza baja con el tiempo | Castiga dos veces: la caballera ya quedó sin energía |

### 2. La resistencia de cada parte y las juntas

**Dónde vive el material:** el ADR-0007 fija que el combate decide con las cápsulas, sin
mirar la malla. Así que el material es **por cápsula,** es decir, por hueso: una tabla por
tipo de zombie (el campesino, todo carne; el soldado con cota, cota en el torso, los
brazos y los muslos; el de armadura completa, placa en todo). Pintar la armadura en la
malla daría más detalle (un yelmo que cubre solo media cabeza), pero obligaría al combate
a leer la malla.

**Cómo se reconoce una junta:**

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Esfera en el espacio** | Alrededor de cada articulación (cuello, axilas, codos, cintura, ingles, rodillas) hay una esfera sin placa | **Casi no cambia nada:** la junta es una parte chica de lo que cruza el plano, y las cápsulas se superponen en las articulaciones. Medido: el mismo tajo cuesta el 90 a 97 % con juntas que sin juntas |
| **Por eje** | Si el plano cruza el eje de una cápsula cerca de una articulación (a menos de 8 cm en el prototipo), toda esa parte se trata como junta: umbral bajo y sin placa | Una parte es junta o no lo es, sin término medio. Medido: con carga 0,25, uno de cada cuatro tajos que pasa por una junta entra, contra ninguno sin juntas; con la carga máxima, atraviesa el 8,7 % contra el 1,2 % |
| **Cápsulas de junta** | El cocinador agrega cápsulas chicas, sin armadura, en las articulaciones | Más cápsulas por zombie para la física (ADR-0016) y para el cocinador (ADR-0017), y se superponen con las de los huesos |
| **Por ángulo** | El ángulo entre la hoja y la placa decide si entra o resbala, como en *Mordhau* o en *Blade & Sorcery* | Un segundo eje de habilidad que el jugador no ve: "¿por qué rebotó?" es difícil de leer en la pantalla |

**Lo que mostró el prototipo:** con cualquier definición, el costo total de cruzar una
armadura completa baja poco (al 84 % en la mediana, con la junta por eje), porque las
demás partes que cruza el plano siguen blindadas. **La junta sirve de verdad para entrar
con poca fuerza y en tajos que cruzan pocas partes:** el cuello sin los hombros, el codo
sin el torso. Es lo que hace la esgrima con armadura (*Harnischfechten*): no corta la
placa, busca las aberturas.

### 3. Cuando la fuerza no alcanza, y el tajo rápido

**Un tajo cargado que no alcanza el umbral de entrada** (solo pasa con metal: la carne no
tiene umbral):

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Rebota con retroceso** | La hoja vuelve, se pierde el resto del barrido y la caballera queda expuesta un instante. Es el rebote de *Monster Hunter* contra una parte dura | Castiga fuerte. Hay que leerlo bien: chispa y sonido de metal |
| **Resbala** | La hoja sigue su arco sin cortar ese zombie, y la fuerza no se gasta | Sin castigo, golpear la armadura no cuesta nada |
| **Empuja** | El zombie recibe un impulso, como con el tajo rápido | No distingue golpear metal de golpear carne |

**El tajo rápido** (sin apuntar ni cargar; según la visión, a veces ni siquiera corta y
cumple el papel de una patada):

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Empuje puro** | Nunca corta: siempre es un impulso que abre espacio | "A veces ni siquiera corta" pasa a ser "nunca corta" |
| **Corta solo lo que atraviesa** | Con su fuerza baja y fija, corta las partes que alcanza a cruzar enteras (un brazo); si no atraviesa la primera parte, empuja. **Nunca se clava** | Una regla propia, distinta de la del tajo cargado |
| **Como un tajo cargado débil** | Las mismas reglas, con poca fuerza | Medido: deja la espada clavada en el 77 % de los tajos contra un campesino |

### 4. La hoja clavada

**Cómo se saca:**

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Tirón** | Mantener el ataque (o tirar del stick hacia atrás) la saca, gastando energía según lo hondo que quedó | Un input más que enseñar |
| **Automática** | La caballera la saca sola, con un costo fijo de energía y de tiempo | Menos control: el jugador no decide cuándo |
| **Sacarla o terminar el corte** | Tirar la saca; cargar de nuevo y soltar termina el corte desde donde quedó | El ADR-0007 no admite una región que empiece en la recta de detención: habría que extenderlo |

**Qué hace el zombie con la hoja adentro:**

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Queda ensartado** | No ataca ni agarra mientras la hoja está adentro: se sacude | Un zombie ensartado es un escudo momentáneo, lo que tal vez sea demasiado fácil |
| **Sigue actuando** | Puede agarrar a la caballera, que está quieta | Castiga mucho quedar clavada, sobre todo en la horda |

**Soltar la espada:** si no se puede, quedar clavada siempre se resuelve sacándola. Si se
puede, la caballera queda desarmada hasta recogerla: hace falta un estado sin arma, que la
visión no tiene.

### 5. El momento del golpe

**Cuándo se emite el pedido de corte** (el ADR-0007 pide la recta y la pose del mismo
paso). La hoja cruza un zombie en **2 o 3 pasos** (el barrido recorre unos 14° por paso, y
un zombie ocupa de 25° a 40° del arco):

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Al salir o detenerse la hoja** | En el paso en que la hoja termina de cruzar o se detiene. La recta y la pose son exactas | Durante esos 2 o 3 pasos, la hoja atraviesa la malla todavía entera |
| **Al primer contacto, con la detención calculada antes** | Al tocar, el combate calcula toda la travesía con las cápsulas de ese paso y emite el pedido ya | Si la herida queda abierta en cuña, la cuña se ve abierta delante de la hoja hasta que ella llega |

**Cuándo empieza la pausa de impacto y a quién afecta.** En los dos casos, los cortes que
sobran caben en los pasos de la pausa solo si la pausa empieza en el paso en que se emite
el pedido (ADR-0007):

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Una por zombie** | Cada zombie que se corta pausa el juego | Con 4 zombies y pausas de 50 a 80 ms, de 200 a 320 ms de pausa sobre un barrido de 260 ms |
| **Una por zombie, cada vez más corta** | La primera completa, las siguientes menos | Un número más que ajustar |
| **Una por tajo** | Solo en el primer zombie | Los siguientes no tienen pausa propia, y sus cortes van al paso siguiente |

Lo que se congela: **todo el juego** (la simulación sigue corriendo pasos, pero el combate,
la animación y la física de lo que se ve quedan quietos) o **solo la caballera y el zombie
golpeado,** como en *Monster Hunter*. Lo segundo mantiene viva la horda, pero necesita un
tiempo propio por entidad.

**Qué cuenta como corte especial** (activa la cámara del tajo, que la visión reserva para
estos casos, "para que no aburra"):

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Reglas fijas** | Varios zombies atravesados en un tajo, una junta acertada que corta la parte, el golpe final al mago; con un tiempo mínimo entre una y otra | Hay que elegir bien la lista |
| **Puntaje del tajo** | Cada tajo suma puntos (zombies, juntas, cabezas) y la cámara sale sobre un umbral | Más difícil de explicar y de anticipar |

### 6. El alcance

La visión arma el plano con el rayo de la cámara, que pasa 0,5 m a la izquierda de la
caballera y más arriba. Medido sobre 20 000 tajos al azar, **el pecho queda a 0,18 m del
plano en la mediana, a 0,35 m en el p90 y hasta 0,53 m.**

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Centro proyectado** | El arco se centra en el punto del plano más cercano al pecho, y la IK de los brazos lo alcanza como puede | Hasta 0,53 m de diferencia: en algunos ángulos, los brazos no llegan (ADR-0015) |
| **Paso hacia el plano** | El paso adelante del tajo (0,35 m) suma una componente lateral, hacia el plano | Cubre el 90 % de los tajos; el resto queda como en la opción anterior. La caballera se mueve de costado al golpear |
| **Plano por el pecho** | El plano pasa por el pecho y por el punto apuntado, con la dirección de la línea dibujada | El corte coincide con la línea en el punto apuntado, pero se corre unos 4 píxeles (8 en el p90) a 15 cm de profundidad de ese punto |
| **Limitar los ángulos** | No se permiten las líneas que dejan el plano lejos del pecho | Con un tope de 0,25 m, se pierde el 30 % de los ángulos: contradice el apuntado libre |

### 7. Vivo o muerto

**Qué es "la cabeza":** los huesos que el rig marca como cabeza. En el maniquí de
Quaternius, los vértices del hueso `Head` bajan hasta el cuello, así que **cortar el
cuello y cortar la cabeza son lo mismo.** La visión dice que cualquier corte en la cabeza
mata. Que la hoja quede clavada en la cabeza también mataría.

**Cuándo sigue vivo el pedazo con la cabeza** (medido con cortes al azar sobre el maniquí):

| Opción | Sigue vivo si… | Muere por corte | Cortes al azar hasta matarlo | Problema |
|---|---|---|---|---|
| **R1** (la de la visión) | la cabeza está completa y conserva al menos un brazo o una pierna | 26 % | 3,0 | Un tajo bien cargado al torso no mata: deja un zombie que se arrastra |
| **R2** | la cabeza está completa y sigue unida al torso | 28 % | 3,0 | Da casi lo mismo que R1: un plano casi nunca quita las cuatro extremidades a la vez |
| **R3** | la cabeza está completa y el torso entero (ningún corte en la pelvis ni en la columna) | 57 % | 1,7 | Contradice la visión: el zombie cortado por la cintura no se arrastra |

**La tensión de la visión:** dice que "si carga y se posiciona bien, la caballera mata casi
cualquier cosa de un solo golpe", y también que "si se corta por la cintura, la mitad de
arriba se arrastra". Con R1 o R2, matar de un golpe exige acertar la cabeza. Con R3, la
mitad de arriba no se arrastra nunca.

**Los estados del zombie mutilado:** con R1, después de un corte al azar, el estado más
común es **sin piernas** (32 %): en la pose agachada, casi todo plano por debajo de la
cintura cruza las dos. Contando una extremidad como útil si conserva sus dos segmentos, y
con espejo izquierda/derecha, salen **cuatro formas de moverse:** caminar, cojear,
arrastrarse con dos brazos y con uno. Sin brazos, el zombie no puede agarrar.

### 8. Cuántos cortes

El ADR-0007 deja que la salida de un corte se vuelva a cortar, incluida una herida abierta.
Qué permite el juego:

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Solo el zombie vivo** | El pedazo vivo y sus heridas se pueden volver a cortar. Los pedazos del suelo solo se empujan | Cortar un pedazo del suelo podría ser satisfactorio, y no se puede |
| **También los pedazos del suelo** | Todo pedazo se puede cortar | El barrido prueba también las cápsulas de hasta 100 pedazos, y cada corte suma pedazos al tope de 100 |
| **Un tope por zombie** | Hasta N cortes por zombie | Una regla arbitraria, que el jugador no ve |
| **Ninguno** | Un zombie cortado ya no se corta | Contradice al zombie que se arrastra: no se lo puede rematar |

### 9. Entero o en pedazos

**Cómo cae un pedazo muerto:**

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Todo rígido** | Cada pedazo es un cuerpo compuesto rígido (ADR-0016) | Un cuerpo sin cabeza cae como una tabla |
| **Ragdoll los grandes** | Los pedazos con pelvis o columna pasan a ragdoll, dentro de su tope; el resto, rígidos. Si el tope está lleno, rígido | Hay que elegir el criterio de "grande" |
| **Ragdoll con más de N huesos** | Un umbral por cantidad de huesos | Un brazo entero (3 huesos) se volvería ragdoll, y gasta el tope |

**Qué pasa entre los personajes y los pedazos del suelo:**

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Obstáculos** | Los controladores chocan con los pedazos | Con 100 pedazos en el suelo, la horda se traba y la navegación (ADR-0023) tiene que esquivarlos |
| **Se ignoran** | Los personajes los atraviesan | Se ve mal en primer plano |
| **Los empujan** | Los personajes empujan los pedazos sin frenarse | Pares de contacto entre 50 controladores y 100 pedazos para la física |

### 10. Los agarres

**Cómo se detecta un agarre** (el ADR-0016 dejó abierto qué otros golpes necesitan otra
detección):

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Consulta al final del ataque** | Cuando la animación de agarre del zombie llega a su punto, una consulta de superposición (una esfera frente a él) busca a la caballera | El agarre no sigue las manos exactas |
| **Contacto de las manos** | Las cápsulas de las manos del zombie tocan las de la caballera | Más pruebas por paso, y falla si las manos atraviesan la armadura entre dos pasos |
| **Distancia** | Basta estar cerca y de frente | No hay ventana para esquivar |

**Cómo se sacude la caballera:**

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Presionar repetidamente** | Cada pulsación avanza la sacudida | Las guías de accesibilidad de Xbox (XAG 107) piden evitarlo, o dar una alternativa |
| **Alternar el stick** | Mover el stick de lado a lado | Igual de exigente para las manos |
| **Mantener y empujar** | Mantener un botón y empujar el stick lejos del zombie; la sacudida avanza mientras se mantiene | Más fácil de cumplir con la accesibilidad, pero menos frenético |

**Varios zombies a la vez:** la visión fija que cada uno más hace que la energía baje más
rápido. Queda decidir si **la caballera se suelta de todos a la vez** o de **uno por vez**.

**Cargar mientras la agarran:**

| Opción | Cómo funciona | Problema |
|---|---|---|
| **No** | El agarre cancela la carga | Simple. Un agarre siempre se resuelve sacudiéndose |
| **Sí** | La carga y la sacudida compiten por la energía, y el tajo corta al que la agarra | Más profundo, pero arriesgado de equilibrar: un tajo podría resolver todos los agarres |

## Investigación

Detalle completo en
[`../research/prototipo-del-combate.md`](../research/prototipo-del-combate.md).

### Prototipo medido

Un prototipo en C++ barre la hoja por las 22 cápsulas del maniquí de Quaternius en la pose
del clip `Zombie_Walk_Fwd_Loop` y compara los cinco modelos de fuerza en tres tipos de
zombie. Además, aplica cortes al azar para probar las reglas de vivo o muerto. Las
mediciones están citadas en cada dimensión. En resumen:

- **A no deja la hoja clavada, y C no rebota.** D y E hacen las dos cosas.
- **B es el más inestable** frente a un error chico de puntería.
- **Con poca fuerza, cualquier modelo que gaste la fuerza al avanzar deja la hoja
  clavada:** el tajo rápido necesita una regla propia.
- **Las juntas sirven para entrar, no para atravesar:** definidas por eje, dejan entrar
  a uno de cada cuatro tajos débiles que de otro modo rebotarían.
- **El costo cabe:** el modelo D a 1° de resolución, con un filtro por plano, usa ~58 µs
  por tajo y zombie en todo el barrido, sin optimizar.
- **Con la regla de la visión (R1), las muertes son por la cabeza,** y el estado más común
  del zombie mutilado es sin piernas.
- **El paso adelante alcanza para el 90 % de los tajos** si se inclina hacia el plano.

### Cómo lo resuelven otros juegos

- ***Blade & Sorcery*** (WarpFrog, 2018–2024): combate en realidad virtual con física. Las
  reseñas destacan las hojas que se clavan, los golpes que pierden fuerza al ser
  desviados, el daño según la zona y que se mata con la fuerza del brazo, no con una
  secuencia de botones. Los jugadores también se quejan de que cortar un miembro es
  difícil de lograr.
- ***Mordhau*** (Triternion, 2019): según el grupo oficial del juego en Steam (2018), la
  decapitación y el desmembramiento tienen en cuenta la fuerza del arma, el ángulo con que
  se golpea la parte y la armadura que la cubre.
- ***Monster Hunter***: las partes duras hacen rebotar el arma, y la caballera de este
  juego heredaría ese castigo si elige "rebota con retroceso".
- ***Metal Gear Rising: Revengeance*** (PlatinumGames, 2013): el modo espada corta casi
  cualquier cosa, pero cuesta energía. El *zandatsu* solo se habilita al debilitar al
  enemigo y ralentiza el tiempo para apuntar.
- ***Dead Space*** (EA Redwood Shores, 2008): el desmembramiento estratégico. Cada
  enemigo es un rompecabezas de extremidades, y la cabeza no mata de un tiro. Es el
  ejemplo opuesto a R1: ahí la cabeza no importa.
- ***Dead Island 2*** (Dambuster, 2023), con su sistema FLESH: las heridas atraviesan
  capas (piel, grasa, músculo, hueso). Es la versión gráfica de lo que aquí decide la
  resistencia por material.

### La armadura de verdad

- **La placa se penetra con un umbral de energía.** Alan Williams (*The Knight and the
  Blast Furnace*, 2003) midió la energía para atravesar placas de distinto espesor y
  calidad: del orden de 100 J para una placa de hierro de ~2 mm, más con acero templado.
  Los números se citan de segunda mano y con críticas, pero la forma (un umbral que crece
  con el espesor) respalda el umbral de entrada de D y E.
- **Contra la placa no se corta: se buscan las juntas.** Los tratados de esgrima con
  armadura (*Harnischfechten*) apuntan a las aberturas: la axila, la garganta, la ingle,
  el codo por dentro, la parte de atrás de la rodilla y la ranura del visor.

### La pausa de impacto

Masahiro Sakurai (*Masahiro Sakurai on Creating Games*, 2022, el video "Stop for Big
Moments!") explica la pausa de impacto (*hit stop* o *hit lag*): congelar unos frames en el
golpe le da peso, y la usa desde los primeros *Kirby*. La visión ya fija 50 a 80 ms, más
larga cuanto más fuerte el golpe.

### Accesibilidad

Las *Xbox Accessibility Guidelines* (XAG 107, entrada) desaconsejan las mecánicas de
presionar repetidamente y las de mantener mucho tiempo, o piden una alternativa menos
exigente. La visión ya pide controles reasignables y números configurables.

## Decisión

Elijo gastar la fuerza por área, con un umbral de entrada que la piel no tiene. Es
literalmente lo que estaba pensando, y da casi lo mismo que la cáscara por un costo
menor. La carga sube rápido al principio y después cada vez más lento: se siente mejor
poder soltar un tajo corto contra los zombies que no piden tanta fuerza, y llegar a la
carga máxima cuesta. Quiero castigar al jugador: tiene que saber muy bien sus tiempos.
Si se queda sin energía cargando, sale de la postura de corte y entra en recuperación, y
si golpea metal sin fuerza suficiente, rebota y queda aturdido un momento.

El ángulo importa contra la armadura, y las juntas también. El ángulo se puede comunicar
muy bien con imagen y sonido cuando el golpe falla, y eso resuelve que sea difícil de
leer.

Cuando la hoja queda clavada en un zombie, entramos en una secuencia de *quick time*:
el jugador presiona rápido y responde a los eventos para terminar de cortar, mientras el
zombie avanza ensartado por la espada hacia la caballera. En ese momento, la caballera y
ese zombie siguen en tiempo real, pero los demás quedan muy lentos o congelados, para
que el jugador le dé todo el foco, sea emocionante y no lo agarre otro zombie, lo que
sería injusto. Si el zombie la alcanza, pasa a un agarre normal, que sí es en tiempo
real para todos: los otros zombies siguen atacando (un zombie con espada podría cortar
al que la agarra, y a ella también). Si se zafa, da un paso atrás y empuja, y la espada
sale deslizándose: el zombie queda con la herida abierta, sin partir.

Pongo el plano del tajo en el pecho, pasando por el punto apuntado: quiero que sea
preciso, que el jugador no sienta que se le regala el tajo, y que sienta que si hizo
contacto, cortó. Si el jugador se pone creativo y le corta las piernas y los brazos, me
entretiene que el zombie siga vivo, y también si le queda la cabeza con un brazo y casi
nada de torso. Los pedazos del suelo también se cortan, porque puede ser entretenido.

| # | Dimensión | Elección |
|---|---|---|
| 1 | Modelo de fuerza | **D: área + umbral de entrada.** La carne no tiene umbral; el metal sí. La fuerza que sobra sigue al zombie siguiente. **Curva de carga rápida al principio** y cada vez más lenta. **Si la energía llega a 0 cargando, se cancela la carga:** la caballera sale de la postura de corte y entra en recuperación, sin golpe |
| 2 | Resistencia y juntas | **Material por cápsula,** en una tabla por tipo de zombie. **El ángulo** entre el plano y el hueso multiplica el umbral de entrada del metal (dividido por el seno, con un piso). **Juntas por eje:** si el plano cruza el eje de una parte cerca de una articulación, esa parte es junta |
| 3 | Fuerza insuficiente y tajo rápido | **Bajo el umbral, rebota con retroceso:** un aturdimiento corto y la energía que se recupera más lento por un momento. En la carne no hay umbral: un tajo débil entra y se clava. **El tajo rápido corta solo lo que atraviesa entero;** si no atraviesa la primera parte, empuja. Nunca se clava |
| 4 | Hoja clavada | **Una secuencia de *quick time*.** Presionar rápido y responder a los eventos termina el corte, y cada pulsación gasta energía. El zombie avanza ensartado por la hoja; la caballera y él van en tiempo real, y los demás, muy lentos o congelados. **Si el zombie la alcanza o la energía llega a 0, pasa al agarre.** Al zafarse, la espada sale y el zombie queda con la herida abierta. La espada no se suelta |
| 5 | Momento del golpe | **El pedido se emite al salir o detenerse la hoja.** **Una pausa por tajo,** que empieza en ese paso, con su duración configurable (0 la apaga). **Se congelan solo la caballera y el zombie golpeado,** como en *Monster Hunter*. **Corte especial por reglas fijas:** muchos zombies en un tajo y el golpe final al mago. La cámara del tajo se puede apagar en las opciones |
| 6 | Alcance | **Plano por el pecho:** pasa por el pecho de la caballera y por el punto apuntado, con la dirección de la línea dibujada |
| 7 | Vivo o muerto | **R1 o R2:** sigue vivo el pedazo con la cabeza completa que **sigue unido al torso o conserva al menos un brazo o una pierna.** Cualquier corte en la cabeza mata |
| 8 | Cuántos cortes | **Todo se puede volver a cortar,** incluidos los pedazos del suelo, salvo que la parte separada quede más delgada que el grosor de la hoja |
| 9 | Entero o en pedazos | **Ragdoll para los pedazos con más de N huesos,** con N configurable; el resto, rígidos. **Los personajes empujan los pedazos** sin frenarse |
| 10 | Agarres | **Consulta al final del ataque del zombie.** Se sacude **presionando rápido y respondiendo a eventos;** con más zombies agarrando, cuesta más, y **al zafarse se libera de todos.** El agarre cancela la carga, pero **si estaba cargando, tiene un margen corto para soltar el tajo** |

## Consecuencias

- **El camino de un tajo,** dentro del paso de simulación (ADR-0012):

  ```
  gameplay: la carga (curva, energía) → al soltar: punto apuntado (rayo de la cámara
            contra las cápsulas) → plano por el pecho, el punto y la línea
  combate:  barrido del paso (ADR-0016) → por cada parte que la hoja alcanza:
            umbral de entrada (material, junta, ángulo) → gasto por área en subpasos
            → si la hoja sale o se detiene en un zombie: ¿queda lugar? → slice_request
            → la pausa, si es la primera del tajo
  corte:    ADR-0007
  gameplay (paso siguiente): vivo o muerto con las máscaras de huesos de los pedazos
  ```

- **El gasto por área, en el engine:** el barrido del ADR-0016 dice qué cápsulas toca la
  hoja en cada paso. El combate integra solo esas, en subpasos de ~1° (medido: el mismo
  resultado que a 0,25°, con un error de 0,23° de media en el punto donde se detiene la
  hoja). El área que la hoja corta en un subpaso es `(b² − a²)/2 · dθ` por cada tramo
  `[a, b]` de cápsula sobre la hoja, por la resistencia de su material. Donde dos
  cápsulas se superponen, cuenta la más resistente. **Presupuesto:** del orden de 60 µs
  por zombie en todo el barrido, sin optimizar (prototipo). Con 4 zombies cabe en los
  2,0 ms de "combate y corte", junto a los 0,2 a 0,3 ms de cada corte. Es una hipótesis
  hasta medirla con el protocolo del ADR-0020.
- **El umbral de entrada** de una parte, la primera vez que la hoja la toca, es el de su
  material, o el de junta si el plano cruza su eje a menos de un radio de una
  articulación, dividido por `max(|dirección del hueso · normal del plano|, piso)` si es
  metal. **Valores de partida,** en `Fmax` (la carga máxima, que atraviesa 4 campesinos
  medianos):

  | Material | Resistencia por área | Umbral de entrada |
  |---|---|---|
  | Carne | 1 | 0 |
  | Cota de malla | 2,5 | 0,10 |
  | Placa | 6 | 0,35 |
  | Junta | 1,5 | 0,03 |

  Radio de junta: 8 cm. Piso del seno: **0,5** (el umbral crece hasta 2×). Con 0,5, la
  carga máxima nunca rebota contra una armadura completa y media carga rebota en el 59 %
  de los tajos; con 0,3, la carga máxima rebota en uno de cada cuatro (prototipo). Fuerza
  del tajo rápido: 0,15. **Todos se ajustan sin recompilar** (cómo, lo decide el
  ADR-0021), y salen de un prototipo, no de jugar.
- **El plano del tajo cambia de definición:** ya no contiene el rayo de la cámara, sino
  el pecho de la caballera, el punto apuntado y la dirección de la línea dibujada (la
  perpendicular al rayo con el ángulo que eligió el jugador). El corte coincide con la
  línea en el punto apuntado y se corre unos 4 píxeles (8 en el p90) a 15 cm de
  profundidad de él. **El centro del arco es siempre el pecho,** así que se cierra el
  problema de alcance que el ADR-0015 dejó abierto. El plano se sigue congelando al
  empezar el barrido (ADR-0016).
- **El punto apuntado** es el primer impacto del rayo central de la cámara contra las
  cápsulas de los zombies y de los pedazos (una consulta del ADR-0016). Si el rayo no toca
  nada, se toma el punto del rayo a la distancia del alcance de la hoja.
- **El tiempo propio por entidad:** la pausa local y el foco de la espada clavada piden
  que la caballera, el zombie golpeado y el resto avancen a ritmos distintos. Cada
  personaje lleva una **escala de tiempo** (1 normal, 0 congelado, entre 0 y 1 lento) que
  aplican el gameplay, el combate, la IA y la animación. Como los personajes son
  controladores cinemáticos (ADR-0016), basta con escalar lo que avanzan. **Los cuerpos
  dinámicos (pedazos y ragdolls) siguen el tiempo global:** frenar algunos cuerpos dentro
  del solver lo complicaría. La simulación sigue corriendo pasos durante la pausa, así
  que los cortes que sobran se reparten en ella (ADR-0007). La escala la fijan las
  reglas del paso, así que la partida grabada se repite igual.
- **La pausa de impacto es una por tajo,** y empieza en el paso en que se emite el primer
  pedido. Con la hoja cruzando un zombie en 2 o 3 pasos y los zombies uno después de
  otro, los pedidos salen de a 1 o 2 por paso, lejos del tope de 4 del ADR-0007. Si la
  pausa vale 0, los cortes que sobran van al paso siguiente, como ya prevé el ADR-0007.
- **La espada clavada** es un estado de la caballera y del zombie:
  1. la hoja se detiene dentro de un zombie vivo: se emite el `slice_request` con la
     recta (la hendidura del ADR-0007) y empieza la secuencia;
  2. el zombie avanza por la hoja hacia la caballera, a lo largo de la recta de la hoja
     (que en el espacio del zombie queda dentro de la ranura). Los demás personajes
     pasan a una escala de tiempo baja, configurable (0 los congela);
  3. **si el jugador completa la secuencia,** se termina el corte: un `slice_request`
     **completo, con el mismo plano** en el espacio del zombie, sobre la herida abierta.
     El ADR-0007 ya admite volver a cortar una herida abierta. El plano pasa justo por el
     fondo de la ranura, un caso casi degenerado que cubren el desempate simbólico y los
     triángulos de área cero que ya tolera; **se suma como caso de test al ticket del
     corte;**
  4. **si el zombie llega a la caballera o la energía llega a 0,** pasa a un agarre.
     Entrar al agarre sin energía es morir, como cualquier agarre que vacía la energía
     (visión);
  5. **si la hoja se clava en la cabeza,** el zombie muere en el acto y la hoja queda
     libre: no hay secuencia. **En un pedazo del suelo no se clava:** corta lo que
     atraviesa y se detiene, como el tajo rápido.
- **El agarre:** cuando la animación de agarre de un zombie llega a su punto, una consulta
  de superposición (una esfera frente a él, del ADR-0016) busca a la caballera. Agarra
  solo si conserva al menos un brazo útil. Mientras dura, todo sigue en tiempo real. La
  energía baja más rápido con cada zombie que agarra, y zafarse pide más progreso con
  cada uno. **Al zafarse,** la caballera da un paso atrás y empuja a todos; si tenía la
  espada clavada, la hoja sale y la herida queda abierta. **Si la agarran cargando,** hay
  un margen corto (configurable) para soltar el tajo, que corta a quien esté en su plano;
  si no lo suelta, la carga se cancela.
- **Accesibilidad:** presionar rápido y los eventos de *quick time* son lo que la guía
  XAG 107 pide compensar. La secuencia de la espada clavada y la del agarre tienen en las
  opciones una alternativa de **mantener** con la misma duración, sin cambiar la
  mecánica.
- **Vivo o muerto** se decide en el gameplay del paso siguiente al corte, con la máscara
  de huesos que el corte escribe en cada pedazo (ADR-0007). La diferencia es de un paso
  (16,7 ms) y no se ve. Las reglas:
  - **la cabeza** son los huesos que el rig marca como cabeza (`Head` en el de
    Quaternius). Si un corte la reparte entre dos pedazos, o la hoja queda clavada en
    ella, el zombie muere. El rig marca esos huesos en el cocinador, igual que las mallas
    que se cortan (ADR-0007): cómo, lo decide su ticket;
  - **sigue vivo** el pedazo con la cabeza completa que conserva `spine_03` (el torso) o
    el segmento superior de un brazo o de una pierna. Todo lo demás muere;
  - **los estados del zombie vivo** salen de sus extremidades útiles (una extremidad es
    útil si conserva sus dos segmentos): caminar (2 piernas), cojear (1), arrastrarse
    con dos brazos o con uno (0 piernas), y **retorcerse** (ninguna extremidad útil, que
    R2 deja vivo). Sin brazos útiles, no agarra. Las animaciones de cada estado las
    hacen el ADR-0015 y los tickets de contenido;
  - **la regla vive en datos del gameplay,** no en el corte: el corte no sabe de vivos ni
    muertos (ADR-0012).
- **Los pedazos del suelo se cortan:** el barrido prueba también sus cápsulas, hasta los
  100 pedazos del ADR-0020. Cada corte suma pedazos y, como dice la visión, los más
  viejos desaparecen. **El mínimo de un pedazo** que el ADR-0007 dejó para fijar
  midiendo queda atado al grosor de la hoja: un corte que dejaría una parte más delgada
  que la hoja no se hace, y la hoja empuja ese pedazo.
- **Ragdoll con más de N huesos:** valor de partida, más de 4. Una pierna entera (4
  huesos) cae rígida; un torso pasa a ragdoll. Si el tope de ragdolls del ADR-0016 está
  lleno, el pedazo cae rígido. **Los personajes empujan los pedazos:** el contacto entre
  un controlador y un pedazo solo mueve al pedazo, y el controlador no se frena. Son
  pares de contacto nuevos para la física (hasta 50 controladores contra 100 pedazos), que
  pasan por su fase amplia.
- **Rebote:** la caballera pierde el resto del barrido y queda aturdida un instante, y la
  energía se recupera más lento durante un rato. Los dos tiempos son configurables. La
  imagen y el sonido del rebote (chispa, metal) son la forma de leer el ángulo y el
  umbral: los deciden el ADR-0025 y el ADR-0024.
- **La cámara del tajo** sale con un mínimo de zombies cortados en un tajo (configurable)
  y en el golpe final al mago, y se puede apagar. **Al mago lo aturde solo un tajo con
  carga máxima.**
- **Determinismo:** ninguna regla usa azar ni el reloj. La secuencia de *quick time* lee
  el input por paso, así que una partida grabada la repite igual (ADR-0020).
- **Tests (ADR-0011):** el gameplay lleva tests después, y la sensación, playtest. Las
  reglas son funciones puras con casos calculados a mano: el área de una cápsula
  perpendicular a la hoja, el umbral con el ángulo y con la junta, la curva de carga, la
  fuerza que sobra entre dos zombies y la regla de vivo o muerto sobre máscaras de huesos
  armadas a mano. Los tests de *replay* cubren la espada clavada y el agarre.
- **La visión cambia** (versión 3, en este mismo ticket): el cansancio cancela la carga
  en vez de debilitar el golpe; la espada clavada es una secuencia; el agarre se resuelve
  presionando rápido y con eventos, y libera de todos; la pausa congela solo a la
  caballera y al zombie golpeado; la regla de vivo o muerto es R1 o R2; los pedazos del
  suelo se cortan.
- **Lo que se sacrifica:**
  - la fidelidad de la cáscara (modelo E) a cómo es de verdad una placa;
  - la coincidencia exacta del corte con la línea dibujada lejos del punto apuntado;
  - la simplicidad de un tiempo global: hay una escala de tiempo por personaje;
  - el apuntado sin castigo: con el ángulo, golpear de costado una armadura rebota.
- **Abierto:**
  - todos los valores de partida, que se ajustan jugando: la curva y el tiempo de carga,
    los materiales, el radio de junta, el piso del ángulo, la duración de la pausa, el
    aturdimiento del rebote, la escala de tiempo de la secuencia, el ritmo del zombie
    ensartado, el margen para soltar el tajo al ser agarrado, N de los ragdolls y el
    mínimo de zombies para la cámara del tajo;
  - el grosor de la hoja, que fija el mínimo de un pedazo y probablemente el ancho de la
    cuña del ADR-0007;
  - el diseño exacto de la secuencia de *quick time* (qué teclas, cuántos eventos), con
    playtest;
  - si los ataques de los zombies con espada cortan a otros zombies con el mismo sistema
    del tajo: quedan fuera del corte vertical (visión);
  - cómo el rig marca los huesos de la cabeza (el ticket del cocinador, ADR-0017).

## Cómo lo defiendo

**¿Por qué la fuerza se gasta por área, y no con un umbral por zombie, que es más
simple?**
Para que pueda pasar que la espada quede clavada. Con un umbral, el tajo atraviesa todo o
rebota, y la hoja nunca se detiene a mitad del cuerpo.

**¿Por qué la carne no tiene umbral de entrada y el metal sí?**
Porque sería raro que la hoja rebotara en la carne. En el metal, en cambio, se espera
que un golpe sin fuerza suficiente rebote.

**¿Por qué las juntas se deciden por el eje de cada parte, y no como zonas en el
espacio?**
Para medir mejor el ángulo de entrada y que el tajo entre de verdad en el zombie. El eje
de cada parte es el mismo que uso para el ángulo, y como zonas en el espacio las juntas
casi no cambiaban nada.

**¿Por qué el plano pasa por el pecho de la caballera y no contiene el rayo de la cámara,
si así el corte se aparta un poco de la línea dibujada?**
Porque tiene más sentido con lo que el jugador siente y observa: la espada sale del
pecho, y el corte pasa por el punto que apuntó.

**¿Por qué la espada clavada pone lentos o congela a los demás zombies, y el agarre no?**
Porque sería demasiado injusto y raro que otros zombies pudieran atacar en ese momento.
El agarre, en cambio, es el peligro de la horda, y ahí todo sigue en tiempo real.

## Referencias

- Visión del juego: [`../../vision/README.md`](../../vision/README.md).
- ADR-0007, corte de mallas: [`../../slicing/adr/0007-corte-de-mallas.md`](../../slicing/adr/0007-corte-de-mallas.md).
- ADR-0015, animación: [`../../animation/adr/0015-sistema-de-animacion.md`](../../animation/adr/0015-sistema-de-animacion.md).
- ADR-0016, física: [`../../physics/adr/0016-fisica.md`](../../physics/adr/0016-fisica.md).
- Nota del prototipo: [`../research/prototipo-del-combate.md`](../research/prototipo-del-combate.md).
- Masahiro Sakurai, *Masahiro Sakurai on Creating Games*, "Stop for Big Moments!" (2022).
  Reseña: <https://automaton-media.com/en/news/20220824-15208/>
- Alan Williams, *The Knight and the Blast Furnace: A History of the Metallurgy of Armour
  in the Middle Ages & the Early Modern Period*, Brill, 2003. Cifras citadas en
  <https://myarmoury.com/talk/viewtopic.php?p=244604>
- Esgrima con armadura y medio espadón: <https://en.wikipedia.org/wiki/Half-sword>,
  <https://www.thearma.org/essays/armoredlongsword.html>
- *Blade & Sorcery*: reseña de UploadVR, <https://uploadvr.com/blade-and-sorcery-review>;
  ficha y reseña de GameGeeker, <https://gamegeeker.com/games/blade-and-sorcery?market=US>;
  queja de un jugador sobre cortar (2019),
  <https://steamcommunity.com/app/629730/discussions/1/1814296273133353033>
- *Mordhau*, cómo funciona el desmembramiento (grupo oficial en Steam, 2018):
  <https://steamcommunity.com/groups/mordhau/discussions/0/1696046342871973951>
- *Metal Gear Rising: Revengeance*: <https://en.wikipedia.org/wiki/Metal_Gear_Rising:_Revengeance>
- *Dead Space*, desmembramiento estratégico:
  <https://www.gamerevolution.com/originals/443251-dead-space-creative-director-necromorph-design>
- *Dead Island 2*, sistema FLESH: <https://en.wikipedia.org/wiki/Dead_Island_2>
- *Monster Hunter*, rebote y filo: <https://monsterhunterworld.wiki.fextralife.com/Sharpness>
- Mecánica de fractura del corte (energía por unidad de área): B. Antarvedi Goda, Z. Ma,
  S. Fregonese y M. Bacca, *Cutting Soft Matter: Scaling relations controlled by
  toughness, friction, and wear*, arXiv:2412.02021, 2024:
  <https://arxiv.org/abs/2412.02021>
- Xbox Accessibility Guidelines, XAG 107:
  <https://devdocs.xbox.com/gaming/accessibility/xbox-accessibility-guidelines/107>
