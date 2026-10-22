# Visión del juego

- **Versión:** 3 (2026-10-22): la espada clavada como secuencia, el agarre, el
  cansancio que cancela la carga y la regla de vivo o muerto (#31). Versión 2
  (2026-10-20): se suma la hoja que queda clavada (#11). Versión 1: 2026-10-18.
- **Ticket:** #13 (`modo:dirigido`: las ideas son del autor y Claude las redacta)
- **Qué es este documento:** el diseño del juego. **No es un ADR.** Les da a las
  decisiones técnicas los números y las prioridades que necesitan (por ejemplo, cuántos
  zombies tiene que soportar el engine). Los números de rendimiento son propuestas, y
  los fija el ADR-0020.

## En una frase

Una caballera con armadura completa se abre paso con un mandoble a través de un castillo
lleno de zombies para rescatar a la princesa. **Cada tajo lo apuntas y lo cargas tú, y
corta la malla del zombie de verdad, por donde lo apuntaste.**

## Qué lo hace único

En la mayoría de los juegos de zombies se dispara o se golpea con animaciones fijas.
Aquí, **el ángulo, la fuerza y el punto del corte los decide el jugador**, y el cuerpo
del zombie se parte exactamente por ahí, en tiempo real.

La referencia de la sensación es Guts, en el manga *Berserk*, barriendo enemigos con su
espada. La referencia del *cómo se siente* es *Sniper Elite V2*: lo memorable no es el
disparo, sino lo que se ve después.

## Pilares de diseño

Un pilar sirve para decir que no: si una idea no respeta los pilares, espera.

1. **El tajo es el protagonista.** Precisión para el ángulo, fuerza para la penetración
   y una sensación de impacto que haga que el jugador diga **"¡Hell yeah!"** al ganar una
   pelea y termine **completamente satisfecho**. Si una feature no hace mejor el tajo,
   espera.
2. **Todo pesa.** El mandoble, la armadura, el trote, el salto en vez de la voltereta y
   la energía que se agota. Nada es gratis ni instantáneo.
3. **La horda es un rompecabezas.** Posicionarse para alinear zombies en un tajo cargado
   vale más que cortar rápido. Cada tipo de zombie obliga a cortar distinto.
4. **Oscuro, pero legible.** Un estilo PSX apagado, con el rojo como único color fuerte:
   en medio de la horda, siempre se sabe dónde se cortó.

## Premisa e historia

- **El accidente.** En una introducción corta, un mago convierte en zombies a casi todo
  el reino **por error**, con una poción. Él mismo termina convertido. Solo se salvan
  algunas personas.
- **El inicio.** Un soldado con armadura medieval completa mira hacia la torre más alta
  del castillo, donde está atrapada la princesa. Mientras la observa, aparecen los
  zombies, y tiene que ir a salvarla.
- **El recorrido.** Atraviesa el castillo y varias zonas a su alrededor, en orden y de
  forma lineal, hasta llegar a la torre.
- **El jefe final** es **el mago zombie**. Un **dragón zombie** es una posibilidad, no un
  compromiso.
- **El final.** Se revela que el soldado es una mujer, y que ella y la princesa son
  pareja. La rescata y escapan juntas a otro reino, al que la poción no llegó.

**El secreto de la protagonista se cuida en todo el juego:**

- el yelmo nunca se quita;
- la silueta bajo la armadura no la delata;
- no tiene voz, o su voz no la delata, hasta el final.

El tono es serio y oscuro, como *Berserk*, aunque el origen del desastre (un error) tiene
algo de tragicomedia.

## Combate

### Apuntar

- **Al mantener apuntar,** la cámara se acerca y se concentra, y el mouse o el stick
  derecho **dibujan la línea del tajo** en la pantalla. No hay un set fijo de golpes: el
  corte sigue el ángulo que eligió el jugador. Cómo se anima lo decide el ADR-0015.
- **Por dónde pasa el corte lo decide la cámara.** La línea pasa por el centro de la
  pantalla, así que el jugador apunta la cámara al cuello, a la cintura o a la junta de
  una armadura, y después elige el ángulo.
- **Apuntar es una habilidad, no un trámite.** La dificultad sale de que los zombies se
  mueven, de que la armadura por partes premia acertar en las juntas, y de que corregir
  cuesta energía.

### Cargar

- Después de apuntar, **mantener el ataque carga la fuerza**. Al soltar, sale el tajo.
- **La fuerza decide qué atraviesa la espada:** un zombie, un zombie con armadura, varios
  zombies en línea o varios con armadura. La precisión decide *por dónde* corta, y la
  fuerza *hasta dónde* llega.
- **La hoja puede quedar clavada.** Si la fuerza alcanza para entrar pero no para
  atravesar, la hoja se detiene dentro del zombie: puede cortar solo un brazo, o llegar
  hasta la mitad del torso y quedarse clavada. Donde se detuvo queda una herida abierta,
  en la que se ve el interior (la geometría la fija el ADR-0007).
- **La espada clavada es una secuencia de *quick time*.** El jugador presiona rápido y
  responde a eventos para terminar de cortar, y cada pulsación gasta energía. Mientras
  tanto, el zombie avanza ensartado por la espada hacia la caballera. Solo ellos dos van
  en tiempo real; los demás zombies quedan muy lentos o congelados, para que el jugador
  le dé todo el foco y no lo agarre otro, lo que sería injusto. Si termina, el zombie
  queda partido. Si el zombie la alcanza, o se queda sin energía, pasa a un agarre.
- **La carne no frena la entrada, el metal sí.** Bajo el umbral de una armadura, el tajo
  **rebota:** la caballera queda aturdida un momento y la energía se recupera más lento
  por un rato. Contra la armadura importa el ángulo: un tajo que cruza el hueso de
  costado entra mejor que uno que corre a lo largo de él. Cuando falla, la imagen y el
  sonido lo tienen que dejar claro. Las reglas exactas las fija el ADR-0018.
- **Mientras se carga, se puede corregir el ángulo, pero cuesta energía.** El costo es
  proporcional a cuánto se gira: un ajuste de 5° casi no cuesta, y uno de 90° vacía la
  energía. Además, **la línea gira más lento mientras se carga**, por el peso de la
  espada.
- **Cargar tiene costo:** gasta energía y deja vulnerable a la caballera. **No se puede
  esquivar mientras se carga.**
- **La carga sube rápido al principio y cada vez más lento:** un tajo corto alcanza
  contra los zombies que no piden tanta fuerza, y llegar a la carga máxima cuesta.
- **Cansancio:** si la energía llega a 0 mientras se carga, **la carga se cancela:** la
  caballera sale de la postura de corte y entra en recuperación, sin golpe. El jugador
  tiene que conocer bien sus tiempos.

### Alcance del tajo

- **Cada tajo lleva un paso adelante** de la caballera, que le da el peso de la espada.
- **El barrido cubre unos 180° frente a ella,** de derecha a izquierda o de izquierda a
  derecha. "Varios zombies en línea" significa **lado a lado, a lo largo del barrido**.
- El alcance es el largo de la espada más ese paso.

### El tajo rápido

Sin apuntar, el ataque es un **tajo rápido sin carga**. A veces ni siquiera corta: solo
**empuja** al zombie. Cumple el papel de una patada, que el juego no tiene. **El empuje
no hace daño real:** solo abre espacio. Corta solo lo que alcanza a atravesar entero
(por ejemplo, un brazo), y **nunca queda clavado.**

### Energía

Un solo recurso para todo: **cargar, corregir el ángulo, trotar, terminar un corte con
la espada clavada y soltarse de un agarre** la gastan, y se recupera sola cuando no se
usa. Correr y cargar compiten.

### Daño y derrota

- **La caballera no tiene barra de vida: su vida es la energía.** Los zombies no la
  golpean hasta matarla. **La agarran.**
- **Agarre:** un zombie que la alcanza la sujeta, y ella tiene que **sacudirse** para
  soltarse, presionando rápido y respondiendo a eventos, lo que gasta energía. **Si la
  energía llega a 0 antes de soltarse, muere.** Al soltarse, da un paso atrás y empuja.
  Si tenía la espada clavada en ese zombie, la hoja sale deslizándose y el zombie queda
  con la herida abierta, sin partir.
- **El agarre es en tiempo real para todos:** los otros zombies siguen atacando. Un
  zombie con espada podría cortar al que la agarra, y a ella también.
- **Varios zombies pueden agarrarla a la vez,** y cada uno más hace que la energía baje
  más rápido y que soltarse cueste más. Si lo logra, se libera de todos. Por eso las
  masas son peligrosas, aunque cada zombie por separado no lo sea.
- **El agarre cancela la carga,** pero si la caballera estaba cargando tiene un margen
  corto para soltar el tajo.
- **Los enemigos especiales usan sus propios ataques** (el soldado con espada, los
  perros, el mago…), definidos para cada uno.
- **El equilibrio:** si carga y se posiciona bien, la caballera mata casi cualquier cosa
  de un solo golpe. El peligro está en no tener tiempo ni espacio para hacerlo.
- **Al morir, se vuelve al último checkpoint.**
- **Accesibilidad:** presionar rápido y los eventos de *quick time* tienen en las opciones
  una alternativa de mantener.

### Moverse

- **Trotar**, no correr: la caballera lleva armadura completa.
- **Esquivar es un salto corto** hacia donde se mueve, no una voltereta. Se siente el
  peso de la espada y de la armadura.

### La sensación del corte

Es la prioridad absoluta del juego.

| Recurso | Qué hace | Cuándo |
|---|---|---|
| **Pausa de impacto** (*hit-stop*) | La caballera y el zombie golpeado se congelan de 50 a 80 ms, como en *Monster Hunter*; el resto sigue. Casi no se nota, pero da peso al golpe. La duración se configura, y en 0 se apaga si queda mejor sin ella | Una vez por tajo, más larga cuanto más fuerte |
| **Cámara del tajo** | Cámara lenta, y la cámara acompaña a la hoja mientras atraviesa el cuerpo. Se ve el interior: carne y hueso. Es el equivalente a la vista de rayos X de *Sniper Elite* | **Solo en los cortes especiales**, para que no aburra: muchos zombies en un tajo y el golpe final al mago. Se puede apagar en las opciones |
| **Salpicadura roja** | Sangre roja saturada al cortar | En cada corte |
| **Sacudida de cámara** | Proporcional a la fuerza | En los golpes cargados |
| **Pedazos con física** | Las mitades caen, y la de arriba se arrastra. Los pedazos del suelo se pueden volver a cortar, y los personajes los empujan sin trabarse | En cada corte |
| **Sonido del corte** | La mitad de la sensación, como en *Sniper Elite*. No es secundario | En cada corte |

**Posibilidad: frames de impacto estilo anime.** Son 2 o 3 frames de imagen invertida o
de alto contraste (siluetas en blanco, negro y rojo) justo en el contacto, un punto
medio entre la pausa de impacto y la cámara del tajo. Todavía no está definido cuándo se
usarían.

## Enemigos

| Enemigo | Qué lo distingue |
|---|---|
| **Campesino** | Lento. Es la horda base, con umbral de corte bajo |
| **Soldado con cota de malla** | Umbral medio. Un tajo sin carga solo lo empuja |
| **Soldado con armadura completa, sin espada** | Umbral alto. Obliga a cargar, o sea a exponerse |
| **Soldado con armadura y espada** | Umbral alto, y sabe usar la espada a su favor: bloquea o contraataca |
| **Zombie con escudo**, sin arma | Bloquea los tajos de frente. Hay que rodearlo o buscarle el ángulo |
| **Perros de caza zombies** | Bajos y rápidos. Obligan a cortar en diagonal o hacia abajo |
| **Caballero y caballo** | **Dos enemigos que funcionan por separado.** Si muere el caballo, el jinete sigue a pie. Si muere el jinete, el caballo sigue solo |
| **El rey y la reina** | Mini jefes |
| **El mago** | Jefe final |
| **El dragón** | Posibilidad |

Reglas que valen para todos. Son la **intención de diseño**: el ADR-0018 evalúa las
alternativas y fija las reglas exactas.

- **Todo se resuelve con cortes.** No hay vida acumulable: varios tajos que solo empujan
  no matan a nadie.
- **Umbral de corte:** cada zombie tiene un umbral de fuerza para partirlo. Bajo ese
  umbral, el tajo no entra: lo empuja o rebota.
- **Cualquier corte en la cabeza lo mata.** Como los zombies se mueven, acertarle a la
  cabeza es difícil.
- **Regla general para el resto:** mientras conserve **la cabeza completa, y siga unida
  al torso o conserve al menos un brazo o una pierna**, el zombie sigue: camina, cojea,
  se arrastra o se retuerce con lo que le queda. Si el jugador se pone creativo y le
  corta las piernas y los brazos, sigue vivo; también si le queda la cabeza con un brazo
  y casi nada de torso. Las reglas exactas las fija el ADR-0018.
- **La armadura va por partes** (yelmo, peto, grebas…), cada una con su umbral. **Las
  juntas lo bajan:** quien apunta bien necesita menos fuerza.
- **Un zombie partido sigue vivo** si cumple la regla anterior: si se corta por la
  cintura, la mitad de arriba (la que tiene la cabeza) se arrastra hacia la caballera. No
  es un tipo aparte: es una característica de todos.

## Dirección de arte

- **Estilo PSX oscuro con rojo.** Geometría low poly, vértices que tiemblan, texturas de
  pocos píxeles sin filtrar, niebla a distancia y una **resolución interna baja**
  escalada a la pantalla.
- **Paleta apagada:** piedra, ocre y gris verdoso. **El rojo es el único color
  saturado**, y la salpicadura roja del corte es parte de la identidad del juego.
- **Low poly, pero orgánico:** siluetas con forma (brazos, cabezas, armaduras), no
  cajas.
- **El interior del zombie es más rico** que un rojo plano: en la cara del corte se ven
  carne, hueso o vísceras. Los zombies con armadura muestran capas, con el metal por fuera
  y la carne por dentro.
- **Iluminación:** luz tenue y cálida de antorchas contra un ambiente frío, con sombras
  simples o ninguna. Es la intención visual: la técnica la elige el ADR-0022.
- Referencias: *Pseudoregalia* (nitidez del estilo PSX), *Signalis* y *Dusk* (PSX
  oscuro), y la página de *Berserk* (tono).

Por qué este estilo encaja con el engine, separando sus dos efectos:

- **Lo low poly** abarata el corte de mallas, que corre en la CPU, y la cantidad de
  vértices por zombie.
- **La resolución interna baja** abarata el trabajo por píxel y el ancho de banda de
  memoria. En una GPU integrada, que comparte la memoria con la CPU, ese suele ser el
  cuello de botella.

Juntos dejan margen para 50 zombies y unos 100 pedazos a la vez. Lo estético y lo
técnico apuntan al mismo lado.

## Cámara

Tercera persona, detrás de la caballera. Al apuntar se acerca un poco y, sobre todo,
cierra el campo de visión, como si se concentrara (al estilo de *God of War*).

| Parámetro | Normal | Apuntando |
|---|---|---|
| Distancia detrás de la caballera | 2,3 m | 2,0 m |
| Altura | 1,9 m | 1,9 m |
| Desplazamiento lateral | 0,5 m a la izquierda | 0,5 m a la izquierda |
| Campo de visión vertical | 51° | 30° |
| Cámara lenta al apuntar | no | no |

Estos valores salen de probar un prototipo, y son **el punto de partida, no
definitivos**. Tienen que poder cambiarse fácil (un archivo de configuración o un panel
de depuración), sin recompilar.

**Colisión con el escenario:**

- cuando una pared u otro objeto se interpone entre la cámara y la caballera, **la
  cámara se acerca a ella** en vez de atravesarlo;
- si la cámara queda muy cerca (a menos de ~1 m), **la caballera se vuelve
  semitransparente** para no tapar la pantalla.

Cómo se detecta la pared y cómo se suaviza el movimiento lo deciden el ADR-0016 (física)
y el ADR-0022 (render).

## Controles

| Acción | Teclado + mouse | Gamepad (Xbox / PlayStation) |
|---|---|---|
| Moverse | WASD | Stick izquierdo |
| Trotar (mantener) | Shift | Presionar el stick izquierdo (L3) |
| Mirar | Mouse o trackpad | Stick derecho |
| **Apuntar** (mantener) | Clic derecho o E | LT / L2 |
| Dirección del tajo, mientras se apunta | Mouse o trackpad | Stick derecho |
| **Tajo rápido** (sin apuntar) | Clic izquierdo o Espacio | RT / R2 |
| **Cargar y soltar** (apuntando) | Mantener clic izquierdo o Espacio, y soltar | Mantener RT / R2, y soltar |
| **Esquivar** (salto, hacia donde se mueve) | Q | B / Círculo |
| Interactuar (puertas, palancas) | F | A / Cruz |
| Pausa | Esc | Start / Options |

- **E y Espacio** existen como alternativa al clic, para jugar con trackpad.
- **Accesibilidad desde el inicio:** todos los controles se pueden reasignar, y la
  sensibilidad, los valores de cámara y los números de la carga (energía por grado,
  velocidad de giro) son configurables. Los del mouse y los del gamepad se ajustan por
  separado, porque el stick es menos preciso.

## Alcance del corte vertical

Es la demo que prueba que todo funciona junto, con calidad final. Es el **corte vertical
objetivo**: se llega a él con varios cortes verticales más chicos, cada uno con su tag
(ADR-0014). **Es para el autor:**
algo que jugar antes de dedicarle mucho tiempo a construir el mundo. Que otros la jueguen
es un bonus, así que no hacen falta una build para Windows ni un tutorial. **Lo único que
realmente importa en ella es que cortar se sienta increíble y no aburra.** Todo lo demás
está al servicio de eso.

- **Lugar:** una arena de ruinas, tipo coliseo, vacía y simple de construir.
- **Ritmo:**
  1. una primera oleada con los tipos clásicos: campesinos, soldados con cota de malla y
     soldados con armadura completa;
  2. una horda que crece hasta ser demasiado, y **termina al matar una cantidad
     determinada** de zombies;
  3. la pelea final contra el mago.
- **Checkpoints** entre las fases: al morir, se vuelve al inicio de la fase.
- **La pelea contra el mago:**
  - **invoca zombies** durante toda la pelea;
  - **solo un tajo de máxima fuerza lo aturde.** Un golpe más débil no lo detiene;
  - **se defiende invocando un zombie con armadura máxima** que se interpone y bloquea.
    Hay que deshacerse del guardaespaldas, o adelantarse a él, para conectar el golpe;
  - el golpe final, el que activa la cámara del tajo, se le da mientras está aturdido.
- **Fuera del corte vertical:** los soldados con espada y con escudo, los perros, el
  caballero y su caballo, el rey y la reina, el dragón, la historia completa y el resto
  de las zonas.

Números propuestos para el engine (los fija el ADR-0020):

| Medida | Propuesta |
|---|---|
| Zombies en pantalla en la primera oleada | 10 a 15 |
| Zombies en pantalla en el pico de la horda | 50 |
| Zombies partidos por un tajo cargado | hasta 4 |
| Tajos por segundo, como máximo | ~2, o sea hasta 8 cortes de malla por segundo |
| Pedazos en el suelo a la vez | ~100. Los más viejos desaparecen |
| Duración de la demo | 5 a 10 minutos |
| Rendimiento | 60 fps estables en la GPU integrada del autor (Intel UHD 620) |

## Qué le exige cada pilar al engine

| Pilar | Qué exige | Dónde se decide |
|---|---|---|
| 1. El tajo es el protagonista | Corte de mallas en tiempo real sin picos, umbrales de fuerza, cámara del tajo que muestra el interior, pausa de impacto, sonido y salpicadura | ADR-0007 (corte), ADR-0018 (combate y desmembramiento), ADR-0022 (render del interior), ADR-0024 (audio), ADR-0025 (VFX) |
| 2. Todo pesa | Una espada que sigue el ángulo del jugador, sin un set fijo de golpes, y pedazos con física | ADR-0015 (animación), ADR-0016 (física) |
| 3. La horda es un rompecabezas | 50 zombies con comportamientos distintos a la vez, y agarres de varios zombies sobre la caballera | ADR-0023 (IA y navegación), ADR-0018 (agarres) |
| 4. Oscuro, pero legible | Estilo PSX a baja resolución, con el rojo legible a distancia | ADR-0022 (pipeline de render) |

## Abierto

- **Cómo se siente apuntar la espada** en detalle: lo afinan el ADR-0018 y el ADR-0015.
- **Cuándo se usan los frames de impacto** estilo anime, si se usan.
- **El dragón** como enemigo.
- **La cara del corte por dentro:** su geometría (triangulación y coordenadas) la genera
  el ADR-0007, y cómo se dibuja (carne, hueso, capas de metal y carne) lo decide el
  ADR-0022.
- **Valores configurables sin recompilar** (cámara, controles, carga): el ADR-0021
  (herramientas de depuración).
- **Checkpoints:** el ADR-0028 (recursos en runtime y guardado).

## Referencias

- Kentaro Miura, *Berserk* (manga): el tono y la sensación de Guts contra los enemigos.
- *Metal Gear Rising: Revengeance* (2013): cortes libres apuntados por el jugador.
- *Die by the Sword* (1998) y *Half Sword*: espada controlada por el jugador, sin golpes
  grabados.
- *Sniper Elite V2* (2012): la cámara de rayos X y la sensación del impacto.
- *God of War* (2018): la cámara que se acerca al apuntar.
- *Pseudoregalia* (2023), *Signalis* (2022) y *Dusk* (2018): estética PSX.
