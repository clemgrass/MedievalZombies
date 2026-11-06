# ADR-0031: Sistema de cámara

- **Estado:** Aceptado
- **Fecha:** 2026-11-03
- **Ticket:** #73
- **Reemplaza / Reemplazado por:** —
- **Fe de erratas:** 2026-11-06 (#74): el ADR-0032 decide el input. **El stick derecho
  marca una posición, no una velocidad:** al apuntar, el stick a fondo es el borde del
  cono y soltarlo devuelve el punto al rumbo; al cargar, el ángulo del stick es el ángulo
  de la línea. El mouse mueve el mismo objetivo con sus deltas, en grados por cuenta. Así
  que la guía de la espada recibe **un objetivo** (el punto en el cono, el ángulo de la
  línea), no "girar tantos grados", y la espada lo sigue con su peso (ADR-0015).
  **Respecto de qué se mueve la caballera:** tanque en la tercera y la primera persona
  (arriba avanza; izquierda y derecha giran el rumbo); en la fija y los rieles, relativo
  a la pantalla, manteniendo la dirección mientras el stick no cambie, aunque la toma
  corte. La capa de input lo convierte en una dirección del mundo antes del paso, con la
  cámara del último frame, así que **la cámara sigue fuera de la simulación.** Mientras
  apunta, la caballera camina muy despacio, sin girar el rumbo. Se corrige en "Qué no
  decide este ADR" y en Consecuencias.
- **Fe de erratas:** 2026-11-06 (#76): el ADR-0034 decide la cámara de las cinemáticas.
  **Sale de Blender, horneada a 30 Hz, y queda fuera de la simulación** como las demás:
  se calcula una vez por frame, desde el tiempo interpolado. **Entra con un corte seco.**
  **Desde el marcador `salida` hasta el final, se mezcla con *smoothstep* hacia la cámara
  del estilo que el jugador eligió,** calculada en vivo; así, al terminar, no queda nada
  que mezclar. El cocinador barre ese tramo en los tres estilos (con la esfera de 0,2 m
  de la Decisión 6) y falla si cruza el escenario o pasa a menos de 0,5 m de la
  caballera. Se corrige en "Qué no decide este ADR".

## Contexto y problema

Hay que decidir **cómo funciona la cámara**: quién la controla, desde dónde mira, qué
parte vive en la simulación y qué parte en el render, cómo se suaviza, cómo evita las
paredes, qué hace en el tajo especial y cómo se sacude.

Hasta ahora nadie es dueño de la cámara, pero la cámara decide cosas de la simulación. La
visión fija un rig en tercera persona que mueve el jugador (2,3 m detrás, 0,5 m a la
izquierda, FOV de 51° que se cierra a 30° al apuntar) y dice que **el corte pasa por el
centro de la pantalla**. El ADR-0018 lo convirtió en regla: **el punto apuntado es el
primer impacto del rayo central de la cámara** contra las cápsulas, y de ahí sale el plano
del tajo. El ADR-0023 usa la cámara del paso anterior para dar ventaja a los zombies que
están en pantalla. Así, la cámara es una entrada de la simulación, y una partida grabada
solo se repite si la cámara se repite.

**La idea del autor** para este ADR:

- **La mejor cámara es la que mejor muestra el corte.** Es el criterio que más pesa, en el
  juego y en la cámara del tajo.
- **No hay fijar objetivo** (*lock-on*): no tenerlo es parte de la dificultad. Apuntar es
  una habilidad (visión).
- **El ratón o el stick derecho guían la espada** (idea del autor al decidir este ADR).
  Cuando no ataca, la caballera es animación. Al atacar, la espada parte apuntando según
  el ratón o el stick, y el jugador la sigue moviendo mientras ataca y carga. Ese
  movimiento es procedural (ADR-0015), **siempre restringido a lo que puede hacer un
  cuerpo humano con la espada en las dos manos**, y es **lento y preciso**. **El
  procedural no puede sentirse flácido ni "espagueti": el peso se mantiene.**
- El autor no se marea con cámaras que se mueven solas; el mareo pesa por la
  accesibilidad, no por él.
- Investigar una cámara **fija o guiada por el nivel, como la del *God of War* original**
  (2005), y **la primera persona como posibilidad**: las cámaras de los prototipos de los
  ADR-0022 y ADR-0025 eran, en la práctica, primera persona sin la espada, y al autor le
  gustó cómo se veían.

Varios ADRs ya aceptados dejaron preguntas para este:

| ADR | Qué dejó abierto |
|---|---|
| Visión | El rig en tercera persona y sus valores, la colisión con el escenario, la caballera semitransparente, la cámara del tajo, la sacudida "proporcional a la fuerza" |
| ADR-0012 | La cámara pide sus consultas (si hay una pared) con una llamada directa a la física. El render interpola el estado y la cámara |
| ADR-0015 | El arco del tajo se mide dentro del plano, desde el pecho (fe de erratas del ADR-0018) |
| ADR-0016 | Las consultas de rayo y de barrido de esfera, "para la cámara que se acerca" |
| ADR-0018 | El punto apuntado sale del rayo central de la cámara. Qué cuenta como corte especial (activa la cámara del tajo) y la cámara lenta |
| ADR-0021 | Los valores de ajuste de la cámara son CVars; las opciones del jugador (apagar la cámara del tajo) no. El *replay* graba el input por paso |
| ADR-0022 | La caballera en *screen-door* cuando la cámara se acerca. **Los rayos X**, de los que depende ver el interior mientras la hoja cruza |
| ADR-0023 | La ventaja de las fichas de ataque usa la cámara del paso anterior |
| ADR-0024 | El paneo del sonido sale de la orientación de la cámara, después del render |
| ADR-0025 | **La sacudida.** Cómo se ven los *frames* de impacto dentro de la cámara del tajo |
| ADR-0026 | Las opciones de la cámara en el menú (la sacudida, la cámara del tajo) |
| ADR-0027 | Si el nivel coloca cámaras, sus zonas o rieles serían marcadores de la zona |

**Qué no decide este ADR:**

| Tema | Dónde |
|---|---|
| Las curvas, la zona muerta y la sensibilidad del mouse y del stick, y cómo entra el input al paso | ADR-0032 (#74). Aquí la guía de la espada (la dirección del rayo y el giro de la línea) recibe un objetivo: el stick marca una posición y el mouse lo mueve con sus deltas (fe de erratas, ADR-0032) |
| La cámara de las cinemáticas y sus transiciones (también si mezcla dos poses de cámara) | ADR-0034 (#76). Decidido: entra con un corte seco y su última toma termina mezclándose con la cámara del estilo del jugador (fe de erratas) |
| Las reglas de qué cuenta como corte especial | ADR-0018 |
| Cómo se corta la malla (también el corte parcial, hasta donde llega la hoja) | ADR-0007 |

Hay que decidir:

1. **quién controla la cámara:** el jugador, el nivel o una mezcla;
2. **la perspectiva:** tercera o primera persona, o una mezcla;
3. **de dónde sale el rayo de apuntar** y cómo es la transición a apuntar;
4. **qué vive en el paso fijo y qué se interpola** en el render;
5. **el suavizado;**
6. **la colisión con el escenario** y la caballera semitransparente;
7. **la cámara del tajo:** encuadre, recorrido, cámara lenta y vuelta;
8. **los rayos X:** ver el interior mientras la hoja cruza;
9. **la sacudida;**
10. **la accesibilidad, los datos, la depuración, las pruebas y el costo.**

## Criterios

En orden de peso:

1. **Ver el corte** (pilar 1, idea del autor): cuánto de la tapa y del objetivo se ve, sin
   que la caballera o la espada lo tapen.
2. **Apuntar con precisión** (pilar 1): cuántos píxeles mide una junta en el punto
   apuntado.
3. **Leer la horda** (pilar 3): cuántos zombies se ven y cuántos atacan desde fuera de
   cuadro.
4. **Determinismo del *replay*** (ADR-0012, ADR-0016, ADR-0021).
5. **Mareo y accesibilidad** (XAG 117, Game Accessibility Guidelines, ADR-0026).
6. **Costo de autoría por nivel,** para una sola persona.
7. **Encajar en el frame y en el presupuesto** (ADR-0020, ADR-0022).
8. **Patrones reconocibles en la industria.**
9. **Aprendizaje.**

## Opciones

### 1. Quién controla la cámara

| Opción | Cómo funciona | Medido | Problema |
|---|---|---|---|
| **El jugador** (visión) | Órbita libre detrás de la caballera; el ratón o el stick derecho la giran | Tapa: 441 px de media tras el corte. Junta: 1,85 px/cm | La caballera tapa el 10 % del objetivo al soltar. Hay zombies cerca fuera de cuadro el 93 % del tiempo |
| **El nivel: fija por zona** (*God of War*, 2005) | El diseñador pone una cámara por zona; solo gira para seguir | Tapa: 139 px. Junta: 0,55 px/cm (una junta de 10 cm mide ~6 px). Amenazas fuera de cuadro: 0 % | **El rayo central ya no apunta:** hay que rehacer el apuntado (un cursor en pantalla) y la regla del ADR-0018. Una cámara por zona para colocar y probar |
| **El nivel: riel** | La cámara se desliza por un recorrido puesto por el diseñador | Tapa: 61 px. Junta: 0,66 px/cm. Amenazas: 0 % | Lo mismo que la fija, y además un recorrido por zona |
| **El jugador, guiada en el tajo** | La libre, y al soltar un tajo cargado se mueve sola a un encuadre del corte, y vuelve | Tapa: **1 526 px** (×3,5 la libre). Apuntado igual que la libre | La cámara se mueve sola un momento. Hay que elegir el encuadre en cada tajo |
| **Un estilo que elige el jugador en las opciones** | Varias cámaras (tercera persona, fija y rieles, primera persona), ninguna movida por el jugador | Cada estilo mide lo suyo (arriba y en la dimensión 2) | El apuntado no puede depender de la cámara. Las cámaras fijas y los rieles se colocan en todas las zonas, aunque el jugador elija otro estilo |

El *lock-on* queda fuera por decisión del autor.

### 2. La perspectiva

| Opción | Medido | Problema |
|---|---|---|
| **Tercera persona** | Ver lo de arriba (libre) | La caballera ocupa el 9,7 % de la pantalla al soltar |
| **Primera persona** | Tapa: **2 931 px**, la que más. Junta: 2,93 px/cm. La caballera no tapa nada | Zombies cerca fuera de cuadro el 100 % del tiempo, **hasta 13 a la vez.** No se ve a la caballera (la postura, que es parte del HUD diegético del ADR-0026). El ojo va en la cabeza, que hay que ocultar |
| **Mixta: primera persona al apuntar** | Tapa: 1 027 px. Junta: 2,93 px/cm | El paso de 0,3 s entre una y otra cambia mucho la imagen dos veces por tajo |

### 3. El rayo de apuntar

| Opción | Cómo funciona | Problema |
|---|---|---|
| **El rayo central de la cámara** (visión, ADR-0018) | El punto apuntado es el primer impacto del rayo que sale del centro de la pantalla | Exige que la cámara sea del jugador |
| **Un cursor en pantalla** | El jugador mueve una mira; el rayo sale de la cámara y pasa por la mira | Necesario con la cámara fija o el riel. Cambia la visión y el ADR-0018 |
| **Desde la caballera** | El rayo sale del pecho; al apuntar, el ratón o el stick derecho mueven su dirección, y una mira en el mundo marca dónde pega | Hay que dibujar la mira en el mundo. Cambia la visión y el ADR-0018. Funciona igual con cualquier cámara |

**La transición a apuntar** (visión): de 2,3 a 2,0 m y de 51° a 30°, con un resorte.
Con la cámara guiada, el encuadre del corte empieza **al soltar**, cuando el plano del
tajo ya quedó congelado (ADR-0016), así que no cambia el apuntado.

### 4. Qué vive en el paso fijo y qué se interpola

| Opción | Cómo funciona | Medido | Problema |
|---|---|---|---|
| **Todo en el frame** | La cámara se mueve y se suaviza con el dt del frame; el rayo sale de la cámara que se dibuja | El *replay* corta 11–22 mm más allá a otra tasa de frames (P5) | Rompe el ADR-0021: la partida grabada no se repite |
| **Todo en el paso** | La cámara se mueve en el paso fijo; el render la interpola como el resto | El mismo punto a 30, 60, 144 FPS e irregular; el mismo hash con GCC y Clang | La sacudida y la cámara del tajo también entrarían al hash sin necesidad |
| **Dos cámaras: la de juego en el paso, la de presentación en el render** | La cámara de juego (la orientación que da el input, la distancia, el FOV y la colisión) vive en el paso y decide el rayo y la ventaja del ADR-0023. La de presentación sale de ella en el render: interpola entre los dos últimos pasos y suma el encuadre guiado, la cámara del tajo y la sacudida | Lo mismo que "todo en el paso" para el rayo; la sacudida no lo mueve (0,00 cm, P8) | Hay que mantener las dos separadas: nada de la de presentación puede volver a la simulación |
| **Toda en el render** | Si nada de la simulación lee la cámara (ni el rayo ni la ventaja del ADR-0023), la cámara es solo presentación: se calcula una vez por frame desde el estado interpolado | El *replay* no depende de la cámara: se puede mirar con cualquier estilo | Exige que el apuntado y la ventaja del ADR-0023 salgan de la caballera, no de la cámara. Los resortes corren con el dt del frame: tienen que ser de forma cerrada (P6) |

### 5. El suavizado

| Opción | Medido (hasta el 95 % de un salto) | Problema |
|---|---|---|
| **`lerp` fijo por frame** | 633 ms a 30 FPS, 132 ms a 144 | Depende de los FPS (casi ×5) |
| **`lerp` exponencial** | 300–306 ms | Arranca con un salto de velocidad |
| **Resorte críticamente amortiguado** (Lowe, *Game Programming Gems 4*) | 479–500 ms con ω = 10 | Un parámetro más que entender. Forma cerrada: no depende del dt |
| **Sin suavizar la orientación** | — | El giro del ratón no debería tener retraso: lo que se suaviza son la distancia, el FOV, la vuelta de la colisión y los encuadres |

### 6. La colisión con el escenario

| Opción | Medido en el pasillo (660 frames) | Problema |
|---|---|---|
| **Ninguna** | 26 frames dentro de la pared | — |
| **Rayo** (ADR-0016) | 0 dentro; **33 con el plano cercano cortando la pared** | Se ve la pared por dentro en las esquinas |
| **Esfera** de 0,2 m (el doble de la media diagonal del plano cercano), de golpe al acercarse y lenta al alejarse | 0 y 0; salto máximo de 58 cm | El salto al doblar una esquina |
| **Esfera con resorte rápido** al acercarse (ω = 22), sin pasar el límite de una esfera de 0,1 m | 0 y 0; salto máximo de 44 cm | Un parámetro más |

Contra qué choca: **solo el escenario estático.** Si chocara contra los zombies, la
cámara saltaría con cada uno que pasa. **La caballera en *screen-door*** cuando la cámara
queda a menos de ~1 m ya está decidida (ADR-0022).

### 7. La cámara del tajo

Sale en los cortes especiales (ADR-0018) y se puede apagar (visión).

| Opción | Medido (tapa en px, media / máx) | Problema |
|---|---|---|
| **Acompaña la hoja** | **1 923 / 4 180**, la que más | La cámara va muy cerca y rápido: el encuadre depende del arco del tajo |
| **Órbita** alrededor del corte | 598 / 1 644 | Gira 110° en 7,7 s: lo que más se mueve |
| **Plano fijo de cine** | 666 / 1 562 | No sigue a la hoja |

**La cámara lenta** del prototipo es ×0,02 mientras la hoja cruza (17,7 ms de simulación
se ven en ~3,9 s) y ×0,2 durante 0,9 s después. **La vuelta** es un corte seco a la cámara
de juego.

### 8. Los rayos X

Mientras la hoja cruza, la tapa todavía no existe (ADR-0022): los pedazos no se separaron.

| Opción | Medido (tapa mientras cruza) | Problema |
|---|---|---|
| **Ninguno**: el interior se ve en la tapa cuando la hoja sale | 82–93 px | Durante la cámara súper lenta no se ve el interior |
| **Cápsulas**: la piel en *screen-door*, los huesos adentro (las cápsulas del ADR-0016, como en *Sniper Elite V2* o los *X-ray* de *Mortal Kombat*, 2011) | 93 px de tapa, más los huesos | Muestra el esqueleto, no el corte. ~20 cilindros más por objetivo |
| **La herida que se abre** (bisagra): la tapa se ve solo donde ya pasó la hoja, y el pedazo de arriba se abre | **371 px (×4)** | Sutil desde el ángulo de la hoja: hay que afinar el ángulo. Cada paso necesita saber hasta dónde llegó la hoja, como el corte parcial del ADR-0007 |

### 9. La sacudida

| Opción | Problema |
|---|---|
| **Al azar en cada frame** | Tiembla en vez de golpear |
| **Trauma con ruido suave** (Eiserloh, GDC 2016): el golpe suma trauma, la sacudida es trauma², y el ruido es continuo | Hay que elegir el decaimiento y la amplitud |
| **Solo rotación o también traslación** | La traslación puede meter la cámara en una pared |

En las dos, **el rayo de apuntar queda donde estaba** si la sacudida se suma solo a la
cámara de presentación (0,00 cm de diferencia, P8).

### 10. Accesibilidad, datos, depuración, pruebas y costo

- **Accesibilidad:** apagar la sacudida y la cámara del tajo (visión, XAG 117), y quizás el
  encuadre guiado; ajustar el FOV (XAG 117, Game Accessibility Guidelines). La
  sensibilidad y la inversión de ejes son del ADR-0032.
- **Datos:** con la cámara libre, el nivel no coloca cámaras (costo de autoría cero). Con
  la fija o el riel, cada zona lleva sus cámaras como marcadores (ADR-0027).
- **Valores de ajuste:** CVars (ADR-0021).
- **Depuración:** una cámara libre de depuración que no toca la simulación (ADR-0021).
- **Pruebas y costo:** si una cámara de juego viviera en el paso, sería núcleo (TDD,
  ADR-0011); si vive en el render, el resorte y el barrido siguen siendo matemática del
  núcleo. Su costo es una consulta de esfera y unos resortes; no se midió.

## Investigación

### Prototipo medido

Un tajo con guion visto con cada cámara, sobre el render del prototipo del ADR-0022, más
un programa sin GPU para el determinismo, medidos en la máquina de referencia el
2026-11-03. Assets CC0 de Quaternius, fuera del repo. El detalle y cómo repetirlo están
en [`../research/prototipo-de-camara.md`](../research/prototipo-de-camara.md). Un video
de 127 s compara las 17 variantes.

| # | Qué | Resultado |
|---|---|---|
| P1 | El tajo con guion | Corte diagonal del hombro a la cadera; la hoja cruza en 17,7 ms |
| P2 | Las cámaras de juego | Libre, fija, riel, guiada, primera persona y mixta |
| P3 | Apuntar | Junta: 1,85 px/cm (libre), 0,55–0,66 (fija y riel), 2,93 (primera persona). La caballera tapa el 10 % del objetivo al soltar (libre) |
| P4 | La horda | Amenazas fuera de cuadro: 0 % (fija y riel), 93 % (libre), 100 % y hasta 13 a la vez (primera persona) |
| P5 | Determinismo | Cámara en el paso: el mismo punto a cualquier tasa. En el frame: 11–22 mm de diferencia |
| P6 | Suavizado y colisión | `lerp` por frame ×5 entre 30 y 144 FPS; resorte, igual. Esfera: 0 frames con el plano cercano en la pared; rayo: 33 |
| P7 | Cámara del tajo y rayos X | Acompañar la hoja muestra más tapa (1 923 px de media). La bisagra, ×4 de tapa mientras la hoja cruza |
| P8 | Sacudida | El rayo de apuntar no se mueve si la sacudida es solo de presentación |
| P9 | Video | 17 clips rotulados |

Lo que mostró, además de los números:

- **La cámara que más muestra el corte no es la que mejor lee la horda.** La primera
  persona muestra 6,6 veces más tapa que la libre y deja hasta 13 zombies cerca fuera de
  cuadro; la fija lee toda la horda y muestra la tapa en ~140 píxeles.
- **La cámara puede cambiar solo durante el tajo.** La guiada es la libre todo el tiempo,
  salvo en el segundo y medio del corte, y triplica la tapa.
- **Lo que decide el rayo tiene que vivir en el paso.** Lo demás (encuadres, cámara del
  tajo, sacudida) puede vivir en el render sin tocar el *replay*.
- **El hueco del tajo es el instante del cruce:** con cámara súper lenta son ~200 frames
  en los que el corte todavía no existe.

### Cómo lo hacen otros

- ***God of War*** (2005): cámaras puestas por el diseñador, fijas o en rieles por zona;
  el jugador no las mueve. ***God of War*** (2018) pasó a una cámara al hombro, cercana y
  del jugador. El combate se rehízo para esa cámara: Mihir Sheth, "Evolving Combat in
  *God of War* for a New Perspective", GDC 2019.
- ***Metal Gear Rising: Revengeance*** (2013): el modo espada pone la cámara al hombro, en
  cámara lenta, y el stick derecho elige el ángulo del corte. Es el antecedente más
  cercano del tajo apuntado de este juego.
- ***Sniper Elite V2*** (2012) y ***Mortal Kombat*** (2011): cámaras de rayos X que
  muestran huesos y órganos en el golpe, en cámara lenta.
- **John Nesky, "50 Camera Mistakes",** GDC 2014 (la cámara de *Journey*): las cámaras se
  notan cuando fallan; el mareo hay que probarlo con jugadores.
- **Squirrel Eiserloh, "Math for Game Programmers: Juicing Your Cameras With Math",**
  GDC 2016: la sacudida por *trauma* (de 0 a 1, que decae; la sacudida es una potencia del
  trauma) con ruido suave, y el suavizado de cámaras.
- **Mark Haigh-Hutchinson, *Real-Time Cameras*** (Morgan Kaufmann, 2009): el libro de
  referencia sobre cámaras de juego, con la colisión, los rieles y las transiciones.
- **Unreal Engine, `USpringArmComponent`:** la colisión es un barrido de esfera
  (`bDoCollisionTest`, `ProbeSize`) y el retraso de la cámara es aparte
  (`CameraLagSpeed`).
- **Unity Cinemachine, *Deoccluder*:** "Pull Camera Forward" acerca la cámara hasta
  delante del obstáculo; `DampingWhenOccluded` (por defecto 0) y `Damping` (0,2) separan
  la velocidad de acercarse de la de volver.
- **Accesibilidad:** XAG 117 pide evitar o poder apagar la sacudida y dejar ajustar el FOV;
  las Game Accessibility Guidelines piden un FOV por defecto adecuado a cómo se juega.

## Decisión

La cámara es una opción del jugador, con tres estilos, y **en ninguno el jugador mueve la
cámara**: tercera persona por detrás (por defecto), cámaras fijas y rieles puestos por
zona, y primera persona. Como la cámara cambia según la opción, **nada de la partida
puede depender de ella**: el apuntado sale de la caballera, con el ratón o el stick
derecho, y la cámara queda fuera de la simulación. Elijo la cámara que mejor muestra el
corte para el momento que más importa: **en los tres estilos, la cámara del tajo acompaña
la hoja**, y mientras la hoja cruza la herida se abre para que se vea el interior.

| # | Dimensión | Elección |
|---|---|---|
| 1 | Quién controla | **Nadie: el jugador elige un estilo en las opciones** y ninguno lo mueve él. **Tres estilos:** tercera persona por detrás (por defecto), fija y rieles por zona, y primera persona. No hay *lock-on* |
| 2 | Perspectiva | **Cada estilo, la suya. Tercera persona:** sigue el rumbo de la caballera con un resorte, con los valores de la visión (2,3 m detrás, 1,9 m de altura, 0,5 m a la izquierda, FOV de 51°); al apuntar, 2,0 m y 30°. **Mientras apunta, la cámara se queda con el rumbo que tenía al empezar a apuntar**, para que la mira se mueva sobre una imagen quieta. **Fija y rieles:** el sector por el que camina la caballera decide la cámara. Cada cámara es un marcador de la zona (ADR-0027) con su volumen: **una fija** gira con un resorte para seguir a la caballera; **un riel** es una curva (en el glTF, una cadena de *empties*: ADR-0017), y la cámara va al punto de la curva más cercano a la caballera. **Entre dos tomas, corte seco.** Al apuntar no cambian. **Primera persona:** el ojo en la cabeza de la caballera, con la cabeza y el cuello ocultos (sus huesos colapsados a un punto); mira hacia el rumbo del cuerpo, **sin el balanceo de la cabeza**; FOV de 70°; **al apuntar, sin zoom** |
| 3 | Apuntado | **No depende de la cámara.** El rayo sale del pecho de la caballera; **al apuntar, el ratón o el stick derecho guían la espada** y con ella la dirección del rayo, dentro de un cono alrededor del rumbo, y **una mira en el mundo** marca dónde pega. **Por fases, con los mismos dos ejes: al apuntar, el ratón o el stick mueven el punto; al cargar, la dirección del rayo queda fija respecto del rumbo y giran la línea** (más lento y con costo de energía, como dice la visión). **Lo fijo es la dirección, no un punto del mundo, y el rayo se vuelve a lanzar al soltar** (ADR-0018). **La guía termina al soltar:** el plano se congela al empezar el barrido (ADR-0016). El movimiento es el de un cuerpo con una espada pesada en las dos manos: **lento, preciso y con peso**, nunca flácido (ADR-0015). **Todo lo que da esa sensación es configurable sin recompilar** (CVars, ADR-0021), para ajustarlo jugando cuando exista la mecánica. El punto apuntado es el primer impacto contra las cápsulas (la consulta de rayo del ADR-0016); si no toca nada, el punto a la distancia del alcance (ADR-0018). El plano del tajo sigue pasando por el pecho y el punto apuntado (ADR-0018): ahora contiene el rayo, y la línea es su giro alrededor del rayo. **El cono cabe en la pantalla de la primera persona**, para que la mira nunca quede fuera |
| 4 | Paso o render | **La cámara queda fuera de la simulación.** Nada del paso fijo la lee: el apuntado sale de la caballera, y **la ventaja de las fichas del ADR-0023 pasa a ser un cono alrededor del rumbo de la caballera** (fe de erratas). La cámara se calcula **una vez por frame, desde el estado interpolado**, antes del render; el audio la usa después (ADR-0024). **Un *replay* se puede mirar con cualquier estilo** |
| 5 | Suavizado | **Resorte críticamente amortiguado de forma cerrada** (Lowe), con el dt del frame: la distancia, el FOV, el seguimiento del rumbo, el giro de la fija, el riel, la vuelta de la colisión. **Ningún `lerp` fijo por frame** (P6: ×5 entre 30 y 144 FPS) |
| 6 | Colisión | Solo en la tercera persona. **Barrido de esfera de 0,2 m** (el doble de la media diagonal del plano cercano) **contra el escenario estático**, no contra los personajes. **Se acerca con un resorte rápido** (ω = 22), sin pasar nunca el límite de una esfera de 0,1 m, **y se aleja lento** (ω = 4). **La caballera en *screen-door*** a menos de ~1 m (ADR-0022). Las fijas y los rieles los valida el cocinador (no pueden quedar dentro del escenario) |
| 7 | Cámara del tajo | **Acompaña la hoja, en los tres estilos.** Va del lado de la normal del plano, con la mitad de la hoja, y cuando la hoja sale se queda en el corte. **Cámara lenta: ×0,02 mientras la hoja cruza** (con 30 ms de margen) **y ×0,2 durante 0,9 s después.** **Vuelve con un corte seco** a la cámara del estilo. Se puede apagar (visión) |
| 8 | Rayos X | **La herida que se abre:** mientras la hoja cruza, la tapa se ve solo del lado por donde ya pasó la hoja (un semiplano en el shader), y el pedazo de arriba gira alrededor del punto del borde más alejado del lado sin barrer, hasta 22°. **El giro es solo de la presentación:** se suma en el render a la transformación del pedazo, no toca su cuerpo físico ni entra al hash; cuando la hoja sale, el giro vuelve a 0 **con un resorte críticamente amortiguado** (Decisión 5; su ω es una CVar), no de golpe, y el pedazo sigue con su física desde su pose real. No hay huesos semitransparentes |
| 9 | Sacudida | **Trauma con ruido suave** (Eiserloh): el golpe cargado suma trauma según su fuerza, el trauma decae, y la sacudida es trauma² por un ruido continuo. **Solo rotación,** para no meter la cámara en una pared. Es de la presentación: **no mueve el rayo de apuntar** (P8). Se puede apagar |
| 10 | Accesibilidad, datos y pruebas | **Opciones del jugador** (ADR-0026, guardadas por el ADR-0028): el estilo, el FOV de la tercera y de la primera persona, **cuánto se cierra el FOV al apuntar en cada estilo** (hasta 0; por defecto, el de la visión en la tercera persona y 0 en los otros dos), la sacudida y la cámara del tajo. **Los valores de ajuste son CVars** (ADR-0021), salvo esas opciones del jugador, que van en `opciones.txt`; su valor por defecto sí es una CVar (fe de erratas del ADR-0021). **Una cámara libre de depuración**, que no toca la partida (ADR-0021). **Datos:** las fijas y los rieles son marcadores de la zona; el cocinador valida que **todo el piso de la navmesh tenga una cámara**. **Pruebas:** el resorte y el barrido, con TDD (son matemática del núcleo, ADR-0011); el sistema de cámara, con tests después |

## Consecuencias

- **El camino de un frame** (ADR-0012):

  ```
  pasos fijos (0..N) ─→ interpolar el estado ─→ cámara del estilo ─→ render ─→ audio
                                                (o del tajo)
                                                + colisión (solo lectura)
                                                + sacudida
  ```

  Y el del apuntado, dentro del paso:

  ```
  input (ratón o stick derecho) ─→ dirección del rayo en el cono ─→ rayo desde el pecho
  ─→ punto apuntado ─→ plano del tajo (ADR-0018) ─→ barrido (ADR-0016)
  ```
- **Lo que se gana:**
  - el jugador elige la cámara que le sirve sin que la partida cambie: el *replay* da el
    mismo hash con cualquier estilo;
  - la mejor vista del corte, en el momento del corte, en los tres estilos;
  - el apuntado se aprende una vez y vale en todas las cámaras;
  - la cámara es presentación pura: se puede iterar sin tocar la simulación.
- **Lo que se sacrifica:**
  - **las cámaras fijas y los rieles se colocan y se prueban en cada zona,** aunque el
    jugador elija otro estilo. El cocinador valida que cubran el piso, pero el encuadre
    se juzga a ojo;
  - **tres estilos que probar** en cada zona y en cada pelea;
  - **apuntar desde el pecho necesita una mira en el mundo,** porque el centro de la
    pantalla ya no dice nada. Con la fija o un riel, una junta mide ~6 píxeles (P3):
    apuntar es más difícil en ese estilo;
  - **la primera persona deja zombies cerca fuera de cuadro** (P4: hasta 13 a la vez), y
    no se ve la postura de la caballera, que es parte del HUD diegético del ADR-0026;
  - **la cámara lenta del tajo cambia el tiempo de la simulación.** Si el jugador la
    apaga, la partida cambia: el encabezado del *replay* guarda esa opción (fe de erratas
    del ADR-0021).
- **Lo que pide al combate (ADR-0018), y queda como fe de erratas:**
  - **el punto apuntado sale del rayo desde el pecho,** no de la cámara;
  - **la cámara del tajo tiene que empezar antes de que la hoja entre,** y la herida que
    se abre necesita los pedazos mientras la hoja cruza. Pero hoy el pedido del corte se
    emite al salir la hoja, y "muchos zombies en un tajo" se sabe al final del barrido.
    **Por eso, al soltar, el combate predice el barrido entero** contra las poses de ese
    paso (una consulta de solo lectura, determinista). Si la predicción cumple las reglas
    de corte especial, **el tajo especial se resuelve en ese paso** con la predicción:
    los pedidos de corte salen al soltar, y la animación de la hoja lo recorre en cámara
    lenta. Los tajos normales siguen como están. **Cuándo y cómo se predice exactamente
    queda por descubrir** al implementarlo (Abierto).
- **Fe de erratas en otros ADRs.** Al aceptar este ADR se corrigieron, como fe de
  erratas (ADR-0014, regla 11):
  - **la visión:** la sección Cámara pasa a los tres estilos; "por dónde pasa el corte lo
    decide la cámara" pasa al rayo desde el pecho; al apuntar, el ratón o el stick mueven
    el punto, y al cargar, la línea; la fila "Mirar" de los controles pasa a "apuntar la
    espada"; el Abierto de la cámara del tajo;
  - **ADR-0018:** el punto apuntado desde el pecho, la mira en el mundo, el cono, las
    fases (punto al apuntar; al cargar, la dirección fija respecto del rumbo y la línea),
    el rayo que se vuelve a lanzar al soltar, y el tajo especial resuelto al soltar con
    la predicción del barrido;
  - **ADR-0016:** la predicción del tajo especial reutiliza la prueba sector contra
    cápsula de la Decisión 7 sobre el arco completo, como consulta de solo lectura; la
    cámara usa el barrido de esfera desde el render;
  - **ADR-0023:** la ventaja de las fichas usa un cono alrededor del rumbo de la caballera,
    no la cámara;
  - **ADR-0012:** la cámara no es un sistema de la simulación: corre una vez por frame,
    después de interpolar, y su consulta de pared es de solo lectura;
  - **ADR-0022:** los rayos X se cierran con la herida que se abre (el semiplano de la
    tapa en el shader y el giro del pedazo, solo del render, que vuelve con un resorte);
    el *screen-door* de la caballera queda igual;
  - **ADR-0025:** la sacudida es la de este ADR; **los *frames* de impacto, dentro de la
    cámara del tajo, duran 2 o 3 frames reales,** no de simulación;
  - **ADR-0026:** el menú suma las opciones de cámara (estilo, FOV, cierre al apuntar,
    sacudida, cámara del tajo); apagar la cámara del tajo cambia la simulación, así que se
    graba como evento del paso; la mira está en el mundo; **la ayuda de energía va por
    defecto en la espalda solo con la tercera persona,** y junto a la mira con la primera
    persona y con las fijas y los rieles, que no siempre ven la espalda;
  - **ADR-0027:** las fijas y los rieles son marcadores de la zona, con sus volúmenes;
  - **ADR-0017:** el cocinador lee las cámaras y los rieles del glTF exportado de la zona,
    como *empties* (un riel es una cadena de *empties*, porque glTF no tiene curvas, y el
    volumen de cada uno es la escala de un *empty*), valida que no queden dentro del
    escenario y que cubran todo el piso de la navmesh (cómo se marcan en Blender se
    decide al implementarlo);
  - **ADR-0021:** el encabezado del *replay* guarda si la cámara del tajo está apagada;
    el FOV y el cierre al apuntar son opciones del jugador, con su valor por defecto como
    CVar;
  - **ADR-0015:** el 90° del arco no cambia (ya es la dirección del pecho al punto
    apuntado). **Suma la guía de la espada mientras apunta y carga:** es procedural, con
    los dos brazos en la empuñadura, limitada por lo que puede hacer el cuerpo, lenta y
    con peso. Cómo se logra el peso es del ADR-0015, con todos sus valores como CVars.
- **Respecto de qué se mueve la caballera** sin una cámara del jugador (decidido en el
  ADR-0032): **tanque en la tercera y la primera persona;** en la fija y los rieles,
  **relativo a la pantalla, manteniendo la dirección** al cambiar de toma mientras el
  stick no cambie. La capa de input lo convierte en una dirección del mundo antes del
  paso, con la cámara del último frame: la simulación nunca lee la cámara.
- **Tickets, en orden:**
  1. el resorte críticamente amortiguado y el barrido de esfera de la cámara, en TDD;
  2. la cámara en tercera persona, con la colisión y el *screen-door*;
  3. el apuntado desde el pecho, con la mira en el mundo (junto al ADR-0018);
  4. la cámara del tajo que acompaña la hoja, con la cámara lenta (junto al ADR-0018);
  5. la herida que se abre (junto a los ADR-0007 y ADR-0022);
  6. la sacudida con trauma;
  7. la primera persona;
  8. las fijas y los rieles como marcadores, con las validaciones del cocinador (junto a
     los ADR-0017 y ADR-0027);
  9. las opciones de cámara en el menú (junto al ADR-0026);
  10. la cámara libre de depuración (junto al ADR-0021).
- **Abierto:**
  - **los valores de partida,** que se ajustan jugando: el resorte del rumbo, el cono de
    apuntado, la cámara lenta (×0,02 y ×0,2), la apertura de la herida (22°), el trauma
    y su decaimiento, y el cono de la ventaja del ADR-0023. **Todos son CVars,** igual
    que los de la guía de la espada (su resorte, su velocidad máxima, los límites del
    cuerpo y el ángulo con que empieza la línea al cargar): **la sensación se ajusta
    jugando cuando exista la mecánica,** no en papel;
  - **cuándo y cómo se predice el barrido del tajo especial:** contra qué poses, qué
    pasa si un zombie se mueve durante la cámara lenta, y cómo se reparte la predicción
    entre los zombies del tajo (ADR-0018);
  - **cómo sobrevive un riel a la exportación:** una cadena de *empties* (ADR-0017); cómo
    se marca y en qué orden se recorre la cadena se decide al implementarlo, y entre los
    puntos la curva puede ser una poligonal o un *spline* (Catmull-Rom);
  - **la herida que se abre se ve sutil** desde el ángulo de la hoja (P7): hay que
    afinar el lado y el ángulo en el corte vertical;
  - **el encuadre de la cámara del tajo cuando el corte especial atraviesa varios
    zombies**: el prototipo cortó uno;
  - **el costo de la cámara** no se midió: es un barrido de esfera y unos resortes por
    frame.

## Cómo lo defiendo

**¿Por qué tres estilos de cámara y no uno?**
Porque ninguna cámara gana en todo, y el prototipo lo midió. La primera persona muestra
6,6 veces más corte que la tercera, pero deja hasta 13 zombies cerca fuera de cuadro.
Las fijas leen toda la horda, pero el corte ocupa unos 140 píxeles. Cada estilo sirve a
un jugador distinto, y como la cámara quedó fuera de la simulación, elegir uno no cambia
la partida: es una opción, como el tamaño del texto.

**¿Por qué el jugador nunca mueve la cámara?**
Porque así sus manos quedan para la espada. El ratón y el stick derecho, que en otros
juegos giran la cámara, aquí guían la espada. Y una cámara que el jugador no controla es
una cámara que puedo diseñar: en las fijas y los rieles, el encuadre lo decido yo, como
en el *God of War* original.

**Si la cámara no decide por dónde pasa el corte, ¿qué lo decide?**
Un rayo que sale del pecho de la caballera. Al apuntar, el ratón o el stick lo mueven y
una mira en el mundo marca dónde pega; al cargar, la dirección queda fija y el mismo
ratón gira la línea; al soltar, el rayo se vuelve a lanzar, así que sigue al zombie si
se movió. Así apuntar se aprende una vez y vale igual con cualquier cámara. Y el
plano del tajo ya pasaba por el pecho (ADR-0018): ahora además contiene el rayo, y la
línea es el giro del plano alrededor de él.

**¿Por qué la cámara está fuera de la simulación?**
Porque si la simulación la leyera, el estilo que elige el jugador cambiaría la partida.
Lo medí: con la cámara en el frame, un *replay* a otra tasa de frames corta entre 11 y
22 mm más allá, y una junta chica mide un par de centímetros. Por eso saqué de la cámara
lo que la simulación usaba: el apuntado sale del pecho, y la ventaja de las fichas de
ataque (ADR-0023) usa un cono alrededor del rumbo de la caballera. Ahora un *replay* se
puede mirar con cualquier estilo y da el mismo hash.

**¿Por qué resortes críticamente amortiguados y no un `lerp`?**
Porque la cámara corre con el dt del frame, y un `lerp` fijo por frame depende de los
FPS: en el prototipo tardó 633 ms en llegar al 95 % a 30 FPS y 132 ms a 144. El resorte
de forma cerrada (Lowe, *Game Programming Gems 4*) da lo mismo con cualquier dt, y además
arranca y frena suave, que es lo que se siente como una cámara con peso.

**¿Por qué una esfera para la colisión y no un rayo?**
Porque el rayo deja la cámara fuera de la pared, pero no su plano cercano: en las
esquinas del pasillo del prototipo se veía la pared por dentro en 33 frames. La esfera,
del doble de la media diagonal del plano cercano, no lo dejó pasar nunca. Es lo mismo
que hacen el `SpringArm` de Unreal y el *Deoccluder* de Cinemachine. Se acerca rápido,
para no atravesar nada, y se aleja lento, para no dar saltos.

**¿Por qué la colisión choca solo contra el escenario y no contra los zombies?**
Porque con una horda alrededor, la cámara saltaría con cada zombie que pasa por detrás.
Si un zombie queda entre la cámara y la caballera, se ve que está ahí, que es
información útil.

**¿Por qué la cámara del tajo acompaña la hoja?**
Porque es la que más muestra el corte, que es lo que más importa: en el prototipo vio
1 923 píxeles de tapa de media, el triple que la órbita o el plano fijo. Va del lado de
la normal del plano, que es donde queda la cara del corte, y cuando la hoja sale se
queda mirando la herida.

**¿Qué son "la herida que se abre" y por qué no unos rayos X como los de *Sniper Elite*?**
Mientras la hoja cruza el cuerpo, los pedazos todavía no se separan, así que la tapa no
se ve. Con la cámara súper lenta, eso son unos 200 frames sin ver el interior. La
herida que se abre muestra la tapa solo donde ya pasó la hoja y entreabre el pedazo de
arriba: en el prototipo, cuatro veces más tapa en ese instante. Los rayos X de huesos
muestran el esqueleto, no el corte, que es lo que quiero que se vea.

**¿Por qué el tajo especial se resuelve al soltar?**
Porque la cámara del tajo tiene que empezar antes de que entre la hoja, y la herida que
se abre necesita los pedazos mientras la hoja cruza. Pero "muchos zombies en un tajo"
solo se sabe al terminar el barrido. Al soltar, el combate predice el barrido entero, que
es determinista porque el plano ya quedó congelado, y si el tajo es especial se resuelve
en ese paso. Cuándo y cómo se predice exactamente todavía lo tengo que descubrir.

**¿Por qué la sacudida es solo de rotación y solo de la presentación?**
De rotación, porque mover la posición puede meter la cámara en una pared. De la
presentación, porque así no toca el apuntado: en el prototipo, con sacudida, el rayo de
apuntar pasó a 0,00 cm del punto, igual que sin ella. Es el modelo de *trauma* de
Eiserloh: el golpe suma trauma, la sacudida es trauma² por un ruido suave, y se siente
como un golpe y no como un temblor.

**¿Cómo sé que guiar la espada no se va a sentir flácido?**
No lo sé todavía, y no se decide en papel. Lo que sí decidí es la regla: la guía es la
de un cuerpo con una espada pesada en las dos manos, lenta, precisa y limitada por lo
que el cuerpo puede hacer, y todos sus valores son CVars. Cuando exista la mecánica, la
ajusto jugando hasta que pese.

## Referencias

- Visión: [`../../vision/README.md`](../../vision/README.md), secciones Combate y Cámara.
- ADR-0012, ADR-0016, ADR-0018, ADR-0021, ADR-0022, ADR-0023, ADR-0025, ADR-0026,
  ADR-0027.
- Nota de investigación: [`../research/prototipo-de-camara.md`](../research/prototipo-de-camara.md).
- Mihir Sheth, "Evolving Combat in *God of War* for a New Perspective", GDC 2019.
  <https://gdcvault.com/play/1026085/Evolving-Combat-in-God-of>
- John Nesky, "50 Camera Mistakes", GDC 2014. <https://gdcvault.com/play/1020460/50-Camera>
- Squirrel Eiserloh, "Math for Game Programmers: Juicing Your Cameras With Math", GDC 2016.
  <https://gdcvault.com/play/1023146/Math-for-Game-Programmers-Juicing>
- Thomas Lowe, "Critically Damped Ease-In/Ease-Out Smoothing", *Game Programming Gems 4*,
  Charles River Media, 2004.
- Mark Haigh-Hutchinson, *Real-Time Cameras*, Morgan Kaufmann, 2009.
- Epic Games, `USpringArmComponent`.
  <https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/USpringArmComponent>
- Unity, Cinemachine Deoccluder.
  <https://docs.unity3d.com/Packages/com.unity.cinemachine%403.1/manual/CinemachineDeoccluder.html>
- Microsoft, Xbox Accessibility Guideline 117: Visual distractions and motion settings.
  <https://learn.microsoft.com/en-us/gaming/accessibility/xbox-accessibility-guidelines/117>
- Game Accessibility Guidelines, field of view.
  <https://gameaccessibilityguidelines.com/if-the-game-uses-field-of-view-3d-engine-only-set-an-appropriate-default-for-the-expected-viewing-environment/>
- Quaternius, *Universal Animation Library* 1 y 2 (CC0), solo para el prototipo.
  <https://quaternius.com/packs/universalanimationlibrary2.html>
