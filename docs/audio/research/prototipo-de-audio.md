# Prototipo del audio

Nota de investigación del [ADR-0024](../adr/0024-audio.md). Guarda las mediciones que
sostienen la decisión, para poder repetirlas.

Fecha: 2026-11-01, en la máquina de referencia del ADR-0020 (Dell XPS 13 9360,
i7-8550U, 8 GB de RAM). Son mediciones de laboratorio para decidir, no del protocolo del
ADR-0020: el reloj de la CPU quedó libre (gobernador `powersave`), así que sirven para
comparar órdenes de magnitud. El prototipo es código descartable, fuera del repo.

| Qué | Versión |
|---|---|
| Linux / glibc | 7.2.8 / 2.44 |
| Servidor de audio | PipeWire 1.6.9 (48 kHz, float32, *quantum* por defecto 1024, mínimo 32) |
| SDL3 | 3.4.18, driver `pipewire` |
| Clang | 23.1.3, con `-O2` (tiempos) y de `-O0` a `-O3` (determinismo) |
| GCC | 16.2.1, de `-O0` a `-O3` (determinismo) |
| Decodificadores | libvorbis 1.3.7, stb_vorbis 1.22, libopus 1.6.1, IMA-ADPCM propio |
| ffmpeg | n9.0.2, para el material de prueba y el video |

## El prototipo

Cuatro programas en C++20 sobre SDL3:

- **`dev`** abre el dispositivo de verdad y mide P1, P2, P3 y P5. Para no molestar, todo
  sale a −80 dB: el audio se procesa entero, pero no se oye.
- **`bench`** mide el mezclador offline (P2) y saca un hash de un render fijo.
- **`codec`** compara los decodificadores (P4).
- **`render`** sintetiza los sonidos y renderiza las variantes del tajo y del sonido 3D a
  un WAV (P6 y P7), del que sale el video.

**El mezclador propio** (el mismo en `dev`, `bench` y `render`):

- un arreglo fijo de voces; cada voz apunta a un sonido mono en float, con su posición
  fraccional, su paso (la relación de frecuencias por el *pitch*) y su ganancia izquierda
  y derecha;
- **remuestreo lineal** (o ninguno, si la fuente ya está a 48 kHz y el *pitch* es 1);
- **rampa de ganancia** a lo largo de cada bloque, para que un cambio de volumen o de
  paneo no haga clics (*zipper noise*);
- un **retardo en frames** por voz, para que un evento empiece dentro del bloque (el
  *timestamp* de P6);
- **paneo de potencia constante** (`cos`/`sin` del ángulo) y un **limitador** de picos
  al final;
- los comandos (tocar, parar, cambiar la ganancia) llegan desde el hilo principal por
  una **cola de un productor y un consumidor**, sin locks: dos índices atómicos con
  *acquire*/*release* sobre un arreglo fijo de 1024 comandos.

**Cómo se cuentan las asignaciones del hilo de audio:** el programa reemplaza `malloc`,
`calloc`, `realloc`, `memalign` y `free` por versiones que llaman a las de glibc y suman
1 a un contador si el hilo que llama está marcado. El hilo se marca en la primera
llamada al callback. Así se ve todo lo que asigna ese hilo, también lo que hace SDL por
dentro. El costo del hilo entero sale de `/proc/self/task/<tid>/schedstat`, que cuenta
los ns que el hilo estuvo en la CPU.

**Los sonidos sintetizados** (decisión del autor: nada bajado de afuera):

| Sonido | Cómo se hace |
|---|---|
| Silbido de la hoja | Ruido por un pasa banda que barre de 250 Hz a ~3 kHz y vuelve. La carga sube la frecuencia y el volumen |
| Carne | Golpe grave (seno de 55 Hz que baja desde 115 Hz), ruido húmedo filtrado con decaimiento y un "chapoteo": ruido por un pasa banda con amplitud irregular |
| Hueso | De 3 a 6 chasquidos cortos y agudos (2,6 kHz) |
| Metal | Síntesis modal: 8 parciales inarmónicos con decaimientos distintos, más un transitorio de ruido |
| Raspado | Ruido agudo (4,2 kHz) con modulación áspera |
| Forcejeo | Ruido grave con trémolo de 9 Hz y un diente de sierra de ~85 Hz que tiembla |
| Gemido de zombie | Diente de sierra con vibrato por dos formantes (520 y 1150 Hz), más aliento |

**El material de prueba de P4** sale de ffmpeg: una "música" de 180 s en estéreo
(acordes con modulación y percusión de ruido, con un pasa bajos a 9 kHz) y 16 s de
efectos en mono (ruido y tonos con caída). Vorbis con `-q:a 4`; Opus a 96 kbps en estéreo
y 64 kbps en mono, en paquetes de 20 ms sin contenedor.

## Resultados

### P1: el dispositivo

**El periodo real del callback** (4 s por tamaño, salida en silencio):

| Buffer pedido | Periodo medio | p99 | Máximo | Mínimo | Bytes por llamada |
|---|---|---|---|---|---|
| 256 frames (5,33 ms) | 5,333 ms | 5,620 | 5,693 | 4,984 | siempre 2048 |
| 512 frames (10,67 ms) | 10,666 ms | 10,960 | 10,988 | 10,290 | siempre 4096 |
| 1024 frames (21,33 ms) | 21,333 ms | 21,675 | 21,699 | 20,923 | siempre 8192 |

SDL3 usa el driver de PipeWire y **respeta el tamaño pedido** con
`SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES`: el periodo coincide con el buffer y el desvío
es de ±0,35 ms. El callback siempre pide exactamente un buffer. La latencia acústica
de punta a punta no se midió (hace falta un lazo físico entre la salida y un
micrófono); lo que se conoce es el tamaño del buffer.

**Empujar desde el hilo principal** (sin callback: cada frame se rellena la cola de SDL
hasta un objetivo), 10 s con frames de 16,7 ms, un frame doble (33 ms) cada 2 s y un
tirón de 250 ms cada 3 s:

| Objetivo en cola | Faltantes | Silencio total |
|---|---|---|
| 30 ms | 71 | 697 ms |
| 60 ms | 58 | 599 ms |
| 120 ms | 39 | 405 ms |
| 300 ms | 0 | 0 |

Casi todo el silencio sale de los tirones: con 30 ms, los 3 tirones de 250 ms dejan
3 × 220 = 660 ms sin audio. **Los frames dobles no hicieron faltar audio** ni con 30 ms.
Empujar desde el hilo principal ata el audio a ese hilo: solo aguanta un tirón si tiene
en cola más que el tirón, y eso es latencia que se suma a cada sonido.

### P2: el mezclador propio

**En el dispositivo** (buffer de 512, 6 s, fuentes a 44,1 kHz remuestreadas a 48 kHz,
paneo, limitador, y 16 comandos por frame desde el hilo principal):

| Voces | Callback p50 / p99 / máx. | Hilo de audio entero, por cada 16,7 ms | `malloc` en el hilo de audio |
|---|---|---|---|
| 32 | 56 / 113 / 211 µs | 0,148 ms (0,9 % de un núcleo) | 0 |
| 64 | 102 / 171 / 231 µs | 0,221 ms (1,3 %) | 0 |
| 128 | 185 / 332 / 487 µs | 0,359 ms (2,2 %) | 0 |

Los 5936 comandos de cada prueba llegaron todos; la cola nunca se llenó.

**Offline** (bloques de 512, mediana de 5 repeticiones):

| Voces | Lineal | Sin remuestreo |
|---|---|---|
| 32 | 4,39 ns por voz y muestra → 0,112 ms por 16,7 ms | 1,93 ns → 0,049 ms |
| 64 | 3,93 ns → 0,201 ms | 1,83 ns → 0,094 ms |
| 128 | 3,80 ns → 0,389 ms | 1,83 ns → 0,187 ms |

**Sin remuestreo, cuesta la mitad.** Si el cocinador deja todo a 48 kHz, el remuestreo
solo hace falta cuando cambia el *pitch* (las variaciones, la cámara lenta).

**Determinismo:** el hash de un render fijo de 40 voces con retardos, bucles y
remuestreo es **el mismo con Clang y GCC, de `-O0` a `-O3`** (`31baac427b526e6a`).
Cambia con `-march=native` y con `-ffast-math`, igual que en el ADR-0016. Sirve para
tests del mezclador que comparan un render offline contra un hash guardado.

### P3: SDL3 como mezclador

Un `SDL_AudioStream` por voz, todos conectados al dispositivo con
`SDL_BindAudioStreams`. Cada stream recibe estéreo a 44,1 kHz desde su callback (con el
paneo ya aplicado), una ganancia con `SDL_SetAudioStreamGain` y una relación de
frecuencia con `SDL_SetAudioStreamFrequencyRatio`. SDL remuestrea y mezcla.

| Voces | Hilo de audio entero, por cada 16,7 ms | Frente al propio | `malloc` en el hilo de audio (6 s) |
|---|---|---|---|
| 32 | 0,399 ms (2,4 %) | × 2,7 | 0 |
| 64 | 0,730 ms (4,4 %) | × 3,3 | 1 |
| 128 | 1,381 ms (8,3 %) | × 3,8 | 2 |

Cuesta de 3 a 4 veces más, pero no es una comparación pareja: **SDL3 remuestrea con
interpolación de banda limitada** (sinc con ventana de Kaiser, 6 cruces por cero con
SSE, según `SDL_audioresample.c`), que suena mejor que la lineal al bajar mucho el
*pitch*. **SDL asignó memoria en el hilo de audio** de vez en cuando, con 64 y 128
streams. Además, la API de un stream por voz no tiene retardo dentro del bloque ni
buses: para eso hay que mezclar antes, en el callback.

### P4: los decodificadores

**Música, 180 s en estéreo** (PCM16 = 32,96 MiB):

| Formato | Velocidad | Por cada 16,7 ms de audio | Tamaño | `malloc` al decodificar |
|---|---|---|---|---|
| PCM16 → float | ~10 600× tiempo real | 0,002 ms | 32,96 MiB (100 %) | 0 |
| IMA-ADPCM propio | 1120× | 0,015 ms | 8,29 MiB (25 %) | 0. SNR 41,4 dB |
| libvorbis 1.3.7 | 274× | 0,061 ms | 2,21 MiB (6,7 %) | **494** (131 más al abrir) |
| stb_vorbis 1.22 | 341–407× | 0,041–0,049 ms | 2,21 MiB (6,7 %) | 0, con un buffer propio de 190 KiB. **0 también al abrir** |
| libopus 1.6.1 (96 kbps) | 231× | 0,072 ms | 2,09 MiB (6,3 %) | 0. Estado: 160 KiB |
| FLAC (solo el tamaño, como fuente) | — | — | 12,96 MiB (39 %) | — |

**Efectos, 16 s en mono** (PCM16 = 1,46 MiB):

| Formato | Velocidad | Tamaño | Nota |
|---|---|---|---|
| PCM16 | ~13 000× | 1,46 MiB | |
| IMA-ADPCM propio | 2079× | 0,37 MiB | **SNR 22,7 dB:** en ruido, el ADPCM se oye |
| libvorbis | 466× | 0,14 MiB | 392 `malloc` al decodificar |
| stb_vorbis | 690–751× | 0,14 MiB | 0 al decodificar; **2 al abrir**, de origen sin identificar (ver abajo) |
| libopus (64 kbps) | 401× | 0,12 MiB | 0 |
| FLAC | — | 0,74 MiB | |

Lo que se ve:

- **Decodificar no es caro en ningún formato:** el peor (Opus) gasta 0,07 ms por cada
  16,7 ms de música estéreo.
- **libvorbis asigna memoria en cada llamada.** No sirve en el hilo de audio. **stb_vorbis
  con un buffer propio no asigna nada** al decodificar, y libopus tampoco.
- **Abrir con stb_vorbis:** en una segunda corrida se contaron aparte los `malloc` de
  `stb_vorbis_open_memory`, con el buffer propio: **0 al abrir la música** y **2 al
  abrir los efectos**. No se identificó de dónde salen esos 2: pueden ser de stb_vorbis
  o del programa de prueba (el contador mira todo lo que asigna el hilo). El ticket de
  la música lo confirma, porque el hilo principal abre los tracks a mitad de partida y
  en el frame no se pide memoria (ADR-0010).
- **libopus solo decodifica paquetes:** sin `opusfile` (no está instalado) hace falta un
  contenedor propio o leer Ogg a mano. stb_vorbis lee el Ogg completo desde la memoria.
- **El ADPCM es simple (~60 líneas), rápido y comprime 4:1,** pero se nota en los sonidos
  con mucho ruido, que son justamente los del corte (carne, raspado).
- **Lo que entra en 64 MiB** (la hipótesis de `presupuestos.md`): unos **11,6 minutos de
  efectos en mono PCM16** a 48 kHz (96 000 bytes por segundo), o 46 minutos en ADPCM.
  **La música estéreo en PCM16 no entra entera** (33 MiB por track de 3 min); comprimida
  en Vorbis u Opus son ~2,2 MiB.

### P5: la música por streaming

La música en Vorbis, decodificada por el hilo principal cada frame hacia un anillo de
muestras (sin locks), que vacía el callback. 10 s, con un tirón de 250 ms cada 3 s,
buffer del dispositivo de 512:

| Anillo | Faltantes | Silencio | Decodificar en el hilo principal (p50 / p99 / máx.) |
|---|---|---|---|
| 100 ms | 47 | 484 ms | 73 / 339 / 380 µs por frame |
| 250 ms | 4 | 33 ms | 66 / 330 / 569 µs |
| 300 ms | 0 | 0 | 79 / 396 / 1113 µs |
| 500 ms | 0 | 0 | 77 / 353 / 755 µs |

- **El anillo tiene que ser más largo que el peor tirón** más un buffer del dispositivo.
  Con 300 ms no faltó nada; con 250 ms, justo, faltaron 33 ms.
- **Los picos de decodificación vienen después del tirón:** el hilo principal rellena
  de golpe todo lo que se vació (hasta 1,1 ms en un frame).
- **0 `malloc` en el hilo de audio** después del arranque (3 al arrancar, de SDL).
- **El archivo puede estar entero en memoria** (2,2 MiB): "streaming" acá es decodificar
  de a poco, no leer el disco de a poco.
- La otra salida, que no necesita anillo: **decodificar en el propio callback** con
  stb_vorbis y su buffer propio (0 `malloc`, ~0,05 ms por cada 16,7 ms). Es lo que hace
  FMOD con los sonidos que guarda comprimidos (`FMOD_CREATECOMPRESSEDSAMPLE`): los
  decodifica en el mezclador. Un tirón del hilo principal ya no corta la música.

### P6: el tajo

Variantes renderizadas a un WAV (ver "El video"):

| # | Variante | Para oír |
|---|---|---|
| 1 | Carne, tajo corto contra 1 campesino | El sonido base |
| 2 | Carga máxima, 3 campesinos, con variaciones (semilla y *pitch* ±6 % por zombie) | El tajo que atraviesa la fila |
| 3 | El mismo tajo **sin** variaciones | El "efecto ametralladora": tres golpes idénticos |
| 4 | Armadura: rebota (metal + raspado) | Si el rebote se lee sin mirar |
| 5 | Armadura: entra por la junta (metal sordo + carne) | Si se distingue del rebote |
| 6 | Hoja clavada: forcejeo, 8 pulsaciones del QTE y el corte final | El ritmo de la secuencia |
| 7a/7b | Pausa de impacto de 80 ms: el audio sigue / el audio se congela con el personaje | Qué hace el audio en la pausa |
| 8a/8b/8c | Cámara del tajo a ×0,25: el tono baja ×0,25 / tono normal / tono a ×0,6 | Qué hace el audio en la cámara lenta |
| 9a/9b | Un golpe cada 6 pasos, sin *timestamp* (bloques de 1024) / con *timestamp* | El ritmo con frames irregulares |
| 9c/9d | Dos golpes en pasos seguidos, sin / con *timestamp* | Si dos golpes se funden en uno |

**El momento en que suena cada evento (9a a 9d):** se simulan frames de duración
irregular (16,7 o 33,3 ms, más ±2 ms) con el paso fijo de 60 Hz y el acumulador del
ADR-0012:

- **Sin *timestamp***, el evento suena al principio del primer bloque de audio después
  del frame que lo emitió. Con bloques de 1024, **los intervalos entre golpes que
  deberían ser de 100 ms van de 85,3 a 106,7 ms**, y **dos golpes de pasos seguidos
  suenan a 0 ms**: se funden en uno.
- **Con *timestamp*** (el tiempo de su paso más una latencia fija de 60 ms), los
  intervalos son **exactamente 100,0 ms**, y los dos golpes quedan a **16,7 ms**.

Es lo que hace Unity con `AudioSource.PlayScheduled` sobre su reloj de DSP: el sonido
se agenda en un tiempo absoluto del reloj del audio, un poco en el futuro, para que no
dependa del ritmo de los frames.

### P7: el sonido 3D

Un zombie gime mientras da una vuelta a 2,5 m de la caballera en 6 s. La caballera mira
hacia +Z y la cámara está 4 m detrás, mirando hacia el mismo lado.

| # | Oyente | Qué se oye |
|---|---|---|
| 10a | En la cámara | Cuando el zombie pasa entre la cámara y la caballera, a 1,5 m de la cámara, suena mucho más fuerte que cuando está adelante, a 6,5 m: la distancia sale de un punto donde no está el personaje |
| 10b | En la caballera | La distancia es la correcta, pero el paneo gira con la caballera, no con lo que se ve |
| 10c | Híbrido: la distancia desde la caballera, el paneo desde la cámara | La distancia es la de la caballera y el paneo, el de la pantalla |
| 10d | Híbrido + pasa bajos cuando el zombie está atrás | Una pista barata de "atrás": el estéreo solo no distingue adelante de atrás |
| 10e | Híbrido con atenuación lineal (1–20 m) | Comparada con la inversa (la de OpenAL, *inverse distance clamped*) |

El híbrido es el patrón que la comunidad de Wwise y de Unreal describe para la tercera
persona: el paneo sale de la cámara y la atenuación, del personaje.

## El video

`comparativas_adr0024.mp4` (84 s, 1280×720): la forma de onda de cada canal y el nombre
de cada variante, de la 1 a la 10e. Se escucha con auriculares. Quedó junto al prototipo,
fuera del repo.

## Lo que mostró

- **El costo no decide nada:** el mezclador propio con 128 voces y el peor decodificador
  usan menos de 0,5 ms por cada 16,7 ms, en otro hilo. Lo que decide es **dónde corre el
  trabajo** y **quién puede cortar el audio**.
- **Un tirón del hilo principal no debe cortar el audio.** Si el audio depende del hilo
  principal (empujar o llenar el anillo), aguanta solo lo que tenga en cola, y esa cola
  es latencia. Si el callback mezcla y decodifica por su cuenta, un tirón solo atrasa los
  sonidos nuevos.
- **El *timestamp* por evento es lo que hace sonar bien el paso fijo:** sin él, el audio
  hereda el ritmo irregular de los frames y funde los golpes de pasos seguidos. Cuesta
  un campo en el comando y un retardo en la voz.
- **SDL3 mezclando cuesta 3–4 veces más y asigna memoria en el hilo de audio** de vez en
  cuando. Su remuestreo es mejor que el lineal.
- **Las variaciones son obligatorias en el corte:** el efecto ametralladora se oye en
  el primer tajo que cruza tres zombies.
- **El estéreo no distingue adelante de atrás.** El pasa bajos es una pista barata.

## Lo que toca a otros ADRs

- **ADR-0004:** el alcance de SDL3 en el audio que dejó abierto.
- **ADR-0010:** la región de audio y las reglas de un hilo que no comparte allocators.
- **ADR-0012:** el hilo de audio sería la segunda excepción al hilo único, después de
  los de Tracy. Los comandos van por una cola, como las órdenes entre sistemas.
- **ADR-0017:** el cocinador recibe los sonidos fuente y los deja en el formato que se
  decida (y a 48 kHz, si se elige no remuestrear).
- **ADR-0018:** el combate tiene que emitir eventos con lo que el audio necesita: el
  material, la fuerza, el área cortada, el rebote y el paso en que ocurrieron.
- **ADR-0019:** los sonidos de terceros solo pueden ser CC0 o CC BY.
- **ADR-0020:** "Audio: 0,5 ms por frame" mezcla dos cosas: lo que gasta el hilo
  principal (comandos, posiciones, quizá decodificar) y lo que gasta el hilo de audio
  en otro núcleo.
- **ADR-0021:** el registro por frame y el panel de ImGui pueden mostrar voces, robos y
  faltantes.
- **ADR-0023:** el ruido que oye la IA es un dato del mundo; el sonido que oye el
  jugador es otro sistema. Pueden salir del mismo evento del combate.

## Cómo repetirlo

```sh
# en una carpeta de trabajo, con src/{common.h,hook.cpp,dev.cpp,bench.cpp,codec.cpp,render.cpp}
# y ext/stb/stb_vorbis.c (v1.22, de github.com/nothings/stb)
clang++ -std=c++20 -O2 src/dev.cpp src/hook.cpp -o dev $(pkg-config --cflags --libs sdl3 vorbisfile)
clang++ -std=c++20 -O2 src/bench.cpp -o bench
clang++ -std=c++20 -O2 src/codec.cpp src/hook.cpp -o codec $(pkg-config --cflags --libs vorbisfile opus)
clang++ -std=c++20 -O2 src/render.cpp -o render

./dev period 512 4            # P1: periodo del callback (256, 512, 1024)
./dev push 30 10              # P1: empujar con tirones (30, 60, 120, 300 ms)
./dev mixer 64 512 6          # P2: mezclador propio en el dispositivo
./bench                       # P2: offline y hash
./dev sdlmix 64 512 6         # P3: SDL3 mezcla
./codec out                   # P4: necesita out/musica.* y out/sfx.* (ffmpeg, ver arriba)
./dev stream 300 512 10 out/musica.ogg   # P5
./render                      # P6 y P7: out/variantes.wav y out/variantes.txt
```

El hash del determinismo se repite compilando `bench.cpp` con cada compilador y
nivel de optimización.
