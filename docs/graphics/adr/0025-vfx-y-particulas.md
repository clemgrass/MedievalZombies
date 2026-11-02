# ADR-0025: VFX y partículas

- **Estado:** Aceptado
- **Fecha:** 2026-11-02
- **Ticket:** #39
- **Reemplaza / Reemplazado por:** —

## Contexto y problema

Hay que decidir **cómo se hacen los efectos visuales**: la salpicadura roja de cada corte,
las chispas cuando la hoja rebota contra la armadura, el polvo, las manchas que quedan en
el piso, la estela de la espada, el fuego de las antorchas y los *frames* de impacto
estilo anime. La visión pone la salpicadura en la identidad del juego: "el rojo es el
único color saturado, y la salpicadura roja del corte es parte de la identidad".

Varios ADRs ya aceptados dejaron preguntas para este:

| ADR | Qué dejó abierto |
|---|---|
| ADR-0007 | La salpicadura que disimula los pedazos chicos que se descartan |
| ADR-0008 | SIMD y datos en SoA solo en los bucles que el profiler marque. Qué sistemas pasan a SoA quedó abierto; las partículas aparecen como ejemplo en sus Opciones |
| ADR-0010 | La salpicadura que disimula el pedazo más viejo cuando el pool se llena |
| ADR-0018 | La imagen del rebote (la chispa), que le dice al jugador si el ángulo sirvió. La pausa de impacto y la escala de tiempo de cada personaje |
| ADR-0022 | Partículas, salpicadura y *frames* de impacto. El *alpha blending* queda para las partículas. El postproceso, si hay, va a 640×360 antes del blit. Abierto: **las capas por profundidad de la tapa** (piel, grasa, carne; metal y carne) |
| Visión | "Salpicadura roja en cada corte", el rojo legible en un estilo PSX oscuro, y los *frames* de impacto como posibilidad |

**El interior del corte ya está decidido** en lo principal: el ADR-0022 eligió una tapa
procedural según la distancia al hueso, con una textura de detalle en la UV plana del
ADR-0007. Este ADR suma **las capas por profundidad**, que el ADR-0022 dejó abiertas
(decisión del autor). **Los rayos X y la sacudida de cámara quedan fuera:** son de la
cámara, no de los VFX.

Hay que decidir:

1. **dónde se simulan las partículas:** en la CPU, en la GPU o mixto;
2. **la relación con la simulación y el tiempo:** si van en el paso fijo o por frame,
   cómo respetan la pausa de impacto y la cámara lenta, y de dónde sale el azar;
3. **cómo se dibujan:** la geometría, la mezcla, el orden y dónde entran en el frame;
4. **la sangre:** la salpicadura, los chorros y las gotas;
5. **las marcas** en el escenario y en los cuerpos (*decals*);
6. **los golpes que no cortan y la espada:** las chispas, el efecto por material y la
   estela;
7. **el polvo, los restos y el ambiente;**
8. **los efectos de pantalla:** los *frames* de impacto;
9. **las capas de la tapa;**
10. **los datos, la autoría, la memoria, el presupuesto, las pruebas y la depuración.**

Lo que ya está decidido y limita este ADR:

- **ADR-0005:** Vulkan 1.3. Un *ring buffer* mapeado para la geometría dinámica,
  protegido con fences. En la GPU integrada, la memoria es compartida.
- **ADR-0008:** reverse-Z con plano lejano infinito y la Y volteada. Sin `-ffast-math`
  ni `-march`.
- **ADR-0009:** sin excepciones ni RTTI. En el código de cada frame no hay contenedores
  que asignen memoria.
- **ADR-0010:** regiones por sistema, reservadas y tocadas al iniciar, con topes duros.
  Los pools que llena el juego tienen una regla de degradación y se pregunta antes.
- **ADR-0011:** TDD estricto en el núcleo; tests después en el gameplay y el renderer.
- **ADR-0012:** un hilo, paso fijo de 60 Hz, hasta 8 pasos por frame e interpolación en
  el render. Las órdenes entre sistemas van por colas.
- **ADR-0016 y ADR-0021:** la misma partida grabada da el mismo hash del estado. Nada
  que no sea la simulación escribe en ella.
- **ADR-0017:** los assets entran crudos al repo y el cocinador los convierte. Las
  cápsulas por hueso las ajusta el cocinador.
- **ADR-0018:** el material es por cápsula (carne, placa, cota). **Una pausa de impacto
  por tajo** (50 a 80 ms) que **congela solo a la caballera y al zombie golpeado**; cada
  personaje tiene su escala de tiempo. La cámara del tajo va en cámara lenta.
- **ADR-0019:** los assets de terceros solo pueden ser CC0 o CC BY.
- **ADR-0020:** 13,3 ms de GPU y 2,5 ms de render en la CPU, en la Intel UHD 620, con
  1 GiB de memoria total. Hasta 8 cortes por segundo y 4 en un mismo paso.
- **ADR-0022:** forward a 640×360, bindless (un *descriptor set* global y el índice del
  objeto en un *push constant*), *dithering* de 15 bits con una matriz de Bayer de 4×4,
  niebla, vértices que tiemblan, *screen-door* para la caballera, una lista fija de
  pasadas escrita a mano y el blit *nearest* al final. La reserva del frame de GPU es de
  3,3 ms.
- **ADR-0024:** el audio lee los eventos del paso y nunca escribe en la simulación; su
  azar no consume números de la simulación.

**Fuera de alcance:**

| Qué | Lo decide |
|---|---|
| Los rayos X y la sacudida de cámara | ADR-0031 (cámara) |
| Dónde se colocan las antorchas y las fuentes de ambiente en un nivel | ADR-0027 |
| La carga de las texturas de VFX y su recarga en caliente | ADR-0028 |
| El HUD | ADR-0026 |

## Criterios

Los cuatro del ticket, más cinco que salen de cruzar lo ya decidido:

1. **Presupuesto de la GPU integrada** (ADR-0020): lo que quede de la reserva de 3,3 ms,
   y en la CPU, la reserva de 0,5 ms por frame.
2. **Impacto visual del corte** (pilar 1 de la visión).
3. **Memoria reservada de antemano** (ADR-0010).
4. **Aprendizaje.**
5. **No romper el determinismo:** los VFX leen la simulación y nunca escriben en ella
   (ADR-0016 y ADR-0021).
6. **Dirección de arte:** PSX oscuro, con el rojo legible a 640×360.
7. **Encaja en el frame del ADR-0022:** forward, lista fija de pasadas y bindless.
8. **Valor para la industria.**
9. **Lo produce una persona:** efectos y texturas sin un editor propio, con licencias
   del ADR-0019.

## Opciones

Los tiempos son de un prototipo medido en la máquina de referencia, con la GPU fija en
700 MHz, como en el ADR-0022 (ver Investigación). La escena es la del ADR-0022 sin
pedazos: 50 zombies, 8 antorchas, niebla y PSX a 640×360. Sin efectos, el frame de GPU
mide **4,2 ms**: 2,67 la pasada de color y 1,45 el blit. Las partículas de los bancos
miden de 2 a 4 píxeles, como gotas de sangre.

### 1. Dónde se simulan las partículas

| Opción | Cómo funciona | Medido (20 000 / 100 000 partículas) | Problema |
|---|---|---|---|
| **En la CPU** | Arreglos SoA (ADR-0008) que se integran en un bucle; cada frame se escriben los registros de dibujo (32 B por partícula) en memoria que lee la GPU | CPU: **0,24 / 1,29 ms** (integrar y escribir). Sube 0,64 / 3,2 MB por frame | Con muchas partículas, la CPU paga más que la GPU, y la subida crece con la cantidad |
| **En la GPU** | Un *compute shader* integra un buffer que vive en la GPU. La CPU solo manda los emisores | GPU: **0,09 / 0,34 ms**. CPU: nada | **La CPU no se entera de lo que pasa**: si una gota toca el piso, la CPU no lo sabe sin leer de vuelta, con 2 frames de atraso. Más difícil de depurar y de probar |
| **Mixto** | Lo que dispara lógica (gotas que dejan manchas, chispas) en la CPU; lo masivo y decorativo (humo, polvo, brasas) en la GPU | Lo de cada parte | Dos sistemas que mantener |

**Dibujarlas cuesta más que simularlas, con cualquiera de las dos:** con 20 000
partículas, el dibujo mide 0,48 ms; con 100 000, 2,4 ms. La simulación en la CPU cuesta
lo mismo que el dibujo en la GPU hasta ~20 000.

**Cuántas hace falta:** la demo del prototipo (un tajo que corta, chorros desde el muñón,
un rebote con chispas, polvo y el fuego de 8 antorchas) tiene **unas 200 partículas vivas
en promedio**, con picos de ~600 en el corte. Cuatro cortes a la vez son unas 2 400. A
esa escala, las dos opciones cuestan menos de 0,1 ms.

### 2. La relación con la simulación y el tiempo

**En qué reloj corren:**

| Opción | Cómo funciona | Problema |
|---|---|---|
| **En el paso fijo, como parte del estado** | Las partículas son estado de la simulación y entran en el hash | Lo cosmético rompe el *replay* si cambia: retocar un color cambia el hash. Si son 100 000, el estado crece |
| **En el paso fijo, fuera del estado** | Se integran una vez por paso con el `dt` fijo, pero no entran en el hash ni la simulación las lee | Con 0 pasos en un frame se ven quietas; con 2 o más, se integran 2 veces. Se interpolan como el resto (ADR-0012) |
| **Por frame, con el tiempo real** | Se integran una vez por frame, con el tiempo del frame | Un tirón hace saltar las partículas; con el tiempo acotado, se ve bien. Es lo que hacen la mayoría de los engines con lo cosmético |

**Qué escribe en la simulación:** las tres opciones pueden leer los eventos del paso
(un corte, un rebote) como el audio del ADR-0024. La diferencia es si las gotas pueden
crear algo que la simulación usa después. Una mancha en el piso es solo visual; un charco
que hace resbalar sí sería simulación.

**El azar:** la simulación no puede gastar sus números (ADR-0024 tomó la misma regla).
O un generador propio de los VFX, o un *hash* del número de paso y de la entidad, como
el `pcg` del prototipo, que además repite el mismo efecto en el *replay*.

**La pausa de impacto y la cámara lenta (ADR-0018):** la pausa congela a la caballera y
al zombie golpeado; el resto sigue. Si la salpicadura sale "del zombie", espera los
66 ms de la pausa; si sale "del mundo", sale en el contacto. Los clips 1 y 5 del video los
comparan. La cámara lenta escala el tiempo de los VFX, y la estela usa el tiempo propio
de la caballera: se congela con ella (clips 6 y 9).

### 3. Cómo se dibujan

**La geometría de cada partícula** (20 000 en la CPU / 100 000 en la GPU):

| Opción | Cómo funciona | Medido | Problema |
|---|---|---|---|
| **Quad con *instancing*** | 4 vértices por instancia; los datos de la partícula como atributo por instancia | **0,48 / 2,40 ms** | Las instancias de 4 vértices aprovechan mal la GPU; aun así, es la mejor de las que se estiran |
| ***Vertex pulling*** | 6 vértices por partícula, sin *instancing*; el vertex shader lee el SSBO | 0,70 / 3,55 ms (**+45 %**) | El mismo resultado que en el ADR-0022: en esta GPU, leer vértices de un SSBO es más lento |
| ***Point sprites*** | Un vértice por partícula; el rasterizador arma un cuadrado (`gl_PointSize`) | **0,12 / 0,64 ms** (4 veces menos) | Siempre un cuadrado alineado con la pantalla: **no se puede estirar** con la velocidad. Tamaño máximo de 64 píxeles en el prototipo (la UHD 620 llega a 255) |

**La mezcla:**

| Opción | Cómo funciona | Medido | Cómo se ve |
|---|---|---|---|
| ***Screen-door*** | Opaca, con escritura de profundidad; el alfa descarta píxeles con la matriz de Bayer del ADR-0022 | 20 000 gotas: 0,49 ms. 1000 humos grandes: 1,15 ms | La trama de la caballera y del *dithering*. **No hay que ordenar** |
| ***Alpha blending*** | Mezcla con lo que ya está | 0,48 ms / 1,0 ms | Suave. **Hay que ordenar de atrás hacia adelante** |
| **Aditiva** | Suma luz | 0,48 ms | **Brilla:** sirve para chispas y fuego, no para sangre (clip 3) |

**Ordenar** (solo con *alpha blending*): con 20 000 partículas, `std::sort` mide 1,1 ms
de CPU y un *radix sort* de 4 pasadas, 0,33 ms; con 50 000, 2,95 y 0,86 ms. Con las ~600
del pico de la demo, cualquiera es despreciable.

**El *overdraw*:** a 640×360 las gotas son baratas, pero una partícula grande cubre
cientos de píxeles. 1000 humos de ~0,8 m miden 1,0 ms; 200, 0,2 ms. Lo que cuesta es
cuántos píxeles tapa, no cuántas partículas hay.

**Dónde entran en el frame:** dentro de la pasada de color, después de lo opaco, o en una
pasada aparte que lea la profundidad (para los *decals* proyectados o el choque de la
GPU). **Partir la pasada de color no costó nada medible** (4,16 contra 4,23 ms).

### 4. La sangre

| Pieza | Qué es | En el prototipo |
|---|---|---|
| **Salpicadura** | Un estallido en el contacto, a lo largo de la hoja y hacia afuera del plano del corte | 220 + 60 gotas de 1 a 4 cm, estiradas con la velocidad, de 0,8 a 2 s |
| **Chorros** | Pulsos desde el muñón, como un corazón que todavía late | 3 pulsos de 40 gotas, a 0,45, 0,85 y 1,25 s |
| **Gotas que dejan marca** | Al tocar el piso, la gota muere y a veces deja una mancha | Una de cada 6 deja una mancha de 10 a 32 cm, más un charco bajo el zombie |

**Contra qué choca una gota:**

| Opción | Cómo funciona | Medido | Problema |
|---|---|---|---|
| **Un plano o la altura del piso** | Analítico: `y < 0` | Gratis | Solo el piso |
| **La física** (ADR-0016) | Un rayo por gota | No medido | Gasta consultas de la física por algo cosmético |
| **El buffer de profundidad** (GPU) | Se proyecta la partícula, se lee la profundidad y se compara (Tchou, *Halo: Reach*, GDC 2011) | +0,01 ms con 20 000; +0,07 ms con 100 000 | Solo choca con lo que se ve: fuera de la pantalla, atraviesa todo. Necesita la profundidad de este frame |

**El color:** la sangre se ilumina con las antorchas (sin normal: solo la atenuación), y
la niebla la apaga. Sin luz, el rojo se vería plano y fuera de la escena; con demasiada,
se lava. El rojo saturado sobrevive al *dithering* de 15 bits.

### 5. Las marcas en el escenario

| Opción | Cómo funciona | GPU (100 / 500 / 2000 marcas) | Problema |
|---|---|---|---|
| ***Decal* proyectado** (Kim, *Space Marine*, SIGGRAPH 2012) | Una caja por marca. Cada píxel de la caja reconstruye su posición desde la profundidad y la lleva al espacio de la caja | **0,25 / 1,16 / 4,7 ms** | Cuesta por píxel de la caja. **Mancha todo lo que está adentro: los pies de los zombies** (clip 10). Evitarlo pide una máscara (*stencil*) |
| ***Decal* de malla** (Lengyel, *Game Programming Gems 2*, 2001) | Al crear la marca, la CPU recorta los triángulos del escenario contra la caja (Sutherland–Hodgman) y guarda una malla chica | **0,06 / 0,23 / 0,95 ms** | **2 µs de CPU por marca** (p99 de 7 a 19 µs). Una malla por marca: el prototipo reservó 96 vértices y usó 30 como máximo |
| **Pintura en una máscara** | Una textura que cubre el piso (512², 7,8 cm por texel); la marca se pinta en ella y el shader del piso la lee | **0 ms medible** | Solo en superficies con una UV única (el piso). Se ve pixelada (clip 12). No se recicla: el piso se llena. 16 µs por marca en la CPU y 256 KB de memoria |

**Con la sangre en la GPU, las marcas también tienen que ser de la GPU:** la CPU no se
entera de dónde cae cada gota. La pintura en la máscara encaja, porque un *compute
shader* puede escribirla.

**En los cuerpos y en la hoja:** un valor de "sangre" por objeto que el shader mezcla
con un ruido. No se midió: es una lectura de textura más.

### 6. Los golpes que no cortan y la espada

**Las chispas del rebote (ADR-0018):** aditivas, estiradas con la velocidad, de 0,15 a
0,35 s, amarillas. Son la imagen que dice "el ángulo no sirvió" (clip 1, a los 2 s).

**El efecto por material:** el ADR-0018 guarda el material por cápsula (carne, placa,
cota). Una tabla `material → efecto` decide qué sale en cada contacto: sangre en la
carne; chispas en la placa; chispas y algo de sangre en la cota.

**La estela de la espada:**

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Ninguna** | — | El arco de 0,26 s del ADR-0015 pasa en 16 frames: el ojo no lo sigue |
| **Una tira de triángulos** | Cada paso se guarda la empuñadura y la punta; la tira se suaviza con Catmull-Rom y se desvanece con la edad | ~100 vértices por frame. Con *screen-door*, la trama se ve en la estela (clip 1) |
| **Un *sprite* de arco pintado** | Una textura de media luna orientada con el plano del tajo | No sigue el arco real si la hoja se detiene en una armadura o queda clavada |

### 7. El polvo, los restos y el ambiente

- **El polvo** cuando cae un pedazo: ~25 partículas grandes y suaves, con *alpha
  blending*, que suben lento (clip 1, a los 2,4 s).
- **Los pedazos descartados** (ADR-0007 y ADR-0010): se tapan con una salpicadura en su
  lugar. Es el mismo efecto de la sangre, con otro emisor.
- **El fuego de las antorchas:** partículas aditivas que suben. Con 8 antorchas son
  ~40 partículas vivas.
- **El humo grande** es lo único que puede costar de verdad, por el *overdraw* de la
  dimensión 3.

### 8. Los *frames* de impacto

Dos o tres frames de imagen invertida o de alto contraste justo en el contacto (visión).
Van en el postproceso a 640×360, antes del blit (ADR-0022).

| Opción | Medido | Cómo se ve |
|---|---|---|
| **Invertir** | 0,10 ms | Un destello. Lee bien, pero no distingue nada (clip 7) |
| **Siluetas en blanco, negro y rojo** | 0,14 ms | Los personajes cercanos en negro, el fondo en blanco y la sangre en rojo (clip 8). Es lo que describe la visión |

**Cómo sabe el postproceso qué es un personaje:** el prototipo usa **el canal alfa del
color**, que no se usaba. El shader de los personajes escribe 0, el resto 1. No cuesta
nada y no necesita otra imagen.

**Cuándo salen:** en cada corte, solo en los cargados o solo en la cámara del tajo. La
visión lo dejó abierto. En los clips 7 y 8 salen en cada corte, y en el 9, en cámara
lenta.

### 9. Las capas de la tapa

La visión pide **capas por profundidad**: piel y grasa en un campesino, y metal,
gambesón y piel en un zombie con armadura. El ADR-0022 dejó la pregunta abierta: hace
falta saber a qué distancia de la piel está cada punto de la tapa.

| Opción | Cómo funciona | GPU en primer plano / a la distancia de juego | Problema |
|---|---|---|---|
| **Desde la cápsula** | Profundidad = radio de la cápsula − distancia a su eje. Usa las cápsulas que la tapa ya lee (ADR-0022) | +0,36 ms / — | **No sigue el borde** (clip 18): la cápsula no es la malla |
| **Desde el contorno, por píxel** | Cada píxel de la tapa mide su distancia a las aristas del borde, que el corte ya tiene | **+1,15 ms** / +0,34 ms (203 aristas) | Exacto (clips 19 y 21), pero cuesta por píxel y por arista: caro en la cámara del tajo |
| **Un campo de distancia** | Al cortar, la CPU llena una textura chica con la distancia al borde en la base `(u, v)` de la tapa | **+0 ms** / +0 ms | Exacto con filtrado lineal (clip 20). Por fuerza bruta mide **3,6 ms de CPU con 64²** (1,0 con 32², 14,5 con 128²): no cabe en los 2 ms del corte. Con una transformada de distancia lineal (Felzenszwalb y Huttenlocher) serían ~0,05 ms (no medido) |
| **Puntos interiores** | La triangulación del ADR-0007 suma vértices dentro de la tapa con su distancia al borde | No medido | Cambia el corte, que hoy solo pone vértices en el borde |

**A la distancia de juego, las capas no se ven,** ni con el triple de grosor (clips 22 y
23): la tapa mide ~25 píxeles. **Solo se leen en la cámara del tajo,** que es cuando el
ADR-0022 muestra la tapa de cerca.

### 10. Los datos, la memoria, el presupuesto y las pruebas

**Cómo se describe un efecto:**

| Opción | Cómo es | Problema |
|---|---|---|
| **En código** | Una función por efecto que llama a los emisores | Cambiar un color recompila |
| **Valores en CVars** (ADR-0021) | El código del efecto, con sus números en CVars: se ajustan en vivo | Sin una vista de todo el efecto junto |
| **Un archivo de datos** cocinado (ADR-0017) | Cada efecto es una lista de emisores con sus curvas, como un *Niagara* mínimo | Un formato y un cargador más; la recarga en caliente es del ADR-0028 |

**La memoria (ADR-0010):** un pool fijo por tipo, reservado al iniciar. El prototipo usa
32 B por partícula: 8192 partículas son 256 KB, más el buffer de dibujo por frame en
vuelo (2 × 256 KB). Las manchas de malla, 96 vértices de 24 B: 256 manchas son 576 KB.
**Cuando el pool se llena:** la partícula nueva no nace (lo más barato), o reemplaza a la
más vieja (se ve mejor en un corte nuevo).

**Las pruebas (ADR-0011):** el renderer va con tests después. La integración y el recorte
de las manchas son funciones puras: un triángulo recortado contra una caja da un
polígono conocido. El aspecto se prueba mirando, y con imágenes de referencia cuando
existan.

**La depuración (ADR-0021):** contadores de partículas vivas, descartadas y manchas en
ImGui; cada pasada con sus *timestamps*.

## Investigación

### Prototipo medido

Un programa en C++20 con Vulkan 1.3, sobre el prototipo del ADR-0022, medido en la
máquina de referencia el 2026-11-02, con la GPU fija en 700 MHz. Dos vueltas de 600
frames por configuración. El detalle y cómo repetirlo están en
[`../research/prototipo-de-vfx.md`](../research/prototipo-de-vfx.md). Un video de 107 s
con 23 variantes acompaña las mediciones.

| # | Qué | Resultado |
|---|---|---|
| P1 | CPU o GPU | 20 000: CPU 0,24 ms, GPU 0,09 ms. 100 000: CPU 1,29 ms, GPU 0,34 ms. El dibujo, igual en las dos: 0,48 y 2,4 ms |
| P2 | Dibujo y mezcla | *Instancing* 0,48; *pulling* 0,70; *point sprites* 0,12 ms (20 000). Las tres mezclas, iguales con gotas chicas. Ordenar 20 000: `std::sort` 1,1, *radix* 0,33 ms. 1000 humos grandes: 1,0 ms |
| P3 | Choque | Contra la profundidad, +0,01 a +0,07 ms. Partir la pasada de color: no medible |
| P4 | Marcas | Proyectadas 0,25 / 1,16 / 4,7 ms; de malla 0,06 / 0,23 / 0,95 ms (100 / 500 / 2000), 2 µs de CPU cada una; pintura, 0 ms medible |
| P5 | La demo de la sangre | ~200 partículas vivas, picos de ~600. Partículas 0,03 ms, manchas 0,2 ms, CPU 0,005 ms |
| P6 | Chispas y estela | Incluidas en P5: no se distinguen del ruido |
| P7 | *Frames* de impacto | Invertir 0,10; siluetas 0,14 ms, solo en los frames en que salen |
| P8 | Capas de la tapa | Primer plano: cápsula +0,36, contorno +1,15, campo de distancia +0 ms en la GPU (3,6 ms de CPU por fuerza bruta con 64²). A la distancia de juego no se ven |

Lo que mostró, además de los números:

- **En la escala de este juego, el costo de los VFX está en los píxeles, no en las
  partículas.** Las gotas son baratas con cualquier técnica; lo caro son el humo grande,
  las cajas de los *decals* proyectados y las capas por píxel en un primer plano.
- **Las partículas en la GPU no le cuentan nada a la CPU.** Si las gotas tienen que
  dejar manchas, las manchas también tienen que hacerse en la GPU.
- **Los *decals* proyectados manchan a los personajes** que pisan la caja.
- **El canal alfa del color sirve de máscara de personajes** para el postproceso.

**Lo que toca a otros ADRs:**

- **ADR-0022:** el canal alfa del color pasa a tener un uso (la máscara de personajes),
  si se eligen las siluetas. La pasada de color se puede partir sin costo medible.
- **ADR-0007:** el campo de distancia o los puntos interiores necesitan el contorno de la
  tapa en la base `(u, v)`, que el corte ya calcula para triangular.
- **ADR-0018:** la tabla `material → efecto` vive junto a la tabla de materiales por
  cápsula.

### Cómo lo hacen otros

- **Los sistemas de partículas** nacen con Reeves (*Star Trek II*, SIGGRAPH 1983):
  emisores, partículas con vida, y atributos con azar alrededor de una media.
- **Partículas en la GPU con *compute*:** Gareth Thomas (AMD, GDC 2014) emite, simula,
  ordena y dibuja todo en la GPU. *inFAMOUS Second Son* (Sucker Punch, GDC 2014) llevó
  casi todos sus efectos a la GPU.
- ***Niagara*** (Unreal Engine) deja elegir por emisor si se simula en la CPU o en la
  GPU: lo que necesita eventos o colisiones con la física, en la CPU; lo masivo, en la
  GPU. El *VFX Graph* de Unity es solo de GPU y choca contra el buffer de profundidad.
- **El choque contra la profundidad:** Chris Tchou, *Halo: Reach Effects Tech* (GDC
  2011).
- **Los *decals*:** Eric Lengyel recorta la malla (*Game Programming Gems 2*, 2001); Pope
  Kim los proyecta desde la profundidad en *Warhammer 40,000: Space Marine* (SIGGRAPH
  2012).
- **Los campos de distancia:** Felzenszwalb y Huttenlocher (2012), en tiempo lineal en la
  CPU; el *jump flooding* de Rong y Tan (I3D 2006), en la GPU.
- **El recorte de polígonos:** Sutherland y Hodgman (1974).

## Decisión

Elijo unos VFX simples, simulados en la CPU. A la escala de este juego (cientos de
partículas, no cientos de miles), lo que cuesta son los píxeles y no la simulación, y en
la CPU sé dónde cae cada gota para dejar su mancha. Las partículas corren en el paso fijo
sin entrar en el hash: respetan la pausa de impacto y la cámara lenta, y el *replay* las
repite iguales. La sangre usa la misma trama que la caballera, así que no hay que
ordenarla y encaja con el estilo PSX. Para las marcas elijo recortar la malla del
escenario: no manchan a los zombies y repiten el recorte de polígonos que ya hace el
corte. Las capas de la tapa salen de un campo de distancia por corte, que no le cuesta
nada a la GPU.

| # | Dimensión | Elección |
|---|---|---|
| 1 | Simulación | **En la CPU**, en arreglos SoA (diseño orientado a datos, ADR-0014 regla 10; fe de erratas del ADR-0008: el SIMD sigue esperando al profiler), con **pools fijos** reservados al iniciar. Un solo sistema. Las partículas en la GPU quedan abiertas para efectos masivos de ambiente, si una medición los pide |
| 2 | Tiempo | **Se integran en el paso fijo, con su `dt`, fuera del estado y del hash.** Leen los eventos del paso (el corte, el rebote, la caída de un pedazo), como el audio del ADR-0024, y **nunca escriben en la simulación**. **Se dibujan en `pos − vel · (1 − α) · dt`:** la posición del paso anterior, reconstruida a velocidad constante, interpolada con el α del ADR-0012, sin guardarla. Así van un paso atrás, como todo lo que dibuja el render, y la sangre no se despega del muñón. La estela termina en la pose interpolada de la espada. **El azar es un `pcg` del número de paso, la entidad y el índice de la partícula:** no gasta números de la simulación, y el *replay* repite los efectos. **Cada efecto usa la escala de tiempo de su dueño:** la estela, la de la caballera; la salpicadura, la del mundo, así que **sale en el contacto**, sin esperar la pausa de impacto. La cámara lenta escala todos |
| 3 | Dibujo | **Quads con *instancing*** (4 vértices por instancia, 32 B por partícula), estirados con la velocidad cuando la partícula lo pide e iluminados en el vértice con la atenuación de las antorchas, con niebla y *dithering*. **La mezcla va por tipo:** la sangre con ***screen-door*** (la matriz de Bayer del ADR-0022, escribe profundidad, sin ordenar); el polvo, el humo y la estela con ***alpha* premultiplicado**, ordenados de atrás hacia adelante con un ***radix sort*** en la CPU; las chispas y el fuego, **aditivos**. **Todo va dentro de la pasada de color, después de lo opaco** |
| 4 | Sangre | **Salpicadura** en el contacto (a lo largo de la hoja y hacia afuera del plano del corte), **chorros** desde el muñón en pulsos, y **gotas** que al tocar el suelo mueren y a veces dejan una mancha. **Un plano por estallido:** al emitir, un rayo hacia abajo desde el punto del corte (la consulta del ADR-0016), **que solo toca lo estático**, da el plano del suelo, contra el que chocan todas las gotas de ese estallido. Si tocara cuerpos, la sangre podría quedar a la altura de una rodilla. Rojo saturado, iluminado por las antorchas |
| 5 | Marcas | ***Decals* de malla** (Lengyel): al crear la marca, la CPU recorta con Sutherland–Hodgman los triángulos estáticos que miran hacia la caja de la marca, y guarda una malla chica en un **pool anillo** (la más vieja se reemplaza). Se dibujan todas en una llamada, con *depth bias* y *alpha* |
| 6 | Golpes y espada | **Una tabla `material → efecto`** junto a la de materiales por cápsula del ADR-0018: carne → salpicadura; placa → chispas; cota → chispas y poca sangre. **Chispas** aditivas y estiradas, de 0,15 a 0,35 s. **Estela:** una tira de triángulos con la empuñadura y la punta de cada paso, suavizada con Catmull-Rom y desvanecida con la edad, **en el tiempo propio de la caballera** (se congela con la pausa), con *alpha* |
| 7 | Ambiente | Polvo al caer un pedazo (*alpha*, suave), la salpicadura que tapa los pedazos descartados (ADR-0007 y ADR-0010) y el fuego de las antorchas (aditivo). **Las partículas grandes (polvo y humo) tienen su propio pool con un tope**, porque su *overdraw* es lo único que puede comerse la reserva |
| 8 | *Frames* de impacto | **Siluetas en blanco, negro y rojo**, de 2 a 3 frames en el contacto, en el postproceso a 640×360 antes del blit. **El canal alfa del color es la máscara de personajes** (0 en los personajes, 1 en el resto). **Salen solo en los cortes cargados que cortan y en la cámara del tajo,** y **se pueden apagar** en las opciones: son destellos de pantalla completa |
| 9 | Capas de la tapa | **Un campo de distancia de 64² por corte** (`R8`, de 0 a 10 cm, en la base `(u, v)` del plano), que el corte llena con una **transformada lineal que propaga la arista más cercana** (una *feature transform*, en la línea de Felzenszwalb y Huttenlocher) y mide la distancia exacta a ese segmento: medir hasta los centros de los texeles da un error de ~5 mm, más que la capa de piel. Las dos tapas de un corte comparten el suyo. La tapa lo lee con filtrado lineal. **Las capas salen del material de la cápsula** (ADR-0018): carne → piel y grasa; placa y cota → metal, gambesón y piel. Los grosores son valores de partida con una CVar que los escala |
| 10 | Datos y presupuesto | **Efectos en código, con sus valores en CVars** (ADR-0021); en Release son constantes. **Pool lleno:** la partícula nueva no nace, y un contador lo muestra; las marcas reemplazan a la más vieja. **Metas (hipótesis):** 0,5 ms de GPU y 0,2 ms de CPU por frame para los VFX, sacados de las reservas. **Tests después** (ADR-0011), con casos calculados a mano para el recorte de las marcas y la integración. **La transformada de distancia la llena el corte, así que lleva el TDD estricto del corte** (ADR-0011), con un caso que la compara con la fuerza bruta, con una tolerancia en milímetros |

## Consecuencias

- **La pasada de color crece por dentro,** sin pasadas nuevas. El frame del ADR-0022
  queda así:

  ```
  640×360:  color y profundidad:
              opacos → screen-door (caballera, sangre) → marcas
              → alpha ordenado (polvo, humo, estela) → aditivo (chispas, fuego)
            → dibujo de depuración
            → postproceso (los frames de impacto, solo cuando salen)
  nativa:   blit nearest al swapchain → ImGui
  ```

- **En el paso**, los VFX se integran **al final**, después de la animación: leen los
  eventos del paso y las poses (los emisores siguen a los huesos). No forman parte del
  estado: el hash del *replay* no los incluye, y quitarlos no cambia la simulación.
- **Los registros de dibujo van por el *ring buffer*** del ADR-0005: con el pool lleno
  son 256 KB por frame, dentro de los 8 MiB de la subida.
- **Memoria (ADR-0010),** con valores de partida. Una región de VFX en la CPU, reservada
  al iniciar, y dos bloques de `gpu::Allocator`:

  | Pool | Dónde | Capacidad | Memoria |
  |---|---|---|---|
  | Partículas chicas (sangre, chispas, fuego) | CPU, región de VFX | 8192 | ~320 KB en SoA |
  | Partículas grandes (polvo, humo) | CPU, región de VFX | 256 | ~10 KB |
  | Marcas de malla, 64 vértices de 24 B cada una | GPU, buffer persistente | 256 | 384 KB |
  | Campos de distancia de las tapas, un arreglo de texturas de 64² | GPU | 128 cortes | 512 KB |

  El prototipo usó 30 vértices por marca como máximo. Si un recorte pasa de 64, se
  descartan los triángulos que sobran y un contador lo muestra.
- **Las marcas viven en un buffer persistente de la GPU,** no en el *ring buffer*: la CPU
  escribe la malla recortada una vez, al crear la marca. Reemplazar la más vieja mientras
  un frame en vuelo la lee sería una carrera, así que un lugar reemplazado pasa por la
  cola de liberación por número de frame (ADR-0010), y el anillo tiene margen para los
  lugares que esperan.
- **Los campos de distancia son del corte.** Un campo se libera cuando se descarta la
  última tapa que lo usa (también en un zombie vivo herido), por la misma cola por número
  de frame. **Con el pool lleno, el corte nuevo reusa el campo más viejo,** y la tapa
  que lo usaba se queda sin capas: de lejos no se ven (P8).
- **Presupuestos (hipótesis):** una fila de GPU, "VFX: partículas, marcas y estela",
  con 0,5 ms (la reserva baja de 3,3 a 2,8 ms), y una fila de CPU, "VFX: integrar, crear
  marcas, ordenar y escribir", con 0,2 ms (la reserva baja de 0,5 a 0,3 ms). La
  integración y el recorte de las marcas (2 µs cada una, cuando una gota toca el suelo)
  corren en cada paso, así que un frame con dos pasos los paga dos veces: el frame con
  dos pasos pasa de 15,8 a 16,0 ms y sigue cabiendo en 16,7. Los *frames* de impacto
  (0,14 ms) entran en la fila de postproceso. **Solo el campo de distancia** va en el
  paso del corte, dentro de los 2,0 ms de "combate y corte".
- **Lo que se gana:** un sistema chico, que se prueba y se depura en la CPU, que gasta
  menos de 0,1 ms en una pelea normal, y que el *replay* repite igual. La sangre se lee
  y deja marcas sin manchar a los zombies.
- **Lo que se sacrifica:**
  - los efectos masivos (decenas de miles de partículas), que en la CPU costarían
    milisegundos;
  - las marcas en superficies que se mueven: el recorte usa solo los triángulos
    estáticos;
  - la suavidad del humo: el tope de partículas grandes limita las nubes;
  - los *frames* de impacto en los tajos rápidos.
- **Features que el engine verifica al iniciar,** junto a las del ADR-0022:
  `shaderDemoteToHelperInvocation` (Vulkan 1.3). Con `--target-env=vulkan1.3`, glslc
  compila `discard` con esa capacidad, y el *screen-door* la usa. La UHD 620 la tiene.
- **Sin texturas nuevas:** las formas (el disco y la mancha con gotas satélite) son
  procedurales.
- **Tickets, en orden** (cada uno crece desde el anterior):
  1. el pool de partículas en SoA, la integración al final del paso y el `pcg`, con
     tests de la integración;
  2. el dibujo con *instancing* y las tres mezclas dentro de la pasada de color, con el
     *radix sort*;
  3. la salpicadura, los chorros y el plano por estallido (junto al ADR-0007 y al
     ADR-0018);
  4. las marcas de malla con su pool anillo, con tests del recorte;
  5. la tabla `material → efecto` y las chispas del rebote;
  6. la estela de la espada;
  7. el polvo, el fuego de las antorchas y el pool de las partículas grandes;
  8. los *frames* de impacto: la máscara en el alfa y la pasada de postproceso;
  9. el campo de distancia y las capas de la tapa (junto al ADR-0007 y al ADR-0022),
     con la transformada en TDD estricto, como el resto del corte;
  10. los contadores en ImGui y las CVars de los efectos.
- **Abierto:**
  - las partículas en la GPU, si un efecto masivo de ambiente las pide;
  - un formato de datos para los efectos, si dejan de caber en el código;
  - la sangre sobre los cuerpos y sobre la hoja (un valor por objeto que el shader mezcla
    con un ruido);
  - las marcas en las paredes (la caja orientada con la normal del impacto);
  - cómo se ven los *frames* de impacto dentro de la cámara del tajo (ADR-0031);
  - dónde viven los VFX en las capas del ADR-0012: consultan la física (el rayo), leen
    las poses y los eventos del combate, y la tabla `material → efecto` es del juego.
    Lo decide el ticket 1; por ejemplo, la simulación y el dibujo en `engine/vfx`,
    encima de la física y la animación, y la tabla y la elección del efecto en `game/`;
  - el detalle del margen del anillo de las marcas para los lugares que esperan la cola
    de liberación (ticket 4);
  - los filtros de las consultas de la física (solo lo estático, sin personajes), que
    decide el ticket del rayo; la línea de vista de la IA (ADR-0023) también los usa;
  - todos los valores de partida, que se ajustan mirando el juego: las cantidades y
    tamaños de cada efecto, la fracción de gotas que dejan mancha, los grosores de las
    capas, el tope de partículas grandes y la duración de los *frames* de impacto.

**Fe de erratas en otros ADRs.** Al aceptar este ADR se corrigieron, como fe de erratas
(ADR-0014, regla 11):

| ADR | Qué se corrige |
|---|---|
| ADR-0007 | El corte llena el campo de distancia de la tapa con su contorno en la base `(u, v)`, propagando la arista más cercana, y su costo cuenta en el paso del corte. El campo es del corte: se libera con la última tapa que lo usa, y con el pool lleno se reusa el más viejo |
| ADR-0008 | El SoA de un pool es diseño orientado a datos (ADR-0014, regla 10), no una optimización: se decide al diseñar el pool. El SIMD sigue esperando al profiler. Se cierra en Abierto "qué sistemas pasan a SoA" para las partículas |
| ADR-0010 | La región de VFX en el mapa de la memoria, con sus pools y su degradación: la partícula nueva no nace; las marcas reemplazan a la más vieja; el campo de distancia más viejo se reusa |
| ADR-0012 | Los VFX se integran al final del paso, fuera del estado y del hash |
| ADR-0018 | La tabla `material → efecto` vive junto a la de materiales por cápsula |
| ADR-0022 | El canal alfa del color es la máscara de personajes, y el swapchain usa `VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR`. La pasada de color incluye los VFX, que son la excepción a tres decisiones: usan *alpha* (partículas, marcas y estela), *instancing* y luz por vértice. La feature `shaderDemoteToHelperInvocation`. Se cierran en Abierto las capas de la tapa y los *frames* de impacto |
| Visión | Se cierran en Abierto cuándo se usan los *frames* de impacto y las capas de la tapa |
| `docs/performance/presupuestos.md` | Las filas de VFX en la GPU y en la CPU, el frame con dos pasos (16,0 ms) y dónde cae la memoria de los VFX |

## Cómo lo defiendo

**¿Por qué simulas las partículas en la CPU, si la industria las lleva a la GPU?**
Porque mi escala es otra. La demo de un corte completo tiene unas 200 partículas vivas,
con picos de 600, y en la CPU cuesta menos de 0,01 ms. La GPU gana con decenas de miles.
Además, en la CPU sé dónde cae cada gota: así deja su mancha. Con las partículas en la
GPU, la CPU no se entera sin leer de vuelta, con 2 frames de atraso. Lo que más cuesta es
dibujarlas, y eso es igual en las dos.

**Si corren en el paso fijo, ¿por qué no son parte del estado de la simulación?**
Porque son cosméticas. Si entraran en el hash del *replay*, cambiar el color de la sangre
rompería las partidas grabadas. Las integro al final de cada paso, después de la
animación, para leer los eventos y las poses de ese paso, pero nunca escriben en la
simulación. Si las borro, el juego da el mismo resultado.

**¿Cómo repite el *replay* la misma sangre sin usar el azar de la simulación?**
Cada número al azar sale de un *hash* `pcg` del número de paso, de la entidad y del
índice de la partícula. No gasta números del generador de la simulación, así que no lo
desordena, y la misma partida da la misma salpicadura. Es la misma regla que tomó el
audio.

**¿Qué pasa con la sangre durante la pausa de impacto y la cámara lenta?**
Cada efecto usa el tiempo de su dueño. La estela es de la caballera, así que se congela
con ella. La salpicadura es del mundo, así que sale en el contacto. Probé las dos
opciones en video, y con la sangre volando mientras los cuerpos están congelados, la
pausa se siente más fuerte. La cámara lenta escala todos los efectos.

**¿Por qué la sangre usa *screen-door* y el polvo usa *alpha blending*?**
El *screen-door* es opaco: descarta píxeles con la misma matriz de Bayer del *dithering*
y de la caballera. No hay que ordenar, y la trama encaja con el estilo PSX. El polvo y el
humo tienen que ser suaves, así que usan *alpha*, ordenados con un *radix sort* que con
estas cantidades no cuesta nada. Las chispas y el fuego son aditivos porque brillan. La
sangre aditiva brillaba como fuego.

**¿Por qué *quads* con *instancing*, si los *point sprites* dibujaron 4 veces más rápido?**
Porque un *point sprite* siempre es un cuadrado que mira a la pantalla y no se puede
estirar con la velocidad. La sangre estirada es la que se lee como salpicadura. Con
20 000 partículas, el *instancing* midió 0,48 ms, y yo tengo cientos. Dejé de lado el
*vertex pulling* por lo mismo que en el ADR-0022: en esta GPU es un 45 % más lento.

**¿Por qué *decals* de malla y no proyectados, como muchos motores modernos?**
El *decal* proyectado pinta todo lo que entra en su caja: en el prototipo manchó los pies
de los zombies que pisaban la sangre. Además cuesta por píxel: 2000 midieron 4,7 ms. El
de malla recorta una vez, en la CPU, los triángulos del escenario contra la caja (2 µs
por marca) y después es una malla chica: 2000 midieron 0,95 ms. Además es el mismo
recorte de polígonos que hace el corte.

**¿Cómo saben las gotas dónde está el piso?**
Al emitir un estallido, lanzo un rayo hacia abajo desde el punto del corte con la
consulta de la física, y todas las gotas de ese estallido chocan contra ese plano. Es un
rayo por corte, no uno por gota, y funciona en escaleras y en rampas. Si la gota cae
afuera del plano, la mancha puede quedar un poco desfasada, y en un efecto de un segundo
eso no se nota.

**¿Cómo funcionan los *frames* de impacto, y por qué no salen en cada tajo?**
Son 2 o 3 frames en blanco, negro y rojo, en el postproceso a 640×360. Para saber qué es
un personaje uso el canal alfa del color, que no se usaba: los personajes escriben 0.
Cuesta 0,14 ms y solo en esos frames. Salen en los cortes cargados y en la cámara del
tajo: en cada tajo cansarían. Y como son destellos de pantalla completa, se pueden
apagar, por la fotosensibilidad.

**¿Cómo se ven las capas dentro del corte, y por qué un campo de distancia?**
Para dibujar piel, grasa o metal hay que saber a qué distancia del borde está cada punto
de la tapa. Medirlo por píxel contra las aristas es exacto, pero costó 1,15 ms en primer
plano. Usar la cápsula es gratis, pero no sigue el borde. Así que el corte llena una
textura de 64² con la distancia, y la tapa la lee sin costo en la GPU. Por fuerza bruta
midió 3,6 ms, así que uso una transformada de distancia lineal que propaga la arista más
cercana y mide la distancia exacta a ella: medir hasta el centro del texel me daba
~5 mm de error, más que la capa de piel. Como vive en el corte, lleva su TDD estricto.

**¿Cómo dibujas una partícula entre dos pasos sin guardar su posición anterior?**
Con `pos − vel·(1−α)·dt`: reconstruyo la posición del paso anterior con la velocidad y me
quedo en el mismo punto intermedio que el resto del render. Si dibujara `pos + vel·α·dt`,
iría un paso adelante, y a 5 m/s la sangre se despegaría ~8 cm del muñón.

**¿Cuánto cuestan los VFX y qué pasa cuando se llenan los pools?**
Una pelea normal mide unos 0,23 ms de GPU y casi nada de CPU. Les reservé 0,5 ms de GPU
y 0,2 ms de CPU, sacados de las reservas. Los pools son fijos y se reservan al iniciar:
si se llena el de partículas, la nueva no nace y un contador lo muestra; si se llena el
de marcas, la nueva reemplaza a la más vieja. Lo único que puede comerse el presupuesto
es el humo grande, por eso tiene su propio pool con un tope.

## Referencias

- William T. Reeves, "Particle Systems—A Technique for Modeling a Class of Fuzzy
  Objects", SIGGRAPH 1983.
- Gareth Thomas, "Compute-Based GPU Particle Systems", *Advanced Visual Effects with
  DirectX 11*, GDC 2014.
- Bill Rockenbeck, "The inFAMOUS: Second Son Particle System Architecture", GDC 2014.
- Chris Tchou, "Halo: Reach Effects Tech", GDC 2011.
- Eric Lengyel, "Applying Decals to Arbitrary Surfaces", en *Game Programming Gems 2*,
  Charles River Media, 2001, pp. 411–415.
- Pope Kim, "Screen Space Decals in Warhammer 40,000: Space Marine", SIGGRAPH 2012
  (*Talks*).
- Ivan E. Sutherland y Gary W. Hodgman, "Reentrant Polygon Clipping", *Communications of
  the ACM*, 1974.
- Pedro F. Felzenszwalb y Daniel P. Huttenlocher, "Distance Transforms of Sampled
  Functions", *Theory of Computing*, 2012.
- Guodong Rong y Tiow-Seng Tan, "Jump Flooding in GPU with Applications to Voronoi
  Diagram and Distance Transform", I3D 2006.
- Epic Games, documentación de Unreal Engine: *Niagara* (simulación en CPU y en GPU).
- Unity Technologies, documentación del *Visual Effect Graph*: *Collide with Depth
  Buffer*.
- Bryce E. Bayer, "An optimum method for two-level rendition of continuous-tone
  pictures", IEEE International Conference on Communications, 1973.
