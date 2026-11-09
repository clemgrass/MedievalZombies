# ADR-0034: Cinemáticas

- **Estado:** Aceptado
- **Fecha:** 2026-11-06
- **Ticket:** #76
- **Reemplaza / Reemplazado por:** —
- **Fe de erratas:** 2026-11-09 (#77): la historia del juego ya no tiene la intro corta
  del mago y la poción: el juego abre en la torre, y el mago pasa a ser el alquimista.
  **El dragón se confirma** como secuencia interactiva: donde este ADR dice "si el
  dragón existe", ya existe. **La protagonista no habla nunca, tampoco al final:** a lo
  sumo hace sonidos, y responde con gestos. Eso cierra el pendiente de su voz. Ninguna
  decisión cambia. Las cinemáticas que pide la historia están en
  `docs/vision/historia.md`.

## Contexto y problema

Hay que decidir **cómo se hacen las escenas de "película"** (la intro, encontrar a la
princesa, la aparición de un jefe, el final) y **cómo se pasa de ellas al juego y de
vuelta**. Ningún ADR ni la visión las mencionan, y varios ya aceptados dejaron cosas para
este:

| ADR | Qué dejó para este |
|---|---|
| Visión | La historia: una intro corta (el mago y la poción), el inicio frente a la torre, el final que revela quién es la protagonista. **El secreto se cuida:** el yelmo nunca se quita y ella no habla, o su voz no la delata, hasta el final |
| ADR-0012 | El paso fijo de 60 Hz, la pausa como "el frame corre 0 pasos" y las capas de módulos. Qué corre en la simulación mientras hay una cinemática |
| ADR-0017 | Los clips de los personajes salen de Blender, muestreados a 30 Hz |
| ADR-0018 | La escala de tiempo por entidad y el *quick time* de la hoja clavada. El caso del dragón (abajo) pide una ventana de tiempo |
| ADR-0021 | El *replay* graba las acciones de cada paso; Dear ImGui solo en los builds de desarrollo |
| ADR-0024 | Los eventos de sonido del paso, con una latencia fija; la música con *crossfade* |
| ADR-0026 | "Las cinemáticas usan los subtítulos de este ADR": la tabla de textos por idioma, el estilo y el tamaño de los subtítulos, la pila de pantallas y la pausa |
| ADR-0027 | "Las cinemáticas son del ADR-0034." Las fases y los triggers son listas cerradas; Blender es el editor del nivel |
| ADR-0028 | Los binarios por zona y el global; los checkpoints y los bits de las fases terminadas |
| ADR-0031 | "La cámara de las cinemáticas y sus transiciones." La cámara queda fuera de la simulación; tres estilos que el jugador elige (tercera persona, fija y rieles, primera persona) |
| ADR-0032 | Los contextos de input apilados: el juego, la secuencia y el menú |

**El caso de referencia es una idea del autor, no una decisión** (la visión dice que el
dragón es una posibilidad): al final, la caballera escapa con la princesa y aparece un
dragón zombie. El jugador no se puede mover; el dragón carga una bola de fuego, y lo único
que se puede hacer es cargar el tajo, sin límite de energía. Si suelta antes, el dragón
ataca; si suelta tarde, la bola pega; **si suelta justo (un *parry*), corta en tiempo
real la bola y al dragón en dos, con un tajo vertical obligado.** Es una cinemática
interactiva: cámara de autor, entrada restringida y una ventana de tiempo. Este ADR la
tiene que hacer posible, aunque no decide si el dragón existe.

**La idea macro del autor:** una cinemática es **quitarle la cámara al jugador** en
momentos puntuales, para mostrar algo (el inicio, el final, la presentación de un
personaje). El final del dragón ocurre **dentro del juego, en tiempo real**, quizás con
movimientos de cámara especiales.

Hay que decidir:

1. **en el engine o en video;**
2. **cómo se crean:** con qué herramienta y en qué formato;
3. **el modelo en runtime:** las pistas y de qué reloj avanzan;
4. **la simulación durante la cinemática:** qué corre y qué se detiene;
5. **las cinemáticas interactivas;**
6. **las transiciones de cámara** del juego a la película y de vuelta;
7. **dónde viven y qué las dispara;**
8. **el audio y los subtítulos;**
9. **saltar, pausar y el estilo** (las bandas, la resolución, el look);
10. **el determinismo, las pruebas, la depuración y el costo.**

Lo que ya está decidido y limita este ADR:

- **ADR-0009 y ADR-0010:** sin excepciones ni RTTI; nada asigna memoria en el frame;
  regiones reservadas al iniciar.
- **ADR-0011:** TDD estricto en el núcleo y en el cocinador.
- **ADR-0012:** un hilo, paso fijo de 60 Hz con acumulador, capas estrictas
  (`platform` → `core` → `gpu` → sistemas → `game`).
- **ADR-0016 y ADR-0021:** la misma partida grabada da el mismo hash.
- **ADR-0020:** 60 fps en la UHD 620, con 16,7 ms de CPU y de GPU por frame; hasta
  6,0 ms por paso de simulación.
- **ADR-0022:** el render es uno solo, a 640×360 con el look PSX.
- **ADR-0031:** la cámara se calcula una vez por frame, desde el estado interpolado, y no
  entra al hash.
- **ADR-0032:** cada evento de input va al paso que cubre su *timestamp* (la regla C).
  **Esc y Start/Options siempre abren la pausa.**
- **`CLAUDE.md`:** todo gratis; cada dependencia nueva necesita un ADR.

**Fuera de alcance:**

| Qué | Lo decide |
|---|---|
| La historia, el guion y qué cinemáticas hay | El documento de historia (#77) |
| Si el dragón existe, su ventana de *parry*, la carga sin energía y el tajo vertical obligado | La visión y el ADR-0018 |
| Cortar algo del tamaño del dragón o una bola de fuego | ADR-0007 y ADR-0025 |
| Cómo se animan los personajes (los clips) | ADR-0015 y ADR-0017 |
| La cámara del tajo | ADR-0031 |

## Criterios

Los cinco del ticket, más cuatro que salen de lo ya decidido:

1. **Que la produzca una sola persona.**
2. **Continuidad visual con el juego:** el mismo look PSX.
3. **Transiciones sin cortes bruscos.**
4. **Determinismo del *replay*** (ADR-0016, ADR-0021).
5. **Aprendizaje:** poder explicar cada pieza.
6. **Reutilizar lo que existe** (la animación, la cámara, el audio, la UI) en vez de un
   sistema paralelo.
7. **Cabe en el frame y en la memoria** (ADR-0010, ADR-0020).
8. **El corte sigue siendo la estrella:** una cinemática interactiva corta de verdad, en
   tiempo real.
9. **Práctica de la industria.**

## Opciones

Los números son de un prototipo medido en la máquina de referencia (ver Investigación).

### 1. En el engine o en video

| Opción | Cómo funciona | Problema |
|---|---|---|
| **En el engine** (*Metal Gear Solid*, *Uncharted 4*) | Las mismas mallas, la misma luz y el mismo render, en tiempo real | Las cinemáticas están limitadas a lo que el engine dibuja |
| **Video pregrabado** (*Final Fantasy VII*) | Se renderiza afuera y se reproduce. Para el juego, un decodificador como `pl_mpeg` (MPEG-1, MIT, un header) | P6: **de 7 a 22 MB por minuto** y de 0,9 a 2,1 ms de CPU por frame. No puede ser interactivo; no muestra el estado de la partida (los cortes, los pedazos) ni el estilo de cámara del jugador; una dependencia nueva con su ADR |
| **Mixto** | Video para la intro y el final, engine para el resto | Las dos cosas, y un salto de look entre ellas |

### 2. Cómo se crean

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Todo en Blender** | La NLA, las cámaras, el FOV y los marcadores de la línea de tiempo, exportados en modo `SCENE` (P1) | Los marcadores y el cambio de cámara **se pierden** en el glTF (hay que copiarlos a los `extras`); el FOV necesita `KHR_animation_pointer`. **Se encuadra sin el render del juego:** sin la luz, la niebla ni los 640×360 |
| **Texto escrito a mano** | Un archivo con las claves de cámara y los eventos, como el texto de la zona del ADR-0027 | Escribir coordenadas de cámara a mano, sin ver la toma |
| **Un editor propio dentro del engine** (como el *Sequencer* de Unreal o el *Timeline* de Unity) | Una línea de tiempo en ImGui (solo en los builds de desarrollo): las tomas se encuadran con la cámara libre, en el render real; guarda un texto que el cocinador valida | **Es una herramienta que hay que construir** (pistas, claves, arrastrar, deshacer); el ADR-0027 había evitado un editor propio para el nivel |
| **Híbrido** | Los clips de los personajes, en Blender (ADR-0017); la secuencia (las tomas, qué hace cada actor, los eventos), en el editor | Dos herramientas, pero cada una hace lo suyo |

### 3. El modelo en runtime

**Un secuenciador:** pistas (la cámara, cada actor, los subtítulos, los sonidos, los
eventos de la partida) que se evalúan en un tiempo `t`. Es el modelo de Unreal, Unity y
Godot. La pregunta es **de qué reloj sale `t`**:

| Opción | Cómo funciona | Problema |
|---|---|---|
| **La suma del dt del frame** | `t += dt_frame` | P2: **a 30 y a 144 FPS cambian 19 o 20 de los 20 hashes de control; a 60 FPS, 1 de 20 sin ruido en el dt y 20 de 20 con ±20 %**, y los eventos se corren hasta ±1 paso |
| **El número de paso** | `t = pasos · 1/60`; la cámara, entre pasos, con el α del ADR-0012 | Ninguno medido: el mismo hash a 30, 60 y 144 FPS |

### 4. La simulación durante la cinemática

| Opción | Cómo funciona | Problema |
|---|---|---|
| **El mundo sigue** | La IA, la física y el combate corren; la cinemática se superpone | Los zombies atacan mientras el jugador no controla. **Saltar no puede ser instantáneo y exacto a la vez** (P2: el salto directo da 0 de 172; adelantar los pasos, ~7 s para 20 s de cinemática) |
| **Pausa total** (0 pasos, como la pausa) | Nada corre, ni el secuenciador | Los actores tampoco se mueven: no sirve |
| **El mundo se detiene; solo corren el secuenciador y sus actores** | Los actores son títeres de sus pistas; la física, la IA y el combate no corren | Lo que estaba en el aire (un pedazo) queda quieto mientras dura. P2: **el salto directo es exacto** (172 de 172) y gratis |

### 5. Las cinemáticas interactivas

El caso del dragón **no es una cinemática que se mira: es gameplay con una cámara de
autor.** Las opciones son tratarlo como una cinemática con excepciones, o como **un
segundo tipo:** una secuencia en la que el mundo corre, el input se restringe con el
contexto "secuencia" del ADR-0032 y no se puede saltar. **Las ramas** (éxito o muerte)
pueden ser un grafo dentro de la cinemática o **secuencias encadenadas** según el
resultado.

**La ventana del *parry*** (P5): con la regla C, el veredicto solo se equivoca en los
bordes, a menos de un paso (16,7 ms), y no depende de los FPS; leer el evento en el borde
del frame duplica el error a 30 FPS. Guardar el instante dentro del paso lo bajaría unas
8 veces.

### 6. Las transiciones de cámara

| Opción | Cómo funciona | Problema (P3) |
|---|---|---|
| **Corte seco** | Se cambia de cámara en un frame | De ida, es la gramática del cine: la primera toma presenta la escena. **De vuelta**, se pasa de mirar de frente a la caballera a mirarla de espaldas |
| **Mezcla lineal** (Cinemachine, Unreal) | Posición lineal, giro por *slerp*, FOV lineal, durante N s | **No sabe nada del mundo:** la vuelta pasa a 0,10–0,14 m del eje de la caballera (la atraviesa); con la cámara fija, cruza un pilar |
| **Resorte** | La cámara persigue a la activa con el resorte del ADR-0031 | Llega tarde a la toma de autor y no termina de asentarse en 3 s |
| **Recorrido** (órbita) | Interpola ángulo, radio y altura alrededor de la caballera | Nunca la atraviesa (≥1,49 m), pero necesita un sujeto |
| **Empalme** (*Uncharted 4*, *God of War*) | **La última toma termina en la cámara del jugador:** en su tramo final se mezcla hacia la cámara del estilo, y al terminar no queda nada que mezclar | 0 °/s al devolver el control. El recorrido lo diseña el autor, así que puede rodear lo que haga falta, pero hay que validarlo para cada estilo |

### 7. Dónde viven y qué las dispara

- **Dónde:** dentro del binario de su zona (ADR-0028), o en el global si la usan varias
  zonas. P6: horneada desde Blender, pesa ~1,5 MB por minuto por personaje de 65 huesos
  (~0,5 MB con los ~23 que deja el ADR-0017). Si los actores solo nombraran clips que ya
  existen (los de los personajes viven en el global), serían unos KB.
- **Qué la dispara:** un trigger (la caballera entra en un volumen) o una fase (empieza la
  pelea con el jefe). Hoy las dos son listas cerradas del ADR-0027.
- **Que no se repita:** al morir, se vuelve al checkpoint (ADR-0028). Si el checkpoint
  queda después de la cinemática, no se vuelve a ver.

### 8. El audio y los subtítulos

Los subtítulos pueden ser texto dentro de la cinemática o **ids de la tabla de textos
del ADR-0026**, que ya está por idioma. Los sonidos, **eventos del paso** con la latencia
fija del ADR-0024 (el error es de un paso como máximo) o una pista de audio continua
sincronizada con el reloj del audio.

### 9. Saltar, pausar y el estilo

- **Saltar:** nunca, solo después de verla una vez, o **siempre**. Con un botón o
  **manteniendo** (XAG 116 lo usa de ejemplo), para no saltar sin querer.
- **Pausar:** Esc y Start/Options ya abren la pausa (ADR-0032); la pausa puede ofrecer
  "Saltar", como *Ratchet & Clank*.
- **Las bandas** a 640×360 (P4): sin bandas; **1,85:1, de 7 px**, una marca discreta, con
  los subtítulos sobre la imagen; **2,39:1, de 46 px**, en las que entra una línea de
  subtítulos de ×2, pero no dos.
- **El render:** el mismo del juego, con el look PSX (dimensión 1).

### 10. Determinismo, pruebas, depuración y costo

- **Saltar tiene que dejar la misma partida que verla entera.** Con el mundo detenido,
  saltar es: los actores a su última clave y los eventos de la partida que faltan, en
  orden; los de la presentación (sonidos, subtítulos) se descartan. Es el
  *Fire Events when Jumping Forwards* de Unreal.
- **El *replay*** graba las acciones de cada paso: saltar es una acción más.
- **La depuración:** reproducir, pausar, avanzar paso a paso y saltar una cinemática, con
  el tiempo, el paso y la toma a la vista.
- **El costo:** muestrear los clips de dos a cuatro actores, con la física, la IA y el
  combate detenidos: menos que un paso de gameplay.

## Investigación

### Prototipo medido

Código en C++20, Python y C con Vulkan, Blender 5.2.2, cgltf 1.15 y `pl_mpeg`, medido en
la máquina de referencia el 2026-11-06. El detalle y cómo repetirlo están en
[`../research/cinematicas.md`](../research/cinematicas.md).

| # | Qué | Resultado |
|---|---|---|
| P1 | Blender → glTF | El modo `SCENE` hornea la NLA, los objetos y las cámaras a 30 Hz. **El FOV solo sale con `KHR_animation_pointer`**, que cgltf 1.15 entrega como JSON crudo. **Los marcadores y el cambio de cámara se pierden**; copiados a los `extras` por el script de exportación, llegan |
| P2 | El reloj y el salto | Con el número de paso, **el mismo hash a 30, 60 y 144 FPS**; con la suma del dt, cambian 19 o 20 de los 20 hashes de control a 30 y a 144 FPS, y a 60 FPS, 1 de 20 sin ruido y 20 de 20 con ±20 % de ruido. **Con el mundo detenido, el salto directo da la misma partida** (172 de 172); con el mundo andando, 0 de 172, y adelantar los pasos cuesta ~7 s por 20 s |
| P3 | Las transiciones (video de 154 s, 14 clips) | La mezcla lineal de vuelta **atraviesa a la caballera** (0,10–0,14 m) y, con la cámara fija, un pilar. El resorte no se asienta en 3 s. La órbita, ≥1,49 m. **El empalme, 0 °/s** |
| P4 | Bandas y subtítulos | 1,85:1 = 7 px; 2,39:1 = 46 px. Una línea de ×2 entra en la banda de 2,39:1; dos no |
| P5 | La ventana del *parry* | Regla C: error solo en los bordes, < 1 paso, igual a cualquier FPS. Borde del frame: el doble a 30 FPS. Con el instante dentro del paso: ~8 veces menos |
| P6 | El costo | En el engine, horneada, **~1,5 MB por minuto por personaje de 65 huesos** (~0,5 MB con ~23); unos KB si los actores solo nombraran clips que ya existen. MPEG-1 a 640×360: 7,3 / 13,9 / 21,9 MB por minuto a 1 / 2 / 4 Mbit/s; decodificar, 0,9 / 1,5 / 2,2 ms de mediana, más 0,8 ms a RGB |

Lo que mostró, además de los números:

- **El reloj de una cinemática es un problema de determinismo,** igual que el input
  (ADR-0032): si el tiempo sale del frame, la partida depende de los FPS.
- **Saltar es fácil solo si el mundo no se mueve.** Por eso las dos decisiones (qué corre
  y cómo se salta) son una sola.
- **Una mezcla entre dos poses no ve el mundo.** La transición que no se nota no es la
  mejor mezcla, sino **la que no necesita mezclar:** la toma termina donde empieza el
  juego.
- **Las bandas de 1,85:1 no alcanzan para los subtítulos:** van sobre la imagen, con el
  fondo del ADR-0026.

### Cómo lo hacen otros

- **Unreal Engine, *Sequencer*:** *Level Sequences* con pistas y una pista de cortes de
  cámara que mezcla la entrada y la salida; cada sección elige *Restore State* o
  *Keep State* al terminar. El viejo *Matinee* tenía *Fire Events when Jumping
  Forwards*, para que saltar una cinemática igual dispare sus eventos.
- **Unity, *Timeline*:** pistas en un *PlayableDirector*, *Signal Tracks* para los eventos
  y Cinemachine para mezclar o cortar entre cámaras.
- **Godot, `AnimationPlayer`:** cualquier propiedad es animable, con pistas de llamada a
  métodos y de audio.
- ***Metal Gear Solid*** (1998) contra ***Final Fantasy VII*** (1997): tiempo real contra
  video pregrabado en la misma consola. En *MGS*, el paso de la historia al juego no tiene
  salto visual.
- **Naughty Dog:** desde *Uncharted 4*, todas sus cinemáticas son en tiempo real, con el
  pivote en el personaje para que la cámara se comporte como la de juego.
- ***God of War*** (2018): ni un corte en todo el juego; su director de fotografía lo
  comparó con escribir sin sustantivos.
- **Los *quick time events*:** de *Dragon's Lair* (1983) a *Shenmue*, que les dio el
  nombre, y a *Resident Evil 4* y *God of War* (2005).
- **Accesibilidad:** XAG 116 ("mantener para saltar"); *Ratchet & Clank* deja pausar y
  saltar cualquier cinemática.

## Decisión

Las cinemáticas son momentos en que **el juego le quita la cámara al jugador** para
mostrarle algo, y pasan **dentro del engine**: es más flexible y más fácil, se ven igual
que el juego y pueden ser interactivas. **Las armo en Blender**, la herramienta que ya
uso para los personajes y el nivel: las cámaras, sus cortes y los actores se animan ahí.
La vuelta al juego la hace el engine, porque solo él sabe qué cámara tiene el jugador:
**la última toma termina interpolando suave hacia esa cámara**, sea cual sea su estilo, y
en Blender la veo de antemano con unas cámaras de referencia. Por eso no necesito un
editor propio. Mientras hay una cinemática, el mundo se detiene, así que saltarla deja la
misma partida que verla entera. Entran con un corte seco, como en el cine, y se pueden
saltar siempre.

| # | Dimensión | Elección |
|---|---|---|
| 1 | Engine o video | **Solo en el engine,** en tiempo real, con el mismo render, la misma luz y los mismos assets. **Sin decodificador de video** |
| 2 | Cómo se crean | **Todo en Blender.** Cada cinemática es **un `.blend`**, que enlaza los rigs de los personajes y la geometría de su zona como referencia. **Los actores** (`actor.<nombre>`) usan la NLA con sus clips; **las cámaras**, su animación y su distancia focal; **los cortes**, marcadores de la línea de tiempo atados a una cámara (como en Blender). **Los subtítulos, los sonidos y los eventos** son marcadores con un prefijo de una lista cerrada (`sub:`, `sonido:`, `evento:`, `salida`), y el tipo (`cinematica` o `interactiva`) es una propiedad de la escena. **El build exporta con `blender -b`** (ADR-0017) en modo `SCENE`, con `KHR_animation_pointer` para el FOV, y el script de exportación **copia los marcadores a los `extras`** (P1). **El cocinador valida y cocina:** un prefijo desconocido, un actor que no existe, un subtítulo sin id en la tabla, un evento que no está en la lista cerrada, un sonido que no existe, una rama que nombra una cinemática que no existe o una cámara dentro del escenario fallan con el nombre del objeto |
| 3 | Runtime | **Un secuenciador, un sistema de `game/`** (como el de las fases del ADR-0027), que avanza **en el paso fijo con el reloj en pasos** (`t = pasos · 1/60`). Los actores y los eventos de la partida entran al hash; **la cámara de la cinemática se calcula una vez por frame, desde el tiempo interpolado** (ADR-0031), y no entra. **La cámara y los actores llegan horneados a 30 Hz.** **Los huesos de los actores se muestrean como cualquier clip** (*nlerp*, con las claves en el mismo hemisferio; ADR-0015 y ADR-0017). **La cámara,** entre dos muestras: la posición lineal, el giro por *slerp* y el FOV lineal |
| 4 | La simulación | **Dos tipos. Una cinemática** (el tipo por defecto): **el mundo se detiene** (la física, la IA y el combate no corren); solo corren el secuenciador y sus actores, que son títeres de sus pistas. Los VFX y el sonido ambiente siguen, porque no entran al hash (ADR-0024, ADR-0025). **Una secuencia interactiva:** es gameplay; el mundo corre, la cámara es de autor y el input se restringe |
| 5 | Lo interactivo | **El contexto "secuencia" del ADR-0032**, con las acciones que la escena permite (una propiedad: por ejemplo, solo cargar y soltar). **No se salta.** **Las ramas son secuencias encadenadas:** el gameplay informa el resultado (éxito o fracaso) y dos propiedades de la escena dicen qué cinemática sigue en cada caso. **Las ventanas se miden en pasos y son CVars** (ADR-0021): se ajustan jugando. Morir lleva al checkpoint, que se pone **después** de la presentación |
| 6 | Transiciones | **De ida, corte seco.** **De vuelta, la última toma termina interpolando hacia la cámara actual del jugador:** desde el marcador `salida` hasta el final, el engine mezcla con *smoothstep* la cámara de Blender con la cámara del estilo que el jugador eligió, calculada en vivo, así que al terminar no queda nada que mezclar. **Lo hace el engine porque solo él conoce el destino:** el estilo, el FOV del jugador y, en la fija y los rieles, el sector donde termina la caballera. **En Blender, un script pone tres cámaras de referencia** (que no se exportan) en la pose de reposo de cada estilo, calculada desde la última posición de la caballera, para diseñar la toma hacia ellas y previsualizar la mezcla con un *constraint*. **El cocinador falla** si el tramo final cruza el escenario estático (el barrido de esfera de 0,2 m del ADR-0031) o pasa a menos de 0,5 m del cuerpo de la caballera, en cualquiera de los tres estilos. **Ni mezclas lineales sueltas ni resortes** |
| 7 | Dónde y qué la dispara | **En el binario de su zona** (ADR-0028), o en el global si la usan varias. Los actores van horneados: ~0,5 MB por minuto por personaje con los ~23 huesos del ADR-0017 (P6); una cinemática de un minuto con tres actores entra en el slot de 24 MiB. **Se dispara con un trigger o desde una fase:** suma un evento de trigger y un comando de fase a las listas cerradas del ADR-0027. **No se repite al morir:** su último evento suele activar el checkpoint (un evento que ya existe) |
| 8 | Audio y subtítulos | **Los subtítulos son ids de la tabla de textos del ADR-0026** (`sub:<id>`) y se ven con las opciones del jugador; cada uno dura hasta el siguiente marcador de subtítulo (`sub:-` lo cierra). **Los sonidos son eventos del paso** con la latencia fija del ADR-0024; la música cambia con el evento que ya tienen los triggers. Ella no habla (visión) |
| 9 | Saltar y estilo | **Se puede saltar siempre, también la primera vez, manteniendo "saltar" 1 s** (CVar), con un anillo que se llena. **"Saltar" es una acción reasignable** del contexto de la cinemática (ADR-0032), con las teclas de "aceptar" por defecto. **La pausa ofrece "Saltar":** elegirla entra como la acción saltar en el primer paso al reanudar, y así la graba el *replay*. **Bandas de 1,85:1 (7 px):** aparecen con el corte de entrada y se retiran durante el tramo final hacia la cámara del jugador. **Los subtítulos, sobre la imagen**, con el estilo del ADR-0026. **Sin HUD mientras dura una cinemática;** en una secuencia interactiva, el HUD que el gameplay necesite. El mismo render a 640×360 con el look PSX |
| 10 | Determinismo, pruebas y costo | **Saltar = los actores a su última clave y los eventos de la partida que faltan, en orden;** los de la presentación se descartan. **Saltar es una acción** del contexto de la cinemática, y el *replay* la graba. **TDD** del muestreo de las pistas, del salto y del cocinador. **Un test de propiedad:** saltar en cualquier paso da el mismo hash que verla entera. **Un panel de ImGui** (solo en desarrollo, ADR-0021) reproduce, pausa, avanza paso a paso y salta una cinemática, y muestra el tiempo, el paso y la toma; al moverse hacia atrás solo muestra las poses y la cámara, sin aplicar los eventos de la partida. **Usarlo para pausar, avanzar o ir hacia atrás corta la grabación del *replay*,** como la recarga en caliente (ADR-0021). **Sin región nueva:** la cinemática vive en el binario de su zona |

## Consecuencias

### El camino de una cinemática

```
Blender:  cine.blend (actores en la NLA, cámaras, cortes, marcadores sub: sonido:
          evento: salida, el tipo en la escena)
   ──blender -b (modo SCENE, KHR_animation_pointer, marcadores → extras)──→ cine.glb
   ──cocinador (valida, hornea a 30 Hz, barre la salida en los tres estilos)──→
          una sección del binario de la zona (o del global)

juego:    un trigger o una fase la dispara
paso:     secuenciador (t = pasos · 1/60)
            cinemática: el mundo se detiene; los actores van a su pose; los eventos
            de la partida se aplican; los sonidos y los subtítulos van al audio y a la UI
            secuencia interactiva: el mundo corre; el input, restringido
frame:    la cámara de la toma en el t interpolado
            desde `salida`: mezcla con la cámara del estilo del jugador → render
            bandas de 1,85:1 mientras dura
saltar:   mantener "saltar" 1 s, o "Saltar" en la pausa → los actores a su última
          clave y los eventos de la partida que faltan, en orden
fin:      el control vuelve, con la cámara ya en la del jugador
```

**El secuenciador** es un sistema de `game/`, como el de las fases: lee la sección
cocinada, muestrea las pistas y emite los eventos por las colas del paso (ADR-0012). La
cámara de la cinemática la calcula el sistema de cámara (ADR-0031), una vez por frame.

### Lo que se gana

- **Una sola imagen:** la cinemática se ve exactamente como el juego, con los cortes y
  los pedazos que haya en la escena, y puede ser interactiva.
- **Una sola herramienta:** Blender ya arma los personajes, los clips y el nivel. No hay
  un editor que construir ni mantener.
- **Saltar es exacto y gratis** (P2: 172 de 172), y el *replay* no se entera de nada
  nuevo: saltar es una acción más.
- **La vuelta al juego no se nota:** la última toma termina donde está la cámara del
  jugador (P3: 0 °/s al devolver el control), en cualquiera de los tres estilos.
- **El reloj no depende de los FPS:** la misma cinemática da el mismo hash a 30, 60 y 144
  FPS.
- **El costo es bajo:** con el mundo detenido, un paso de cinemática cuesta menos que uno
  de gameplay.

### Lo que se sacrifica

- **El mundo detenido se ve:** un pedazo que caía queda quieto mientras dura la
  cinemática. Conviene dispararlas en momentos tranquilos: al terminar una fase o al
  cruzar un trigger.
- **La última toma tiene que funcionar en los tres estilos:** si el tramo final cruza el
  escenario o a la caballera en alguno, el build falla, y hay que rediseñar la toma.
- **En Blender, el final es una aproximación:** las cámaras de referencia usan el FOV por
  defecto, sin la luz, la niebla ni los 640×360 del juego. Lo exacto se ve recién en el
  juego.
- **Los actores van horneados:** ~0,5 MB por minuto por personaje (P6), en vez de los
  pocos KB que costaría nombrar clips que ya existen.
- **La exportación depende de dos trucos:** copiar los marcadores a los `extras` y leer
  el FOV de `KHR_animation_pointer`, que cgltf 1.15 no conoce y entrega como JSON crudo.
- **Una secuencia interactiva no se salta:** es gameplay.
- **La caballera termina donde la deja la cinemática:** el juego sigue desde su última
  clave.

### Tickets, en orden

El corte vertical no tiene cinemáticas (visión), así que estos tickets van después,
salvo que el autor sume una.

1. el secuenciador puro, en TDD: el muestreo de las pistas, el reloj en pasos, el salto
   (con el test de propiedad: saltar en cualquier paso da el mismo hash que verla
   entera) y las ramas;
2. la exportación de Blender: el script de exportación (modo `SCENE`,
   `KHR_animation_pointer`, los marcadores a los `extras`) y el de las cámaras de
   referencia (junto a los ADR-0017 y ADR-0031);
3. el cocinador de cinemáticas: los prefijos, los actores, los ids de los subtítulos, las
   cámaras dentro del escenario y el barrido de la salida en los tres estilos; la sección
   del binario de la zona (junto a los ADR-0027 y ADR-0028);
4. el mundo detenido: qué sistemas corren en el paso durante una cinemática (junto al
   ADR-0012);
5. la cámara de la cinemática y la salida hacia la del jugador (junto al ADR-0031);
6. dispararla: el evento de trigger y el comando de fase (junto al ADR-0027);
7. los subtítulos, los sonidos, la música y las bandas (junto a los ADR-0024 y ADR-0026);
8. saltar: el contexto de la cinemática, la acción "saltar" (mantener, reasignable),
   "Saltar" en la pausa y el *replay* (junto a los ADR-0021, ADR-0026 y ADR-0032);
9. la secuencia interactiva: el contexto "secuencia" con las acciones permitidas, las
   ramas y las ventanas en pasos (junto a los ADR-0018 y ADR-0032);
10. el panel de ImGui.

### Fe de erratas en otros ADRs

| ADR | Qué cambia |
|---|---|
| ADR-0012 | Durante una cinemática, el paso fijo sigue corriendo, pero solo el secuenciador y sus actores |
| ADR-0017 | Blender también arma las cinemáticas; la exportación en modo `SCENE`, con el FOV y los marcadores; lo que valida el cocinador |
| ADR-0021 | El panel de cinemáticas en ImGui; usarlo para pausar, avanzar o ir hacia atrás corta la grabación |
| ADR-0026 | Durante una cinemática no hay HUD; las bandas de 1,85:1; el anillo de mantener para saltar y "Saltar" en la pausa |
| ADR-0027 | Un evento de trigger y un comando de fase para disparar una cinemática |
| ADR-0028 | La sección de cinemáticas en la parte de la CPU del binario de la zona o del global |
| ADR-0031 | La cámara de las cinemáticas y la salida hacia la cámara del estilo |
| ADR-0032 | Un contexto nuevo, la cinemática (solo saltar y pausar). Saltar es reasignable, con las teclas de "aceptar" por defecto, y "Saltar" desde la pausa entra como la acción al reanudar. La secuencia también sirve a las secuencias interactivas |
| `presupuestos.md` | Una fila en el historial: una cinemática horneada vive en el slot de su zona |

### Lo que queda abierto

- **El dragón:** si existe, su ventana de *parry*, la carga sin energía y el tajo
  vertical obligado (la visión y el ADR-0018); cortar algo de su tamaño (ADR-0007) y una
  bola de fuego (ADR-0025).
- **Los valores de partida,** que se ajustan jugando (CVars): la duración de la salida,
  el tiempo de mantener para saltar y las ventanas de las secuencias interactivas.
- **Las voces de los otros personajes:** si hablan, son sonidos con su subtítulo. La voz
  de la protagonista al final es parte de la historia (#77).
- **Nombrar clips en vez de hornearlos,** si un día la memoria de las zonas lo pide.

## Cómo lo defiendo

**¿Por qué no usaste video, si es lo que hacían los juegos de PlayStation?**
Porque el video no puede ser interactivo, y el final del dragón lo es. Además, no
muestra lo que pasó en la partida (los zombies cortados, los pedazos), no respeta el
estilo de cámara del jugador y pesa de 5 a 15 veces más que la cinemática en el engine.
*Metal Gear Solid* ya mostró en la misma consola que el tiempo real borra el salto entre
la historia y el juego.

**¿Por qué Blender y no un editor propio?**
Lo pensé. Lo que me tentaba del editor era que la última toma pasara suave a la cámara
del jugador, pero esa mezcla la tiene que hacer el engine de todas formas: solo él sabe
qué estilo y qué FOV eligió el jugador. Blender ya es mi herramienta para los personajes
y el nivel, y con unas cámaras de referencia veo hacia dónde tiene que terminar la toma.
Un editor sería un sistema entero para construir y mantener.

**¿Por qué el tiempo de la cinemática sale del número de paso?**
Porque si sale del frame, la partida depende de los FPS. Lo medí: sumando el dt del
frame, a 30 y a 144 FPS cambian 19 o 20 de los 20 hashes de control. A 60 FPS exactos
cambia solo 1, pero con un poco de ruido en el dt, como en un frame real, cambian los 20,
y los eventos se corren hasta un paso. Contando pasos, el hash es el mismo. Es la misma
regla que el input.

**¿Por qué se detiene el mundo? ¿No se ve raro?**
Porque es lo que hace que saltar sea exacto: con el mundo quieto, saltar es llevar a los
actores a su última clave y aplicar los eventos que faltan, y da la misma partida que
verla entera. Con el mundo andando, o salto mal o tengo que simular todos los pasos que
faltan, que son segundos de espera. Se nota si algo estaba en el aire, así que disparo
las cinemáticas en momentos tranquilos.

**¿Cómo se salta sin romper el *replay*?**
Saltar es una acción del paso, como atacar. El *replay* graba las acciones, así que
reproduce el salto en el mismo paso, y el salto es determinista.

**¿Por qué un corte seco al entrar y una mezcla al salir?**
Al entrar, el corte es la gramática del cine: la primera toma presenta la escena. Al
salir, un corte me lleva de mirar a la caballera de frente a verla de espaldas, y eso sí
desorienta. Por eso la última toma termina interpolando hacia la cámara del jugador: al
devolver el control no queda nada que mezclar.

**¿Por qué no una mezcla lineal entre la toma y la cámara de juego?**
Porque una mezcla entre dos poses no sabe nada del mundo. En el prototipo, la mezcla de
vuelta pasó a diez centímetros del eje de la caballera (la atravesó) y, con la cámara
fija, cruzó un pilar. Con la salida, el recorrido lo diseño yo, y el cocinador comprueba
que no cruce el escenario ni a la caballera en ninguno de los tres estilos.

**¿Qué diferencia hay entre una cinemática y una secuencia interactiva?**
En una cinemática, el jugador mira: el mundo se detiene y se puede saltar. Una secuencia
interactiva es gameplay con una cámara de autor: el mundo corre, el input se restringe a
lo que la escena permite y no se salta. El dragón sería una secuencia interactiva.

**¿Por qué la ventana del *parry* se mide en pasos y no en milisegundos?**
Porque la simulación vive en pasos. Con la regla del input del ADR-0032, el veredicto
solo se equivoca en los bordes de la ventana, por menos de un paso, y no depende de los
FPS: es lo que hacen los juegos de pelea, que juzgan por *frames*. El ancho es una CVar y
lo ajusto jugando.

**¿Por qué bandas de 1,85:1?**
Marcan que el jugador no controla sin comerse la imagen: son 7 píxeles por lado a
640×360. Las de 2,39:1 se llevaban 46, más de un cuarto de la pantalla.

**¿Por qué se puede saltar desde la primera vez?**
Porque el tiempo es del jugador, y es una pauta de accesibilidad. Para no saltar sin
querer, hay que mantener el botón un segundo.

**¿Cuánto cuesta?**
Horneada, una cinemática pesa ~0,5 MB por minuto por personaje, dentro del slot de su
zona. En CPU, menos que un paso de gameplay, porque la física, la IA y el combate están
detenidos.

## Referencias

- Epic Games, *Cinematic Event Track in Unreal Engine* (Sequencer):
  <https://dev.epicgames.com/documentation/de-de/unreal-engine/cinematic-event-track-in-unreal-engine>
- Epic Games, `InterpTrackEvent` (*Fire Events when Jumping Forwards*):
  <https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/InterpTrackEvent?application_version=5.0>
- Foro de Unreal Engine, *Keep State* en la última sección de cortes de cámara:
  <https://forums.unrealengine.com/t/how-do-i-stop-return-to-player-camera-after-a-level-sequence/132549>
- Unity, `UnityEngine.Timeline`:
  <https://docs.unity3d.com/Packages/com.unity.timeline@1.0/api/UnityEngine.Timeline.html>
- R. Bowling, *A practical primer to using Unity Timeline* (2019):
  <https://www.blog.radiator.debacle.us/2019/11/practical-primer-to-using-unity.html>
- Godot Engine, *Animation Track types*:
  <https://docs.godotengine.org/en/stable/tutorials/animation/animation_track_types.html>
- Khronos, `KHR_animation_pointer`:
  <https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_animation_pointer>
- D. Szablewski, `pl_mpeg`: <https://github.com/phoboslab/pl_mpeg>
- N. Druckmann y B. Straley, *Creating the Active Cinematic Experience of Uncharted 2*,
  GDC 2010: <https://gdcvault.com/play/1012348/Creating-the-Active-Cinematic-Experience>
- *Interactive Cinematics in Uncharted 4* (Game Anim):
  <https://www.gameanim.com/?p=13879>
- *Barlog: God of War's single-shot camera trick was a tough sell for devs* (Game
  Developer):
  <https://gamedeveloper.com/production/barlog-i-god-of-war-i-s-single-shot-camera-trick-was-a-tough-sell-for-devs>
- *Kojima Station discusses 5 groundbreaking aspects of Metal Gear Solid 1* (Metal Gear
  Informer):
  <https://www.metalgearinformer.com/kojima-station-discusses-5-groundbreaking-aspects-of-metal-gear-solid-1/>
- *The Essentials: Metal Gear Solid* (Game Informer):
  <https://gameinformer.com/b/features/archive/2015/05/03/the-essentials-metal-gear-solid>
- *Quick time event* (Wikipedia): <https://en.wikipedia.org/wiki/Quick_time_event>
- Microsoft, *Xbox Accessibility Guideline 116: Time limits*:
  <https://learn.microsoft.com/en-us/gaming/accessibility/xbox-accessibility-guidelines/116>
- *Bypass* (Accessible Player Experiences):
  <https://accessible.games/accessible-player-experiences/challenge-patterns/bypass/>
