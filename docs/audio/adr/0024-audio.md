# ADR-0024: Audio

- **Estado:** Aceptado
- **Fecha:** 2026-11-01
- **Ticket:** #38
- **Reemplaza / Reemplazado por:** —

## Contexto y problema

Hay que decidir **cómo suena el juego**: quién saca el sonido al dispositivo, quién
mezcla las voces, en qué hilo, cómo suena un zombie a la izquierda o atrás, en qué
formato se guardan los sonidos y la música, y cómo se entera el audio de lo que pasa en
la simulación. La visión pone la vara alta: **el sonido del corte es "la mitad de la
sensación"** y no es secundario. Contra la armadura, el sonido es lo que le dice al
jugador si el ángulo sirvió.

Varios ADRs ya aceptados dejaron preguntas para este:

| ADR | Qué dejó abierto |
|---|---|
| ADR-0004 | **El alcance de SDL3 en el audio:** solo salida al dispositivo, o también mezcla |
| ADR-0012 | El engine tiene un hilo. El callback de audio de SDL3 corre en otro: hay que decidir si se usa y con qué reglas |
| ADR-0017 | El formato y la cocina de los sonidos |
| ADR-0018 | Cómo suena el tajo según el material, el rebote contra la placa, la hoja clavada y su secuencia, y la pausa de impacto |
| ADR-0020 | Que el audio quepa en **0,5 ms por frame** y **64 MiB**, todavía hipótesis |
| Visión | "Sonido del corte en cada corte", la cámara del tajo en cámara lenta y 50 zombies que gimen a la vez |

Hay que decidir:

1. **qué hace SDL3:** solo salida, o también mezcla;
2. **el hilo y la comunicación:** dónde se mezcla y cómo le llegan las órdenes;
3. **el mezclador:** voces, qué pasa cuando no alcanzan, buses, remuestreo y límite;
4. **el sonido 3D:** paneo, atenuación, pistas de "atrás" y dónde está el oyente;
5. **los formatos y la decodificación:** en el repo, cocinados y en el juego;
6. **la música:** cómo se guarda, quién la decodifica y cuánto se adapta al juego;
7. **el diseño del sonido del corte:** capas, variaciones, la pausa de impacto y la
   cámara lenta;
8. **la relación con la simulación:** cómo emite el paso sus sonidos sin romper el
   determinismo y en qué momento suena cada uno;
9. **los datos y la autoría:** cómo se describe un sonido y de dónde salen;
10. **la memoria, el presupuesto, las pruebas y la depuración.**

Lo que ya está decidido y limita este ADR:

- **ADR-0004:** SDL3 es la capa de plataforma, también para el audio, escondida detrás
  de una interfaz propia: **ningún tipo de SDL sale del módulo de plataforma**.
- **ADR-0006:** las dependencias entran como paquete del sistema o con `FetchContent` y
  una versión fija, o copiadas en el repo, como cgltf y stb_image en el ADR-0017.
- **ADR-0008:** sin `-ffast-math` ni `-march`.
- **ADR-0009:** sin excepciones ni RTTI. En el código de cada frame no hay contenedores
  que asignen memoria ni `virtual` por entidad.
- **ADR-0010:** regiones de memoria por sistema, reservadas y tocadas al iniciar, con
  topes duros. **Ningún allocator se comparte entre hilos.**
- **ADR-0011:** TDD estricto en el núcleo; tests después en el gameplay y el renderer.
- **ADR-0012:** un hilo (con la excepción de los hilos de Tracy), paso fijo de 60 Hz con
  acumulador, hasta 8 pasos por frame, e interpolación en el render. Las órdenes entre
  sistemas van por colas.
- **ADR-0016 y ADR-0021:** la misma partida grabada da el mismo hash del estado. Nada
  que no sea la simulación puede escribir en ella.
- **ADR-0017:** los assets entran crudos al repo y el cocinador los convierte en el
  build. Git simple, nunca LFS.
- **ADR-0018:** el material es **por cápsula** (carne, placa, cota), con juntas. El tajo
  rebota si no supera el umbral de la placa, y deja a la caballera aturdida. **La pausa
  de impacto es una por tajo** (50 a 80 ms) y **cada personaje tiene su escala de
  tiempo**. En la hoja clavada, "la sensación de forcejeo sale de efectos que no mueven
  huesos (la cámara, el sonido, la espada que tiembla)".
- **ADR-0019:** los assets de terceros solo pueden ser CC0 o CC BY.
- **ADR-0020:** la máquina de referencia es el XPS 13 9360 (i7-8550U, 4 núcleos y 8
  hilos), con 1 GiB de memoria total.
- **ADR-0022:** la cámara del tajo, en cámara lenta, solo en los cortes especiales.
- **ADR-0023:** **el ruido que oye la IA es un dato del mundo** (eventos con posición y
  radio en un arreglo fijo). No es el sonido que oye el jugador, aunque los dos pueden
  salir del mismo evento del combate.

**Fuera de este ADR:** qué sonidos carga cada nivel y cuánto viven en memoria (lo decide
el ADR de recursos en runtime), dónde se colocan los sonidos de ambiente del escenario
(el de niveles) y la pantalla de opciones con los volúmenes (el de UI y menús).

## Criterios

Los cinco del ticket, más cuatro que salen de cruzar lo ya decidido:

1. **Latencia baja:** el golpe se oye cuando se ve.
2. **El hilo de audio no asigna memoria, no toma locks y no hace I/O.** Es la regla de
   oro del audio en tiempo real (Bencina, 2011).
3. **Cada dependencia, justificada** (ADR-0004 y `CLAUDE.md`).
4. **Aprendizaje.**
5. **Gratis.**
6. **No romper el determinismo:** el audio lee la simulación y nunca escribe en ella
   (ADR-0016 y ADR-0021).
7. **El corte se lee por el oído:** carne, metal, rebote y hoja clavada se distinguen
   sin mirar. Es lo que pide la visión.
8. **Cabe en el presupuesto** del XPS 13: 0,5 ms por frame y 64 MiB (ADR-0020).
9. **Lo produce una persona:** los sonidos salen gratis, con una licencia que acepta el
   ADR-0019, y se itera sobre ellos sin un editor propio.

## Opciones

Los tiempos son de un prototipo medido en la máquina de referencia (ver Investigación).
"Por cada 16,7 ms" es lo que cuesta producir el audio de un frame a 60 fps.

### 1. Qué hace SDL3

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Solo salida, con callback** | `SDL_OpenAudioDeviceStream` abre el dispositivo y llama a una función propia cada vez que necesita audio. La mezcla, el 3D y la decodificación son propios | Hay que escribir el mezclador. **Medido:** 0 `malloc` en el hilo de audio; 0,15 / 0,22 / 0,36 ms por cada 16,7 ms con 32 / 64 / 128 voces |
| **SDL3 mezcla** | Un `SDL_AudioStream` por voz, conectados al dispositivo con `SDL_BindAudioStreams`. SDL remuestrea (sinc con ventana de Kaiser) y suma | **Medido:** de 3 a 4 veces más caro (0,40 / 0,73 / 1,38 ms) y **asignó memoria en el hilo de audio** con 64 y 128 streams. No tiene retardo dentro del bloque ni buses |
| **SDL3_mixer** | La biblioteca oficial aparte: canales, música y decodificadores | Otra dependencia, con su ADR. Hace justo el trabajo que este ADR quiere decidir |
| **miniaudio** | Un solo archivo en C, dominio público o MIT-0, con sus propios backends (PipeWire, PulseAudio, ALSA) y un motor con grafo de nodos | Reemplaza a SDL3 en el audio: serían dos capas de plataforma. Su motor también hace el trabajo de este ADR |
| **PipeWire o ALSA directo** | Sin capa: el engine habla con el servidor de audio | Solo Linux, contra la portabilidad del ADR-0004. Más código de plataforma que no es el juego |

### 2. El hilo y la comunicación

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Mezclar en el callback** | El callback (en el hilo de audio de SDL) mezcla. El hilo principal le manda comandos (tocar, parar, mover) por una **cola de un productor y un consumidor sin locks** | Es la **segunda excepción al hilo único** del ADR-0012, después de Tracy. Dentro del callback rigen las reglas de tiempo real: nada de `malloc`, locks ni I/O. Depurar dos hilos |
| **Mezclar en el hilo principal y empujar** | Cada frame, el hilo principal mezcla lo que falta y lo pone en la cola de SDL (`SDL_PutAudioStreamData`). Todo en un hilo | **Medido:** un tirón de 250 ms deja 220 ms de silencio si la cola es de 30 ms. Solo con 300 ms en cola no se corta, y esa cola es latencia en cada sonido |
| **Mezclar en el hilo principal hacia un anillo** | Lo mismo, pero el callback copia de un anillo propio | El mismo problema: el audio vive del ritmo del hilo principal |
| **Un hilo de mezcla propio** | Un hilo del engine mezcla hacia un anillo y el callback solo copia. Es lo que hacen los motores con hilo mezclador propio | Suma un anillo de latencia y otro hilo, sin ganar nada frente a mezclar en el callback, que ya es un hilo aparte |

**Medido:** con el mezclador en el callback, 16 comandos por frame durante 6 s
(5936 comandos), **ninguno se perdió** y la cola de 1024 nunca se llenó.

### 3. El mezclador

**Cuántas voces y qué pasa cuando no alcanzan:**

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Pool fijo, sin robo** | N voces; si no hay libre, el sonido nuevo no suena | Con 50 zombies, un golpe importante puede no sonar porque los gemidos ocupan todo |
| **Pool fijo, robo por prioridad** | Cada sonido tiene una prioridad. Si no hay voz libre, se roba la de menor prioridad y, entre iguales, la más baja o la más vieja | Hay que asignar prioridades. FMOD recomienda pocas (Studio expone solo 5) |
| **Voces virtuales** (FMOD, Wwise) | Más voces lógicas que reales: las que no se oyen siguen contando su tiempo sin mezclarse, y vuelven si se vuelven audibles | Más estado y más reglas. Sirve para muchos sonidos en bucle (50 gemidos) |
| **Límite por tipo de sonido** | Como máximo K instancias del mismo sonido (por ejemplo, 6 gemidos) | Complementa a las otras; sola no protege el total |

**El remuestreo:**

| Opción | Problema |
|---|---|
| **Ninguno:** el cocinador deja todo a 48 kHz | No se puede cambiar el *pitch* (variaciones, cámara lenta). **Medido:** 1,8 ns por voz y muestra |
| **Lineal** | Pierde agudos y deja *aliasing* cuando el *pitch* cambia mucho. **Medido:** 3,8–4,4 ns por voz y muestra, el doble |
| **Lineal solo si el *pitch* no es 1**, sin remuestreo si es 1 | Dos caminos que probar |
| **Cúbico o sinc** (el de SDL3) | Mejor calidad, más costo y más código |

**Los buses:** un grupo de voces con su volumen. Lo mínimo es música, efectos y UI; un
bus del "mundo" que la pausa del juego congela, y la UI, no. **El límite:** un limitador
de picos al final evita que 50 gemidos y un corte saturen la salida.

### 4. El sonido 3D

**La espacialización:**

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Estéreo: paneo + atenuación** | Paneo de potencia constante (`cos`/`sin` del ángulo) y una curva de atenuación por distancia | **El estéreo no distingue adelante de atrás** |
| **Lo anterior + pistas baratas** | Un pasa bajos cuando la fuente está atrás; oclusión con un rayo de la física (ADR-0016) que baja los agudos detrás de un muro | Un filtro por voz; el rayo es un costo por voz y por frame |
| **HRTF** | Filtros medidos de la cabeza y las orejas: con auriculares, la fuente se oye arriba, atrás o adelante | Con parlantes no sirve. Una biblioteca (Steam Audio) es una dependencia grande, y una propia necesita datos medidos y convolución |
| **Doppler** | El tono sube cuando la fuente se acerca | Los zombies caminan lento: no se nota. Necesita remuestreo |

**La curva de atenuación:** la inversa con límite de OpenAL (*inverse distance
clamped*: volumen 1 hasta una distancia de referencia y cae como 1/d) o la lineal hasta
una distancia máxima, en la que el sonido llega a 0 y no hay que mezclarlo. En la
variante 10e se puede escuchar la diferencia.

**Dónde está el oyente:**

| Opción | Problema |
|---|---|
| **En la cámara** | La distancia sale de un punto donde no está el personaje. **Escuchado (10a):** un zombie entre la cámara y la caballera suena mucho más fuerte que uno igual de cerca, pero adelante |
| **En la caballera** | El paneo gira con la caballera y no con lo que se ve en pantalla |
| **Híbrido: la distancia desde la caballera y el paneo desde la cámara** | Dos puntos que mantener. Un sonido justo detrás de la caballera, pero adelante de la cámara, puede panear raro. **Es el patrón que describen las comunidades de Wwise y Unreal para la tercera persona** |

### 5. Los formatos y la decodificación

**En el repo** (el ADR-0017 guarda los fuentes en git simple):

| Opción | Problema |
|---|---|
| **WAV** | Lo más simple. El doble de grande que FLAC |
| **FLAC** | Sin pérdida y la mitad del tamaño (**medido:** 39–50 % del PCM16). El cocinador necesita un decodificador de FLAC |

**Cocinado y en el juego:**

| Opción | Velocidad (**medido**, música estéreo) | Tamaño | Problema |
|---|---|---|---|
| **PCM16** | ~10 600× tiempo real | 100 % | 64 MiB = 11,6 min de efectos en mono. La música no entra entera |
| **IMA-ADPCM propio** (~60 líneas) | 1120× | 25 % | **Se oye en los sonidos con ruido:** SNR de 22,7 dB en los efectos, que son justo los del corte |
| **Vorbis con stb_vorbis** (dominio público, un archivo) | 341–407× | 6,7 % | Con un buffer propio de 190 KiB, **0 `malloc` al decodificar**. Formato con pérdida |
| **Vorbis con libvorbis** | 274× | 6,7 % | **494 `malloc` al decodificar:** no sirve en el hilo de audio |
| **Opus con libopus** | 231× | 6,3 % | 0 `malloc` al decodificar. Sin `opusfile` hace falta un contenedor propio. Es el códec más moderno (RFC 6716, 2012) |

**Dónde vive el decodificador:** solo en el cocinador (en el juego todo es PCM o
ADPCM), o también en el juego (para la música, o para todo lo comprimido).

### 6. La música

**Cómo se guarda y quién la decodifica:**

| Opción | Problema |
|---|---|
| **Decodificada entera en memoria** | 33 MiB por cada 3 min en estéreo PCM16: dos tracks se comen el presupuesto |
| **Comprimida en memoria; el hilo principal decodifica cada frame hacia un anillo** | **Medido:** 66–79 µs por frame en p50 y hasta 1,1 ms después de un tirón. El anillo tiene que ser más largo que el peor tirón: con 250 ms faltaron 33 ms y con 300 ms, nada. Esos 300 ms son latencia solo para la música |
| **Comprimida en memoria; el callback la decodifica** | Con stb_vorbis y su buffer propio: 0 `malloc` y ~0,05 ms por cada 16,7 ms. Un tirón del hilo principal no la corta. Es lo que hace FMOD con los sonidos que guarda comprimidos. Suma trabajo dentro del callback |
| **Streaming desde el disco, con un hilo de I/O** | Lo que hacen los juegos con horas de música. Aquí, con 2,2 MiB por track, es un tercer hilo sin necesidad |

**Cuánto se adapta al juego:**

| Opción | Problema |
|---|---|
| **Un track por nivel, en bucle** | Lo más simple. No acompaña el ritmo del combate |
| **Cambio por estado con *crossfade*** (exploración, combate, jefe) | Hay que decidir qué es "combate" y evitar que cambie a cada rato |
| **Capas verticales** (el mismo tema con instrumentos que entran y salen según la tensión) | Exige componer en capas sincronizadas, algo difícil para una persona |

### 7. El diseño del sonido del corte

**De dónde sale el sonido:**

| Opción | Problema |
|---|---|
| **Un sonido por evento** ("corte en carne") | Se repite igual: **escuchado (variante 3)**, tres zombies en una fila suenan como una ametralladora |
| **Capas cocinadas, elegidas y mezcladas con datos de la simulación** | Silbido de la carga, entrada, material (carne, metal, hueso), salida. La fuerza decide el volumen y qué capa suena; el material decide la capa; el área cortada, la duración. Más sonidos que producir |
| **Síntesis procedural en el juego** | El metal modal y el ruido filtrado del prototipo, generados en el callback. Sin assets, pero suena sintético y cuesta CPU en el hilo de audio |
| **Capas cocinadas + variaciones** | Lo anterior, con varias tomas por capa (*round robin* sin repetir la última) y ±5 % de *pitch* y volumen al azar |

**El azar de las variaciones** no puede salir del generador de la simulación, porque
el replay dejaría de dar el mismo hash si el audio consume números. Puede ser un
generador propio del audio, o un hash del paso y de la entidad (que además hace que el
replay suene igual).

**La pausa de impacto** (variantes 7a y 7b):

| Opción | Problema |
|---|---|
| **El audio no se congela:** el golpe suena al empezar la pausa y todo sigue | El sonido "corre" mientras la imagen está quieta |
| **Los sonidos del personaje se congelan con él** | El silbido se corta y vuelve: se oye como un error |

**La cámara del tajo en cámara lenta** (variantes 8a, 8b y 8c):

| Opción | Problema |
|---|---|
| **El tono baja con el tiempo** (×0,25) | Muy grave y largo: se pierde el carácter del golpe |
| **Tono normal; solo se separan los eventos** | El golpe suena "rápido" en un mundo lento |
| **Tono a medias** (×0,6) y algo de pasa bajos | Un valor más que ajustar |

### 8. La relación con la simulación

**Cómo le llegan los sonidos al audio:**

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Llamadas directas** | El gameplay llama a `play(sonido, posición)` donde pasa algo | El audio queda mezclado con la lógica del paso. Sin el paso, no hay *timestamp* |
| **Eventos del paso** | Cada paso deja eventos en una cola (qué pasó, dónde, el paso y los datos: material, fuerza). Una vez por frame, el sistema de audio los traduce a comandos | Una cola más que dimensionar. Es como el ADR-0012 ya conecta los sistemas |
| **Eventos para lo puntual y emisores para lo continuo** | Lo puntual (un golpe) va por eventos. Lo que dura (un gemido, el forcejeo, el fuego) es un componente "emisor" que el audio lee una vez por frame, con la posición interpolada | Dos caminos. Lo continuo necesita saber cuándo empieza y termina |

**En qué momento suena cada evento** (variantes 9a a 9d):

| Opción | Problema |
|---|---|
| **Al principio del próximo bloque de audio** | **Medido:** golpes que deberían ir cada 100 ms quedan entre 85,3 y 106,7 ms, y **dos golpes de pasos seguidos se funden** (0 ms entre ellos) |
| **Con *timestamp*:** el tiempo de su paso más una latencia fija | **Medido:** exactamente 100,0 ms y 16,7 ms. Cuesta un campo en el comando y un retardo por voz. Suma una latencia fija (en el prototipo, 60 ms) que hay que ajustar |

**La pausa del juego y el replay:** con la pausa, el bus del mundo se congela y la UI
sigue. En el replay, el audio recibe los mismos eventos, así que suena igual.

### 9. Los datos y la autoría

**Cómo se describe un sonido:**

| Opción | Problema |
|---|---|
| **Una tabla en C++** (sonido, variantes, rango de *pitch*, prioridad, bus, límite) | Cambiar un volumen exige recompilar |
| **Un archivo de datos que cocina el build**, al estilo de los eventos de FMOD, con recarga en caliente | Un formato propio más, con su cocinador y sus errores |
| **Una herramienta de *middleware*** (FMOD Studio, Wwise) | Propietarias: no cumplen "gratis" sin condiciones ni "desde cero" |

**De dónde salen los sonidos:**

| Opción | Problema |
|---|---|
| **Sintetizados por una herramienta propia, offline** | Los del prototipo: se reconocen, pero suenan sintéticos. Aprendizaje de síntesis |
| **Grabados por el autor** (foley: verduras, cuero, metal) | Necesita un micrófono, un lugar silencioso y tiempo |
| **Terceros CC0** (Kenney, la parte CC0 de Freesound) | Calidad variable. Hay que revisar la licencia de cada uno: Freesound mezcla CC0, CC BY y CC BY-NC, y la última no se acepta (ADR-0019) |
| **Terceros CC BY** | Cada uno con su fila en `CREDITS.md` |

### 10. La memoria, el presupuesto, las pruebas y la depuración

- **Memoria:** la región de audio guarda los sonidos cocinados del nivel, el pool de
  voces, la cola de comandos y los buffers de decodificación. **Medido:** en 64 MiB
  entran 11,6 min de efectos en mono PCM16, o 46 min en ADPCM. Los decodificadores de
  stb_vorbis y Opus ocupan 160–190 KiB cada uno.
- **El presupuesto de CPU:** la fila "Audio: 0,5 ms por frame" de `presupuestos.md`
  junta lo que gasta el hilo principal (traducir eventos, posiciones, quizá decodificar)
  con lo que gasta el hilo de audio en otro núcleo. Se puede separar en dos filas.
- **La latencia:** el buffer del dispositivo es de 256, 512 o 1024 frames: 5,3, 10,7 o
  21,3 ms. **Medido:** SDL3 respeta el tamaño pedido con PipeWire, con ±0,35 ms de
  desvío.

**Las pruebas:**

| Opción | Problema |
|---|---|
| **Solo escuchar** | Nada atrapa una regresión |
| **Tests unitarios de las piezas** (cola, paneo, decodificador, robo de voces) | No dicen si la mezcla entera cambió |
| **Lo anterior + render offline contra un hash** | **Medido:** el mismo hash con Clang y GCC de `-O0` a `-O3`, en la misma máquina. Un cambio a propósito obliga a volver a grabar el hash |

**La depuración:** el panel de ImGui y el registro por frame del ADR-0021 pueden mostrar
las voces activas, las robadas, los faltantes del dispositivo y el uso de la cola.

## Investigación

### Prototipo medido

Cuatro programas en C++20 sobre SDL3, medidos en la máquina de referencia (ADR-0020) el
2026-11-01, fuera del protocolo. Todo sale a −80 dB: el audio se procesa entero, pero no
se oye. El detalle y cómo repetirlo están en
[`../research/prototipo-de-audio.md`](../research/prototipo-de-audio.md).

| # | Qué | Resultado |
|---|---|---|
| P1 | Dispositivo | SDL3 respeta el buffer pedido con PipeWire: 5,33 / 10,67 / 21,33 ms, ±0,35 ms. **Empujar desde el hilo principal** con un tirón de 250 ms: silencio, salvo con 300 ms en cola |
| P2 | Mezclador propio en el callback | 0,15 / 0,22 / 0,36 ms por cada 16,7 ms con 32 / 64 / 128 voces; **0 `malloc`**; ningún comando perdido. Sin remuestreo, la mitad. **El mismo hash** con Clang y GCC de `-O0` a `-O3` |
| P3 | SDL3 mezcla | De 3 a 4 veces más caro y **asignó memoria en el hilo de audio**. Remuestrea mejor (sinc) |
| P4 | Decodificadores | Todos baratos (≤ 0,07 ms por cada 16,7 ms). **libvorbis asigna en cada llamada;** stb_vorbis y libopus, no. ADPCM: SNR de 22,7 dB en ruido |
| P5 | Música por streaming | El anillo tiene que superar el peor tirón (300 ms). Decodificar en el callback lo evita |
| P6 | El tajo | Variaciones, capas por material, hoja clavada, pausa de impacto, cámara lenta y *timestamps* (85,3–106,7 ms sin ellos, 100,0 ms con ellos) |
| P7 | Sonido 3D | Oyente en la cámara, en la caballera e híbrido; pasa bajos para "atrás"; atenuación inversa y lineal |

Un video de 84 s (`comparativas_adr0024.mp4`, fuera del repo) deja escuchar las 20
variantes de P6 y P7, con su nombre en pantalla. **Los sonidos son sintetizados**
(decisión del autor), así que sirven para comparar, no para juzgar cómo sonará el juego.

Lo que mostró, además de los números:

- **El costo no decide nada.** Lo que decide es **dónde corre el trabajo** y **quién
  puede cortar el audio**: si el audio depende del ritmo del hilo principal, cada tirón
  se oye o se paga con latencia.
- **El *timestamp* es lo que hace sonar bien el paso fijo:** sin él, el audio hereda el
  ritmo irregular de los frames.
- **Las variaciones son obligatorias en el corte:** la ametralladora se oye en el
  primer tajo que cruza tres zombies.
- **El estéreo no distingue adelante de atrás.**

**Lo que toca a otros ADRs:**

- **ADR-0004:** responde el alcance de SDL3 que dejó abierto.
- **ADR-0010:** la región de audio; el hilo de audio no comparte allocators.
- **ADR-0012:** si se mezcla en el callback, es la segunda excepción al hilo único.
- **ADR-0017:** el cocinador suma los sonidos: formato fuente, formato cocinado y, si
  se elige, la conversión a 48 kHz.
- **ADR-0018:** el combate emite eventos con lo que el audio necesita (material,
  fuerza, área, rebote, paso).
- **ADR-0020:** la fila "Audio" se puede partir en hilo principal e hilo de audio.
- **ADR-0021:** contadores de voces, robos y faltantes en el registro por frame.
- **ADR-0023:** el ruido de la IA y el sonido pueden salir del mismo evento del combate.

### Cómo lo hacen otros

- **La regla del tiempo real** (Bencina, *Real-time audio programming 101: time waits
  for nothing*, 2011): en el hilo de audio, nada de `malloc`, locks, I/O ni llamadas que
  bloqueen; comunicarse con colas FIFO sin locks y reservar todo antes. Vale para
  JACK, ALSA, CoreAudio y WASAPI por igual.
- **FMOD** (manual del Core API 2.03): voces **virtuales y reales** según cuánto se oyen
  (distancia, volumen, prioridad, oclusión). Una voz de prioridad alta nunca la roba una
  de prioridad baja. FMOD Studio expone solo 5 prioridades, a propósito. Los sonidos
  guardados comprimidos (`FMOD_CREATECOMPRESSEDSAMPLE`) se decodifican en el mezclador.
- **SDL3:** `SDL_BindAudioStreams` mezcla todos los streams conectados a un dispositivo;
  su remuestreo es de banda limitada (sinc con ventana de Kaiser).
- **miniaudio** (Reid): un solo archivo, dominio público o MIT-0, con un motor de sonido
  3D sobre un grafo de nodos.
- **OpenAL 1.1:** los modelos de distancia (`AL_INVERSE_DISTANCE_CLAMPED` y los
  lineales) que usan casi todos los motores.
- **Unity**, `AudioSource.PlayScheduled`: agenda un sonido en un tiempo absoluto del
  reloj del audio (`AudioSettings.dspTime`), un poco en el futuro, para que no dependa
  del ritmo de los frames.
- **El oyente en tercera persona:** en las comunidades de Wwise y Unreal se describe
  el paneo desde la cámara y la atenuación desde el personaje. PlatinumGames (blog de
  Audiokinetic) pone el oyente por defecto junto al jugador, con presets por zona que se
  interpolan para que los sonidos no salten.
- **Somberg (ed.),** *Game Audio Programming: Principles and Practices* (CRC Press, 5
  volúmenes, 2016–2025): la referencia de la programación de audio para juegos. El
  volumen 3 trata el manejo y la virtualización de voces; el 5, la arquitectura
  asíncrona con varios hilos.

## Decisión

Elijo usar SDL3 solo para sacar el sonido al dispositivo y escribir el mezclador. El
prototipo mostró que el costo no decide: mezclar 64 voces cuesta 0,22 ms por cada
16,7 ms en otro núcleo. Lo que decide es quién puede cortar el audio. Por eso mezclo en
el callback, que corre en su propio hilo y no depende del ritmo del hilo principal: un
tirón del juego no se oye. SDL3 mezclando costaba de 3 a 4 veces más y asignaba memoria
en el hilo de audio, y además me quitaba justo la parte que quiero entender.

El hilo de audio es tonto a propósito: ejecuta comandos y mezcla. Todas las decisiones
(qué voz usar, a quién robar, cuánto suena cada cosa) se toman en el hilo principal,
donde se pueden probar y depurar. Los sonidos llevan el tiempo de su paso, así el audio
no hereda el ritmo irregular de los frames y dos golpes seguidos no se funden en uno.

Para el corte, que es la mitad de la sensación, elijo capas por material y por fuerza,
con variaciones, porque un tajo que cruza tres zombies con el mismo sonido suena a
ametralladora. La música queda comprimida en memoria y la decodifica el callback, así
que tampoco se corta con un tirón.

| # | Dimensión | Elección |
|---|---|---|
| 1 | Qué hace SDL3 | **Solo la salida, con callback** (`SDL_OpenAudioDeviceStream`), detrás de la interfaz de plataforma (ADR-0004). **El mezclador, el sonido 3D y la decodificación son propios** |
| 2 | Hilo y comunicación | **Se mezcla en el callback,** en el hilo de audio de SDL3: la **segunda excepción al hilo único** del ADR-0012. Le llegan comandos por **una cola de un productor y un consumidor sin locks**, y devuelve su reloj y sus contadores en atómicos. En el callback no hay `malloc`, locks, I/O ni registro |
| 3 | Mezclador | **64 voces fijas.** **Cuatro prioridades;** un sonido nuevo solo roba una voz de prioridad menor (la más baja, después la que menos suena y después la más vieja), y si no hay, no suena. **Límite de instancias por sonido** (por ejemplo, 8 gemidos). **Sin voces virtuales.** **Remuestreo lineal solo cuando el *pitch* no es 1.** **Buses de música, mundo y UI,** y **un limitador** al final |
| 4 | Sonido 3D | **Estéreo:** paneo de potencia constante y **atenuación inversa con límite** (la de OpenAL) con una distancia máxima, más allá de la cual la voz no se mezcla. **El oyente es híbrido:** la distancia se mide desde la caballera y el paneo sale de la cámara. **Un pasa bajos cuando la fuente está detrás de la caballera.** Sin HRTF, sin Doppler y sin oclusión en el corte vertical |
| 5 | Formatos | **En el repo, FLAC.** **Cocinados:** los efectos en **PCM16 mono a 48 kHz**; la música en **Vorbis estéreo a 48 kHz**. En el juego, **stb_vorbis** con un buffer propio, para la música. El cocinador lee FLAC con **dr_flac** y codifica Vorbis con **libvorbis**; el juego no los enlaza |
| 6 | Música | **Comprimida en memoria y decodificada en el callback.** **Cambia por estado** (exploración, combate, jefe) **con *crossfade***, con un tiempo mínimo en cada estado para que no cambie a cada rato |
| 7 | Sonido del corte | **Capas cocinadas según el material y la fuerza,** con **varias tomas por capa** (*round robin* sin repetir la última) y **±5 % de *pitch* y de volumen.** El azar sale de **un hash del paso, la entidad y la capa**, no del generador de la simulación. **La pausa de impacto no congela el audio:** el golpe suena al empezar la pausa. **En la cámara del tajo, el bus del mundo baja el tono** (valor de partida ×0,6, configurable) con algo de pasa bajos |
| 8 | Relación con la simulación | **Lo puntual va por eventos del paso; lo continuo, por emisores** (un componente que el audio lee una vez por frame, con la posición interpolada). **Cada evento lleva el tiempo de su paso** y suena en ese tiempo **más una latencia fija** |
| 9 | Datos y autoría | **Una tabla de sonidos en un archivo de texto que cocina el build,** que **se puede recargar en los builds de desarrollo** (cómo, lo decide el ADR-0028). **Los sonidos del corte los grabo yo o salen de CC0;** el resto, de CC0, de CC BY o de síntesis propia |
| 10 | Memoria, presupuesto, pruebas y depuración | **Una región de audio** reservada y tocada al iniciar (ADR-0010). **La fila "Audio" de `presupuestos.md` se parte** en hilo principal e hilo de audio. **Buffer del dispositivo de 512** (10,7 ms), configurable. **Tests de las piezas y un render offline contra un hash.** **Un panel de ImGui** con las voces, y contadores de robos y faltantes en el registro por frame |

## Consecuencias

- **El frame** (ADR-0012), con el audio:

  ```
  input → pasos de simulación (cada uno deja sus eventos de sonido)
        → render (interpola el estado y la cámara)
        → audio: eventos → voces → comandos con tiempo → cola
  callback (en su hilo): comandos → voces → música → limitador → dispositivo
  ```

  El sistema de audio corre **una vez por frame, después del render**, con la cámara y
  las posiciones que el render ya interpoló. Es código que corre cada frame, con las
  reglas del ADR-0009 (fe de erratas del ADR-0012). Nunca escribe en el estado de la
  simulación, así que no entra en el hash del replay.
- **El hilo de audio solo ejecuta.** El hilo principal lleva **una copia de cada voz**
  (qué sonido, cuándo empezó, su prioridad, su ganancia) y decide todo con ella: qué
  voz usar, a quién robar, cuándo termina un sonido (lo sabe por su largo y su *pitch*).
  El callback recibe comandos ya resueltos (tocar en la voz 12 desde el frame de audio
  N, cambiar la ganancia, parar) y mezcla. Esto deja **la lógica del audio en un solo
  hilo**, donde se prueba sin dispositivo, y el callback queda chico y fácil de revisar.
- **El contrato del callback** (Bencina, 2011):
  - no asigna memoria, no toma locks, no hace I/O, no registra y no llama a nada que
    bloquee. **La única excepción es Tracy** con un servidor conectado, en los builds
    de desarrollo: pide memoria mientras corre (ADR-0010), así que una sesión con Tracy
    conectado no sirve para verificar el contrato;
  - solo toca memoria reservada al iniciar: las voces, los buffers de mezcla, los
    sonidos cocinados (que solo lee) y los decodificadores de la música;
  - **entrada:** la cola de comandos (1024 para empezar; un comando que no entra se
    descarta y se cuenta);
  - **salida:** atómicos con el reloj del audio (cuántos frames de audio mezcló) y sus
    contadores (CPU máxima por llamada, llamadas tarde).
- **La segunda excepción al hilo único** (fe de erratas del ADR-0012): el hilo de audio
  de SDL3 no toca el estado del juego, igual que los hilos de Tracy. No usa allocators,
  así que la regla del ADR-0010 (ningún allocator compartido entre hilos) se cumple sin
  *scratch* propias.
- **Liberar sonidos:** un sonido cocinado no se libera mientras una voz lo lee. Antes de
  liberar (al cambiar de nivel), el hilo principal para las voces que lo usan y espera a
  que el reloj del audio pase el bloque en el que se aplicó la orden. Cuándo se cargan y
  se liberan los sonidos de un nivel lo decide el ADR-0028.
- **Los eventos de sonido del paso** van en **un arreglo fijo del sistema de audio,
  con vida de frame** (por ejemplo, 256 eventos: hasta 8 pasos por frame). La simulación
  lo escribe y el audio lo lee y lo vacía al final del frame. No son las colas del
  ADR-0012, que mueren con el paso, ni la arena del frame, que es del render (ADR-0010).
  **Si se llena,** el evento nuevo reemplaza al de menor prioridad si es más importante;
  si no, se descarta y se cuenta. Cada evento lleva: qué sonido, el paso, la entidad, la
  posición y los datos del combate (material, fuerza normalizada a `Fmax`).
- **El tiempo de cada evento:** el audio guarda un **desfase** entre el tiempo de la
  simulación y el reloj del audio, y agenda cada evento en **el tiempo de su paso más
  ese desfase**. El desfase se fija al iniciar, con **una latencia de partida de 2 pasos
  (33 ms)** sobre el reloj del audio, y **solo se reajusta** cuando un evento caería en
  el pasado o demasiado lejos: después de un tirón, una pausa o la deriva entre el reloj
  del sistema y el del dispositivo. Un evento que llega tarde suena en cuanto puede y se
  cuenta. El callback convierte el tiempo en un retardo dentro del bloque. Es lo que hace
  Unity con `PlayScheduled` sobre su reloj de DSP. La latencia total queda en ~33 ms más
  el buffer del dispositivo (10,7 ms). **El margen tiene que cubrir todo lo que pasa
  entre el paso y el envío de los comandos:** los pasos, el render (que corre antes del
  audio) con sus esperas, y un bloque del dispositivo. Si el render espera la *fence* o
  el *present* antes del audio, eso puede pasar de 33 ms (unos 39 ms en la peor cuenta),
  y el desfase se reajustaría hacia arriba. El ticket del game loop decide si esas
  esperas van después del audio o si sube la latencia de partida; el contador de
  eventos tarde lo muestra.
- **El robo de voces:** la voz robada baja a 0 con una rampa de un bloque, para no hacer
  clic, y el sonido nuevo empieza después. Como los eventos se agendan con 33 ms de
  margen, no se nota. **Las prioridades** (de mayor a menor): **crítica** (el corte, el
  rebote, el agarre, la UI: nunca se roban), **alta** (los golpes, la caballera),
  **normal** y **baja** (los gemidos, el ambiente). **Al pasar el límite de un sonido,**
  el nuevo roba la instancia que menos suena de ese mismo sonido si va a sonar más; si
  no, se descarta.
- **El mezclador:** float, estéreo intercalado y 48 kHz. Cada voz tiene una rampa de
  ganancia por bloque (sin *zipper noise*), un retardo dentro del bloque y un camino sin
  remuestreo cuando el *pitch* es 1. **Los buses:** música, mundo y UI, cada uno con su
  volumen. **La pausa del juego congela el bus del mundo** (sus voces dejan de avanzar
  y siguen donde estaban) y la UI sigue sonando.
- **El sonido 3D:**
  - **la distancia** se mide desde la caballera y **el paneo** sale de la cámara (su
    posición y su orientación): un zombie se oye a la izquierda si está a la izquierda en
    la pantalla;
  - **la atenuación** es `ref / (ref + rolloff · (d − ref))`, con la distancia limitada
    a `[ref, max]`. Cada sonido trae su `ref` y su `max` en la tabla, con `rolloff` 1.
    Una voz más allá de `max` sigue avanzando, pero no se mezcla;
  - **el pasa bajos de "atrás"** es un filtro de un polo por voz, que se mezcla según
    cuánto está la fuente detrás de la caballera, con la orientación de la cámara (en el
    prototipo, corte en 1,8 kHz);
  - todo se calcula **una vez por frame** en el hilo principal y viaja como ganancia
    izquierda, ganancia derecha y cantidad de filtro; el callback hace la rampa.
- **Los formatos y el cocinador** (fe de erratas del ADR-0017):
  - **los fuentes son FLAC**, en la carpeta de assets del ADR-0017, en git simple;
  - **los efectos** se cocinan a **PCM16 mono a 48 kHz**; la UI y la música pueden ser
    estéreo;
  - **la música** se cocina a **Vorbis estéreo a 48 kHz**, con calidad 4 para empezar
    (~100 kbps medidos);
  - **el cocinador falla el build** si un fuente no está a 48 kHz (así no hay remuestreo
    en el cocinador), si un sonido 3D no es mono o si la tabla nombra una toma que no
    existe;
  - **cocinar dos veces da los mismos bytes,** como el resto del cocinador;
  - **dependencias nuevas:**
    - **dr_flac** (dominio público o MIT-0, un header) **solo en el cocinador**, copiado
      en `tools/third_party/` como cgltf y stb_image (ADR-0017);
    - **libvorbis con libogg** (BSD-3) **solo en el cocinador**, como paquete del
      sistema (ADR-0006): no son un header suelto. La CI los instala con `pacman`, con
      la fecha del snapshot, como el resto del toolchain (ADR-0029);
    - **stb_vorbis** (dominio público o MIT, un archivo) **en el juego**, solo para la
      música, copiado en el repo con su versión a la vista. La carpeta la fija su
      ticket.

    Cada una lleva su fila en `CREDITS.md`, y `REUSE.toml` suma su anotación cuando
    entren los archivos (ADR-0019).
- **La música:**
  - **dos decodificadores de stb_vorbis**, cada uno con su buffer propio de 256 KiB
    reservado al iniciar: uno para el track que suena y otro para el que entra en el
    *crossfade*;
  - **el hilo principal abre el track** en el decodificador libre (stb_vorbis lee el Ogg
    desde la memoria, con el buffer propio: 0 `malloc` al abrir la música en el
    prototipo) y se lo pasa al callback con un comando. Desde ahí, solo el callback lo
    toca;
  - **los estados:** exploración, combate y jefe. Para empezar, "combate" es que algún
    zombie esté alertado (ADR-0023) a menos de una distancia; el jefe lo marca su
    encuentro. **Un tiempo mínimo en cada estado** (por ejemplo, 4 s) y **un *crossfade*
    de ~2 s**, los dos configurables. Qué track tiene cada nivel lo decide el ADR-0027.
- **El sonido del corte.** El combate (ADR-0018) emite los eventos, y la tabla dice qué
  capas suenan:

  | Evento | Capas |
  |---|---|
  | El tajo sale | Silbido; la fuerza sube su volumen y su largo |
  | Entra en carne | Carne + hueso |
  | Entra en cota de malla | Anillas (metal agudo y corto) + carne + hueso |
  | Entra por una junta | Metal sordo + carne + hueso |
  | Atraviesa la placa | Metal fuerte + carne + hueso |
  | Rebota | Metal de placa + raspado. Es lo que le dice al jugador que el ángulo no sirvió |
  | Hoja clavada | La entrada, más **el forcejeo en bucle** (un emisor mientras dura la secuencia), **una toma corta por pulsación** del *quick time* y **el final** (carne + hueso fuertes) |

  **La fuerza** (normalizada a `Fmax`) elige entre la toma "liviana" y la "pesada" de
  cada capa, y escala el volumen. **Cada zombie que cruza el tajo** dispara sus capas en
  su paso: en una fila de tres suenan tres entradas distintas, separadas por sus pasos.
  **El azar** sale de un hash del paso, la entidad y la capa, así que **el replay suena
  igual** y la simulación no gasta números. *Round robin* sin repetir la última toma:
  el hash elige, y si sale la última, se toma la siguiente.
- **La pausa de impacto y la escala de tiempo:** las voces no usan la escala de tiempo
  de los personajes (ADR-0018). Lo que se congela es la imagen; el golpe es lo que da
  peso a la pausa. **La cámara del tajo** (ADR-0022) baja el tono del bus del mundo
  mientras dura y le pone un pasa bajos; los eventos llegan con sus pasos, así que en
  cámara lenta se separan solos.
- **Los emisores** son un componente (`audio_emitter`, nombre de partida, ADR-0013):
  qué sonido en bucle, su ganancia y si está activo. El audio los recorre una vez por
  frame: si uno se activa, le da una voz; si se apaga, la para con una rampa. Sirven
  para el gemido de cada zombie, el forcejeo y el fuego. **Los gemidos de la horda**
  usan el límite por sonido: suenan los 8 que más se oyen, y los demás esperan una voz
  libre.
- **La tabla de sonidos:** por cada sonido, sus tomas, el bus, la prioridad, el límite
  de instancias, los rangos de *pitch* y de volumen, si es 3D (con `ref` y `max`) y si
  es un bucle. El formato del archivo lo fija su ticket. **En los builds de desarrollo,
  la tabla tiene que poder recargarse** sin cerrar el juego, y los cambios valen para
  los sonidos que empiezan después. **Cómo se recarga** lo decide el ADR-0028, que
  decide la recarga en caliente de los recursos (ADR-0017). Una toma nueva pide volver a
  cocinar.
- **La memoria:** la región de audio guarda los sonidos cocinados del nivel, la música
  comprimida, los dos decodificadores (2 × 256 KiB), las 64 voces y su copia en el hilo
  principal, la cola de comandos, el arreglo de eventos y los buffers de mezcla. Todo
  se reserva y se toca al iniciar. **La meta sigue en 64 MiB, como hipótesis:** entran
  ~11 min de efectos en mono PCM16 más la música comprimida.
- **El presupuesto** (fe de erratas de `presupuestos.md`): la fila por frame pasa a
  ser **"Audio en el hilo principal"** (eventos, emisores, sonido 3D y comandos), con
  los mismos 0,5 ms. Se suma **"Hilo de audio": 0,5 ms por cada 16,7 ms de audio, en
  otro núcleo**, como hipótesis. El prototipo midió 0,22 ms con 64 voces, más ~0,05 ms
  de la música.
- **El buffer del dispositivo** es de 512 frames (10,7 ms). Es una CVar que se lee al
  abrir el dispositivo; cambiarla lo reabre.
- **La plataforma** (ADR-0004): `platform::` abre el dispositivo, recibe una función
  propia como callback y le da un buffer de float estéreo. Ningún tipo de SDL sale del
  módulo. El mezclador vive en un módulo de audio por encima de la plataforma.
- **Tests (ADR-0011):**
  - **la cola SPSC es un contenedor, así que lleva TDD estricto;**
  - **los pasos de audio del cocinador** llevan el TDD estricto del cocinador
    (ADR-0017): leer un FLAC conocido, fallar con 44,1 kHz, fallar con un sonido 3D en
    estéreo y cocinar dos veces los mismos bytes;
  - **el resto lleva tests después,** con casos calculados a mano: el paneo y la
    atenuación, el robo de voces y el límite por sonido, la conversión del paso al
    reloj del audio y su reajuste, el limitador y la elección de tomas por hash;
  - **un render offline del mezclador** (la lógica del hilo principal y el callback,
    sin dispositivo, con un guion de eventos) **contra un hash guardado**. El paneo usa
    `cos` y `sin` de la glibc, así que el hash se compara con la glibc fija de la CI
    (ADR-0029). **La CD, que compila en *sniper* con otra glibc, se salta esa
    comparación**, igual que la del *replay* (fe de erratas del ADR-0029). Un cambio a
    propósito vuelve a grabar el hash.
- **Depuración (ADR-0021):**
  - **un panel de ImGui** con las voces (sonido, bus, prioridad, ganancia, distancia),
    los eventos del frame y el estado de la música;
  - **contadores en el registro por frame:** voces activas, robadas y descartadas por
    límite, eventos tarde, comandos descartados, CPU máxima del callback y llamadas que
    llegaron tarde (más de 1,5 periodos desde la anterior);
  - **una zona de Tracy en el callback,** en los builds de desarrollo;
  - **dibujo de depuración:** cada fuente con su distancia máxima.
- **Lo que se gana:** un audio que no se corta con los tirones del juego, que suena en
  el paso exacto de cada golpe y que cabe de sobra en el presupuesto. El corte se oye
  distinto según el material y la fuerza, y el rebote se lee sin mirar. Todo lo que
  decide está en un hilo, se prueba sin dispositivo y se explica pieza por pieza.
- **Lo que se sacrifica:**
  - escribir el mezclador, que SDL3 ya trae (y con mejor remuestreo);
  - un segundo hilo, con sus reglas, y la disciplina de no tocar nada compartido;
  - ~33 ms de latencia fija a cambio del ritmo exacto;
  - la espacialización fina: sin HRTF, el adelante y el atrás solo se distinguen por el
    pasa bajos;
  - las voces virtuales: un gemido que pierde su voz no sigue avanzando en silencio;
  - grabar o buscar los sonidos del corte, en vez de sintetizarlos.
- **Tickets, en orden** (cada uno crece desde el anterior):
  1. el dispositivo en `platform::`, con callback, el buffer configurable y el reloj
     del audio;
  2. la cola SPSC, con TDD;
  3. el mezclador: voces, rampas, retardo, remuestreo, buses y limitador, con el render
     offline contra un hash;
  4. el cocinador de audio y la tabla de sonidos, con TDD;
  5. el sistema de audio en el hilo principal: eventos del paso, el desfase del reloj,
     las voces, las prioridades, el robo y los límites;
  6. el sonido 3D: el oyente híbrido, la atenuación, el paneo y el pasa bajos;
  7. los emisores;
  8. la música: los decodificadores, los estados y el *crossfade*;
  9. los sonidos del corte: los eventos del combate y sus capas;
  10. el panel, los contadores, Tracy y el dibujo de depuración;
  11. la recarga de la tabla en desarrollo, cuando el ADR-0028 decida cómo se recargan
      los recursos.
- **Abierto:**
  - todos los valores de partida, que se ajustan escuchando y jugando: la latencia fija,
    el buffer, las prioridades de cada sonido, los límites, los rangos de variación, las
    distancias, el corte del pasa bajos, el tono de la cámara del tajo, el *crossfade* y
    el tiempo mínimo de cada estado de la música;
  - el formato del archivo de la tabla (su ticket);
  - cómo se recarga la tabla en desarrollo (ADR-0028);
  - dónde espera el render (la *fence* y el *present*) respecto del audio, para que el
    margen de 33 ms alcance (el ticket del game loop);
  - qué sonidos carga cada nivel y cuánto viven (ADR-0028), el ambiente y la música de
    cada nivel (ADR-0027) y los volúmenes en las opciones (ADR-0026);
  - los golpes de los pedazos contra el suelo, que necesitan eventos de contacto de la
    física (ADR-0016);
  - la oclusión, la HRTF, el Doppler y las voces virtuales, si el juego los pide;
  - la latencia de punta a punta, que pide un lazo físico entre la salida y un
    micrófono para medirla.

**Fe de erratas en otros ADRs.** Al aceptar este ADR se corrigen, como fe de erratas
(ADR-0014, regla 11):

| ADR | Qué se corrige |
|---|---|
| ADR-0004 | El alcance de SDL3 en el audio: solo la salida, con callback; la mezcla es propia. Se cierra en Consecuencias |
| ADR-0006 | Cómo entran las dependencias del audio: stb_vorbis copiado en el repo, en el juego; libvorbis y libogg como paquetes del sistema, solo en el cocinador. Se corrige en la Decisión 5 |
| ADR-0010 | Un hilo que no usa allocators (el de audio, los de Tracy) no necesita *scratch* propias. Se corrige en Consecuencias |
| ADR-0012 | El hilo de audio de SDL3 es la segunda excepción al hilo único: ejecuta comandos y mezcla, sin tocar el estado del juego. El sistema de audio corre una vez por frame, después del render, y es código que corre cada frame. Se corrige en el orden del frame, la Decisión 5 y Consecuencias |
| ADR-0017 | Los sonidos: fuentes en FLAC; efectos en PCM16 mono y música en Vorbis, a 48 kHz; el build falla con otra frecuencia; dr_flac y libvorbis solo en el cocinador; la tabla de sonidos cocinada |
| ADR-0029 | La CD en *sniper* se salta toda comparación contra un hash guardado (el *replay* y el render offline del audio), porque solo vale con la glibc del snapshot. Se corrige en la Decisión 10 y en Consecuencias |
| `docs/performance/presupuestos.md` | "Audio en el hilo principal" por frame y la meta nueva del hilo de audio |
| `CREDITS.md` | stb_vorbis (juego), dr_flac, libvorbis y libogg (cocinador) |

## Cómo lo defiendo

**SDL3 ya mezcla. ¿Por qué escribiste tu propio mezclador?**
Lo medí: con un stream de SDL3 por voz, el hilo de audio gastó de 3 a 4 veces más
(0,73 ms contra 0,22 ms por cada 16,7 ms, con 64 voces) y asignó memoria de vez en
cuando, que es justo lo que no puede pasar en ese hilo. Además, la API de SDL no tiene
lo que necesito: no se puede empezar un sonido en la mitad de un bloque y no tiene buses.
SDL remuestrea mejor que mi lineal, eso es cierto, pero yo solo remuestreo cuando cambia
el *pitch*. Y el mezclador es una parte que quiero entender: son unas pocas decenas de
líneas de suma, rampas y paneo, no un sistema enorme.

**Tu engine es de un hilo. ¿Por qué el audio corre en otro?**
Porque el audio tiene un plazo duro que nada más en el juego tiene. Un frame de video que
llega tarde se repite y casi no se nota; un buffer de audio que llega tarde se oye como
un corte. Si mezclo en el hilo principal, el audio depende de su ritmo: en el prototipo,
un tirón de 250 ms dejó 220 ms de silencio con 30 ms en cola, y solo con 300 ms en cola
no se cortó, pero esos 300 ms son latencia en cada sonido. En el callback, un tirón del
juego solo atrasa los sonidos nuevos. Es la segunda excepción al hilo único, como Tracy:
el hilo de audio no toca el estado del juego.

**¿Qué no puede hacer el callback, y por qué?**
No puede asignar memoria, tomar un lock, leer el disco, escribir el log ni llamar a nada
que pueda bloquear, porque ninguna de esas cosas tiene un tiempo máximo garantizado. Es la
regla de Ross Bencina: *time waits for nothing*. Un `malloc` puede entrar al sistema
operativo, y un lock puede quedar esperando a un hilo que el planificador sacó de la
CPU. Por eso todo lo que el callback toca se reserva al iniciar. Por esa misma regla
descarté libvorbis en el juego: en el prototipo hizo casi 500 `malloc` al decodificar
3 minutos de música, y stb_vorbis con un buffer propio no hizo ninguno.

**¿Cómo se hablan los dos hilos sin locks?**
Con una cola de un productor y un consumidor: un arreglo fijo y dos índices. El hilo
principal es el único que escribe el índice de escritura, y el callback, el único que
escribe el de lectura. El productor primero copia el comando en su lugar y después
publica el índice con *release*; el consumidor lee el índice con *acquire*, así que ve el
comando completo. Como cada índice tiene un solo dueño, no hace falta comparar e
intercambiar. De vuelta, el callback solo publica atómicos: su reloj y sus contadores.

**¿Por qué el callback no decide nada?**
Porque así toda la lógica del audio vive en un solo hilo, el principal, donde la puedo
probar sin un dispositivo y depurar con las herramientas de siempre. El hilo principal
lleva una copia de cada voz y decide qué voz usar, a quién robar y cuándo termina cada
sonido. El callback recibe órdenes ya resueltas ("toca esto en la voz 12 desde el frame
de audio N") y mezcla. Un callback chico es fácil de revisar contra las reglas de tiempo
real, y un error de lógica no queda escondido en el hilo más difícil de depurar.

**¿Qué es el *timestamp* de cada sonido y por qué hace falta?**
Mi simulación corre a 60 Hz con un acumulador, así que un frame puede tener cero, uno o
más pasos (hasta 8). Si cada sonido empieza al principio del próximo bloque de audio, hereda ese
ritmo irregular: en el prototipo, golpes que debían sonar cada 100 ms quedaron entre 85 y
107 ms, y dos golpes de pasos seguidos sonaron en el mismo instante, como uno solo. Con
el *timestamp*, cada sonido suena en el tiempo de su paso más una latencia fija, y los
intervalos quedaron exactos. Es lo mismo que hace el render con la interpolación: no
mostrar el ritmo de los frames, sino el de la simulación.

**¿Eso no agrega latencia?**
Sí: unos 33 ms fijos (dos pasos), más los 10,7 ms del buffer del dispositivo. Lo elijo a
propósito, porque una latencia fija se percibe mucho menos que un ritmo irregular, y el
golpe del corte tiene que sonar en el paso exacto en que la hoja entra. El desfase entre
la simulación y el reloj del audio no se recalcula en cada frame, porque eso traería de
vuelta el temblor de un bloque; solo se reajusta después de un tirón o de una pausa. La
latencia y el buffer son configurables, y se afinan escuchando.

**¿Cómo suena distinto la carne del metal, y por qué tantas variaciones?**
El combate me dice qué material cruzó la hoja y con cuánta fuerza, y la tabla de sonidos
decide qué capas suenan: carne y hueso; metal sordo por una junta; metal fuerte y
raspado en un rebote. El rebote tiene que leerse sin mirar, porque es lo que le dice al
jugador que el ángulo no sirvió. Las variaciones son obligatorias: un tajo que cruza tres
zombies con la misma toma suena a ametralladora, y lo escuché en el prototipo. El azar
de las variaciones sale de un hash del paso, la entidad y la capa, no del generador de la
simulación: si el audio gastara números de ese generador, el replay dejaría de dar el
mismo hash. Y así el replay también suena igual.

**En tercera persona, ¿dónde está el oyente?**
En dos lugares a la vez. La distancia se mide desde la caballera y el paneo sale de la
cámara. Si el oyente está solo en la cámara, un zombie que pasa entre la cámara y la
caballera suena mucho más fuerte que uno igual de cerca, pero adelante. Si está solo en
la caballera, el paneo gira con ella y no con lo que se ve en la pantalla. Con el híbrido,
un zombie que veo a la izquierda suena a la izquierda, y su volumen depende de qué tan
cerca está del personaje. Es el patrón que describen las comunidades de Wwise y Unreal
para la tercera persona. El estéreo no distingue adelante de atrás, así que uso un pasa
bajos cuando la fuente está detrás de la caballera.

**¿Qué pasa cuando 50 zombies gimen y además hay un corte?**
Hay 64 voces y cuatro prioridades. El corte, el rebote, el agarre y la UI son críticos y
nunca se roban. Un sonido nuevo solo puede robar una voz de prioridad menor, y la robada
baja a 0 en un bloque para no hacer clic. Además, cada sonido tiene un límite de
instancias: suenan los 8 gemidos que más se oyen y los demás esperan una voz libre. Así el
corte siempre suena, y la horda se oye como horda sin tapar lo importante.

**¿Por qué Vorbis y no Opus, que es más moderno? ¿Y por qué decodificar en el callback?**
Los dos decodifican de sobra a tiempo y no asignan memoria. Opus pesó un poco menos, pero
sin `opusfile` necesitaría un contenedor propio, y stb_vorbis lee el archivo Ogg completo
desde la memoria con un solo archivo de código. Los efectos no se comprimen: son PCM16,
porque el ADPCM se oía en los sonidos con mucho ruido, que son justo los del corte. La
música se queda comprimida en memoria (2,2 MiB por 3 minutos, contra 33 MiB sin
comprimir) y la decodifica el callback, por la misma razón que mezclo ahí: un tirón del
hilo principal no la corta. Decodificar cuesta ~0,05 ms por cada 16,7 ms de música.

**¿Cómo pruebas algo que se escucha?**
Separo lo que se puede verificar de lo que se escucha. La cola es un contenedor y lleva
TDD estricto, y lo que hace el cocinador con los sonidos también. El paneo, la
atenuación, el robo de voces y el *timestamp* son funciones con casos calculados a mano.
Además, renderizo offline una escena guionada, sin dispositivo, y comparo su hash contra
uno guardado: en el prototipo, el hash fue el mismo con Clang y GCC, de `-O0` a `-O3`.
Si cambio la mezcla a propósito, vuelvo a grabar el hash. Si suena bien o no, se decide
escuchando.

## Referencias

- Ross Bencina, *Real-time audio programming 101: time waits for nothing* (2011):
  <http://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing>
- FMOD Engine User Manual 2.03, *Managing resources in the Core API* (voces virtuales y
  prioridades): <https://fmod.com/docs/2.03/api/managing-resources-in-the-core-api.html>
- SDL3, `SDL_BindAudioStreams`: <https://wiki.libsdl.org/SDL3/SDL_BindAudioStreams>
- SDL3, el remuestreo de banda limitada:
  <https://github.com/libsdl-org/SDL/blob/main/src/audio/SDL_audioresample.c>
- miniaudio: <https://miniaud.io/>
- stb_vorbis 1.22 (Sean Barrett, dominio público): <https://github.com/nothings/stb>
- *Definition of the Opus Audio Codec*, RFC 6716 (2012):
  <https://www.rfc-editor.org/rfc/rfc6716>
- Xiph.Org, *Vorbis I specification*: <https://xiph.org/vorbis/doc/Vorbis_I_spec.html>
- OpenAL 1.1 Specification and Reference (2005), modelos de distancia.
- Unity, `AudioSource.PlayScheduled`:
  <https://docs.unity3d.com/ScriptReference/AudioSource.PlayScheduled.html>
- Audiokinetic Q&A, *Unity: distance relative to the player, panning relative to the
  camera*: <https://www.audiokinetic.com/qa/6112/unity-distance-relative-the-player-panning-relative-camera>
- PlatinumGames, blog de Audiokinetic sobre el oyente:
  <https://platinumgames.com/official-blog/article/10027>
- Guy Somberg (ed.), *Game Audio Programming: Principles and Practices*, vols. 1–5,
  CRC Press, 2016–2025.
