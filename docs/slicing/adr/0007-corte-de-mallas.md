# ADR-0007: Corte de mallas

- **Estado:** Aceptado
- **Fecha:** 2026-10-20
- **Ticket:** #11
- **Reemplaza / Reemplazado por:** —

## Contexto y problema

El corte es la feature estrella del juego: **el tajo corta de verdad la malla del zombie,
por donde lo apuntó el jugador, en tiempo real** (visión). El zombie es una malla animada
(*skinned mesh*), se está moviendo cuando lo cortan, y lo que queda de él sigue vivo:
la mitad con la cabeza se arrastra hacia la caballera.

Hay que decidir:

1. la **técnica**: cortar la geometría de verdad, simularlo en el shader o cortar en
   puntos fijos;
2. **hasta dónde corta:** según la fuerza, la hoja puede rebotar, cortar solo un brazo
   o quedarse clavada a mitad del torso. Qué pasa con la parte que la hoja no atravesó
   entera;
3. cómo se **clasifican los vértices** sin que el redondeo deje agujeros;
4. cómo se conoce la **conectividad** de la malla, que glTF duplica en las costuras;
5. cómo se construye **la tapa**, la cara nueva que deja el corte;
6. qué **pesos y posición de referencia** reciben los vértices nuevos (lo dejó abierto
   el ADR-0015);
7. cómo se forman **los pedazos**, que pueden ser más de dos;
8. cómo se **guardan los pedazos** y cómo llegan a la GPU;
9. qué **invariantes de malla** valida el cocinador (lo dejó abierto el ADR-0017);
10. **cuándo corre** el corte, cuántos por paso, y qué pasa con los cortes repetidos y
    los pedazos muy chicos;
11. el **formato de vértice** de las mallas animadas y la **precisión de los pesos** (lo
    dejó abierto el ADR-0017).

Lo que ya está decidido y limita este ADR:

- **ADR-0008:** `float` de 32 bits, sin `-ffast-math`. La matemática deja preparado el
  tipo plano, la distancia con signo y las tolerancias explícitas. Los predicados
  robustos del corte los decide este ADR.
- **ADR-0011:** TDD estricto en las funciones del corte, tests de invariantes de malla y
  fuzzing. Cuáles invariantes valen para el corte, y con qué excepciones, lo fija este
  ADR (ver Consecuencias).
- **ADR-0012:** ECS con *sparse sets*, un hilo, paso fijo de 60 Hz, y el orden
  gameplay → combate → corte → física → animación. El combate pide el corte con un
  `slice_request` en una cola.
- **ADR-0015:** LBS con hasta 4 huesos por vértice, en la GPU. **La CPU deforma solo el
  zombie que se corta,** en la pose con que terminó el paso anterior (~50 µs). El pedazo
  vivo conserva el esqueleto completo, con los huesos perdidos enmascarados. Los pedazos
  heredan la pose anterior, para que el render interpole.
- **ADR-0016:** la forma física de los pedazos son las cápsulas de sus huesos,
  recortadas en el plano. El corte deja los pedazos como entidades, y la física crea sus
  cuerpos en su turno. El plano del tajo se congela al empezar el barrido.
- **ADR-0017:** el cocinador lee glTF, valida y escribe el binario propio. Este ADR fija
  qué valida de las mallas que se cortan, el formato de vértice y si hace falta un
  índice que una los vértices duplicados.
- **ADR-0020:** "combate y corte" tienen **2,0 ms por paso** para hasta 4 cortes
  (hipótesis). R5: 50 zombies, 100 pedazos, 4 cortes en un paso y 8 por segundo. Los
  cortes tienen una cuota por paso, y los que sobran esperan en un almacenamiento propio
  del corte, no en la arena por frame. En su documento vivo
  (`docs/performance/presupuestos.md`), los pedazos tienen 32 MiB, y la subida de
  geometría, N × 8 MiB por frame en vuelo.
- **ADR-0005:** la geometría dinámica sube por un *ring buffer* mapeado, una región por
  frame en vuelo. El número de frames en vuelo se fija con este ADR y el ADR-0010.
- **La visión:** un tajo cargado parte hasta 4 zombies. La fuerza decide hasta dónde
  llega la hoja, y puede quedar clavada (intención de diseño sumada a la visión en este
  ticket). En la cara del corte se ve el interior (carne, hueso, capas de metal y carne):
  la geometría de la tapa la genera este ADR, y cómo se dibuja la decide el ADR-0022.

**Fuera de alcance:**

| Qué | Lo decide |
|---|---|
| Qué pedazo sigue vivo, cuánto frena cada parte y cada armadura, cuándo rebota la hoja, cuánta energía cuesta sacarla | ADR-0018 |
| La animación de la espada clavada y de sacarla | ADR-0015 y ADR-0018 |
| Cómo se ve la tapa por dentro (materiales, capas) | ADR-0022 |
| La sangre y las salpicaduras | ADR-0025 |
| Cómo se implementan los pools, las arenas y el *ring buffer* | ADR-0010 |

## Criterios

Los cinco del ticket (el aprendizaje va junto con el valor para defenderlo), más la
robustez, que salió de la investigación:

1. **Impresión visual:** el corte pasa por donde se apuntó, sin grietas ni escalones.
2. **Rendimiento en tiempo real,** dentro de los 2,0 ms por paso, sin picos.
3. **Robustez:** todo pedazo queda cerrado, en cualquier pose y con cualquier plano,
   incluidos los degenerados.
4. **Compatibilidad con la animación y la física** (ADR-0015 y ADR-0016).
5. **Complejidad de implementación** para una sola persona.
6. **Aprendizaje y valor para defenderlo.**

## Opciones

### 1. La técnica

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Cortes en puntos fijos** | Cada zombie viene partido de antemano en el cuello, los hombros, la cintura… y el golpe elige la división más cercana. Es el desmembramiento por miembros de *Dead Space*, o el de casi todos los juegos de acción | El corte no pasa por donde se apuntó: contradice el pilar 1 de la visión |
| **Corte visual en el shader** | El fragment shader descarta los píxeles de un lado del plano, y la tapa se simula (por ejemplo, pintando de rojo las caras traseras que quedan a la vista). *Left 4 Dead 2* descartaba así un elipsoide para abrir heridas, con geometría de herida hecha a mano (Vlachos, GDC 2010) | No hay geometría nueva: los pedazos no existen para la física ni para la animación. Cada corte suma un plano por instancia, y la tapa falsa necesita mallas cerradas y un truco por cámara |
| **Corte geométrico de la superficie, en la CPU** | Se parten los triángulos que cruzan el plano, se crean los vértices nuevos y se cierra cada lado con una tapa. Es lo que hacen el `SliceProceduralMesh` de Unreal (para mallas estáticas), EzySlice en Unity y los plugins comerciales que cortan *skeletal meshes* | Hay que resolver la robustez, las tapas, los pesos y los pedazos. Es justo el trabajo de este ADR |
| **Malla volumétrica** (tetraedros) | El zombie es un volumen de tetraedros: al cortar, las caras internas ya existen, y la tapa sale gratis | Es el enfoque de la simulación quirúrgica (Wu, Westermann y Dick, 2015). Hay que tetraedrizar en el cocinador, el corte sigue las caras de los tetraedros salvo que se subdividan, y la malla de render se tiene que mantener aparte |

Cortar en la GPU con un compute shader no es una opción aparte: el ADR-0015 ya decidió
que la CPU deforma el zombie que se corta, y los contornos y las tapas son trabajo
secuencial que la física necesita en la CPU.

### 2. Hasta dónde corta

La visión dice que "la precisión decide *por dónde* corta, y la fuerza *hasta dónde*
llega". La intención de diseño (sumada a la visión en este ticket): **según la fuerza,
la hoja puede no entrar, cortar solo un brazo, o entrar hasta la mitad del torso y
quedarse clavada.** Entonces la caballera tiene que sacarla gastando energía. Bajo el
umbral de corte, el tajo empuja o rebota. Las reglas (cuánto frena cada parte y cada
armadura, qué pasa bajo el umbral, cuánta energía cuesta sacarla) las fija el ADR-0018.

**Quién decide dónde se detiene la hoja:** el combate, con las cápsulas. La detección del
ADR-0016 da los golpes en el orden en que la hoja los alcanza, y el combate gasta la
fuerza en cada uno hasta que se acaba. El corte no necesita la malla para eso: recibe el
plano y **dónde se detuvo la hoja**, es decir, la recta de la hoja en el plano en ese
instante (de la guarda a la punta) y el sentido del barrido. La **región barrida** es lo
que queda del lado ya barrido de esa recta. Un corte completo no lleva recta. Si la
fuerza no alcanza para entrar en lo primero que toca, no hay corte. El roce de la punta
(una parte que la hoja apenas alcanzó) también lo resuelve el combate con las cápsulas,
igual que dónde se detiene la hoja: el corte solo conoce la recta.

La recta se toma en el paso en que se emite el pedido, y la pose del zombie que usa el
corte es la de ese mismo paso. El centro del arco se mueve hasta 35 cm durante el tajo
(ADR-0016), así que reconstruir el barrido entero con un solo centro se correría varios
centímetros. La recta de detención, en cambio, es el único borde de la región que decide
qué queda dentro.

**Cómo usa el corte la región barrida:** arma los contornos de la sección, como en un
corte completo, y compara cada contorno con la región:

| El contorno | Qué significa | Qué hace el corte |
|---|---|---|
| **Entero dentro** | La hoja atravesó esa parte (el brazo) | La corta de verdad: el brazo cae |
| **Entero fuera** | La hoja no llegó | Nada |
| **En parte dentro** | La hoja se detuvo dentro de esa parte (el torso) | Depende de la opción de abajo |

Los contornos se conocen antes de partir los triángulos: solo hace falta recorrer los que
cruzan el plano. Después se parten solo los que corresponden.

**Qué pasa con la parte que la hoja no atravesó entera:**

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Herida dibujada** | La malla no cambia ahí. La espada queda clavada, unida al hueso más cercano al punto donde se detuvo, y la herida la dibuja el render: una calcomanía o un descarte en el shader, como las heridas de *Left 4 Dead 2* | La herida es pintura, no una abertura: al sacar la espada no se ve el interior. Un segundo tajo por el mismo lugar no sabe que ahí había una herida |
| **Hendidura geométrica** | Se parten solo los triángulos cuyo segmento cae dentro de la región. La malla queda con una ranura que termina donde se detuvo la hoja, y las dos caras de la tapa se juntan en el fondo. Sigue siendo un solo pedazo, cerrado, que se puede volver a cortar. El fondo sigue las aristas de la malla en vez del punto exacto, porque la espada lo tapa | Más compleja: donde el fondo llega a la piel, los triángulos se parten en abanico, sin separarse. Además, la ranura no se ve si no se abre (ver abajo). Probada en el prototipo: pedazos cerrados en todos los casos, fuera del defecto del asset |
| **Todo o nada** | Si la hoja cruzó más de cierta fracción del contorno, se corta entero; si no, nada | Contradice la intención de la visión: la espada nunca queda clavada |

**Un caso borde de la hendidura:** si a un contorno le falta un solo segmento para estar
entero dentro, la cuerda que cierra la ranura coincide con ese segmento, y esa arista
quedaría con 4 caras. Como la hoja cruzó todo menos una astilla, ese contorno se corta
completo.

**Cómo se abre la ranura.** Recién cortada, sus dos caras coinciden: sin abrirla, no se
ve el interior.

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Pesos por lado** | Cada cara se des-deforma con los pesos del vértice de su lado: coinciden en la pose del corte, y se separan cuando el zombie se mueve | Medido: se abre de 0,1 a 4,3 mm, porque a los dos lados manda casi el mismo hueso. A 640×360 y a 3 m, un píxel cubre ~0,45 cm apuntando (30°) y ~0,8 cm sin apuntar (51°): la ranura abre a lo sumo ~1 píxel |
| **Abertura en cuña** | Al crear la ranura, cada cara se separa de la otra a lo largo de la normal del plano: el máximo en la boca de la herida y 0 en el fondo, donde se detuvo la hoja. El desplazamiento se des-deforma, como los vértices | Un parámetro que ajustar a ojo (el ancho máximo). La herida ya no conserva el volumen exacto: le falta la cuña |
| **Las dos** | La cuña da el ancho, y los pesos por lado agregan el movimiento | Dos mecanismos para un efecto que la cuña ya da casi entero |

### 3. Clasificar los vértices

Decidir de qué lado del plano está cada vértice es un predicado geométrico (ADR-0008):
con el vértice casi sobre el plano, el redondeo puede dar respuestas contradictorias.

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Plano grueso** (Ericson) | Los vértices a menos de ε del plano cuentan como "sobre el plano", y los triángulos tienen casos extra para ellos | Más casos (vértice sobre el plano, arista sobre el plano, triángulo entero sobre el plano), y hay que elegir ε |
| **Una sola clasificación por vértice, con desempate simbólico** | Cada vértice se clasifica una vez, por el signo de su distancia, y una distancia exactamente 0 cuenta como positiva. Como nadie vuelve a preguntar, las respuestas no se pueden contradecir | Un vértice muy cerca del plano deja un triángulo casi sin área y puntos de corte casi repetidos, que la tapa tiene que tolerar |
| **Predicados exactos** (Shewchuk) | Aritmética de precisión adaptativa: exacta solo cuando el resultado es dudoso | Resuelven otro problema: el de un mismo hecho geométrico evaluado de formas distintas en lugares distintos (por ejemplo, `orient3d` con los puntos permutados). Aquí el plano es un dato, no tres puntos de la malla |

### 4. La conectividad

glTF duplica un vértice en cada costura de normal o de UV (ADR-0017). Dos triángulos
vecinos a lo largo de una costura no comparten índices, y sin conectividad no se pueden
encadenar los contornos ni saber qué pedazo es cuál.

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Emparejar por posición al cortar** | Los puntos de corte se juntan con una tabla hash de posiciones, con tolerancia | Comparar `float` con tolerancia es frágil: dos puntos de cáscaras distintas pueden caer juntos, y uno partido por el redondeo, no |
| **Índice soldado del cocinador** | El cocinador calcula, para cada vértice, el primero con la misma posición de referencia bit a bit (el *shadow index buffer* de meshoptimizer). El corte clasifica y calcula los puntos sobre el índice soldado | Un arreglo más por malla (4 B por vértice) |
| **Estructura de medias aristas** (*half-edge*) | La malla guarda, para cada arista, su vecina y su siguiente (Botsch y otros) | Más memoria y más trabajo al crear la malla nueva de cada pedazo. Lo que el corte necesita lo da el índice soldado |

### 5. La tapa

**Los contornos:** cada triángulo partido aporta un segmento entre dos puntos de corte.
Si cada punto de corte se identifica por su arista soldada, los segmentos se encadenan
por índice, sin comparar posiciones.

**Los agujeros:** un contorno puede estar dentro de otro (una armadura hueca alrededor
del cuerpo deja un anillo). Por la orientación, los exteriores giran en un sentido y
los agujeros en el otro. El sentido se mide en una base del plano `(u, v)` con
`u × v = n` (mano derecha, ADR-0008): así, "área positiva" significa "antihorario visto
desde `n`".

**La triangulación:**

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Cortar orejas** (*ear clipping*, Eberly) | Se busca un vértice convexo cuyo triángulo no contenga otro vértice, se corta y se repite. Los agujeros se unen al exterior con un puente | O(n²), y hay que cuidar los puntos repetidos y alineados |
| **earcut** (Mapbox, ISC) | Cortar orejas con una tabla de orden Z, agujeros y casos degenerados, basado en Held (FIST) y Eberly. Es la que usan los mapas | Una dependencia, o una reescritura de su lógica. No garantiza una triangulación correcta con polígonos que se cruzan: busca un resultado aceptable |
| **Partir en polígonos monótonos** (de Berg y otros) | Se divide el polígono en piezas monótonas con un barrido, y cada una se triangula en tiempo lineal | O(n log n), con más código y más casos. EzySlice usa una variante solo para tapas convexas |
| **Delaunay restringida** | Triángulos de buena forma, que respetan los bordes | La más compleja. La forma de los triángulos no importa en una tapa plana |

**Las coordenadas de textura de la tapa:** una proyección plana, en esa misma base del
plano.
Cómo se dibuja el interior lo decide el ADR-0022.

### 6. Pesos y posición de referencia de los vértices nuevos

Viene del ADR-0015: el corte se calcula en la pose del instante, pero el vértice nuevo se
guarda en la pose de referencia para que el pedazo se siga animando.

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Interpolar** | La posición de referencia se interpola con el mismo `t` de la arista | Con LBS no es exacto: medido, el vértice se sale del plano **hasta 10 mm** en el cuello agachado (ver Investigación) |
| **Des-deformar** | `b = (Σᵢ wᵢ·Mᵢ)⁻¹·p`, con `p` el punto del corte y `w` los pesos finales | Una inversa de 3 × 3 por punto de corte. Si la matriz mezclada es casi singular (una torsión extrema, el colapso del "envoltorio de caramelo" del ADR-0015), el vértice sale disparado: hace falta un respaldo |

**Los pesos** del vértice nuevo son la mezcla de los de sus dos extremos. Si pasan de 4
huesos, se conservan los 4 de mayor peso y se renormalizan.

**El respaldo para una matriz casi singular:** si el determinante de la parte 3 × 3 queda
bajo un umbral, el vértice pasa entero a su hueso dominante (peso 255) y se des-deforma
con esa sola matriz. Esa matriz solo tiene la rotación, la traslación y la escala de ese
hueso, así que se puede invertir mientras su escala no sea 0; su determinante también se
revisa. El vértice sigue cayendo justo en el plano, a cambio de un salto de pesos en ese
punto. Si también ese determinante queda bajo el umbral (un hueso con escala 0), el último
recurso es interpolar la posición de referencia, y el error queda fuera del plano.

**La normal de la tapa** también se guarda en la pose de referencia: se des-deforma con
la inversa de la transformación que el shader aplica a las normales (si el shader usa la
parte 3 × 3 `A` de la matriz mezclada, la inversa es `A⁻¹`), y se renormaliza.

### 7. Los pedazos

**Un corte puede dar más de dos pedazos:** un plano a la altura de la cintura también
corta los antebrazos que cuelgan, y un plano a la altura de las rodillas separa las dos
pantorrillas. Además, los personajes suelen estar hechos de varias cáscaras cerradas
que se superponen (medido: el maniquí de Quaternius tiene 61).

| Opción | Cómo se forman | Problema |
|---|---|---|
| **Solo por conectividad** | Cada componente conexa de cada lado es un pedazo | Cada cáscara suelta (un ojo, una placa de armadura, una falange) sale como un pedazo propio: 61 pedazos debajo de la cintura del maniquí |
| **Solo por el esqueleto** | Se decide de qué lado queda cada hueso, y cada triángulo va con su hueso dominante | Dos regiones separadas por el corte pero con el mismo hueso dominante (el antebrazo cortado al medio) quedan juntas, y un triángulo que cruza dos huesos puede ir al pedazo equivocado |
| **Fragmentos conexos, unidos por los huesos** | Primero la conectividad, dentro de cada cáscara cruzada. Después, dos fragmentos del mismo lado se unen si comparten un hueso dominante | Si dos fragmentos del mismo lado comparten un hueso sin estar unidos, se juntan |

### 8. Cómo se guardan los pedazos y cómo llegan a la GPU

| Opción | Qué tiene cada pedazo | Problema |
|---|---|---|
| **Una copia completa** | Su propio buffer de vértices y de índices | Medido: +0,23 ms y ~290 KB por corte, y memoria repetida en cada pedazo |
| **El buffer original compartido** | Índices propios sobre el buffer de vértices del modelo, que no cambia, más un buffer chico con los vértices nuevos (los de las aristas y los de las tapas) | Un pedazo dibuja desde dos buffers. Hay que resolverlo al dibujar: dos llamadas por pedazo (los triángulos originales y los que tocan vértices nuevos, con copias de sus vértices originales), o leer los vértices desde un *storage buffer* (*vertex pulling*) |

**Los frames en vuelo** (ADR-0005): con 2, la CPU prepara un frame mientras la GPU
dibuja el anterior. Con 3, se absorben mejor los picos, a cambio de un frame más de
latencia y un tercio más de memoria en el *ring buffer*.

### 9. Las invariantes de las mallas que se cortan

| Opción | Qué valida el cocinador | Problema |
|---|---|---|
| **Cáscaras cerradas y *manifold*** | Cada arista soldada la comparten exactamente dos triángulos, con orientaciones opuestas. Si no, falla el build (ADR-0017) | Los modelos de terceros pueden traer defectos que hay que arreglar en Blender |
| **Aceptar mallas abiertas** | El corte cierra lo que puede: un contorno abierto se queda sin tapa | Por esa abertura se ve el interior hueco del modelo. Medido: el único defecto del maniquí hace fallar 44 de 3000 cortes de prueba, en cada una de las dos poses |
| **Reparar en el cocinador** | El cocinador cierra los agujeros y separa las aristas que no son *manifold* | Un algoritmo más (reparar mallas es un problema abierto), que cambia el modelo sin que el autor lo vea |

### 10. Cuándo corre y cuánto

**La cuota:** el ADR-0020 fija 4 cortes por paso, y los que sobran esperan.

| Opción | Cuándo se procesan los que sobran | Problema |
|---|---|---|
| **En el paso siguiente** | Si un tajo alcanza más zombies que la cuota, corta 4 en este paso y el resto en el siguiente | Un paso de diferencia (16,7 ms) entre zombies del mismo tajo, y el zombie diferido ya cambió de pose: el corte tiene que usar la pose guardada con el pedido |
| **Dentro de la pausa de impacto** | La pausa (50 a 80 ms, de 3 a 5 pasos) congela la animación, así que los cortes se reparten en esos pasos sin que se vea | Solo sirve si hay pausa: el tajo rápido que sí corta tiene que tenerla |

**Los cortes repetidos:** si la salida de un corte es una entrada válida (pedazos
cerrados, con el mismo formato), se puede volver a cortar el pedazo vivo o un pedazo
en el suelo. Que se pueda es una propiedad del algoritmo; si se permite en el juego, lo
decide el ADR-0018.

**Los pedazos chicos:** un plano que roza una mano deja un pedazo de pocos triángulos.
Ocupa uno de los 100 lugares de pedazos (R5) y un cuerpo físico por algo que casi no se
ve.

| Opción | Qué pasa con un pedazo bajo un mínimo | Problema |
|---|---|---|
| **Se conserva** | Es un pedazo más | Gasta lugares y cuerpos en astillas |
| **Se descarta** | Desaparece (la salpicadura del ADR-0025 lo disimula) | Algo que estaba deja de estar |
| **Se queda con el pedazo vecino** | No se separa: se vuelve a juntar con el pedazo del otro lado | Ese trozo queda "atravesado" por el corte, sin separarse |

### 11. El formato de vértice y la precisión de los pesos

El vértice de una malla animada lleva posición, normal, coordenadas de textura, 4
índices de hueso y 4 pesos (ADR-0015). Los índices ya son de 8 bits (hasta 256 huesos,
ADR-0017). El ADR-0020 estimó 32 B por vértice.

| Pesos | Tamaño | Problema |
|---|---|---|
| **`float`** (como los exporta Blender) | 16 B | La mitad del vértice se va en pesos |
| **`unorm16`** | 8 B | Precisión de sobra para un juego |
| **`unorm8`** (glTF lo permite) | 4 B | Pasos de 1/255. Que sumen exactamente 1 exige repartir el redondeo |

**La precisión toca al corte:** la GPU deforma con los pesos cuantizados, así que el
corte tiene que des-deformar **con esos mismos pesos**. Si des-deforma con los pesos en
`float` y después los cuantiza, el vértice nuevo ya no cae justo en el plano. Lo mismo
vale para la normal y las coordenadas de textura: los vértices nuevos se guardan en el
mismo formato que los originales, y lo que se cuantiza se cuantiza antes de usarlo.

## Investigación

### Prototipo medido

Un prototipo en C++20 cortó el maniquí de Quaternius (8546 vértices, 13 744 triángulos,
65 huesos, CC0) en una pose agachada del clip `Zombie_Walk_Fwd_Loop`. Hizo el skinning
en la CPU, la clasificación con desempate simbólico, el índice soldado, las tapas
cortando orejas, la des-deformación y los pedazos por fragmentos unidos por huesos.
Medido en la máquina de referencia (ADR-0020) el 2026-10-20, fuera del protocolo. El
detalle y cómo repetirlo están en
[`../research/prototipo-del-corte.md`](../research/prototipo-del-corte.md).

| Qué | Resultado |
|---|---|
| Tiempo por corte, con el skinning en la CPU incluido | **0,53 a 0,83 ms**, sin optimizar (el mejor de 300 repeticiones; entre ejecuciones varió hasta un 30 %). La malla tiene ~3 veces los vértices de un zombie del juego: del orden de 0,2 a 0,3 ms por corte |
| Pedazos cerrados y volumen | Ninguna arista abierta nueva, y el volumen se conserva con un error relativo de ~10⁻⁸ |
| Pedazos por corte | De 2 a 5. En la cintura: 1 arriba y 3 abajo (las piernas con la pelvis y las dos manos) |
| Error de **interpolar** la posición de referencia | Hasta **10,07 mm** fuera del plano (en el cuello). En la pose de referencia, 0 |
| Error de **des-deformar** | 0,0002 mm |
| Puntos de corte con más de 4 huesos | Hasta 44 de 183 |
| Copiar los vértices de cada pedazo | +0,23 ms y ~290 KB por corte |
| Robustez: 3000 planos (2000 al azar y 1000 que pasan justo por vértices), en dos poses | **Todos los fallos** (44 en cada pose) cruzan la única arista no *manifold* del modelo. Ningún otro |
| Orejas forzadas al triangular la tapa | En ~26 % de los cortes al azar hubo que forzar alguna oreja (contornos degenerados). El pedazo queda cerrado, pero esos triángulos pueden quedar degenerados o encimados. Ninguno de los cinco cortes de referencia forzó una |
| **Hendidura:** la hoja se detiene en el centro del torso | 2 pedazos: cae el antebrazo, y el cuerpo queda entero, con la ranura. 0,76 ms |
| Hendidura: 3000 planos con rectas de detención al azar, en dos poses | Ningún fallo fuera de la arista defectuosa |
| Hendidura: cuánto se abre sola, con pesos por lado | De 0,1 a 4,3 mm en poses posteriores del clip: no se ve a 640×360 |

Lo que mostró, además de los números:

- **Los modelos reales son varias cáscaras superpuestas,** no una malla cerrada: el
  torso con la cabeza, los brazos, las piernas y cada falange van por separado. La
  conectividad sola da 61 pedazos debajo de la cintura; unidos por los huesos, 3.
- **LBS cruza la malla consigo misma** en las articulaciones dobladas. El corte de esa
  zona da contornos con la orientación invertida, y la tapa los tiene que tolerar.
- **Los contornos degenerados existen:** un plano que roza una cáscara justo en un
  vértice deja un contorno de 3 puntos con área exactamente 0.

### Robustez numérica

- **Christer Ericson** (*Real-Time Collision Detection*, 2004) describe cómo partir
  polígonos contra un plano con un "plano grueso" (una tolerancia ε) y por qué la
  clasificación tiene que ser **consistente**: dos triángulos que comparten una arista
  tienen que estar de acuerdo sobre de qué lado está cada extremo, o queda una grieta.
- **La consistencia sale gratis si cada vértice se clasifica una sola vez** y cada punto
  de corte se calcula una sola vez por arista, siempre desde el mismo extremo. El
  prototipo lo hace con el índice soldado, y no necesitó tolerancias para que los
  pedazos quedaran cerrados.
- **El desempate simbólico** es la idea de *Simulation of Simplicity* (Edelsbrunner y
  Mücke, 1990): se simula una perturbación infinitesimal de los datos que elimina los
  casos degenerados, sin calcularla. Aquí, "0 cuenta como positivo" equivale a mover el
  plano un infinitésimo.
- **Los predicados adaptativos de Shewchuk** (1997) son exactos y rápidos cuando el
  resultado es claro. Hacen falta cuando el mismo hecho se evalúa varias veces con
  datos distintos (por ejemplo, la orientación de tres puntos de la malla). Con un plano
  dado y una clasificación por vértice, el redondeo puede cambiar de qué lado queda un
  vértice casi sobre el plano, pero no puede romper la topología.

### Cómo cortan otros juegos y motores

- ***Metal Gear Rising: Revengeance*** (PlatinumGames, 2013) tiene un "modo espada" en el
  que el jugador corta enemigos y objetos por cualquier plano. Es la referencia más
  cercana a este juego.
- ***Left 4 Dead 2*** (Vlachos, *Rendering Wounds in Left 4 Dead 2*, GDC 2010) descarta en
  el shader los píxeles dentro de un elipsoide para abrir heridas, y pone geometría de
  herida hecha a mano. No crea pedazos nuevos.
- ***Dead Island 2*** (Dambuster, 2023) tiene FLESH, un sistema de heridas por capas
  (piel, músculo, hueso y órganos) según dónde y con qué se golpea. Desmiembra, pero no
  por planos arbitrarios.
- **Unreal** tiene `SliceProceduralMesh`, que corta con un plano una malla procedural
  (no animada) y crea la tapa como una sección nueva, con su material.
- **EzySlice** (Unity, MIT) corta mallas con un plano, interpola UV, normales y
  tangentes con coordenadas baricéntricas, y triangula la tapa con una cadena monótona,
  que solo es correcta si la tapa es convexa.
- **Los plugins comerciales de corte de *skeletal meshes*** (por ejemplo, *Slice
  SkeletalMesh* para Unreal) parten un personaje en dos mallas animadas completas, con
  su skinning, y permiten volver a cortar los pedazos.
- ***Blade & Sorcery*** (WarpFrog, 2018) es un juego de combate con física en el que la
  hoja se detiene donde el cuerpo la frena y queda clavada: hay que tirar para sacarla.
  Es la referencia de la hoja clavada, aunque ahí la decide la física y aquí la deciden
  la fuerza del tajo y las reglas del ADR-0018.
- **La simulación de cortes en cuerpos deformables** (Wu, Westermann y Dick, *Computer
  Graphics Forum*, 2015) clasifica los métodos por representación: mallas de
  tetraedros, de hexaedros y poliédricas, y métodos sin malla. Es otro problema: cuerpos
  blandos que se deforman y se cortan de a poco.

### Triangulación de la tapa

- **Cortar orejas** (Eberly, *Triangulation by Ear Clipping*): todo polígono simple de
  más de 3 vértices tiene al menos dos orejas (el teorema de las dos orejas de Meisters).
  Los agujeros se unen al exterior con un puente desde el vértice del agujero con mayor
  `x`, y el polígono resultante se triangula igual.
- **earcut** (Mapbox, ISC) acelera la búsqueda de orejas con una curva de orden Z, y
  maneja agujeros, polígonos que se cruzan y degeneraciones "de forma aceptable, sin
  garantizar una triangulación correcta". Se basa en FIST (Held, 2001) y en Eberly.
- **Partir en monótonos** (de Berg, Cheong, van Kreveld y Overmars, *Computational
  Geometry*, capítulo 3) es O(n log n). Con cortes de 58 a 183 puntos en total,
  repartidos en 1 a 3 contornos, como los medidos, la ventaja asintótica casi no se nota.

### Pesos de la geometría nueva

- **La cuenta del ADR-0015** predijo el error de interpolar: se anula en la pose de
  referencia y crece con la rotación relativa entre los huesos. El prototipo lo
  confirmó: 0 en la pose de referencia y hasta 10 mm agachado.
- **Abdrashitov y otros** (SIGGRAPH Asia 2023) transfieren pesos a geometría nueva en
  dos etapas. Aquí no hace falta: los vértices nuevos nacen sobre aristas, y sus pesos
  salen de los extremos de esa arista.

## Decisión

Elijo cortar la geometría de verdad, en la CPU, porque es lo que calza con lo que buscamos
en este proyecto: el corte pasa por donde apunté. La fuerza decide hasta dónde llega la
hoja: puede rebotar, cortar solo un brazo, o llegar hasta la mitad del torso y quedarse
clavada en el zombie, y entonces hay que sacarla gastando energía. Donde la hoja se
detiene queda una hendidura de verdad, abierta en cuña. Es lo más entretenido y lo más
impresionante al jugar, y lo visualizo como una gran venta del juego. Además, la espada
va a ser gruesa, como la Matadragones de *Berserk*, así que la herida se va a notar.

| # | Dimensión | Elección |
|---|---|---|
| 1 | Técnica | **Corte geométrico de la superficie, en la CPU** |
| 2 | Hasta dónde corta | **Solo la región barrida por la hoja.** El combate decide dónde se detiene, con las cápsulas y las reglas del ADR-0018, y el `slice_request` lleva la recta donde se detuvo. Un contorno entero dentro se corta completo; uno en parte dentro deja una **hendidura geométrica**, abierta **en cuña** (máxima en la boca de la herida, 0 en el fondo). Si a un contorno le falta un solo segmento, se corta completo |
| 3 | Clasificación | **Una sola vez por vértice soldado, con desempate simbólico:** una distancia exactamente 0 cuenta como positiva. Cada punto de corte se calcula una sola vez por arista soldada, desde el extremo de menor índice |
| 4 | Conectividad | **Índice soldado, calculado por el cocinador** |
| 5 | Tapa | **Cortar orejas propio** (Eberly), en una base del plano con `u × v = n`. Los agujeros se buscan solo en la misma cáscara; los contornos invertidos se triangulan solos, con su propia orientación, y los de área cero, como exteriores. Coordenadas de textura por proyección plana |
| 6 | Vértices nuevos | **Des-deformar**, con los pesos ya mezclados, recortados a 4 y cuantizados. Si la matriz mezclada es casi singular, el vértice pasa entero a su hueso dominante. La normal de la tapa también se des-deforma |
| 7 | Pedazos | **Fragmentos conexos, unidos cuando comparten un hueso dominante del mismo lado.** El lado se mira por triángulo: un fragmento con hendidura tiene los dos. Un corte puede dar N pedazos |
| 8 | Memoria y GPU | **El buffer de vértices del modelo se comparte;** cada pedazo tiene sus índices y un buffer chico con sus vértices nuevos. **2 frames en vuelo** |
| 9 | Invariantes | **Cada cáscara de una malla que se corta es cerrada y *manifold*.** Si no, el cocinador falla el build |
| 10 | Cuándo | **4 cortes por paso.** Los que sobran se reparten **en los pasos de la pausa de impacto**, o en el paso siguiente si no hay pausa, y cada pedido guarda la pose del paso que lo emite. Caben 20 pendientes; si no queda lugar, el combate no emite el pedido y el zombie solo recibe el empuje. El algoritmo admite volver a cortar un pedazo. **Los pedazos bajo un mínimo se descartan**, disimulados con la salpicadura |
| 11 | Pesos | **`unorm8`**, que suman exactamente 255 |

## Consecuencias

- **El camino de un corte,** dentro del paso de simulación (ADR-0012):

  ```
  combate: barrido del tajo (ADR-0016) → fuerza gastada en cada cápsula (ADR-0018)
           → slice_request { entidad, plano, recta de detención, pose }
  corte:   skinning en la CPU (pose del pedido) → clasificar → contornos
           → contornos vs región → partir triángulos → tapas → hendiduras en cuña
           → pedazos → entidades con sus componentes
  física:  crea los cuerpos de los pedazos (ADR-0016)
  ```

- **El `slice_request` lleva:** la entidad, el plano, la recta donde se detuvo la hoja
  (de la guarda a la punta) con el sentido del barrido, y la pose del zombie. **La recta y
  la pose son del mismo paso: el que emite el pedido.** La pose es la vigente en ese
  paso, es decir, la que dejó el paso anterior (ADR-0015), la misma que usó el combate
  con las cápsulas. Un corte completo no lleva recta.
  El combate emite un solo pedido por zombie y por tajo; cuándo lo emite (al salir la
  hoja o al detenerse) es una regla del ADR-0018.
- **El combate decide dónde se detiene la hoja con las cápsulas,** sin mirar la malla: la
  detección del ADR-0016 da los golpes en orden, y la fuerza se gasta en cada uno según
  las reglas del ADR-0018. También resuelve el roce de la punta. Si no alcanza para
  entrar en lo primero que toca, no hay corte: el tajo empuja o rebota, según el
  ADR-0018.
- **Qué cápsulas se recortan:** las de los contornos que el corte separa. Una cápsula se
  recorta (con la regla del ADR-0016) si los vértices que mueve su hueso quedan en
  pedazos distintos, o si su hueso y el de una cápsula vecina que la toca quedan en
  pedazos distintos. Las cápsulas que solo toca una hendidura no cambian. Cuando cae el
  antebrazo y el torso queda con una ranura, en el mismo corte, se recortan las del
  antebrazo y no las del torso. Queda registrado en el ADR-0016 como fe de erratas.
- **La geometría de recortar una cápsula vive en `engine/core`,** junto al skinning de la
  CPU (ADR-0015). La especifica y la prueba la física (ADR-0016), pero la llama el corte.
  Como el corte y la física están en la misma capa y no se enlazan entre sí (ADR-0012),
  vive debajo de los dos. Sus tests siguen siendo los del ADR-0016.
- **Lo que el corte escribe en cada pedazo,** como componentes del ECS:
  - la **máscara de huesos:** los huesos con peso distinto de cero en algún vértice del
    pedazo, el mismo criterio de "mueve algún vértice" del ADR-0015 y del ADR-0017. En el
    pedazo vivo, la máscara no quita matrices: la jerarquía completa se sigue evaluando y
    subiendo, como pide el esqueleto completo del ADR-0015. Los pedazos rígidos usan sus
    matrices congeladas en la pose del corte (ADR-0016);
  - las **cápsulas de esos huesos,** ya recortadas;
  - para poder volver a cortarlo: el **índice soldado extendido** a sus vértices nuevos,
    sus cáscaras y sus huesos dominantes. El corte los arma por construcción, nunca
    comparando posiciones: cada vértice nuevo apunta a su punto de corte **de su lado**.
    Los dos lados de un punto partido tienen identificadores distintos (en una
    hendidura, las dos caras de la ranura quedan en el mismo pedazo, y con un mismo
    identificador cada arista de la boca tendría 4 caras). Un punto unido, el del fondo
    de la ranura, tiene uno solo. Así, el vértice del borde de la piel y su copia en la
    tapa del mismo lado siguen soldados entre sí. El cocinador calcula el índice de la
    malla original, y el corte solo lo extiende.

  La animación y la física solo leen estos componentes. Una hendidura no cambia la
  máscara.
- **La cuña:** las dos caras de la ranura se separan a lo largo de la normal del plano,
  con un ancho total `h` que va del máximo en la boca de la herida hasta 0 en el fondo.
  En la pose de referencia, cada vértice de una cara se desplaza
  `Δb = A⁻¹·(±(h/2)·n)`, con `+` en la cara del lado positivo, `−` en la del negativo y
  la misma `A` de des-deformar. Así, en la pose del corte, las caras quedan exactamente a
  `h` una de otra. El vértice del borde de la piel y su copia en la tapa, que están en la
  misma posición y del mismo lado, se desplazan igual: si no, se abre una grieta en la
  boca de la herida. `p` y `n` están en el espacio del modelo, el que sale del skinning.
  El ancho máximo es un parámetro que se ajusta sin recompilar (ADR-0021). Probablemente
  sale del grosor de la hoja: la espada será gruesa.
- **El fondo de la ranura sigue las aristas de la malla,** no la recta exacta donde se
  detuvo la hoja. La espada clavada lo tapa.
- **Lo que guarda el cocinador** para cada malla que se corta (ADR-0017): el índice
  soldado (4 B por vértice), las cáscaras y los huesos dominantes de cada una. Además
  valida que cada cáscara sea cerrada y *manifold*: cada arista soldada la comparten
  exactamente dos triángulos, con orientaciones opuestas. Un modelo de terceros con
  defectos se arregla en Blender.
- **El orden del cocinador,** después de la rotación de ejes del ADR-0017: fundir los
  huesos que se quitan → conservar los 4 de mayor peso → renormalizar → cuantizar a
  `unorm8` → calcular los huesos dominantes de cada cáscara y ajustar las cápsulas. Los
  dominantes y las cápsulas salen de los pesos ya cuantizados, los mismos que ven la GPU
  y el corte: con los pesos en `float`, un empate 0,5/0,5 podría elegir otro hueso, y un
  peso menor que ~1/510 se vuelve 0 al cuantizar (y ese hueso deja de "mover algún
  vértice").
- **El formato de vértice de todas las mallas animadas,** no solo de las que se cortan
  (la GPU las deforma a todas con el mismo formato): índices de hueso `uint8` × 4
  (ADR-0017) y pesos `unorm8` × 4.
  Al cuantizar, el redondeo se reparte para que sumen exactamente 255 (por ejemplo, el
  resto va al peso que más perdió). **El corte des-deforma con los pesos ya cuantizados,**
  los mismos que usa la GPU: si no, el vértice nuevo se sale del plano. El formato de los
  demás atributos (normal y coordenadas de textura) lo cierra el ticket del cocinador,
  en la definición del formato de `engine/core` (ADR-0017), con la misma regla: lo que se
  cuantiza se cuantiza antes de usarlo en el corte. El objetivo es el de 32 B por vértice
  del ADR-0020. Con todo en `float` serían 40 B, así que llegar a 32 B obliga a comprimir
  la normal (por ejemplo, octaédrica en 4 B). La normal de la tapa se renormaliza antes
  de cuantizarla.
- **La normal en el shader:** la des-deformación de la normal es la inversa de lo que el
  shader le hace a la normal. El shader y el skinning de la CPU (ADR-0015) fijan esa
  transformación juntos, y el test que los compara cubre también las normales.
- **Los pedazos en memoria:** el buffer de vértices del modelo vive una sola vez en la
  GPU. Cada pedazo suma sus índices y un buffer chico con sus vértices nuevos (de 180 a
  570 por corte en el maniquí), dentro de los 32 MiB de pedazos (ADR-0020). Qué lleva
  ese buffer chico depende de cómo se dibuje, y eso lo decide el ADR-0022. Por ejemplo:
  con dos llamadas, lleva también copias de los vértices originales de los triángulos
  del borde; con *vertex pulling*, solo los nuevos; si el buffer del modelo y los
  vértices nuevos viven en el mismo `VkBuffer`, basta una llamada con índices de 32 bits.
- **El *ring buffer* es el camino de subida** (ADR-0005), no donde viven los pedazos. En
  la GPU dedicada, los datos se copian de ahí al pool de pedazos (*device-local*). En la
  integrada, se escriben directo en el pool. Liberar el lugar de un pedazo descartado
  espera a que terminen los frames en vuelo que lo usan. Lo implementa el ADR-0010.
- **2 frames en vuelo:** es lo que ya suponía `docs/performance/presupuestos.md`, el
  documento vivo del ADR-0020, y queda anotado ahí. Para dimensionar el *ring buffer*, el
  ADR-0010 cuenta:
  - el peor caso de cortes en un frame: 4 por paso × 8 pasos por frame (ADR-0012) = 32;
  - las matrices de los huesos por frame (74 KB con 50 zombies de ~23 huesos, ADR-0015 y
    ADR-0017);
  - las matrices de los ragdolls activos (con su tope, ADR-0016) y la transformación de
    cada pedazo rígido.
- **Presupuesto:** del orden de 0,2 a 0,3 ms por corte en un zombie de ~3000 vértices
  (extrapolado del prototipo, sin optimizar y con hasta un 30 % de variación entre
  ejecuciones). 4 cortes caben en los 2,0 ms de "combate y corte". Es una hipótesis
  hasta medirla con el protocolo del ADR-0020.
- **Los cortes que esperan** viven en un almacenamiento propio del corte, no en la arena
  por frame (ADR-0020). Lo implementa el ADR-0010:
  - **cada pedido guarda la pose:** las matrices de skinning y la transformación del
    zombie vigentes en el paso que emite el pedido (las que dejó el paso anterior), el
    mismo paso de la recta (unos 4 KB con 65 huesos,
    menos con el rig recortado). El corte diferido usa esa pose, no la actual, para que
    corte por donde el jugador vio la espada (ADR-0015);
  - **el tope es de 20 pedidos** (4 por paso × 5 pasos de pausa). Antes de emitir un
    pedido, el combate pregunta al corte si queda lugar, con una consulta directa hacia
    abajo (ADR-0012). Si no queda, no lo emite, y el zombie solo recibe el empuje, como
    un golpe bajo el umbral. Así la degradación que pide el ADR-0020 queda en el juego,
    donde están las reglas del ADR-0018, sin un canal del corte hacia arriba. Con hasta 4
    zombies por tajo (visión), en la práctica no debería pasar;
  - **el reparto en la pausa** solo funciona si la pausa de impacto sigue corriendo
    cuando se emite el pedido. Si empieza al tocar y termina antes de que la hoja salga
    o se detenga, los cortes que sobran van al paso siguiente. Cuándo empieza la pausa
    lo decide el ADR-0018.
- **Los pedazos chicos:** el mínimo (por volumen o por cantidad de triángulos) se fija
  midiendo, y se ajusta sin recompilar. La salpicadura que lo disimula es del ADR-0025.
- **Volver a cortar:** la salida de un corte es una entrada válida (pedazos cerrados y
  *manifold*, con el mismo formato), así que el pedazo vivo o uno en el suelo se pueden
  volver a cortar, incluida una herida abierta. Si el juego lo permite, y cuándo, lo
  decide el ADR-0018.
- **Los datos pasan por el ECS** (ADR-0012), como en el ADR-0015 y el ADR-0016: el corte
  lee las matrices de los huesos como componente, usa el skinning de la CPU que vive
  junto a `core`, y deja los pedazos como entidades nuevas. Los pedazos heredan la pose
  y la transformación anteriores del zombie (ADR-0015).
- **Determinismo:** los identificadores de los puntos de corte y de los vértices nuevos se
  asignan en el orden de los triángulos. Las tablas hash solo se consultan, nunca se
  recorren. El mismo paso da los mismos pedazos, y una partida grabada reproduce los
  mismos cortes (ADR-0020).
- **Tests (ADR-0011):**
  - **TDD y 100 % de cobertura** en todo el corte, con casos calculados a mano:
    - clasificar, incluido un vértice exactamente sobre el plano;
    - los casos de partir un triángulo (el solitario de cada lado, conservando el
      sentido de giro), incluido uno con un vértice sobre el plano: el punto de corte
      cae sobre ese vértice y queda un triángulo de área cero, sin abrir el pedazo;
    - el índice soldado, con una costura de UV;
    - encadenar contornos, incluido uno abierto (malla defectuosa: falla con un error,
      no se cuelga);
    - cortar orejas, con un agujero, un contorno invertido, uno de área cero y puntos
      alineados;
    - des-deformar (el vértice vuelve al plano), incluida una matriz casi singular (una
      torsión de casi 180° entre dos huesos con pesos 0,5/0,5: el resultado es finito,
      cae en el plano y el pedazo queda cerrado) y un hueso dominante con escala 0 (se
      interpola y el resultado es finito), y mezclar, recortar y cuantizar pesos (suman
      255);
    - los pedazos: un cubo da 2; una "U" con un hueso distinto en cada pata, cortada a
      través de las dos patas, da 3; dos cubos superpuestos con el mismo hueso dan 2, no
      4; un torso con hendidura se une con las piernas por sus huesos del lado de abajo,
      y no salen como pedazos aparte;
    - volver a cortar un pedazo ya cortado, incluida una herida abierta: el índice
      soldado extendido encadena bien los contornos nuevos, y ninguna arista de la boca
      de la herida queda con más de dos caras;
    - la hendidura: los estados de los puntos, el abanico en el fondo, la cuerda
      compartida, la cuña (0 en el fondo y signos opuestos en cada cara) y el contorno al
      que le falta un solo segmento;
    - qué cápsulas se recortan, con un corte que separa el antebrazo y deja una ranura en
      el torso;
  - **invariantes de malla** después de cada corte. Son las del ADR-0011, acotadas a lo
    que este ADR decide:
    - cada pedazo cerrado y orientado, y ninguna arista con más de dos caras;
    - ningún triángulo con índices repetidos. Los triángulos de área cero solo se toleran
      en dos casos, para que el pedazo quede cerrado: en las tapas de contornos
      degenerados (área cero, puntos repetidos o alineados) y al partir un triángulo con
      un vértice sobre el plano, o tan cerca que el punto de corte coincide con él;
    - en los cortes completos, en la pose del corte: cada vértice **original** del lado
      que le dio su clasificación (con el 0 del lado positivo), y cada vértice **nuevo** a
      menos de una tolerancia explícita del plano (ADR-0008), porque cae sobre él;
    - en los cortes completos, el volumen de los pedazos igual al original, medido antes
      de descartar los pedazos chicos. Con la hendidura y la cuña, el volumen no se
      conserva;
  - **determinismo:** cortar dos veces la misma malla con el mismo `slice_request` da los
    mismos índices, vértices y pedazos, byte a byte;
  - **pruebas de propiedades** con semilla fija: planos y rectas de detención al azar,
    incluidos planos que pasan justo por vértices;
  - **fuzzing** del corte con mallas y planos arbitrarios: nunca lee fuera de un buffer,
    y una malla inválida da un error;
  - **las mallas de prueba** son chicas y se arman en el código de los tests, como datos
    ya cocinados en memoria. No son archivos: la única excepción a "en el repo, solo la
    fuente" siguen siendo los fixtures del cocinador (ADR-0017);
  - **lo que hace el cocinador para el corte** (el índice soldado, las cáscaras, los
    huesos dominantes y la validación *manifold*) lleva TDD en la biblioteca del
    cocinador (`tests/tools/`, ADR-0017). La validación tiene casos a mano que hacen
    fallar el build: una arista abierta, una arista con tres caras y dos caras vecinas
    con la misma orientación.
- **Lo que se sacrifica:**
  - la tapa con LBS cruzado sobre sí mismo: en una articulación muy doblada, parte de la
    tapa puede quedar encimada o mirando hacia adentro. El pedazo sigue cerrado;
  - los triángulos degenerados: los contornos de área cero y las orejas forzadas (~26 %
    de los cortes al azar en el prototipo) dejan triángulos sin área o encimados en la
    tapa, y un vértice sobre el plano deja uno sin área al partir. El pedazo sigue
    cerrado;
  - el salto de pesos en un vértice de matriz casi singular, que pasa entero a su hueso
    dominante;
  - el volumen exacto en la herida: la cuña le quita material;
  - el fondo exacto de la ranura, que sigue las aristas de la malla;
  - los modelos de terceros con defectos, que hay que arreglar antes de usarlos;
  - el interior volumétrico: la tapa es una superficie, no un volumen de tetraedros.
- **Abierto:**
  - el ancho máximo de la cuña, probablemente atado al grosor de la hoja (ADR-0021 para
    ajustarlo);
  - la mecánica de la espada clavada: cuánta energía cuesta sacarla, qué hace el zombie
    mientras tanto y si se puede soltar la espada (ADR-0018). Su animación (ADR-0015 y
    ADR-0018);
  - cuándo se emite el pedido (al salir la hoja o al detenerse) y cuándo empieza la pausa
    de impacto, que deciden si los cortes que sobran caben en la pausa (ADR-0018);
  - cómo se dibuja la tapa por dentro y cómo se dibuja un pedazo desde dos buffers
    (ADR-0022);
  - el almacenamiento de los cortes que esperan, los pools de los pedazos y el tamaño del
    *ring buffer* (ADR-0010);
  - el mínimo de un pedazo y el umbral del determinante, que se fijan midiendo;
  - cómo se marca una malla que se corta, para que el cocinador sepa a cuáles aplicar lo
    de este ADR (tener esqueleto no basta: la caballera también lo tiene). Lo decide el
    ticket del cocinador; por ejemplo, junto a la lista de huesos que conserva cada rig;
  - si se exige MC/DC además de la cobertura de ramas en las funciones más críticas del
    corte, como clasificar y los estados de la hendidura (lo dejó abierto el ADR-0011).

## Cómo lo defiendo

**¿Por qué cortar la geometría en la CPU y no simularlo en el shader, como las heridas de
*Left 4 Dead 2*?**
Porque la idea es reconstruir la geometría real y crear caras nuevas en el zombie. En el
shader solo se esconden píxeles: la malla sigue entera, y la física no entraría en juego
cuando un pedazo tiene que caer. Tampoco habría una mitad que siguiera arrastrándose con
la animación.

**¿Cómo evitas que el redondeo de `float` deje agujeros o grietas en el corte?**
Cada vértice se clasifica una sola vez, y todos los triángulos usan esa misma respuesta.
Las grietas aparecen cuando dos triángulos vecinos no se ponen de acuerdo sobre de qué
lado está un mismo vértice. Si un vértice queda justo sobre el plano, una regla fija lo
manda al lado positivo, y cada punto de corte se calcula una sola vez por arista.

**glTF duplica los vértices en las costuras. ¿Cómo sabes qué triángulos son vecinos para
cerrar la tapa?**
El cocinador calcula un índice para identificar los triángulos vecinos: cada vértice
apunta al primero que tiene exactamente su misma posición. Así, dos triángulos separados
por una costura de textura siguen siendo vecinos para el corte.

**Cortas a un zombie agachado. ¿Por qué no interpolas la posición de referencia del
vértice nuevo, si es lo más simple?**
Porque con el zombie en pose, interpolar no es exacto. El vértice nuevo se guarda en la
pose de referencia para que se siga animando, y si lo interpolo ahí, al volver a
deformarlo se sale del plano: en el cuello de un zombie agachado llegó a 10 mm. Por eso
lo des-deformo: busco dónde tiene que estar en la pose de referencia para caer justo en
el punto del corte.

**El maniquí son 61 cáscaras sueltas. ¿Cómo sabes cuántos pedazos salieron de un corte?**
Por conectividad, cada cáscara suelta saldría como un pedazo propio: cada falange sería un
pedazo. Por eso, después de mirar la conectividad, junto las partes del mismo lado que
comparten su hueso principal. En el maniquí, eso deja 3 pedazos debajo de la cintura en
vez de 61.

**La hoja se queda clavada a mitad del torso. ¿Qué le pasa a la malla, y cómo se ve la
herida?**
La fuerza decide hasta dónde llega la hoja. Si no logra atravesar, se crea la geometría
nueva del interior del zombie sin separar esa sección, y sigue siendo un solo pedazo. La
ranura se abre en cuña, ancha en la boca y cerrada en el fondo, donde quedó la espada,
para que se vea el interior.

**¿Cómo pruebas que un corte quedó bien, si a ojo puede verse bien y estar roto?**
Con invariantes que se tienen que cumplir siempre: cada arista de un pedazo la comparten
exactamente dos triángulos, y en un corte completo los pedazos suman el volumen del
original. Lo probé con miles de planos al azar, y el único fallo vino de un defecto del
modelo. Por eso el cocinador rechaza las mallas que no son cerradas y *manifold*.

**¿Por qué los pedazos no copian sus vértices?**
Porque ya están en la GPU, en el modelo original, así que solo se suben los vértices
nuevos del borde y de la tapa, más la lista de triángulos de cada pedazo (y, según cómo
se dibuje, copias de algunos vértices del borde, ADR-0022). Copiar todo costaba 0,23 ms
y unos 290 KB más por corte.

**¿Por qué pesos `unorm8` y no `unorm16`?**
Porque el vértice tiene 32 B en total, y 4 bytes menos de pesos dejan lugar para la
normal y las coordenadas de textura. Además, una precisión de 1/255 alcanza para
personajes low poly.

**¿Por qué escribir tu propio "cortar orejas" en vez de usar earcut?**
Por aprendizaje, y porque es una dependencia menos: cada una necesita su ADR. Además, los
cortes medidos son chicos (de 58 a 183 puntos en total), así que la velocidad de earcut
no hace falta.

## Referencias

- Christer Ericson, *Real-Time Collision Detection*, Morgan Kaufmann (2004): partir
  polígonos contra un plano y robustez numérica.
- Herbert Edelsbrunner y Ernst Peter Mücke, *Simulation of Simplicity: A Technique to
  Cope with Degenerate Cases in Geometric Algorithms*, ACM Transactions on Graphics 9(1),
  1990: <https://arxiv.org/abs/math/9410209>
- Jonathan Richard Shewchuk, *Adaptive Precision Floating-Point Arithmetic and Fast
  Robust Geometric Predicates*, Discrete & Computational Geometry 18(3), 1997.
- David Eberly, *Triangulation by Ear Clipping*, Geometric Tools:
  <https://www.geometrictools.com/Documentation/TriangulationByEarClipping.pdf>
- Martin Held, *FIST: Fast Industrial-Strength Triangulation of Polygons*, Algorithmica
  30(4), 2001.
- Mapbox, earcut (ISC): <https://github.com/mapbox/earcut>
- Mark de Berg, Otfried Cheong, Marc van Kreveld y Mark Overmars, *Computational
  Geometry: Algorithms and Applications*, 3.ª ed., Springer (2008), capítulo 3.
- Mario Botsch, Leif Kobbelt, Mark Pauly, Pierre Alliez y Bruno Lévy, *Polygon Mesh
  Processing*, A K Peters (2010).
- Jun Wu, Rüdiger Westermann y Christian Dick, *A Survey of Physically Based Simulation
  of Cuts in Deformable Bodies*, Computer Graphics Forum 34(6), 2015:
  <https://diglib.eg.org/handle/10.1111/cgf12528>
- Alex Vlachos, *Rendering Wounds in Left 4 Dead 2*, GDC 2010:
  <https://www.valvesoftware.com/publications/2010/gdc2010_vlachos_l4d2wounds.pdf>
- Epic Games, `SliceProceduralMesh`:
  <https://dev.epicgames.com/documentation/unreal-engine/BlueprintAPI/Components/ProceduralMesh/SliceProceduralMesh>
- David Arayan, EzySlice (MIT): <https://github.com/DavidArayan/ezy-slice>
- *Slice SkeletalMesh* (plugin de Unreal):
  <https://www.fab.com/listings/9cd4f90f-f9be-40be-87cf-795b8f6033e5>
- Arseny Kapoulkine, meshoptimizer (*shadow index buffer*):
  <https://github.com/zeux/meshoptimizer>
- Rinat Abdrashitov y otros, *Robust Skin Weights Transfer via Weight Inpainting*,
  SIGGRAPH Asia 2023: <https://dl.acm.org/doi/fullHtml/10.1145/3610543.3626180>
- UploadVR, *Hands-On With Blade & Sorcery's Brutally Realistic VR Combat System*:
  <https://uploadvr.com/hands-on-with-blade-sorcerys-brutally-realistic-vr-combat-system/>
- *Metal Gear Rising: Revengeance*:
  <https://en.wikipedia.org/wiki/Metal_Gear_Rising:_Revengeance>
- *Dead Island 2* y FLESH: <https://en.wikipedia.org/wiki/Dead_Island_2>
- Quaternius, *Universal Animation Library* 2 (CC0):
  <https://quaternius.com/packs/universalanimationlibrary.html>
