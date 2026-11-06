# ADR-0026: UI, HUD y menús

- **Estado:** Aceptado
- **Fecha:** 2026-11-02
- **Ticket:** #40
- **Reemplaza / Reemplazado por:** —
- **Fe de erratas:** 2026-11-02 (#41): el ADR-0027 decide un mundo continuo, partido en
  zonas que se cargan mientras se camina. **La pantalla de carga aparece al empezar y al
  volver a un checkpoint, nunca al caminar.** Si la simulación espera a una zona que no
  llegó, no hay pantalla de carga; si hay algún indicador queda abierto. Se corrige en la
  Decisión 6 y en Abierto.
- **Fe de erratas:** 2026-11-03 (#42): el ADR-0028 decide la carga y el guardado. **La
  pantalla de carga es fija:** el fondo y "Cargando…" en el idioma elegido, sin barra.
  **Si la simulación espera a una zona,** el frame queda quieto y, **pasados 0,5 s,
  aparece un indicador chico** en una esquina. **"Continuar"** solo aparece si la
  tarjeta de memoria tiene el bit de partida, y **"Nueva partida"** pide confirmar si lo
  tiene, porque hay una sola partida. **Las opciones** se guardan en `opciones.txt`, en
  la carpeta de configuración (XDG), al salir de su pantalla. Se corrige en la
  Decisión 6 y en Abierto.
- **Fe de erratas:** 2026-11-03 (#73): el ADR-0031 hace de la cámara una opción del
  jugador con tres estilos (tercera persona, fijas y rieles, primera persona) y saca el
  apuntado de la cámara. **La mira está en el mundo,** donde pega el rayo que sale del
  pecho, no en el centro de la pantalla: "junto a la mira" sigue a la mira. **La energía
  en la espalda solo sirve si la cámara ve la espalda:** en la tercera persona sigue
  siendo la ubicación por defecto; en la primera persona y en las fijas y los rieles, la
  ayuda va por defecto junto a la mira. **Las opciones suman la cámara:** el estilo, el
  FOV de la tercera y de la primera persona, cuánto se cierra al apuntar en cada estilo,
  la sacudida y la cámara del tajo. **Apagar la cámara del tajo cambia la simulación**
  (quita la cámara lenta), así que es de las opciones que se graban como evento del paso.
  Se corrige en las Decisiones 5 y 6.
- **Fe de erratas:** 2026-11-06 (#74): el ADR-0032 decide el input. **La simulación
  recibe las acciones ya procesadas** (el movimiento, la guía y los botones con sus
  contadores), así que **la sensibilidad y la alternativa de mantener
  quedan fuera de la simulación** y ya no se graban como evento del paso; apagar la
  cámara del tajo sí, porque quita la cámara lenta. **Los íconos cambian** con un botón o
  con un eje que pasa la zona muerta, no con cualquier evento (P6 del ADR-0032: con
  cualquier evento, un mando quieto con deriva los cambia 292 veces por minuto). **Los
  menús son fijos,** con alternativas (flechas y WASD, Enter y Espacio, Esc; la cruceta,
  el stick, A/Cruz y B/Círculo), y **el cursor queda encerrado en la ventana mientras
  tenga el foco**, porque en un compositor donde el foco sigue al mouse, si el cursor
  sale, el teclado se va con él. Se corrige en las Decisiones 6 y 7.

## Contexto y problema

Hay que decidir **cómo se construye la interfaz del jugador**: el HUD, los subtítulos, el
menú de pausa y el de opciones, el texto y sus fuentes, los idiomas, y cómo se navega
todo eso con teclado, mouse y gamepad. También dónde termina la interfaz del juego y
empieza la de depuración (Dear ImGui, ADR-0021).

La visión ya fija lo que el HUD tiene que contar: **la caballera no tiene barra de vida,
su vida es la energía**, que se gasta al cargar, al corregir el ángulo, al trotar y al
soltarse de un agarre. Además, la carga del tajo, la línea del tajo mientras se apunta
y los eventos de *quick time* de la espada clavada y del agarre.

**La idea del autor** para este ADR:

- **0 HUD por defecto, con técnicas diegéticas:** la energía se lee en la animación de
  la caballera (cómo carga, cuándo está cansada y cuándo está por cansarse).
- **Las ayudas en pantalla son opciones de accesibilidad,** que el jugador agrega una por
  una: una barra de carga y una de energía clásicas, pero puestas donde tengan sentido en
  el mundo.
- **El juego no tiene niveles de dificultad:** agregar o quitar ayudas es lo que lo hace
  más fácil o más difícil.
- **Subtítulos sí o sí,** aunque sean pocos (la protagonista casi no habla).
- **Español e inglés desde el principio,** y que sumar otro idioma sea fácil.

Varios ADRs ya aceptados dejaron preguntas para este:

| ADR | Qué dejó abierto |
|---|---|
| ADR-0021 | Dear ImGui es solo de desarrollo (no se compila en Release). Mandaba al ADR-0028 el menú de opciones y dónde se guardan: el menú es de este ADR y el guardado sigue en el ADR-0028 (fe de erratas) |
| ADR-0022 | El HUD y los menús. ImGui va a la resolución nativa, después del blit; el postproceso, a 640×360 |
| ADR-0024 | La pantalla de opciones con los volúmenes. El bus de la UI sigue sonando en la pausa |
| ADR-0025 | El HUD. Los *frames* de impacto (siluetas) van en el postproceso a 640×360 |
| Visión | La energía como vida, la línea del tajo en pantalla, los *quick time* y su alternativa de mantener (accesibilidad), los controles reasignables |

Hay que decidir:

1. el **modelo de la UI** (*immediate* o *retained*, propia o biblioteca) y su frontera
   con ImGui;
2. **dónde y a qué resolución se dibuja:** antes o después del blit, y su lugar en el
   frame;
3. **el texto y las fuentes:** cómo se rasterizan, qué caracteres y con qué licencia;
4. **los textos y los idiomas;**
5. **el HUD:** qué muestra, cómo y cuándo;
6. **los menús y el flujo de pantallas,** con la pausa;
7. **la navegación** con teclado, mouse y gamepad, y los íconos de los botones;
8. **los datos y la autoría** de las pantallas;
9. **la accesibilidad;**
10. **la memoria, el presupuesto, las pruebas y la depuración.**

Lo que ya está decidido y limita este ADR:

- **ADR-0004:** SDL3 solo para ventana, input, tiempo y audio, detrás de `platform::`.
  Rasterizar fuentes con SDL_ttf sería una dependencia nueva.
- **ADR-0005:** Vulkan 1.3. Un *ring buffer* mapeado para la geometría dinámica.
- **ADR-0009:** sin excepciones ni RTTI. En el código de cada frame no hay contenedores
  que asignen memoria.
- **ADR-0010:** regiones por sistema, reservadas y tocadas al iniciar, con topes duros.
- **ADR-0011:** TDD estricto en el núcleo; tests después en el gameplay y el renderer.
- **ADR-0012:** un hilo, paso fijo de 60 Hz, hasta 8 pasos por frame e interpolación en
  el render. Capas estrictas: `platform` → `core` → `gpu` → sistemas → `game`.
- **ADR-0016 y ADR-0021:** la misma partida grabada da el mismo hash del estado. Un valor
  que cambia la simulación mientras se graba se graba como evento de ese paso.
- **ADR-0017:** los assets entran crudos al repo y el cocinador los convierte.
- **ADR-0019:** los assets de terceros, **solo CC0 o CC BY**. Ninguna otra licencia se
  acepta.
- **ADR-0020:** 13,3 ms de GPU y 2,5 ms de render en la CPU, en la Intel UHD 620. Una
  pantalla de referencia de 3200×1800 (×5).
- **ADR-0021:** Dear ImGui dibujado por el renderer propio, con el input desde
  `platform::`, solo en `debug` y `relwithdebinfo`. CVars, que en Release son constantes.
- **ADR-0022:** forward a 640×360 con una lista fija de pasadas, blit *nearest* por un
  factor entero con bordes negros, y una reserva de GPU de 2,8 ms.
- **ADR-0024:** buses de música, mundo y UI; la pausa congela el del mundo.
- **ADR-0025:** el canal alfa del color a 640×360 es la máscara de personajes que lee el
  postproceso.

**Fuera de alcance:**

| Qué | Lo decide |
|---|---|
| Las acciones abstractas, la reasignación de controles, las curvas del stick y cómo se detecta el dispositivo | ADR-0032 (input y controles) |
| Dónde se guardan las opciones del jugador y su formato | ADR-0028 |
| La cámara, el rayo que decide el punto apuntado y la cámara del tajo | ADR-0031 (cámara) |
| La pantalla de carga: cuánto dura y qué se carga mientras | ADR-0028 |
| Los niveles, los checkpoints y qué pantalla lleva a cuál nivel | ADR-0027 y ADR-0028 |
| Las cinemáticas (usan los subtítulos de este ADR) | ADR-0034 |
| La animación que muestra el cansancio | ADR-0015 |

## Criterios

Los cinco del ticket, más cinco que salen de cruzar lo ya decidido:

1. **Claridad para el jugador.**
2. **Soporte de gamepad:** todo se maneja sin mouse.
3. **Costo de implementación.**
4. **Dependencias justificadas** (ADR-0004, ADR-0019).
5. **Aprendizaje.**
6. **Coherencia con el estilo PSX** (pilar 4 de la visión): que la UI no parezca de otro
   juego.
7. **No romper el determinismo:** la UI no cambia la simulación sin dejarlo grabado
   (ADR-0016, ADR-0021).
8. **Encaja en el frame del ADR-0022** y en las reservas de `presupuestos.md`.
9. **Lo hace una sola persona:** lo que cuesta sumar una pantalla, un texto o un idioma.
10. **Estándar de la industria.**

## Opciones

Los números son de un prototipo medido en la máquina de referencia (ver Investigación).
**La GPU quedó con el reloj libre** (~300 MHz en promedio, porque la escena la deja casi
siempre en el mínimo), así que sus tiempos son pesimistas frente a los de 700 MHz de los
ADR-0022 y ADR-0025. Todos los modos corrieron en las mismas condiciones.

### 1. El modelo de la UI

| Opción | Cómo es | Medido (menú de pausa de 8 filas) | Problema |
|---|---|---|---|
| ***Immediate mode* propia** | Cada frame se llama a cada control (`if (button("Continuar"))`). El estado de la UI es el índice del foco; el valor de cada opción vive en el juego (Muratori, 2005) | 52 líneas. CPU: 23 µs quieto, 51 µs con cambios. 0 asignaciones | Rearma los vértices cada frame. Las animaciones (un panel que entra, un valor que se desliza) necesitan estado guardado por ID |
| ***Retained mode* propia** | Los controles se crean una vez en un árbol; el *layout* se calcula cuando algo cambia, y los vértices se guardan | 68 líneas. CPU: 4,4 µs quieto (vértices guardados), 38 µs con cambios. 0 asignaciones | Dos copias del estado (el control y el juego) que hay que sincronizar. El caché necesita una llave que no se olvide de nada |
| **Dear ImGui también en el juego** | El mismo ImGui del ADR-0021, con un estilo propio | El panel del ADR-0021: 37 µs | ImGui no existe en Release (ADR-0021): habría que compilarlo siempre. Está pensado para herramientas: el estilo, las animaciones y el foco con gamepad son difíciles de llevar al aspecto del juego |
| **Una biblioteca de UI de juego** | RmlUi (MIT): documentos tipo HTML y CSS, su propio *layout*, y devuelve vértices para el renderer propio | No medido | Una dependencia grande, con su propio motor de *layout*. Se aprende CSS, no cómo funciona una UI |

**La frontera con ImGui,** con cualquiera de las propias: las dos dibujan con el mismo
tipo de pipeline (quads con textura y alfa) y reciben el input de `platform::`. Lo que no
comparten: ImGui no existe en Release. Si ImGui tiene el foco del teclado o del mouse
(`WantCaptureKeyboard`, `WantCaptureMouse`), la UI del juego no recibe ese input.

### 2. Dónde y a qué resolución se dibuja

| Opción | Cómo se ve | Medido a ×5: HUD / menú (GPU) | Problema |
|---|---|---|---|
| **A 640×360, antes del blit** | El texto y las formas tienen el mismo píxel que la escena. El texto de cuerpo mide ~58 px a 1080p | **0,02 / 0,14 ms** | El texto no se puede achicar sin cambiar de fuente; agrandarlo va por saltos enteros (×2, ×3) |
| **A la resolución nativa, después del blit** | Texto nítido y fino sobre una escena de píxeles grandes: parece de otro juego | 0,12 / **2,84 ms** | El costo crece con los píxeles que cubre: oscurecer la pantalla detrás del menú cuesta **1,8 ms** a ×5 (0,08 a 640×360). Las fuentes nativas a ×5 ocupan 780 KB de atlas (33 KB las de píxeles) |
| **Mixto** | Las formas a 640×360 y el texto a la resolución nativa | 0,06 / — | Dos pasadas y dos atlas. Los íconos quedan mitad y mitad |

**Dónde en el frame:** después del postproceso, para que los *frames* de impacto del
ADR-0025 no inviertan el HUD ni los subtítulos. Aun así, durante esos 3 frames el fondo
es blanco y **el subtítulo color hueso sin fondo casi desaparece**; con fondo se lee.

### 3. El texto y las fuentes

**Cómo se rasteriza:**

| Opción | Cómo funciona | Medido (ASCII y Latin-1) | Problema |
|---|---|---|---|
| **Fuente de píxeles cocinada** | El cocinador rasteriza la fuente en su grilla exacta (1 píxel de diseño = 1 píxel), sin antialiasing, a un atlas y una tabla de glifos con su *kerning* | 15 KB por fuente, 0,5 ms de cocinado | Solo los tamaños múltiplos de la grilla quedan nítidos. Cada idioma con otra escritura necesita una fuente que la cubra |
| **Fuente con antialiasing, cocinada** | Lo mismo con una fuente normal, a los tamaños de la resolución nativa | 99 KB a 36 px; 1,3 ms | Solo sirve a la resolución nativa (a 640×360 se ve borrosa). Un atlas por factor de escala |
| **Rasterizar en el juego** (`stb_truetype` o FreeType) | El atlas se arma al iniciar o cuando aparece un carácter nuevo | — | Una dependencia en el juego en vez de en el cocinador. FreeType es grande; con HarfBuzz para escrituras complejas, más |
| **SDF o MSDF** (Green, 2007; Chlumský, `msdfgen`) | Un campo de distancia por glifo que escala sin perder bordes | 163 KB a 32 px; 81 ms | Sirve para escalar a muchos tamaños. A tamaño fijo no aporta, y a 640×360 se pierde |

`stb_truetype` (dominio público o MIT, ~5000 líneas) alcanza para cocinar cualquiera de las
tres primeras. En el cocinador, como `dr_flac` en el ADR-0024, no llega al juego.

**Qué fuente y con qué licencia:**

| Opción | Ejemplos | Problema |
|---|---|---|
| **OFL** (SIL Open Font License 1.1) | Jersey 10 (texto) y Jacquard 12 (gótica pixelada, para títulos), de Google Fonts | El ADR-0019 no la acepta: necesita una fe de erratas. Es la licencia estándar de las fuentes libres: deja incluirla en un juego, incluso comercial; no deja venderla sola, y una versión modificada no puede usar el nombre reservado |
| **CC0** | monogram extended (datagoblin), Public Pixel (GGBotNet), Not Jam Font Pack | No se encontró una gótica pixelada CC0 para los títulos. monogram tiene una ambigüedad: itch.io dice CC0 y su repositorio, MIT |
| **Propia** (CC BY 4.0, ADR-0019) | Dibujada por el autor, glifo por glifo | ~190 glifos por fuente. Es un proyecto de arte, no de engine |

**Qué caracteres:** ASCII, Latin-1 y Latin Extended-A cubren el español, el inglés y casi
todos los idiomas de Europa occidental y central. El griego, el cirílico y el CJK
necesitan otra fuente (y el CJK, un atlas de miles de glifos). El árabe, el hebreo y el
tailandés necesitan además *shaping* (HarfBuzz).

### 4. Los textos y los idiomas

| Opción | Cómo funciona | Problema |
|---|---|---|
| **En el código** | `"Continuar"` en el lugar donde se usa | Sumar un idioma es buscar cada texto en el código |
| **Una tabla con un ID por texto y una columna por idioma** | Un archivo (CSV o TSV) que el cocinador convierte en una tabla binaria; el código usa el ID | Un archivo que todos los idiomas comparten; los conflictos de git aparecen con varios traductores |
| **Un archivo por idioma** (como gettext) | El ID o el texto en inglés como llave, y un archivo por idioma | El cocinador tiene que verificar que todos los idiomas tengan todas las llaves |

**Lo que cualquiera de las dos últimas necesita:**

- **UTF-8** en los archivos y en el juego;
- **que el texto quepa:** el español es más largo que el inglés ("With background" y "Con
  fondo" son la excepción). Con fuentes de píxeles, el ancho de cada texto se conoce al
  cocinar, así que **el cocinador puede fallar si un texto no cabe en su caja**;
- **plurales y variables** ("quedan 3 zombies"): en español y en inglés hay dos formas;
  el polaco o el ruso tienen más (las reglas de plural de CLDR). Con dos idiomas, una
  regla propia alcanza; con más, ICU MessageFormat o equivalente;
- **cambiar de idioma en vivo:** con IDs, cambiar el idioma es cambiar una columna.

### 5. El HUD

**La línea del tajo** no es una ayuda: es el control del tajo (la visión). Se muestra
siempre que se apunta. Su forma la afinan este ADR y el de la cámara.

**Las ayudas** (energía, carga y avisos de botón), en tres variantes, todas opcionales:

| Opción | Cómo es | Medido (GPU, 640×360) | Problema |
|---|---|---|---|
| **0 HUD con señales diegéticas** | Brillo que sube por el filo con la carga y destello en la máxima; vaho del yelmo más seguido con el cansancio; postura que se inclina y espada que baja | 0 | Se lee "poca" o "casi llena", no "60 %". Depende de que la espada y la caballera estén a la vista, de oír la respiración y de la animación (ADR-0015). Si la señal es chica, no existe: el primer brillo del prototipo (menos de 1 cm) no se veía |
| **A. En el cuerpo** (espacial, como el RIG de *Dead Space*) | La energía, una barra en la espalda (la cámara la mira de atrás); la carga, una barra que recorre el filo | 0,021 ms | Se tapa: la sangre, un zombie entre la cámara y la caballera, la caballera semitransparente cuando la cámara se acerca |
| **B. Junto a la mira** | Dos arcos al lado del centro de la pantalla, donde el jugador ya mira al apuntar; se apagan con la energía llena y sin cargar | 0,019 ms | Chicos: con la horda detrás, el arco de la energía baja se confunde con el fondo |
| **C. Esquinas** (referencia clásica) | Barras arriba a la izquierda | 0,021 ms | El más legible y el que más saca de la escena |

**Los avisos de botón** (*quick time*) van junto al zombie en A y B, y abajo al centro en
C. **La alternativa de mantener** en vez de presionar rápido (visión) cambia la
simulación: es una opción que se graba en el *replay*.

**Las ayudas como dificultad:** el autor quiere que el juego no tenga niveles de
dificultad y que las ayudas cumplan ese papel. La idea tiene un límite: para un jugador
sordo, la respiración no existe; para uno con daltonismo, el rojo del brillo se confunde
con la escena (P7). Para ellos, las ayudas no hacen el juego más fácil: lo hacen
jugable. XAG pide que la información esté en más de un canal. *Celeste* (2018) tampoco
tiene niveles de dificultad, y presenta su *Assist Mode* como una opción de
accesibilidad.

### 6. Los menús y el flujo de pantallas

**Las pantallas:** título, pausa, opciones (ayudas, subtítulos, idioma, volúmenes del
ADR-0024, controles del ADR-0032), muerte y vuelta al checkpoint, y carga.

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Una pila de pantallas** | Abrir opciones desde la pausa apila una pantalla; volver la saca. Solo la de arriba recibe input | Hay que decidir qué pantallas dejan ver (y dibujar) a las de abajo |
| **Una máquina de estados** | Cada pantalla es un estado, con transiciones explícitas | Las pantallas que se abren desde varios lugares (opciones desde el título y desde la pausa) necesitan recordar a dónde volver |

**La pausa y la simulación:** con la pausa, el acumulador del paso fijo no avanza
(ADR-0012): 0 pasos, nada que grabar. El bus del mundo se congela y el de la UI sigue
(ADR-0024). La escena se sigue dibujando, quieta, detrás del menú.

**Las opciones que cambian la simulación** (mantener en vez de presionar rápido, la
sensibilidad, que usa el apuntado): si cambian durante una grabación, se graban como
evento del paso (ADR-0021). Las que no la cambian (las ayudas, los subtítulos, el
idioma) no se graban.

### 7. La navegación

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Listas verticales con un índice de foco** | Arriba y abajo mueven el foco; izquierda y derecha cambian el valor; aceptar y volver. El *hover* del mouse mueve el foco | Una pantalla con columnas o una grilla necesita otro modelo |
| **Navegación espacial** | El foco salta al control más cercano en la dirección pedida (como las teles y las consolas) | Más código, y a veces salta a donde no se espera |
| **Vecinos explícitos** | Cada control declara quién está arriba, abajo, a la izquierda y a la derecha | Se escribe a mano para cada pantalla |

**Los íconos de los botones:** siguen al último dispositivo usado. SDL3 da el tipo de
gamepad (`SDL_GetGamepadType`: PS3, PS4 y PS5 dan los de PlayStation; el resto, los de
Xbox). El prototipo los dibuja con primitivas, sin texturas. **El stick** navega con
repetición: 0,35 s y después cada 0,12 s.

### 8. Los datos y la autoría

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Pantallas en código** | Cada pantalla es una función (*immediate*) o un árbol que arma una función (*retained*) | Ajustar una posición es recompilar, salvo con CVars |
| **Pantallas en un archivo cocinado** | Un archivo describe los controles y su *layout*; el cocinador lo valida | Un formato y un lector propios, o una biblioteca (RmlUi usa HTML y CSS) |
| **Un editor visual** | Como UMG en Unreal | Un proyecto entero, que no es lo que el proyecto quiere aprender |

Con pocas pantallas (título, pausa, opciones, muerte y carga), la diferencia entre las dos
primeras es chica.

### 9. La accesibilidad

Lo que se midió en el prototipo (P7) y lo que piden las guías de Xbox (XAG):

- **Tamaño del texto (XAG 101):** 18 px de alto a 1080p en PC y 26 en consola, como
  mínimo, y que el jugador lo pueda agrandar hasta el 200 %. El texto a 640×360 mide
  ~58 px a 1080p (Jersey 10), así que cumple de sobra. A 640×360, agrandar va por saltos
  enteros de la grilla de la fuente.
- **Contraste (XAG 102):** 4,5:1 contra la parte de menos contraste del fondo (3:1 para lo
  grande; WCAG 2.2 pide 3:1 para los elementos de la UI y los gráficos). **El rojo del
  juego (196, 28, 22) no llega a 4,5:1 nunca, ni con una placa oscura detrás, y a 3:1
  solo en algunas zonas** (3,0:1 como mucho; la mediana con placa, de 2,6 a 3,1). El
  color hueso pasa con placa en todas las zonas (peor caso 4,2:1). **Los subtítulos sin fondo fallan** (peor caso 2,6:1 contra el piso de la
  arena).
- **Color (XAG 103):** con protanopía y deuteranopía, el rojo se vuelve oliva oscuro, como
  los zombies y el piso: **la sangre y el nombre en rojo se pierden**. Ningún dato del
  HUD puede depender solo del rojo.
- **Subtítulos (XAG 104):** el nombre de quien habla, un fondo con opacidad ajustable, y
  que se puedan apagar los subtítulos de fondo por separado.
- **Lo que pide la visión:** la alternativa de mantener a presionar rápido, apagar la
  cámara del tajo y la sacudida (ADR-0031), y los controles reasignables (ADR-0032).
- **Menos destellos:** apagar los *frames* de impacto (ADR-0025 ya lo permite).

**Un modo para quien no distingue el rojo** (P7b, con las mismas simulaciones): la
distancia de color (ΔE en CIELAB) del candidato a la escena (el piso, los zombies y la
mediana de la imagen), en el peor de los tres. Sobre ~20 se distingue de un vistazo.

| Color | Normal | Protanopía | Deuteranopía | Tritanopía |
|---|---|---|---|---|
| Rojo actual (196, 28, 22) | 62 | **10** | 27 | 68 |
| Rojo claro (255, 80, 64) | 67 | **10** | 36 | 70 |
| Magenta (230, 40, 200) | 94 | 55 | 45 | 56 |
| Azul (40, 110, 255) | 84 | 74 | 82 | 40 |
| Cian (40, 220, 230) | 66 | 52 | 53 | 65 |

Aclarar el rojo no sirve con protanopía. El magenta y el cian se distinguen en las
cuatro visiones; el cian, además, con contraste de luminancia (6,5:1 a 7,7:1).

**Con qué estándar medirse:**

| Opción | Qué es | Problema |
|---|---|---|
| **Game Accessibility Guidelines** | La guía de la industria, en tres niveles: básico (lo fácil y de mucho alcance), intermedio (pide planificar) y avanzado (adaptaciones complejas), con seis categorías (motor, cognición, visión, audición, habla y general) | No es una norma: no hay certificación |
| **Xbox Accessibility Guidelines** (XAG) | Las de Microsoft, con números: tamaño del texto, contraste, subtítulos | Pensadas para la tienda de Xbox; algunas piden funciones de la plataforma |
| **Normas ISO** | No hay una ISO de accesibilidad de videojuegos. Las cercanas, vigentes en noviembre de 2026: **ISO/IEC 40500:2025** (es WCAG 2.2 publicada como norma ISO, gratis; reemplazó a la de 2012, que era WCAG 2.0): el texto pide 4,5:1 de contraste (3:1 si es grande, criterio 1.4.3), y los elementos de la UI y los gráficos, 3:1 (criterio 1.4.11). **ISO 9241-171:2025** (guía de accesibilidad del software; reemplazó a la de 2008). **ISO/IEC 30071-1:2019** (código de práctica para el proceso de hacer un producto accesible; confirmada en 2024) | Hablan de software y de la web en general; aplicarlas a un juego es elegir qué criterios tienen sentido |

### 10. La memoria, el presupuesto, las pruebas y la depuración

- **Memoria:** el atlas de las fuentes de píxeles pesa 33 KB (dos fuentes, Latin-1); el
  de las nativas a ×5, 780 KB. Los vértices: de 546 (HUD) a 2544 (menú con HUD), de
  20 B cada uno: ~50 KB por frame en vuelo. **Los subtítulos del prototipo asignan memoria
  cada frame** (13 asignaciones, por `std::string`); en el engine, el corte de línea se
  hace una vez, cuando cambia el subtítulo.
- **Presupuesto:** a 640×360, el HUD cuesta 0,02 ms de GPU y el menú 0,14 ms; la CPU, de
  20 a 70 µs. Cualquiera entra en la reserva de GPU (2,8 ms) y en la de CPU.
- **Pruebas:** el *layout*, el corte de línea, el foco y la navegación son lógica pura,
  testeable sin GPU. Con fuentes de píxeles, la imagen de una pantalla es exacta al
  píxel: una prueba puede comparar una captura contra una de referencia (*golden image*).
- **Depuración:** un panel de ImGui con los controles, el foco, el idioma y el dispositivo,
  y las cajas de cada control en el dibujo de depuración.

## Investigación

### Prototipo medido

Un programa en C++20 con Vulkan 1.3 y SDL3, sobre el prototipo del ADR-0025, medido en
la máquina de referencia el 2026-11-02, con el reloj de la GPU libre. Tres ejecuciones
de 600 frames por configuración. El detalle y cómo repetirlo están en
[`../research/prototipo-de-ui.md`](../research/prototipo-de-ui.md). Un video de 91 s
con 14 variantes acompaña las mediciones, y con `win=1` el prototipo se juega con
teclado, mouse o gamepad.

| # | Qué | Resultado |
|---|---|---|
| P1 | Fuentes | Las de píxeles se rasterizan en su grilla (Jersey 10 a 18 px, Jacquard 12 a 21, monogram a 16). Atlas de ASCII y Latin-1: píxeles 15 KB en 0,5 ms; AA 99 KB en 1,3 ms; SDF 163 KB en 81 ms |
| P2 | Resolución | 640×360: el texto mide ~58 px a 1080p y le caben 84 caracteres por línea. Nativo: nítido, pero de otro estilo. Los *frames* de impacto no invierten la UI |
| P3 | *Immediate* o *retained* | 52 y 68 líneas; 23 y 4,4 µs quieto, 51 y 38 µs con cambios; 0 asignaciones en los dos |
| P4 | Costo | HUD 0,02 ms (640×360) y 0,12 (nativo); menú 0,14 y 2,84 ms; oscurecer la pantalla, 1,8 ms nativo |
| P5 | Variantes del HUD | Clips 1 a 5 del video |
| P6 | Navegación | Teclado, mouse y gamepad con íconos que siguen al dispositivo. Pendiente: la prueba del autor con un DualShock 4 |
| P7 | Accesibilidad | El rojo no llega a 4,5:1; el hueso con placa sí. Con protanopía y deuteranopía, el rojo se pierde |

Lo que mostró, además de los números:

- **El costo de la UI está en los píxeles que cubre, no en los vértices.** A ×5, un
  rectángulo de pantalla completa con mezcla cuesta más que todo el HUD.
- **Dibujar a 640×360 da texto grande por construcción.** El problema no es el mínimo de
  las guías, sino cuánto texto cabe.
- **El rojo, que es la identidad del juego, no sirve para la UI:** ni por contraste ni
  para quien no distingue el rojo.
- **El *retained* gana en reposo, y el *immediate* es más corto y sin estado duplicado.**
  La diferencia de rendimiento es de microsegundos.

**Lo que toca a otros ADRs:**

- **ADR-0019:** una fuente OFL necesita una fe de erratas que acepte la OFL para las
  fuentes.
- **ADR-0021:** si ImGui se usara para el juego, dejaría de ser solo de desarrollo.
- **ADR-0022:** una pasada de UI nueva en la lista fija, a 640×360 o a la resolución
  nativa.
- **ADR-0017:** el cocinador suma las fuentes y la tabla de textos.
- **ADR-0025:** la sangre roja se pierde con protanopía y deuteranopía. Un filtro o un
  color alternativo para la sangre sería suyo.

### Cómo lo hacen otros

- **La clasificación de los elementos de una interfaz:** Fagerholt y Lorentzon (Chalmers,
  2009) cruzan si el elemento existe en la ficción y si existe en el espacio del juego:
  diegético (en los dos), espacial (en el espacio, no en la ficción), meta (en la ficción,
  no en el espacio) y no diegético.
- ***Dead Space*** (Visceral, 2008) lleva la vida en la columna del traje y la munición
  en el arma: todo el HUD es diegético. Dino Ignacio lo contó en GDC 2013.
- **Juegos sin HUD:** *Hellblade: Senua's Sacrifice* (2017) no muestra ninguno, y *Ghost
  of Tsushima* (2020) guía con el viento en vez de con un mapa. *The Last of Us Part II*
  (2020) tiene más de 60 opciones de accesibilidad, muchas para el HUD.
- **La UI *immediate mode*:** Casey Muratori la presentó en 2005; Sean Barrett publicó
  una implementación ese año. Dear ImGui (Omar Cornut) es la más usada para herramientas.
- **La UI *retained*:** Unreal (Slate y UMG), Unity (uGUI y UI Toolkit) y Godot (nodos
  `Control`) arman árboles de controles. RmlUi (MIT) y Gameface (comercial) usan HTML y
  CSS.
- **El texto:** el campo de distancia de Valve (Green, SIGGRAPH 2007) para escalar
  glifos; `stb_truetype` (Sean Barrett) y FreeType para rasterizar; HarfBuzz para las
  escrituras que necesitan *shaping*.
- **La accesibilidad:** las Xbox Accessibility Guidelines (XAG 101 a 104: tamaño,
  contraste, color y subtítulos) y las Game Accessibility Guidelines. Los colores con
  daltonismo se simulan con las matrices de Machado, Oliveira y Fernandes (2009).

## Decisión

Elijo una UI propia en *immediate mode*, dibujada a 640×360 con fuentes de píxeles, para
que la interfaz tenga el mismo píxel que el juego. Por defecto no hay HUD: la energía y
la carga se leen en la caballera y en su espada. Todo lo demás son opciones, y quiero
muchas, porque la accesibilidad es parte del juego y no un agregado. Las ayudas en
pantalla no se presentan como dificultad: el juego no tiene niveles de dificultad, y
prender o apagar una ayuda es decisión del jugador. Los subtítulos no se pueden apagar,
porque pueden ser la única forma de saber lo que se dice. Las fuentes son las
estilizadas (una gótica pixelada para los títulos); la OFL sigue el mismo espíritu que
CC0 y CC BY, así que la acepto para las fuentes.

| # | Dimensión | Elección |
|---|---|---|
| 1 | Modelo | ***Immediate mode* propio** (Muratori): cada frame, cada pantalla llama a sus controles; el estado de la UI es el foco, y el valor de cada opción vive en el juego. **Estado por ID solo para lo que se anima** (una transición, un valor que se desliza), en un arreglo fijo. **Dear ImGui sigue solo de desarrollo** (ADR-0021): cuando tiene el foco del teclado o del mouse, la UI del juego no recibe ese input. Lo general (la lista de dibujo, el texto, los controles y el foco) va en el engine; las pantallas y el HUD, en el juego |
| 2 | Resolución | **A 640×360, en una pasada propia después del postproceso y antes del blit:** los *frames* de impacto no la invierten. **Una sola llamada por frame**, con los vértices por el *ring buffer* del ADR-0005. Alfa premultiplicado, sin escribir el alfa del destino (la máscara de personajes del ADR-0025) |
| 3 | Texto y fuentes | **Fuentes de píxeles cocinadas en su grilla exacta** (1 píxel de diseño = 1 píxel): **Jersey 10** para el texto (18 px por em) y **Jacquard 12** para los títulos (21 px por em), las dos **OFL 1.1** (fe de erratas del ADR-0019). El cocinador las rasteriza con **`stb_truetype`** (dominio público o MIT; se usa bajo MIT), que no llega al juego, a un atlas `R8` con una tabla de glifos y otra de *kerning*. **Caracteres:** ASCII, Latin-1 y lo que las fuentes tengan de Latin Extended-A. **Sin opción de fuente:** solo las estilizadas. **Dos tamaños:** ×1 (18 y 21 px por em) y ×2 (36 y 42 px por em), nítidos porque son múltiplos de la grilla. **El ×2 es una opción para los subtítulos, los avisos de botón y el HUD;** los menús quedan en ×1 (ya miden ~58 px a 1080p, y a ×2 una fila de opción tendría ~23 caracteres) |
| 4 | Textos e idiomas | **Un archivo por idioma** (TSV, UTF-8) con un **ID** y su texto. **Español e inglés.** El cocinador lo convierte en una tabla binaria por idioma y **falla** si a un idioma le falta un ID, si un texto usa un carácter que la fuente no tiene o **si un texto no cabe en su caja**. **El ancho máximo de cada ID es un dato de los assets** (un archivo de anchos: ID → ancho en unidades de 640×360, y el tamaño en que se valida), no del código: el cocinador lo valida y lo guarda en la tabla cocinada, y la pantalla lo lee de ahí, así hay una sola fuente. **Plurales:** una regla por idioma en el código (dos formas en los dos). **Las tablas de las fuentes y las de todos los idiomas se cargan al iniciar** y viven todo el programa: **cambiar de idioma en vivo no carga nada** |
| 5 | HUD | **0 HUD por defecto, con señales diegéticas:** un brillo que sube por el filo con la carga y un destello en la carga máxima (ADR-0025), el vaho del yelmo, la respiración (ADR-0024) y la postura (ADR-0015), más seguidos o más marcados con el cansancio. **Las ayudas se prenden una por una:** energía, carga y avisos de botón. **Dónde van:** en el cuerpo (energía en la espalda y carga en el filo, por defecto), junto a la mira o en las esquinas. **Por defecto según el estilo de cámara** (fe de erratas, ADR-0031): en el cuerpo con la tercera persona; junto a la mira con la primera persona y con las fijas y los rieles, que no siempre ven la espalda. **La línea del tajo y la mira en el mundo se muestran siempre que se apunta** (la mira, decidida en el ADR-0031; la forma de la línea, con el ticket 9 de este ADR). **Ningún dato depende solo del color:** las barras son color hueso sobre una placa oscura, y la energía baja parpadea |
| 6 | Menús y pausa | **Una pila de pantallas:** título, pausa, opciones, muerte y vuelta al checkpoint, y carga. **La pantalla de carga aparece al empezar y al volver a un checkpoint, nunca al caminar:** el mundo es continuo (ADR-0027). Solo la de arriba recibe input. **Las opciones** tienen: las ayudas y dónde van, los subtítulos (estilo, opacidad, tamaño y los de sonidos), el idioma, el color de acento, el tamaño del texto, la opacidad de las placas, los *frames* de impacto, los volúmenes de música, mundo y UI (ADR-0024), los controles (ADR-0032) y **la cámara** (fe de erratas, ADR-0031: el estilo, el FOV de la tercera y de la primera persona, cuánto se cierra al apuntar en cada estilo, la sacudida y la cámara del tajo). **Se guardan en `opciones.txt`, en la carpeta de configuración, al salir de su pantalla** (ADR-0028; fe de erratas del ADR-0021: el menú es de este ADR). **"Continuar"** solo aparece si la tarjeta de memoria tiene el bit de partida, y **"Nueva partida"** pide confirmar si lo tiene (ADR-0028). **La pantalla de carga es fija** (el fondo y "Cargando…"), y **si la simulación espera a una zona más de 0,5 s, aparece un indicador chico** en una esquina (ADR-0028). **La pausa deja el paso fijo en 0 pasos** (nada que grabar), congela el bus del mundo y deja sonar el de la UI (ADR-0024); la escena se sigue dibujando, quieta. **Una opción que cambia la simulación** (apagar la cámara del tajo) **se graba como evento del paso** si cambia durante una grabación (ADR-0021). Las demás no: las ayudas, los subtítulos, el idioma, el color y **los controles** (fe de erratas, ADR-0032: la sensibilidad, las curvas, la reasignación y la alternativa de mantener se aplican antes del paso, y la simulación recibe las acciones ya procesadas) |
| 7 | Navegación | **Listas verticales con un índice de foco:** arriba y abajo mueven el foco; izquierda y derecha cambian el valor; aceptar y volver. **El *hover* del mouse mueve el foco** y el clic acepta. **El stick repite** a los 0,35 s y después cada 0,12 s. **Los íconos siguen al último dispositivo usado** (PlayStation, Xbox o teclado), dibujados con primitivas, sin texturas; **un dispositivo cuenta como usado con un botón o con un eje que pasa la zona muerta** (fe de erratas, ADR-0032). **Las acciones de los menús son fijas, con alternativas:** flechas y WASD, Enter y Espacio, Esc; la cruceta, el stick, A/Cruz y B/Círculo. **El cursor queda encerrado en la ventana mientras tenga el foco** (ADR-0032). Las acciones, sus contextos y la reasignación del juego son del ADR-0032 |
| 8 | Datos | **Pantallas en código:** cada una es una función. Las posiciones, los tiempos y los colores son **CVars** en desarrollo (ADR-0021) y constantes en Release |
| 9 | Accesibilidad | **Meta: todo el nivel básico y el intermedio de las Game Accessibility Guidelines, y del avanzado lo que aplique;** los números, de las XAG (101 a 104); el contraste, de **ISO/IEC 40500:2025** (WCAG 2.2), medido contra la parte de menos contraste del fondo: **4,5:1 para todo el texto**, aunque el de 640×360 cuente como grande (más estricto que el 3:1 de la norma), y **3:1 para las barras, los arcos, los íconos y el foco** (criterio 1.4.11). Una **lista viva** (`docs/ui/accesibilidad.md`) dice qué pauta se cumple, cómo y con qué prueba. **Subtítulos siempre:** el jugador elige el estilo (con fondo, por defecto, con opacidad ajustable, o sin fondo) y el tamaño (×1 o ×2), **pero no los apaga**. El nombre de quien habla va en hueso, no en rojo. **Subtítulos de sonidos importantes** (un gruñido detrás), aparte y apagables. **Tamaño ×2** también para los avisos de botón y el HUD. **Color de acento:** rojo (por defecto), magenta o cian; cambia la sangre y el rojo de las siluetas de los *frames* de impacto (ADR-0025) y los acentos rojos de la UI. **Opacidad de las placas** del HUD y de los subtítulos. Además: mantener en vez de presionar rápido (visión), apagar los *frames* de impacto (ADR-0025), y lo de la cámara (ADR-0031) y los controles (ADR-0032) |
| 10 | Memoria, presupuesto y pruebas | **Una región de UI** (ADR-0010) con topes fijos: 8192 vértices por frame (160 KB; el prototipo usó 2544 como máximo), el estado por ID, el texto de los subtítulos ya cortado, y **las tablas de glifos, de *kerning*, de anchos y de textos de todos los idiomas**, cargadas al iniciar. **Llena, aborta con el nombre de la región:** lo que dibuja la UI lo decide el autor, no el jugador, así que es un error de dimensionamiento. **El corte de línea se hace una vez,** cuando cambia el subtítulo: **0 asignaciones por frame**. **Metas (hipótesis):** 0,2 ms de GPU y 0,1 ms de CPU, sacados de las reservas. **TDD estricto** en el cocinador de fuentes y de textos (ADR-0011 y ADR-0017) y en la lógica pura de la UI (*layout*, corte de línea, UTF-8, foco y repetición del stick). **Tests después** para las pantallas, con capturas de referencia comparadas con la tolerancia que fije el ADR-0011. **Un panel de ImGui** con los controles, el foco, el idioma y el dispositivo, y las cajas de cada control en el dibujo de depuración |

## Consecuencias

- **El frame del ADR-0022 suma una pasada** (fe de erratas del ADR-0022):

  ```
  640×360:  color y profundidad (opacos, screen-door y los VFX del ADR-0025)
            → dibujo de depuración → postproceso (si hay)
            → UI (HUD, subtítulos y menús)
  nativa:   blit nearest al swapchain → ImGui
  ```

- **Lo que se gana:** una interfaz con el píxel del juego, texto grande por construcción
  (unos 58 px a 1080p, más de tres veces el mínimo de XAG 101), una UI que cuesta
  0,02 ms de GPU durante el juego y que no necesita bibliotecas en el juego.
- **Lo que se sacrifica:**
  - el texto no se puede achicar: en una línea caben ~84 caracteres, y los textos largos
    (un diálogo, una descripción de opción) se cortan en más líneas;
  - el tamaño de texto va por saltos enteros (×1 o ×2), no por porcentajes;
  - **los idiomas sin alfabeto latino** (griego, cirílico, CJK, árabe) necesitan otras
    fuentes y, algunos, *shaping*: no entran sin una fuente nueva;
  - no hay opción de fuente más simple para quien lee con dificultad.
- **Las fuentes** entran en la carpeta de assets con su licencia OFL, una anotación en
  `REUSE.toml`, el texto de la licencia en `LICENSES/` y una línea en `CREDITS.md`, en el
  ticket que las suma (ADR-0019). `stb_truetype` se suma a `CREDITS.md` y a las
  dependencias del cocinador (ADR-0006) en su ticket.
- **El cocinador** (ADR-0017) suma las fuentes (atlas, glifos y *kerning*) y las tablas de
  textos, y falla el build por un ID que falta, un carácter que la fuente no tiene o un
  texto que no cabe.
- **El color de acento** pasa a ser un valor del frame que leen la sangre, las marcas y
  el rojo de las siluetas de los *frames* de impacto (ADR-0025) y la UI, en vez de un rojo
  fijo.
- **Presupuestos (hipótesis):** una fila de GPU "UI: HUD, subtítulos y menús (640×360)" de
  0,2 ms, sacada de la reserva (2,8 → 2,6 ms), y una de CPU de 0,1 ms (mediana), sacada
  de la reserva de CPU. A 640×360, el prototipo midió ~34 µs el HUD con subtítulos y de
  23 a 51 µs el menú de pausa, con un p99 de 108 µs al navegar. Ese pico no preocupa: el
  menú solo aparece en la pausa, con 0 pasos de simulación, y en esos frames sobran unos
  6 ms de CPU.
- **Memoria (ADR-0010):** la región de UI (~200 KB: los vértices que se arman en la CPU
  antes de copiarse al *ring buffer*, el estado por ID, los subtítulos y las tablas de
  glifos, de *kerning*, de anchos y de textos de los dos idiomas, unas decenas de KB) cae
  en "Mundo y ECS"; los vértices de cada frame en vuelo usan la subida de geometría dinámica (160 KB
  de los 8 MiB). El atlas (~165 KB con los dos tamaños: 33 KB a ×1 y unos 130 a ×2) cae en
  "Recursos de la GPU".
- **Fe de erratas en otros ADRs** (al aceptar): ADR-0019 (la OFL para las fuentes),
  ADR-0022 (la pasada de UI), ADR-0025 (el color de acento de la sangre y de las
  siluetas), ADR-0017 (las fuentes, los textos y los anchos en el cocinador), ADR-0006
  (`stb_truetype` en el cocinador), ADR-0010 (la región de UI), ADR-0021 (el menú de
  opciones es de este ADR; el guardado, del ADR-0028) y `presupuestos.md`.
- **Tickets, en orden** (cada uno crece desde el anterior):
  1. el cocinador de fuentes: atlas, glifos y *kerning*, en TDD estricto;
  2. el cocinador de textos: tablas por idioma, el archivo de anchos y sus validaciones,
     en TDD estricto;
  3. la lista de dibujo y la pasada de UI a 640×360;
  4. el texto: UTF-8, *layout* y corte de línea, en TDD;
  5. los controles en *immediate mode*, el foco y la navegación, en TDD;
  6. la pila de pantallas y la pausa (junto al game loop y al ADR-0024);
  7. los íconos por dispositivo (junto al ADR-0032);
  8. los subtítulos: estilos, tamaño, nombre de quien habla y sonidos importantes;
  9. las ayudas del HUD en sus tres ubicaciones, los avisos de botón y la línea del tajo
     (junto al ADR-0031);
  10. las señales diegéticas (junto al ADR-0015, ADR-0024 y ADR-0025);
  11. la accesibilidad: el color de acento, el tamaño ×2 (subtítulos, avisos y HUD), la
      opacidad de las placas y la lista viva de pautas;
  12. el panel de ImGui y las cajas en el dibujo de depuración.
- **Abierto:**
  - **la prueba del autor con un DualShock 4** (P6 del prototipo; es la misma que deja
    pendiente el ADR-0032);
  - **la tapa del corte con el color de acento:** el interior de la carne (ADR-0022)
    también es rojo; si cambia con el acento lo decide su ticket, mirando el juego;
  - los idiomas sin alfabeto latino, si alguna vez se suman;
  - el arte de la pantalla de título y el fondo de la pantalla de carga (decidido en el
    ADR-0028: una pantalla fija, sin barra);
  - si una espera por una zona que no llegó (ADR-0027: la simulación corre 0 pasos)
    muestra algún indicador (decidido en el ADR-0028: uno chico, pasados 0,5 s);
  - qué pautas avanzadas de las Game Accessibility Guidelines aplican, en la lista viva.

## Cómo lo defiendo

**¿Por qué una UI propia y no Dear ImGui, que ya está en el proyecto?**
Porque ImGui es una herramienta de desarrollo: no se compila en Release, y está pensado
para paneles, no para un menú que se maneja con un gamepad y tiene el estilo del juego.
La UI del juego es chica (un HUD, subtítulos y cinco pantallas): en el prototipo, el menú
de pausa entero eran 52 líneas. Escribirla me enseña lo mismo que ImGui hace por dentro.

**¿Por qué *immediate mode* si el *retained* fue más rápido en reposo?**
Porque la diferencia es de microsegundos: 23 contra 4,4 µs con el menú quieto, y casi lo
mismo cuando algo cambia. Lo que pesa es el diseño. En *immediate mode* el valor de cada
opción vive en un solo lugar, el juego, y la UI solo guarda el foco. En *retained* hay
dos copias del estado y un caché que se rompe si su llave se olvida de algo. Es la idea
de Casey Muratori (2005) y la de Dear ImGui.

**¿Por qué dibujas la UI a 640×360 y no a la resolución de la pantalla?**
Porque a la resolución nativa el texto se ve nítido pero parece de otro juego, encima de
una escena de píxeles grandes. Además, el costo de la UI está en los píxeles que cubre:
el mismo menú midió 0,14 ms a 640×360 y 2,84 ms a la resolución nativa, y solo oscurecer
la pantalla detrás costaba 1,8 ms. Y el texto queda grande por construcción: unos 58 px
a 1080p, más de tres veces el mínimo de las guías de Xbox.

**¿No es un problema que el texto no se pueda achicar?**
Es lo que sacrifico. En una línea caben unos 84 caracteres, así que los textos largos se
cortan en más líneas. A cambio, el ancho de cada texto se conoce al cocinar, y el
cocinador falla si un texto no cabe en su caja: un error de traducción se ve al
compilar, no en el juego. Agrandar sí se puede, al doble, porque una fuente de píxeles
sigue nítida en múltiplos enteros de su grilla.

**¿Por qué rasterizas las fuentes en el cocinador y no en el juego?**
Porque a 640×360 solo uso dos fuentes en dos tamaños fijos: no hay nada que decidir en
el juego. `stb_truetype` queda en el cocinador, como el lector de FLAC del audio, y el
juego carga un atlas de unos 165 KB. Probé SDF: tarda 160 veces más en cocinarse y no
aporta nada a tamaño fijo.

**El ADR-0019 no aceptaba la OFL. ¿Por qué la cambiaste?**
Porque la OFL sigue el mismo espíritu que CC0 y CC BY: deja usar la fuente en un juego,
incluso comercial, y solo pide no venderla sola ni usar su nombre en una versión
modificada. Es la licencia estándar de las fuentes libres, y las que mejor funcionaron
(una gótica pixelada para los títulos) son OFL. La acepté solo para las fuentes, como
fe de erratas.

**Si la vida es la energía, ¿cómo sabe el jugador cuánta le queda sin HUD?**
Por la caballera: un brillo sube por el filo con la carga y destella en la máxima; el
vaho del yelmo sale más seguido, la respiración se acelera y la postura cae con el
cansancio. Se lee "poca" o "mucha", no un porcentaje, y es lo que quiero: mirar la
pelea, no una barra. Quien necesite el dato exacto prende las ayudas, una por una.

**¿Las ayudas no son un modo fácil?**
El juego no lo dice nunca. No tiene niveles de dificultad, y prender una ayuda es una
decisión del jugador. Para alguien que no oye la respiración o que no distingue el rojo,
la ayuda no hace el juego más fácil: lo hace jugable. Por eso están en las opciones de
accesibilidad.

**¿Por qué la barra de energía va en la espalda?**
Porque la cámara mira a la caballera desde atrás: la espalda está siempre a la vista,
como la columna del traje en *Dead Space*. La carga va en el filo, que es justo lo que
va a cortar. Si a alguien no le sirve (la sangre o un zombie la tapan), la puede poner
junto a la mira, donde ya mira al apuntar, o en las esquinas.

**¿Por qué no se pueden apagar los subtítulos?**
Porque pueden ser la única forma de saber lo que se dice: es posible que el juego no
tenga voces al principio. Se elige el estilo, el fondo y el tamaño. Por defecto van con
fondo: sin él, el piso claro de la arena les dejaba un contraste de 2,6:1, y durante los
*frames* de impacto el fondo es blanco y el texto desaparecía.

**El rojo es la identidad del juego. ¿Qué pasa con quien no lo distingue?**
Medí la escena con simulaciones de protanopía y deuteranopía: el rojo se vuelve oliva,
del mismo color que los zombies y el piso, y la sangre se pierde. Aclarar el rojo no
sirve. Por eso hay un color de acento que se elige (rojo, magenta o cian): el magenta y
el cian se distinguen en todas las visiones. Y ningún dato del HUD depende solo del
color: las barras son color hueso sobre una placa oscura, y la energía baja parpadea.

**¿Con qué estándar mides la accesibilidad?**
No hay una norma ISO para videojuegos. Me pongo como meta todo el nivel básico y el
intermedio de las Game Accessibility Guidelines, que son la guía de la industria. Los
números salen de las guías de Xbox, y el contraste, de ISO/IEC 40500:2025, que es WCAG
2.2 como norma ISO: 4,5:1 para todo el texto y 3:1 para las barras y los íconos. Una
lista viva dice qué pauta se cumple y cómo se probó.

## Referencias

- Erik Fagerholt y Magnus Lorentzon, *Beyond the HUD: User Interfaces for Increased
  Player Immersion in FPS Games*, tesis de máster, Chalmers University of Technology,
  2009.
- Dino Ignacio, "Crafting Destruction: The Evolution of the Dead Space User Interface",
  GDC 2013: <https://gdcvault.com/play/1017723/>
- Casey Muratori, "Immediate-Mode Graphical User Interfaces", 2005:
  <https://caseymuratori.com/blog_0001>
- Omar Cornut, *Dear ImGui*: <https://github.com/ocornut/imgui>
- Mikke89 y colaboradores, *RmlUi* (MIT): <https://github.com/mikke89/RmlUi>
- Chris Green, "Improved Alpha-Tested Magnification for Vector Textures and Special
  Effects", SIGGRAPH 2007 (*Advanced Real-Time Rendering in 3D Graphics and Games*).
- Viktor Chlumský, *msdfgen* (MIT): <https://github.com/Chlumsky/msdfgen>
- Sean Barrett, `stb_truetype.h` (dominio público o MIT): <https://github.com/nothings/stb>
- Microsoft, Xbox Accessibility Guidelines, versión 3.2 (2023): XAG 101 (texto), 102
  (contraste), 103 (color) y 104 (subtítulos):
  <https://learn.microsoft.com/gaming/accessibility/guidelines>
- Game Accessibility Guidelines (niveles básico, intermedio y avanzado):
  <https://gameaccessibilityguidelines.com/full-list/>
- ISO/IEC 40500:2025, *Information technology — W3C Web Content Accessibility Guidelines
  (WCAG) 2.2* (edición 2, 2025-09; reemplaza a la de 2012, WCAG 2.0):
  <https://www.iso.org/standard/91029.html>
- W3C, *Web Content Accessibility Guidelines (WCAG) 2.2*, criterios 1.4.3 (*Contrast
  (Minimum)*) y 1.4.11 (*Non-text Contrast*):
  <https://www.w3.org/WAI/WCAG22/Understanding/contrast-minimum.html>
- ISO 9241-171:2025, *Ergonomics of human-system interaction — Part 171: Guidance on
  software accessibility* (reemplaza a la de 2008).
- ISO/IEC 30071-1:2019, *Information technology — Development of user interface
  accessibility — Part 1: Code of practice for creating accessible ICT products and
  services* (confirmada en 2024): <https://www.iso.org/standard/70913.html>
- Gustavo M. Machado, Manuel M. Oliveira y Leandro A. F. Fernandes, "A
  Physiologically-based Model for Simulation of Color Vision Deficiency", *IEEE
  Transactions on Visualization and Computer Graphics*, 2009.
- SIL International, *SIL Open Font License 1.1*: <https://openfontlicense.org>
- Unicode CLDR, reglas de plural:
  <https://cldr.unicode.org/index/cldr-spec/plural-rules>
- Fuentes del prototipo: Jersey 10 y Jacquard 12 (Sarah Cadigan-Fried, OFL), monogram
  (datagoblin), Noto Sans y Noto Serif (Google, OFL).
