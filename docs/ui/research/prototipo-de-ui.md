# Prototipo de UI, HUD y menús

Nota de investigación del [ADR-0026](../adr/0026-ui-hud-y-menus.md). Guarda las
mediciones que sostienen la decisión, para poder repetirlas.

Fecha: 2026-11-02, en la máquina de referencia del ADR-0020 (Dell XPS 13 9360,
i7-8550U, Intel UHD 620). Son mediciones de laboratorio para decidir, no del protocolo
del ADR-0020. **La GPU no se fijó en 700 MHz** como en los ADR-0022 y ADR-0025: el reloj
quedó libre (de 300 a 1150 MHz) y la frecuencia leída cada 15 frames promedió de 290 a
390 MHz, porque la escena deja a la GPU casi siempre en el mínimo. Los tiempos de GPU son
entonces **pesimistas** frente a los de 700 MHz, y sirven para comparar las variantes
entre sí (todas corrieron en las mismas condiciones) y para saber el orden de magnitud.
La CPU quedó libre. El prototipo es código descartable, fuera del repo.

| Qué | Versión |
|---|---|
| Linux | 7.2.8 |
| Mesa (ANV) | 26.2.5, Vulkan 1.4.354 |
| GCC | 16.2.1, con `-O2`, sin `-march` (ADR-0008) |
| glslc | shaderc 2026.4, `--target-env=vulkan1.3 -O` |
| SDL3 | 3.4.18 |
| stb_truetype | 1.26 (dominio público o MIT) |
| Encabezados de Vulkan | Khronos Vulkan-Headers v1.4.363 |

## El prototipo

Es el prototipo del ADR-0025 con lo de este ADR encima: la escena de 50 zombies a
640×360, con niebla, *dithering*, los VFX y el blit *nearest*. Suma:

- **Una caballera provisional:** la malla del zombie, quieta, con tono de metal, y una
  espada (una caja de 1 m). La cámara es la de la visión: 2,3 m detrás, 1,9 m de alto y
  0,5 m a la izquierda, con 51° de campo de visión.
- **Un guion de 5 s que se repite:** carga de 1,4 s y tajo que corta; carga corta de
  0,6 s y tajo que rebota. La carga sube rápido al principio y cada vez más lento (visión:
  `1 − (1 − t)²`). La energía baja 0,30 por segundo mientras carga y sube 0,07 por segundo
  en reposo, así que en el segundo ciclo la caballera llega cansada (energía bajo 0,25).
  Un aviso de botón ("Terminar el corte") sale un segundo después de cada corte, como si
  la hoja hubiera quedado clavada.
- **Señales diegéticas** (con 0 HUD), aproximadas, porque la animación es del ADR-0015:
  - un brillo que recorre el filo desde la empuñadura hasta la punta a medida que sube la
    carga, con un destello en la carga máxima (partículas emisivas del ADR-0025);
  - vaho del yelmo, cada 1,4 s con energía alta, cada 0,8 s con energía media y cada
    0,45 s con poca energía, con más partículas cuanto más cansada;
  - la postura: el cuerpo se inclina hasta 16° y la punta de la espada baja con el
    cansancio, y se mueve un poco con la respiración.
- **La UI**, en `ui.h` (~520 líneas sin comentarios):
  - **un atlas R8** con las fuentes de los dos destinos, rasterizadas con
    `stb_truetype`. Las de píxeles (Jersey 10 para el texto y Jacquard 12 para los
    títulos, las dos con licencia OFL) se rasterizan sin antialiasing en su grilla
    exacta: Jersey 10 a 18 px por em y Jacquard 12 a 21. Las nativas son Noto Sans y Noto
    Serif Bold con antialiasing, a 12,5 y 17 px por em multiplicados por el factor de
    escala;
  - **una lista de dibujo** con vértices de 20 B (posición, UV y color RGBA8) y
    primitivas: rectángulo, triángulo, línea gruesa, arco, disco y texto UTF-8 con
    *kerning*. Todo se describe en unidades virtuales de 640×360; cada lista tiene una
    escala (1 a 640×360, o el factor del blit a la resolución nativa);
  - **una tabla de textos** con un ID por texto y una columna por idioma (español e
    inglés);
  - **el HUD** en tres variantes, **los subtítulos** (nombre de quien habla en rojo,
    corte de línea a 520 unidades, con o sin fondo) y **el menú de pausa** escrito dos
    veces, en *immediate mode* y en *retained mode*, con el mismo aspecto;
  - **los íconos de los botones** dibujados con primitivas, sin texturas: ✕ y ○ de
    PlayStation, A y B de Xbox, y teclas con su nombre.
- **El frame:** la UI va en una pasada propia de una sola llamada. **A 640×360**, después
  del postproceso (así los *frames* de impacto del ADR-0025 no la invierten) y antes del
  blit. **A la resolución nativa**, después del blit. El modo **mixto** dibuja las formas
  a 640×360 y el texto a la resolución nativa. El pipeline es el mismo en los dos
  destinos (solo cambia el formato): alfa premultiplicado, sin escribir el alfa del
  destino, que a 640×360 lleva la máscara de personajes del ADR-0025.
- **Con `win=1`**, una ventana a pantalla completa con el mayor factor entero que cabe y
  bordes negros. El menú se maneja con teclado, mouse o gamepad (SDL3), y los íconos
  cambian al último dispositivo usado.

## Resultados

### P1: fuentes

| Fuente | Licencia | Grilla | Tamaño en que 1 píxel de diseño = 1 px | Cobertura |
|---|---|---|---|---|
| Jacquard 12 (gótica pixelada) | OFL | 60 unidades | 21 px por em | ASCII, 84 de 96 de Latin-1, 99 de 128 de Latin Extended-A; sin griego, cirílico ni CJK |
| Jersey 10 | OFL | 75 unidades | 18 px por em | igual que Jacquard 12 |
| VT323 | OFL | (a mano) | 20 px por em | Latin-1 completo, Latin Extended-A casi completo |
| Press Start 2P | OFL | 125 unidades | 8 px por em | Latin-1, Latin Extended-A, 74 de griego y 178 de cirílico |
| Pixelify Sans | OFL | no tiene (curvas) | — | — |
| monogram extended | CC0 en itch.io; el repositorio trae una licencia MIT | 64 unidades (a mano: algún punto cae fuera) | 16 px por em | 95 de 96 de Latin-1, 127 de 128 de Latin Extended-A, 87 de griego y 72 de cirílico |

- **Las licencias chocan con el ADR-0019:** los assets de terceros solo pueden ser CC0 o
  CC BY, y las fuentes de Google Fonts son OFL (SIL Open Font License 1.1), la licencia
  estándar de las fuentes libres. La OFL deja usarlas e incluirlas en un juego, incluso
  comercial, pero no venderlas solas, y una versión modificada no puede usar el nombre
  reservado de la fuente. Hay fuentes de píxeles CC0 que cubren el español: monogram
  extended (datagoblin), Public Pixel (GGBotNet) y el Not Jam Font Pack. La de monogram
  tiene una ambigüedad: la página de itch.io dice CC0, pero el `LICENSE` de su repositorio
  es MIT (2024). Hay que aclararla antes de usarla. **Ninguna gótica pixelada CC0** se
  encontró para los títulos.
- **monogram extended a 16 px** es más fina (trazos de 1 píxel) y más baja que Jersey 10
  a 18: mide 12,7 unidades de alto (38 px a 1080p, sobre el mínimo de XAG 101) contra
  19,3, con el mismo ancho por letra (~6 unidades): en una línea de 520 unidades caben
  86 caracteres de monogram y 84 de Jersey 10. Jacquard 12 a 21 px: 21 de alto y 65
  caracteres.
- **La grilla se calcula** como el máximo común divisor de las coordenadas de los
  contornos. Si una fuente de píxeles se rasteriza a otro tamaño, los píxeles se deforman.
  Pixelify Sans no tiene grilla: con curvas, sus "píxeles" no caen en píxeles enteros.
- **Las de 12 de 96 que les faltan a Jacquard 12 y Jersey 10 en Latin-1** son símbolos
  poco usados; las letras del español (ñ, tildes, ü, ¿, ¡) están todas.
- **Cocinar el atlas de ASCII y Latin-1** (en el cocinador, no en el juego):

  | Modo | Bytes de glifos | Tiempo |
  |---|---|---|
  | Píxeles (Jersey 10 a 18 px) | 14,8 KB | 0,5 ms |
  | Antialiasing (Noto Sans a 36 px) | 99 KB | 1,3 ms |
  | SDF (Noto Sans a 32 px, `stbtt_GetCodepointSDF`) | 163 KB | 81 ms |

- **En el juego:** las dos fuentes de píxeles ocupan **33 KB** del atlas. Las dos nativas,
  a ×5 (3200×1800), **780 KB**: no cabían en un atlas de 512², y crecen con el cuadrado del
  factor.
- **El SDF** (Green, 2007) sirve para escalar un mismo atlas a muchos tamaños, para
  contornos y sombras. Con texto a un tamaño fijo, no aporta nada que el atlas con
  antialiasing no dé, y a 640×360 se pierde: el texto de píxeles no tiene bordes suaves.
- **El texto a 640×360 con antialiasing se ve borroso** (los rótulos de las capturas del
  P1 en Noto Sans a 9 px). Una fuente de píxeles en su grilla es la única que queda nítida
  a esa resolución.

### P2: a qué resolución se dibuja

Capturas del mismo menú en los tres modos, y los clips 8 a 10 y 13 del video.

- **A 640×360:** el texto y las formas tienen el mismo píxel que la escena; la UI parece
  parte del juego. Jersey 10 a 18 px por em mide **~19 unidades** de alto (del descendente
  al ascendente), o sea **~58 px a 1080p** (×3) y **~97 px a ×5**. XAG 101 pide un mínimo
  de 18 px a 1080p en PC y 26 en consola: el texto a 640×360 queda **3,2 veces por encima
  del mínimo de PC** sin hacer nada. El límite es el contrario: **en una línea de 520
  unidades caben unos 84 caracteres** de Jersey 10, y el texto no se puede achicar sin
  cambiar de fuente. **Agrandarlo sí se puede:** una fuente de píxeles a 2 o 3 píxeles
  por píxel de diseño sigue nítida (Jersey 10 a 36 px por em), así que la opción de
  tamaño de texto de XAG 101 (hasta 200 %) cabe a 640×360, en saltos enteros.
- **A la resolución nativa:** el texto es nítido y fino, pero se ve como de otro juego,
  sobre una escena de píxeles grandes. Permite texto más chico y líneas más largas.
- **Mixto:** texto nítido sobre formas de píxeles. Los íconos quedan mitad y mitad (el
  círculo pixelado y la letra nítida).
- **Los *frames* de impacto:** con la UI después del postproceso, las siluetas invierten
  la escena pero no el HUD ni los subtítulos (clip 14). Pero el fondo de esos 3 frames es
  blanco, y **el subtítulo color hueso sin fondo casi desaparece** durante 50 ms. Con
  fondo, se sigue leyendo.

### P3: *immediate* frente a *retained*

El mismo menú de pausa (8 filas, título y pie con íconos), escrito de las dos formas.
"Quieto" es sin input; "recorrido" es un cambio cada 30 frames (foco o valor). Mediana y
p99 de 540 frames, mediana de 3 ejecuciones, a ×5.

| Variante | Líneas de código | CPU quieto (µs) | CPU recorrido (µs) | Asignaciones por frame |
|---|---|---|---|---|
| ***Immediate*** (`ImmMenu`) | 52 | 22,6 / 61 | 51 / 108 | 0 |
| ***Retained*** (`RetMenu`, con los vértices guardados) | 68 | **4,4** / 26 | 38 / 85 | 0 |

- **Las dos comparten** 31 líneas: el panel, una fila y el pie.
- **El *retained* gana en reposo** porque reutiliza los vértices mientras nada cambie (la
  llave junta el foco, los valores, el idioma y el dispositivo). Cuando algo cambia, los
  dos cuestan lo mismo.
- **El *immediate* es más corto y no tiene estado que sincronizar:** el valor de una
  opción vive en `Options` y nada más. En el *retained*, cada control guarda un puntero a
  su valor, y el caché necesita una llave que no se olvide de nada: si un cambio no entra
  en la llave (en el prototipo, el foco, los valores, el idioma y el dispositivo), el menú
  muestra algo viejo.
- **El foco con el gamepad cuesta lo mismo en las dos:** un índice, que sube y baja en
  una lista. El *immediate* lo resuelve con un frame de atraso (sabe cuántos controles hay
  al terminar), como Dear ImGui.
- **Los costos son de microsegundos:** cualquiera de las dos entra en el frame con
  sobra. La diferencia que importa es de diseño, no de rendimiento.

### P4: costo

Mediana de 540 frames y de 3 ejecuciones, a ×5, con el reloj libre (~300 MHz).

| Qué | GPU (ms) | CPU (µs) | Vértices |
|---|---|---|---|
| Escena sin UI | 0,003 | 0,5 | 0 |
| Ayudas A (en el cuerpo) + subtítulos con fondo, a 640×360 | **0,021** | 34 | 546 |
| Lo mismo, nativo | 0,122 | 65 | 546 |
| Lo mismo, mixto | 0,062 | 65 | 546 |
| Ayudas B (junto a la mira), a 640×360 / nativo | 0,019 / 0,116 | 35 / 70 | 744 / 1248 |
| Ayudas C (esquinas), a 640×360 / nativo | 0,021 / 0,118 | 34 / 65 | 546 |
| Menú de pausa, a 640×360 / nativo | **0,14 / 2,84** | 23 / 40 | 1524 / 2028 |
| Menú sin oscurecer la pantalla detrás, a 640×360 / nativo | 0,06 / 0,86 | — | — |

- **El costo de GPU de la UI está en los píxeles que cubre, no en los vértices.** El menú
  a la resolución nativa cuesta 20 veces más que a 640×360 con casi los mismos vértices:
  ×5 son 25 veces más píxeles. **Oscurecer la pantalla detrás del menú cuesta 1,8 ms a
  ×5** (un rectángulo de 5,76 Mpx con mezcla) y 0,08 ms a 640×360.
- **El HUD es barato en cualquier modo:** 0,02 ms a 640×360 y 0,12 ms nativo.
- **La CPU:** de 20 a 70 µs, casi todo en el texto (buscar cada glifo en una tabla hash y
  armar 6 vértices). **Los subtítulos asignan memoria cada frame** (13 asignaciones):
  arman `std::string` para partir las líneas. En el engine, el corte de línea se haría una
  vez, cuando cambia el subtítulo, en la arena del frame o en una región fija (ADR-0010).
- **Con los mismos vértices, nativo cuesta el doble de CPU** que a 640×360. No se
  perfiló; la hipótesis es el *kerning*: el prototipo lo busca en la fuente en cada par
  de letras (`stbtt_GetCodepointKernAdvance`), y Noto Sans tiene una tabla mucho más
  grande que Jersey 10. En el engine, el *kerning* se cocina en una tabla.

### P5: las variantes del HUD

Clips 1 a 5 del video, y las hojas de capturas del guion (frames 50, 120, 350 y 560).
Son impresiones de Claude sobre el video y las capturas; las que cuentan son las del
autor.

- **0 HUD con señales diegéticas** (clip 1 frente al 2): el brillo es un punto claro de
  ~6 cm que sube por la hoja con la carga. La primera versión (menos de 1 cm) no se veía:
  **el tamaño de la señal decide si existe**. Se lee mientras la espada está en guardia
  alta y a la vista, pero la carga no se puede leer con precisión: se sabe "poca" o "casi
  llena", no "60 %". El destello de la carga máxima sí se nota. El vaho y la postura se
  leen como "cansada" sin mirar una barra, pero solo en conjunto, y la diferencia entre
  energía media y baja es sutil. Sin animación real, la prueba es parcial.
- **A, en el cuerpo:** la barra de la espalda se lee siempre que la caballera está en
  pantalla, porque la cámara la mira de atrás. La de la carga en el filo es la más
  "natural": la carga recorre la misma hoja que va a cortar. Problemas: la de la espalda
  queda tapada por la sangre, por un zombie que pasa entre la cámara y la caballera o por
  la caballera semitransparente (visión); la del filo depende de que la espada esté a la
  vista.
- **B, junto a la mira:** los arcos están donde el jugador ya mira al apuntar (el centro
  de la pantalla decide el corte, según la visión), así que se leen sin mover los ojos. Se
  apagan solos con la energía llena y sin cargar. Son chicos: con la horda detrás, el arco
  de la energía baja se confunde con el fondo.
- **C, esquinas:** el más legible y el que más saca de la escena; es la referencia
  clásica.
- **El aviso de botón** junto al zombie se lee bien en A y B; en C va abajo al centro.

### P6: navegación

Con `win=1`, el menú se maneja con:

- **teclado:** flechas o WASD, Intro o Espacio para aceptar, Esc para pausar y volver;
- **mouse:** el *hover* mueve el foco y el clic acepta;
- **gamepad:** la cruceta o el stick izquierdo (con repetición: 0,35 s y después cada
  0,12 s), Cruz o A para aceptar, Círculo o B para volver, y Options o Start para pausar.
  Fuera del menú, Triángulo o Y cambia la variante de las ayudas, y Cuadrado o X las
  prende y apaga.

**Los íconos siguen al último dispositivo usado** (`SDL_GetGamepadType`: PS3, PS4 y PS5
dan los de PlayStation, el resto los de Xbox). Los clips 8, 11 y 12 muestran los tres
juegos de íconos. **Pendiente: la prueba del autor con un DualShock 4** en la máquina de
referencia.

### P7: accesibilidad

**Contraste** (la fórmula de WCAG 2, la que usan XAG 102 e ISO/IEC 40500:2025) de los
colores del HUD contra el fondo real
detrás de cada elemento, en 4 frames del guion sin UI. XAG 102 pide medir contra la parte
de menos contraste y pide 4,5:1 (3:1 para elementos grandes). La "placa" es el fondo
negro al 59 % que el prototipo pone detrás de las barras.

| Zona | Color | Sin placa: peor / mediana | Con placa: peor / mediana |
|---|---|---|---|
| Espalda (A) | hueso (226, 219, 200) | 4,04 / 6,22 | **10,3** / 11,9 |
| Espalda (A) | rojo (196, 28, 22) | 1,03 / 1,44 | 2,39 / 2,76 |
| Filo (A) | hueso | 1,14 / 8,62 | 4,17 / 13,0 |
| Filo (A) | rojo | 1,02 / 2,00 | 1,00 / 3,02 |
| Mira (B) | hueso | 2,97 / 9,83 | **9,08** / 13,5 |
| Mira (B) | rojo | 1,01 / 2,28 | 2,11 / 3,12 |
| Esquina (C) | hueso | 8,50 | 13,0 |
| Esquina (C) | rojo | 1,97 | 3,01 |
| Subtítulos | hueso | **2,61** / 4,99 | **8,57** / 11,2 |
| Subtítulos | rojo | 1,00 / 1,23 | 1,99 / 2,59 |

- **El rojo del juego no sirve como color de UI sobre la escena:** nunca llega a 4,5:1,
  ni con placa. Contra negro puro mide 3,52. Un rojo más claro, (255, 80, 64), mide 6,47
  contra negro y 5,89 contra la escena oscurecida, pero se acerca al hueso (2,35:1 entre
  los dos).
- **El hueso pasa con placa** en todas las zonas. **Sin placa, los subtítulos fallan**
  (peor caso 2,61): el piso claro de la arena queda detrás. XAG 104 y las guías de
  subtítulos piden que el fondo se pueda prender y que su opacidad se pueda ajustar.
- **Daltonismo** (matrices de Machado, Oliveira y Fernandes, 2009, severidad 1): con
  protanopía, el rojo se vuelve (87, 76, 17) y con deuteranopía (127, 112, 6), un oliva
  oscuro. **La sangre y el nombre en rojo se pierden** contra el piso y los zombies, que
  también son oliva. Con tritanopía, el rojo sigue rojo. Contra el hueso, el rojo sigue
  distinguible por luminancia (6,2:1 y 3,6:1). Las capturas
  `p7_daltonismo_A.png` y `p7_daltonismo_B.png` muestran el original y las tres
  simulaciones.
- **Lo que esto implica:** ningún dato del HUD puede depender solo del color rojo (XAG
  103). La forma o la posición tienen que decir lo mismo: la barra llena o vacía, el arco
  que crece, el parpadeo de la energía baja.

### P7b: un color que reemplace al rojo

`tools/accent.py`: para cada color candidato y cada visión (normal y las tres
simulaciones de Machado), la distancia ΔE76 en CIELAB contra el piso, los zombies y la
mediana de la escena (se reporta la menor), y el contraste de luminancia contra la
mediana.

| Color | Normal | Protanopía | Deuteranopía | Tritanopía |
|---|---|---|---|---|
| Rojo actual (196, 28, 22) | ΔE 62 / 2,0:1 | **ΔE 10** / 1,4:1 | ΔE 27 / 2,4:1 | ΔE 68 / 2,2:1 |
| Rojo claro (255, 80, 64) | 67 / 3,6 | **10** / 2,6 | 36 / 4,3 | 70 / 3,0 |
| Magenta (230, 40, 200) | 94 / 3,1 | 55 / 2,4 | 45 / 3,5 | 56 / 3,2 |
| Azul (40, 110, 255) | 84 / 2,7 | 74 / 3,1 | 82 / 2,6 | 40 / 3,2 |
| Cian (40, 220, 230) | 66 / 7,0 | 52 / 7,7 | 53 / 6,5 | 65 / 7,4 |
| Blanco frío (235, 240, 255) | 59 / 10,3 | 60 / 10,3 | 58 / 10,3 | 55 / 10,3 |

- **Aclarar el rojo no sirve con protanopía** (ΔE 10, igual que el actual): el rojo se
  vuelve oliva, como el resto de la escena.
- **El magenta y el cian se distinguen en las cuatro visiones;** el cian, además, con
  contraste de luminancia. El blanco frío se distingue, pero se confunde con el color
  hueso de la UI.

## El video

`comparativas_adr0026.mp4` (1280×720, la imagen de 640×360 a ×2, rotulado):

1. 0 HUD (por defecto): brillo en el filo con la carga, vaho y postura con el cansancio
2. 0 HUD sin las señales diegéticas, para comparar
3. Ayudas A, en el cuerpo: energía en la espalda, carga en el filo, aviso de botón
4. Ayudas B, junto a la mira: dos arcos que se apagan en reposo
5. Ayudas C, esquinas (referencia clásica)
6. Subtítulos sin fondo (español), fuente de píxeles a 640×360
7. Subtítulos con fondo (inglés)
8. Menú de pausa a 640×360 (*immediate mode*), íconos de PS
9. El mismo menú a la resolución nativa (Noto Sans y Noto Serif)
10. Mixto: formas a 640×360, texto a la resolución nativa
11. Íconos de teclado
12. Íconos de Xbox
13. Ayudas A a la resolución nativa
14. *Frames* de impacto: la UI se dibuja después del postproceso y no se invierte

## Cómo repetirlo

El prototipo vive fuera del repo (`u26/`, junto al del ADR-0025). Necesita los
encabezados de Vulkan, SDL3, `stb_truetype.h` y las fuentes (Google Fonts, carpeta
`ofl/`).

```sh
./build.sh                       # shaders y binario
./u26 demo=1 decals=mesh blend=0 aids=1 hudStyle=0 sub=2 os=3 frames=120 shot=a.ppm
./u26 win=1                      # ventana: Esc u Options para el menú, H o Triángulo cambia las ayudas
./run_matrix.sh                  # P3 y P4 → results.jsonl
./make_video.sh                  # el video
./fontlab out && ./coverage ...  # P1
python3 -I tools/a11y.py         # P7, sobre capturas de 640×360 sin UI
```

Claves de `u26`: `aids` (0/1, o `aidE`, `aidC`, `aidP` por separado), `hudStyle` (0 en
el cuerpo, 1 junto a la mira, 2 esquinas), `diegetic`, `sub` (0, 1, 2 con fondo), `lang`
(0 español, 1 inglés), `menu` (`imm` o `ret`), `navScript`, `dev` (`kbd`, `xbox`, `ps`),
`uires` (`low`, `native`, `mixed`), `os` (factor del blit), `dim` (oscurecer detrás del
menú) y `win`.
