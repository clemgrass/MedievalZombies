# ADR-0021: Herramientas de depuración, profiling y logging

- **Estado:** Aceptado
- **Fecha:** 2026-10-28
- **Ticket:** #43
- **Reemplaza / Reemplazado por:** —

## Contexto y problema

Antes de escribir el primer sistema hay que decidir **cómo se va a ver lo que pasa
dentro del engine:** dónde se va el tiempo, cuánta memoria se usa, qué hizo el corte y
por qué, y cómo se repite exactamente una partida. Sin eso, el corte de mallas se
depura a ciegas y los requisitos del ADR-0020 no se pueden medir.

Varios ADRs ya aceptados dejaron preguntas para este:

| ADR | Qué dejó abierto |
|---|---|
| ADR-0020 | Instrumentar R1 a R4 (tiempo de CPU por sistema, timestamps de GPU, tiempos de presentación, memoria). Grabar el input **por paso de simulación, no por frame**. Cómo cuentan para R1 los frames de recuperación después de un tirón |
| ADR-0011 y ADR-0016 | El *replay* se diseña una sola vez, para los tests y para los escenarios del protocolo. Si el hash del estado tiene que ser estable entre máquinas, para compararlo en la CI |
| ADR-0007, ADR-0015, ADR-0018 y la visión | Valores que se ajustan **sin recompilar**: los del tajo, el ancho de la cuña, las fuerzas y umbrales del combate, la cámara y los controles |
| ADR-0010 | Cómo se ven el uso y los picos de cada región de memoria durante el juego |
| ADR-0017 | Herramientas chicas dentro del engine (ver un asset cocinado, ajustar las cápsulas). Necesitan una interfaz |
| ADR-0030 | Si la macro de traza del verificador de diagramas se comparte con el profiling |

Hay que decidir:

1. la **interfaz de depuración** dentro del juego;
2. el **profiler de CPU**;
3. las herramientas de **GPU**;
4. cómo se **mide R1 a R4** para el protocolo del ADR-0020;
5. el **logging**: niveles, categorías, salida y formato;
6. el **dibujo de depuración** (*debug draw*) y qué muestra del corte;
7. los **valores ajustables sin recompilar**;
8. la **grabación y el *replay***, con los controles de tiempo;
9. las **herramientas de memoria**;
10. **qué entra en cada preset** de build.

Lo que ya está decidido y limita este ADR:

- **ADR-0004:** SDL3 escondido detrás de `platform::`. Wayland primero, X11 de respaldo.
  El input se lee una vez por frame y se convierte en el estado de input del juego.
- **ADR-0005:** Vulkan 1.3. **RenderDoc y las validation layers** son las herramientas de
  depuración de la GPU, con las layers activas en los builds de desarrollo.
- **ADR-0006:** tres presets: `debug` (ASan, UBSan y validation layers),
  `relwithdebinfo` (optimizado con símbolos, para perfilar) y `release`. GDB como
  depurador. SDL3 entra como paquete del sistema o con `FetchContent` y versión fijada.
- **ADR-0008:** x86-64 base, sin `-march` y sin `-ffast-math`.
- **ADR-0009:** sin excepciones ni RTTI. En el código que corre cada frame no se usan
  contenedores que asignan memoria. Los `assert` con mensaje tienen su propio ticket.
- **ADR-0010:** regiones de memoria por sistema y por tiempo de vida, tocadas al
  iniciar, con límites duros. Una arena por paso de simulación y otra por frame.
- **ADR-0011:** tests de *replay* que comparan un hash del estado. Los tests corren en
  Debug.
- **ADR-0012:** un solo hilo. Paso fijo de 60 Hz, con hasta 8 pasos por frame. Las colas
  de eventos se pueden registrar para depurar.
- **ADR-0013:** las macros van en mayúsculas.
- **ADR-0016:** determinismo con el mismo ejecutable y la misma máquina: paso fijo, un
  hilo, orden fijo, memoria inicializada, nada del reloj real.
- **ADR-0019:** cada dependencia nueva va a `CREDITS.md` con su licencia, y su licencia
  tiene que ser compatible con MIT.
- **ADR-0020:** el protocolo mide en `relwithdebinfo`, en la máquina de referencia, con
  escenarios grabados y versionados, al menos 5 ejecuciones y percentiles con
  *bootstrap*.
- **`CLAUDE.md`:** todo gratis. Cada dependencia externa necesita un ADR que la
  justifique.

**Fuera de alcance:**

| Qué | Lo decide |
|---|---|
| Las macros de `assert` y el tipo `result<T, E>` | Su ticket (ADR-0009) |
| El envenenamiento de ASan dentro de los allocators | ADR-0010 |
| Qué se automatiza en la CI | ADR-0029 |
| El editor de niveles | ADR-0027 |
| Las opciones que ve el jugador (menú de opciones) y dónde se guardan | ADR-0028 |
| El diseño del game loop (ajustar el `dt` al refresco, el ruido del vsync) | Ticket del game loop |

## Criterios

Los cinco del ticket, más dos que salen de cruzar lo ya decidido:

1. **Gratis.**
2. **Impacto nulo en Release:** lo que no se usa en Release no se compila en Release.
3. **Ayuda real para depurar el corte y la memoria,** que son los dos sistemas donde un
   error es más difícil de ver.
4. **Cada dependencia justificada.**
5. **Aprendizaje.**
6. **No falsear lo que se mide.** El protocolo del ADR-0020 mide en `relwithdebinfo`, así
   que la instrumentación **está activa al medir**. "Impacto nulo" vale para Release; en
   `relwithdebinfo` el costo tiene que ser despreciable y estar medido.
7. **No romper el determinismo.** Una herramienta de depuración no cambia el estado de
   la simulación sin que el cambio quede grabado.

## Opciones

### 1. La interfaz de depuración

| Opción | Cómo es | Problema |
|---|---|---|
| **Dear ImGui** (MIT) | Interfaz *immediate mode*: cada frame se describe la interfaz con llamadas (`ImGui::SliderFloat(...)`), y la biblioteca devuelve listas de vértices para dibujar. Es la más usada en la industria para herramientas internas. Medido: un panel típico cuesta 37 µs por frame (p99 93 µs), sin asignar memoria en régimen | Una dependencia de ~43 000 líneas que nadie del proyecto escribió. ~900 KB de código en los builds que la incluyen |
| **microui** (MIT) | Lo mismo en ~1100 líneas de C. Trabaja en una región de memoria fija y solo pide dibujar rectángulos y texto | Pocos controles: no trae gráficas, tablas ni árboles. Lo que falte se escribe |
| **Una propia** | Una interfaz *immediate mode* escrita en el engine, al estilo de la charla de Casey Muratori (2005) | Es un proyecto en sí: texto, foco, layout, scroll. Compite en tiempo con el corte |
| **Ninguna** | Consola de texto y archivos de configuración | Ajustar una curva o ver una gráfica de tiempos con texto es lento. El editor de cápsulas del ADR-0017 no tiene dónde vivir |

**Una pregunta dentro de esta dimensión: quién dibuja la interfaz.** ImGui trae un
backend de Vulkan (~2050 líneas, con su propio pipeline y sus descriptores). La otra
opción es dibujar sus listas de vértices con el renderer propio: un pipeline, una
textura (el atlas de la fuente, 256 KiB) y los datos por el *ring buffer* del ADR-0005.

### 2. El profiler de CPU

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Tracy** (BSD-3) | Instrumentación: una macro (`ZoneScoped`) marca cada zona, el cliente la escribe en una cola sin locks y un hilo propio la manda por red a un servidor con GUI. También mide GPU (Vulkan), memoria, valores (`TracyPlot`) y muestreo. Medido: 10–14 ns por zona conectado, 4,6 ns sin servidor con `TRACY_ON_DEMAND` | Sin `TRACY_ON_DEMAND` y sin servidor guarda todo en RAM (640 MB por 10 M de zonas). Agrega un hilo y un socket. El servidor no está en los repositorios de Arch: se compila. Cliente y servidor tienen que ser de la misma versión |
| **Uno propio** que exporta a Perfetto | Una macro de zona con `rdtsc` escribe eventos en una región fija. Al terminar (o con una tecla) se exportan al formato de Chrome o al de Perfetto, que se ven en el navegador (`ui.perfetto.dev`). Medido: 4–8 ns por zona | La GUI es de otros, pero el formato, el buffer y la exportación son propios. El JSON pesa: ~650 MB para 10 M de zonas (estimación) |
| **Solo muestreo** (`perf`) | El kernel interrumpe el programa ~1000 veces por segundo y anota la pila. No hace falta tocar el código. Medido: +9 % de tiempo con *frame pointers* | No sabe qué es un frame: dice dónde se va el tiempo en promedio, no por qué *un* frame tardó 30 ms. Los picos del ADR-0020 (p99,9 y máximo) se pierden |
| **Contadores propios en pantalla** | Cada sistema mide su tiempo por frame y se muestra en un panel, como el `stat unit` de Unreal | Muestra el síntoma, no la causa. No hay línea de tiempo |

Estas opciones no son excluyentes: la instrumentación y el muestreo responden preguntas
distintas.

### 3. La GPU

| Opción | Para qué sirve | Problema |
|---|---|---|
| **Timestamps propios** (`vkCmdWriteTimestamp`) | Medir cuánto tarda cada pasada en la GPU. Resolución en la UHD 620: 83 ns | Hay que elegir bien la etapa: un timestamp en `TOP_OF_PIPE` incluyó la espera del swapchain y dio 6 ms para limpiar una imagen. Mide tiempo de pared en la GPU, compartida con el compositor |
| **Zonas de GPU de Tracy** (`TracyVkZone`) | Lo mismo, con la línea de tiempo de la GPU junto a la de la CPU, calibradas con `VK_KHR_calibrated_timestamps` | Depende de Tracy |
| **RenderDoc** (ya decidido en el ADR-0005) | Capturar un frame y ver cada llamada, cada recurso y cada estado. Se puede disparar desde el código con su API *in-app* | No mide tiempos de forma fiable: estando dentro de RenderDoc, el trabajo del prototipo pasó de ~60 a ~120 µs. **En Wayland no funciona:** hay que correr el juego en XWayland |
| **Herramientas de Mesa** (`INTEL_MEASURE`, `intel_gpu_top`) | Tiempos por pasada desde el driver, y uso de la GPU del sistema | Son específicas de Intel y no se probaron aquí |

### 4. Cómo se mide R1 a R4

El protocolo necesita, por frame: el tiempo de CPU de cada sistema (R1), el de la GPU
(R2), el momento real de presentación (R3) y la memoria (R4). Después se calculan
percentiles con *bootstrap* entre ejecuciones.

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Desde las capturas de Tracy** | `tracy-capture` graba sin GUI (funcionó en el prototipo) y `tracy-csvexport` exporta las zonas | El protocolo depende de un programa externo, de su versión y de un socket. La presentación y la memoria igual hay que mandarlas como valores |
| **Un registro por frame propio** | El engine escribe una fila por frame en una región fija (~100 B: número de frame, pasos de simulación, tiempo de cada sistema, GPU, presentación, memoria) y la vuelca a un archivo al terminar. 25 000 frames (7 minutos) son ~2,5 MB | Hay que escribir el registro, el volcado y el script que calcula los percentiles |
| **Las dos** | El registro propio para el protocolo; Tracy para entender un pico | Dos caminos que mantener. El registro tiene que medir lo mismo que las zonas |

**Lo que da esta máquina para medir R3:** `VK_EXT_present_timing` funciona en Wayland y
da el momento en que sale el primer píxel: p50 16,672 ms y p99 16,680 ms entre frames,
contra un reloj de CPU que ve hasta 24 ms. En XWayland no da esa etapa, solo el fin de
la cola. `VK_KHR_present_wait2` es el respaldo en las dos.

**Una pregunta dentro de esta dimensión: los frames de recuperación.** Después de un
tirón externo (el sistema operativo, el compositor), el loop corre hasta 8 pasos y se
recupera solo (ADR-0020). Esos frames pasan de 33,3 ms con 5 pasos o más:

| Opción | Problema |
|---|---|
| **Cuentan como cualquier otro** | Un tirón que no causó el juego empeora su R1 |
| **Se marcan y se reportan aparte** | Hay que detectar qué fue "externo". El registro sabe cuántos pasos corrió cada frame y cuánto tardó cada uno, así que un frame con varios pasos rápidos después de un hueco es distinguible |
| **Se descartan** | Se esconde un problema real: si el juego causa el tirón, se borra la prueba |

### 5. El logging

**Niveles y categorías:** casi todos los engines usan 4 o 5 niveles (error, aviso,
info, debug, traza) y una categoría por módulo (`core`, `platform`, `gpu`, `slicing`,
`physics`…), que se filtra por separado. Las opciones están en el formato y en la
salida.

| Opción de formato | Cómo funciona | Medido | Problema |
|---|---|---|---|
| **`std::format_to_n` a un buffer fijo** | Se formatea en el momento, a una entrada de tamaño fijo, sin asignar | ~450 ns por línea, 0 asignaciones | El costo cae en el frame, en el sistema que loguea |
| **`printf` y familia** | Igual, con el formato clásico de C | ~400 ns | Sin chequeo de tipos más allá de `-Wformat` |
| **Formato diferido** (estilo NanoLog o Quill) | Se guardan el id del formato y los argumentos crudos (32 B). Se formatean después, fuera del frame o en otro proceso | **14 ns** | Mucho más complejo: un registro de formatos, cuidado con las cadenas y un paso de lectura. Su ventaja se nota con miles de líneas por frame |
| **spdlog** (MIT) | Biblioteca completa con *sinks*, niveles y modo asíncrono | — | Una dependencia más para algo chico. Por defecto usa excepciones (`SPDLOG_NO_EXCEPTIONS` las apaga) |

| Opción de salida | Problema |
|---|---|
| **stderr** | Se pierde al cerrar la terminal, y es lento si la terminal es lenta |
| **Archivo, vaciado una vez por frame** | Lo último se pierde si el programa se cae, salvo que el error fatal vacíe antes de abortar |
| ***Ring buffer* en memoria** | Guarda las últimas N líneas para mostrarlas en una consola dentro del juego o volcarlas al fallar. No sobrevive a un `SIGKILL` |

**Qué se elimina al compilar:** cada nivel puede quedar en el binario o desaparecer
por preset. Por ejemplo, la traza solo en `debug`, y en `release` solo error y aviso.

### 6. El dibujo de depuración

Es una API para dibujar líneas, esferas, cápsulas, planos, flechas y texto en el mundo
3D desde cualquier sistema, como el `DrawDebugLine` de Unreal. Lo que el corte necesita
ver: el plano del tajo y su barrido, las cápsulas de cada hueso (y cuáles tocó la
hoja), la espada clavada, el contorno del corte, la cuña de la hendidura y los pedazos
coloreados por grupo de huesos.

| Opción | Cómo funciona | Problema |
|---|---|---|
| ***Immediate mode* propio** | Cada llamada agrega primitivas a un buffer. El render las dibuja como una lista de líneas (por el *ring buffer*), con o sin prueba de profundidad. Una duración opcional, en pasos, las mantiene varios frames | Las llamadas ocurren dentro de los pasos (de 0 a 8 por frame) y el dibujo, una vez por frame. El buffer no puede vivir en la arena del paso, que se borra al terminar cada paso (ADR-0010) |
| **Objetos de depuración persistentes** | Se crea un objeto "línea" con su vida, y se borra después | Hay que acordarse de borrarlos. Es justo el error que el *immediate mode* evita |
| **Solo en 2D, con la lista de fondo de ImGui** | Se proyecta cada punto a la pantalla y se dibuja con ImGui | Sin profundidad: no se ve qué está delante. Depende de la interfaz elegida |

### 7. Los valores ajustables sin recompilar

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Variables de consola** (*CVars*) | Cada valor se registra en el código con nombre, tipo, valor por defecto, rango y banderas (por ejemplo, "afecta la simulación"). Se cambian desde una consola, un panel o un archivo. Es el modelo de Quake (1996) y de Unreal (`TAutoConsoleVariable`) | Un registro global que hay que inicializar en orden. Los nombres son cadenas |
| **Un archivo de datos con recarga en caliente** | Los valores viven en archivos de texto, fuera del código. El engine los relee al cambiar (`inotify`) | Necesita un formato y un parser (propio o una dependencia). El código no dice qué valores existen |
| **Sliders atados a variables** | La interfaz edita la variable directamente | Lo que se ajusta se pierde al salir, salvo que alguien lo copie al código |
| **Registro de CVars + archivo + panel** | El registro es la fuente de verdad. El archivo guarda los valores que se cambiaron, y el panel los edita | Tres piezas, aunque cada una es chica |

**El determinismo:** si un valor que afecta la simulación cambia en medio de una
partida grabada, el replay deja de dar lo mismo. Hay dos caminos: el cambio se graba
como un evento del paso en que ocurrió, o los valores que afectan la simulación no se
pueden cambiar mientras se graba. En los dos casos, el replay guarda todos los valores
al empezar.

**En Release:** los valores pueden quedar como constantes (con los valores ajustados ya
copiados al código) o seguir leyéndose de un archivo. Lo que ve el jugador (cámara,
controles) es otra cosa: son opciones del jugador, y su guardado lo decide el ADR-0028.

### 8. La grabación y el replay

**Qué se graba:** el estado de input de cada paso, ya convertido de los eventos de
SDL (ADR-0004), no los eventos crudos. Así lo pide el ADR-0020. El ADR-0012 lee el input
una vez por frame, pero el replay necesita saber qué input vio **cada paso**. Por
ejemplo, si un frame corre 3 pasos, el movimiento del mouse de ese frame lo consume
solo el primero, o se reparte entre los tres. Esa regla es del game loop, y el replay
graba el resultado. ~24 B por paso son ~86 KB por minuto, sin comprimir.

**El encabezado:** versión del formato, commit, escenario o nivel de partida, semilla,
y los valores ajustables del inicio (dimensión 7).

| Opción para comprobar un replay | Cómo funciona | Problema |
|---|---|---|
| **Solo el hash final** | Al terminar, se compara un hash del estado | Si difiere, no dice dónde |
| **Hashes de control cada N pasos** | El archivo guarda un hash cada, por ejemplo, 60 pasos. La reproducción se detiene en el primero que difiere | Hay que definir qué entra en "el estado" y recorrerlo en orden fijo (ADR-0016) |

**Entre máquinas** (lo dejó abierto el ADR-0016). El prototipo muestra que con las
banderas del ADR-0008, Clang y GCC en -O0, -O2 y -O3 dan **los mismos bits** en la
misma máquina, pero `sinf`, `cosf` y `expf` de glibc no redondean correctamente, y
`atan2f` y `acosf` cambiaron en glibc 2.41:

| Opción | Problema |
|---|---|
| **Hash solo en la misma máquina** (lo que ya garantiza el ADR-0016) | En la CI solo se puede comprobar que dos ejecuciones en el mismo proceso dan lo mismo, no contra un hash guardado |
| **Funciones trascendentales propias** en `engine/core` | `sin`, `cos`, `exp`, `atan2` y `acos` con resultados idénticos en cualquier máquina x86-64. Es trabajo de matemática numérica, y cada función usada en la simulación tiene que salir de ahí |
| **La CI en un contenedor con la misma glibc** | La garantía depende de una imagen que hay que mantener al día con la máquina de desarrollo |

**Cuándo un replay deja de servir:** cualquier cambio de gameplay cambia el resultado.
Un escenario del protocolo se vuelve a grabar cuando eso pasa, y el commit del
encabezado dice con qué versión se grabó.

**Grabar siempre:** con ~86 KB por minuto, los últimos 5 minutos de input caben en
~430 KB. Un *ring buffer* que graba siempre permite adjuntar el replay a un reporte de
bug (ADR-0011), que se reproduce exacto.

**Controles de tiempo:** pausar la simulación, avanzar un paso, cámara lenta (menos
pasos por segundo de reloj, el mismo `dt`) y reproducir un replay hasta un paso dado.
Como el `dt` no cambia, nada de esto rompe el determinismo.

### 9. Las herramientas de memoria

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Contadores en cada región** | Cada arena y cada pool lleva su uso, su pico (*high-water mark*), su capacidad y sus fallos o degradaciones. Cuesta una o dos sumas por asignación | Si están siempre, cuestan algo en Release. Si no, Release no sabe cuándo se acerca al límite |
| **Panel y gráfica** | La interfaz de depuración muestra una tabla por región y una gráfica del pico | Depende de la dimensión 1 |
| **Eventos de memoria de Tracy** (`TracyAllocN`) | Cada asignación se manda a Tracy, que muestra qué está vivo y desde dónde se pidió | Pensado para el heap: una arena con miles de asignaciones por paso manda miles de eventos. Sirve más para los pools de pedazos que para las arenas |
| **La memoria del proceso y de la GPU** para R4 | El pico del proceso (`VmHWM` de `/proc/self/status`) y el uso de `VK_EXT_memory_budget`, en el registro por frame | El presupuesto de la GPU cambia con la RAM libre del sistema (de 1,7 a 2,0 GiB medido): el uso es el dato estable, el presupuesto no |

### 10. Qué entra en cada preset

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Una macro por herramienta** | Profiler, interfaz, dibujo de depuración, valores ajustables y replay se activan por separado, y cada preset define su combinación en `CMakePresets.json` | Muchas combinaciones posibles; se prueban solo las de los presets |
| **Una sola macro de desarrollo** | Todo junto: en `debug` y `relwithdebinfo` está, en `release` no | No se puede medir `relwithdebinfo` sin la interfaz, aunque esté oculta |
| **Siempre compilado, activado en tiempo de ejecución** | Todo existe en todos los builds y se prende con una bandera | Rompe el criterio 2: el código y sus ramas están en Release |

**Con la traza del ADR-0030:** el verificador de diagramas necesita una macro de traza
activa donde corren los tests (en Debug). El ADR-0030 dejó tres salidas: definirla en el
preset `debug`, solo en los targets de test, o un preset nuevo. Si las zonas del
profiler y la traza son la misma macro, una sola instrumentación sirve para las dos.

## Investigación

### Prototipo medido

Seis programas en C++20, medidos en la máquina de referencia (ADR-0020) el 2026-10-23,
fuera del protocolo. El detalle y cómo repetirlos están en
[`../research/herramientas-de-depuracion.md`](../research/herramientas-de-depuracion.md).

| # | Qué | Resultado |
|---|---|---|
| P1 | Costo por zona | Propia con `rdtsc`: 4–8 ns. Tracy conectado: 10–14 ns. Tracy *on demand* sin servidor: 4,6 ns. **Tracy sin *on demand* y sin servidor: 640 MB por 10 M de zonas** |
| P2 | `perf` | Funciona sin privilegios (`perf_event_paranoid = 2`, solo espacio de usuario). +9 % de tiempo a 999 Hz con *frame pointers*, +15 % y 46 MB con DWARF |
| P3 | GPU y presentación | Timestamps de 83 ns. Primer píxel cada 16,672 ms (p99 16,680 ms) en Wayland. **RenderDoc no corre en Wayland**, sí en XWayland. Las validation layers encontraron un error real que el driver aceptaba |
| P4 | Logging | ~450 ns por línea formateada, 0 asignaciones, `std::format` sin excepciones. Formato diferido: 14 ns |
| P5 | Replay | **Mismo hash con Clang y GCC en -O0, -O2 y -O3** con las banderas del ADR-0008. `-march=native` lo cambia desde el primer paso (FMA). `sinf` difiere del correctamente redondeado en el 1 % de los valores |
| P6 | Dear ImGui | Panel típico: 37 µs por frame (p99 93 µs), 0 asignaciones en régimen. ~900 KB de código |

Lo que mostró, además de los números:

- **El costo de la instrumentación no es el problema; su memoria sí puede serlo.**
  14 ns por zona son despreciables (500 zonas por frame son 7 µs). Pero una
  configuración por defecto de Tracy, sin servidor, crece ~115 MB por minuto sin
  avisar, y el protocolo mide R4 justo así: sin GUI.
- **Las herramientas de GPU y la plataforma se cruzan.** RenderDoc necesita XWayland, y
  el primer píxel de `VK_EXT_present_timing` necesita Wayland. La capa de plataforma
  (ADR-0004) tiene que dejar elegir el driver de video al arrancar.
- **El determinismo entre compiladores está, el de entre máquinas no.** Las banderas
  del ADR-0008 ya dan bits iguales con dos compiladores. Lo que queda afuera es la libm.
- **Medir la GPU tiene trampas propias:** la etapa de un timestamp decide si se mide el
  trabajo o la espera, y en corridas iguales el p99 saltó de 90 µs a 3,2 ms. Una sola
  corrida no alcanza: por eso el ADR-0020 pide 5.

### Cómo lo hacen otros

- **Jason Gregory** (*Game Engine Architecture*, capítulo "Tools for Debugging and
  Development") describe el conjunto clásico: logging con canales y niveles, dibujo de
  depuración *immediate mode* con duración, menús de depuración dentro del juego, una
  consola, controles de tiempo (pausa, avance de a un frame, cámara lenta), estadísticas
  en pantalla y profilers con zonas jerárquicas.
- **Unreal Engine:** variables de consola (`TAutoConsoleVariable`, registradas en
  `IConsoleManager`) declaradas en el código,
  comandos `stat` en pantalla, `DrawDebugHelpers` para el dibujo de depuración y Unreal
  Insights como profiler con línea de tiempo.
- **Quake** (id Software, 1996) introdujo las *cvars*: variables con nombre, cambiables
  desde la consola y guardables en un archivo de configuración.
- **Overwatch** (Philip Orwig, GDC 2017): el sistema de *replay* que alimenta la *kill
  cam* y las jugadas destacadas también sirve para reproducir bugs.
- **NanoLog** (Yang, Park y Ousterhout, USENIX ATC 2018): logging con formato diferido.
  En el momento del log solo se copian los argumentos a un archivo binario; un programa
  aparte lo convierte a texto después.
- **Tracy** (Bartosz Taudul): profiler híbrido de instrumentación y muestreo, con zonas
  de CPU y GPU, memoria y valores. Su manual explica cómo consigue pocos ns por zona:
  `rdtsc` y una cola sin locks por hilo.
- **El determinismo de punto flotante:** Glenn Fiedler (2010) y Bruce Dawson (2013)
  coinciden en que es posible con el mismo binario, pero entre compiladores, banderas y
  bibliotecas hay que controlar la contracción a FMA, `-ffast-math` y las funciones
  trascendentales.

## Decisión

Elijo las herramientas estándar de la industria para medir y depurar: Dear ImGui para la
interfaz, Tracy y `perf` para el profiling, y RenderDoc y las validation layers para la
GPU. La mayoría de estas elecciones no necesitan una razón más profunda: son lo que se
usa, están probadas y son gratis. Lo que me importa es medir bien lo que construyo. Si
escribo mis propias herramientas de medición y quedan mal hechas, no van a medir el
engine correctamente, y no voy a saber si el error está en el engine o en la
herramienta. Además, crear demasiadas cosas desde cero es otro proyecto, y compite con
el corte de mallas.

Lo que sí escribo es lo que es propio de este engine y ninguna herramienta trae: la
frontera que esconde las dependencias, el registro por frame del protocolo, el dibujo de
depuración del corte, los valores ajustables y el *replay* determinista.

| # | Dimensión | Elección |
|---|---|---|
| 1 | Interfaz | **Dear ImGui, dibujado por el renderer propio.** Sin sus backends de SDL3 ni de Vulkan: el input llega desde `platform::` (ADR-0004), y sus listas de vértices se dibujan con un pipeline propio, el atlas de la fuente como textura y los datos por el *ring buffer* (ADR-0005). Sus asignaciones van a la región de depuración, con `ImGui::SetAllocatorFunctions` |
| 2 | CPU | **Tracy detrás de una macro de zona propia, con `TRACY_ON_DEMAND` siempre.** El engine no nombra a Tracy fuera de un solo módulo. **`perf`** para el muestreo |
| 3 | GPU | **Timestamps propios** para el registro por frame, **zonas de GPU de Tracy** para diagnosticar y **RenderDoc en XWayland**, con una tecla que dispara la captura por su API *in-app*. Las validation layers, como ya decidieron el ADR-0005 y el ADR-0006 |
| 4 | R1 a R4 | **Un registro por frame propio**, que no depende de Tracy, para el protocolo. Tracy, para entender un pico. **Los frames de recuperación se marcan y se reportan aparte, nunca se descartan** |
| 5 | Logging | **Cinco niveles** (error, aviso, info, debug, traza) y **una categoría por módulo**. Se formatea con `std::format_to_n` a un *ring buffer* de entradas fijas, que se vacía a un archivo una vez por frame y siempre ante un error fatal. Sin formato diferido y sin spdlog. Lo que no corresponde a cada preset se elimina al compilar |
| 6 | Dibujo de depuración | ***Immediate mode* propio**, con una duración opcional en pasos, en la **región de depuración**: ni en la arena del paso ni en la del frame |
| 7 | Valores ajustables | **Un registro de variables de consola (CVars), un archivo con los valores cambiados y un panel en ImGui.** Si cambia un valor que afecta la simulación mientras se graba, el cambio se graba como un evento de ese paso. **En Release, cada CVar es una constante** |
| 8 | *Replay* | **Input por paso**, un encabezado con la versión, el commit, el escenario, la semilla y los valores ajustables del inicio, y **un hash de control cada 60 pasos**. El hash se compara **solo en la misma máquina**. En los builds de desarrollo, un *ring buffer* graba siempre los últimos 5 minutos. Controles de tiempo: pausa, avance de un paso y cámara lenta |
| 9 | Memoria | **Contadores siempre activos** en cada región (uso, pico y fallos), como ya pide el ADR-0010. Un panel en los builds de desarrollo. El pico del proceso (`VmHWM`) y el uso de `VK_EXT_memory_budget` en el registro por frame. `TracyAllocN` solo para el pool de pedazos |
| 10 | Presets | **Una macro por herramienta**, con la combinación de cada preset fijada en `CMakePresets.json`. La macro de zona alimenta a Tracy en `relwithdebinfo` y a la traza del verificador del ADR-0030 en `debug` |

## Consecuencias

- **Qué tiene cada preset:**

  | Herramienta | `debug` | `relwithdebinfo` | `release` |
  |---|---|---|---|
  | Macro de zona | Alimenta la traza del ADR-0030 | Alimenta a Tracy, *on demand* | No existe |
  | Registro por frame | Sí | Sí | No |
  | Interfaz (ImGui) | Sí | Sí, oculta al medir | No |
  | Dibujo de depuración | Sí | Sí, apagado al medir | No |
  | Valores ajustables | Editables | Editables | Constantes |
  | Grabar y reproducir *replays* | Sí (los tests de *replay* corren aquí, ADR-0011) | Sí (los escenarios del protocolo) | No |
  | Niveles de log compilados | Los cinco | Hasta info | Error y aviso |
  | Contadores de memoria | Sí | Sí | Sí |
  | Validation layers | Sí (ADR-0006) | No | No |

  Release no compila ninguna herramienta, salvo los contadores de memoria y el log de
  errores y avisos: es el criterio 2. Los nombres de las macros (en mayúsculas,
  ADR-0013) se fijan con el código.
- **Las dos dependencias nuevas, Dear ImGui (MIT) y Tracy (BSD-3), son solo de
  desarrollo:** no se compilan en Release. Ninguna está en los repositorios oficiales de
  Arch, así que entran con `FetchContent` y una versión fijada (ADR-0006). La versión
  fijada importa además porque el cliente y el servidor de Tracy tienen que hablar la
  misma versión del protocolo. Las dos se agregan a `CREDITS.md` (ADR-0019) en el ticket
  que las suma al build.
- **Una frontera para cada dependencia:** solo un módulo incluye `imgui.h`, y solo uno
  incluye `Tracy.hpp`. El resto del engine usa la macro de zona propia y una interfaz
  propia para los paneles. Si un día se cambia Tracy por otra cosa, cambia un módulo. Dónde
  viven esos módulos dentro de las capas del ADR-0012 lo decide su ticket.
- **El servidor de Tracy se compila** desde el código de la misma versión, con CMake (unos
  6 minutos en esta máquina). `tracy-capture` graba sin GUI.
- **Una región nueva de memoria, la de depuración** (ADR-0010): su dueño son las
  herramientas, vive todo el programa y existe solo en `debug` y `relwithdebinfo`. Guarda
  el buffer del dibujo de depuración (que se vacía después del render), el *ring buffer*
  del log, las asignaciones de ImGui, el registro por frame y el *ring buffer* del
  *replay*. Como el protocolo mide en `relwithdebinfo`, esta región cuenta en R4 al medir.
  Su tamaño es una hipótesis (`presupuestos.md`) hasta medirla.
- **El protocolo mide con las herramientas compiladas, pero apagadas:** Tracy sin servidor
  cuesta 4,6 ns por zona y no guarda nada; la interfaz está oculta y el dibujo de
  depuración, apagado. Con `TRACY_ON_DEMAND` siempre activo, la acumulación sin tope del
  prototipo (640 MB por 10 M de zonas) no puede ocurrir.
- **`relwithdebinfo` se compila con `-fno-omit-frame-pointer`,** para que `perf` arme las
  pilas con *frame pointers* (+9 % al muestrear, contra +15 % y 46 MB con DWARF). Como el
  protocolo mide ese mismo preset, el costo del registro que se pierde queda dentro de la
  medición. **Fe de erratas del ADR-0006** (Decisión 7).
- **R3 se mide en Wayland, y RenderDoc corre en XWayland.** El primer píxel de
  `VK_EXT_present_timing` solo está en Wayland, y RenderDoc no expone la superficie de
  Wayland. SDL3 elige el driver al arrancar (`SDL_VIDEO_DRIVER=x11`), así que no hace
  falta nada nuevo en `platform::`. El registro por frame anota el driver con el que se
  midió. `VK_KHR_present_wait2` es el respaldo si no hay present timing.
- **Los tiempos de presentación llegan tarde:** `vkGetPastPresentationTimingEXT` devuelve
  frames ya presentados, varios frames después. El registro guarda cada frame con su
  *present id* y completa R3 cuando llega el reporte.
- **Los frames de recuperación** (resuelve un punto abierto del ADR-0020): el registro
  guarda, por frame, cuántos pasos corrió y cuánto tardó cada uno. Un frame con varios
  pasos después de un hueco en el que el engine no trabajó se marca como recuperación de
  un tirón externo. El resumen del protocolo reporta R1 con y sin esos frames, y cuántos
  hubo. **Fe de erratas del ADR-0020** (Abierto).
- **El log en el frame:** formatear cuesta ~450 ns por línea, así que el código de cada
  frame loguea poco. Vaciar el *ring buffer* al archivo es un costo por frame, al
  terminar el frame. Un error fatal vacía todo antes de abortar; se diseña junto con las
  macros de `assert` (ADR-0009).
- **El dibujo de depuración cruza los pasos:** la simulación escribe primitivas durante
  sus pasos (de 0 a 8 por frame), el render las dibuja una vez y la región se vacía
  después. Una primitiva con duración en pasos sobrevive a ese vaciado hasta que se le
  acaba la duración. Así, pausar la simulación no borra lo que se está mirando.
- **Las CVars y el determinismo:** cada CVar declara si afecta la simulación. El *replay*
  guarda todas en su encabezado y graba como evento cualquier cambio de una que afecta la
  simulación. Reproducirlo aplica los mismos cambios en los mismos pasos.
- **Las CVars en Release son constantes:** un valor ajustado en el archivo de cambios
  pasa al código (su valor por defecto) con un PR antes de valer en Release. Las
  opciones que ve el jugador (cámara, controles) no son CVars: su guardado lo decide el
  ADR-0028.
- **El hash entre máquinas** (resuelve el punto abierto del ADR-0016): por ahora, solo en
  la misma máquina. Con las banderas del ADR-0008, Clang y GCC en -O0, -O2 y -O3 dan los
  mismos bits; lo que no está garantizado es la libm de otra máquina. En la CI, un test de
  *replay* compara dos ejecuciones en el mismo proceso, como ya pide el ADR-0016. Las
  dos salidas para comparar contra un hash guardado (funciones trascendentales propias o
  una CI con la misma glibc) quedan medidas para el ADR-0029. **Fe de erratas del
  ADR-0016** (Abierto).
- **Si un día se agrega `-march`** (el ADR-0008 lo permite si el profiler lo pide), hay
  que agregar también `-ffp-contract=off`: con FMA, los *replays* grabados divergen desde
  el primer paso.
- **El input por paso** necesita una regla en el game loop: qué parte del input del frame
  ve cada paso (por ejemplo, el movimiento del mouse lo consume el primero o se reparte).
  El *replay* graba el resultado, sea cual sea la regla.
- **Un *replay* para cada bug:** el *ring buffer* de 5 minutos (~430 KB) se puede volcar
  a un archivo y adjuntar a un reporte de bug (ADR-0011). Así, un bug del corte se
  reproduce exacto, paso por paso.
- **La traza del ADR-0030 queda resuelta** (su punto abierto h): es la misma macro de
  zona, y se activa con una definición del preset `debug`, la primera de las tres salidas
  que listó ese ADR. Una sola instrumentación sirve para el profiling y para verificar
  los diagramas. **Fe de erratas del ADR-0030** (Consecuencias).
- **Lo que se gana:** herramientas que la industria usa y que un evaluador reconoce,
  mediciones en las que puedo confiar porque las herramientas no son mías, y el esfuerzo
  propio puesto donde el juego lo necesita: ver el corte por dentro y repetir una
  partida exacta.
- **Lo que se sacrifica:**
  - dos dependencias externas en los builds de desarrollo, y un servidor de Tracy que hay
    que compilar;
  - RenderDoc obliga a correr en XWayland;
  - el determinismo entre máquinas no está garantizado;
  - los *frame pointers* cuestan un registro en `relwithdebinfo`.
- **Abierto:**
  - los nombres y el prefijo de las macros, y dónde viven los módulos de ImGui y de Tracy
    (sus tickets);
  - el formato del archivo de CVars, sin una dependencia nueva (su ticket);
  - dónde viven en el repo los escenarios grabados del protocolo (su ticket);
  - la regla del input por paso (ticket del game loop);
  - con qué se calculan los percentiles y el *bootstrap* a partir del registro (su
    ticket o el ADR-0029);
  - el hash entre máquinas para la CI (ADR-0029);
  - el tamaño de la región de depuración, que se mide;
  - el editor de cápsulas del ADR-0017 y las demás herramientas de assets: tienen
    interfaz (ImGui), pero cada una necesita su ticket.

## Cómo lo defiendo

<!-- Lo escribe el autor: preguntas que haría un evaluador, respondidas con sus palabras. -->

## Referencias

- Nota de investigación: [`../research/herramientas-de-depuracion.md`](../research/herramientas-de-depuracion.md).
- ADR-0004, capa de plataforma: [`0004-capa-de-plataforma.md`](0004-capa-de-plataforma.md).
- ADR-0005, API gráfica: [`../graphics/adr/0005-api-grafica.md`](../graphics/adr/0005-api-grafica.md).
- ADR-0010, memoria: [`0010-estrategia-de-memoria.md`](0010-estrategia-de-memoria.md).
- ADR-0011, testing: [`0011-estrategia-de-testing.md`](0011-estrategia-de-testing.md).
- ADR-0016, física: [`../physics/adr/0016-fisica.md`](../physics/adr/0016-fisica.md).
- ADR-0020, rendimiento: [`0020-requisitos-y-presupuestos-de-rendimiento.md`](0020-requisitos-y-presupuestos-de-rendimiento.md).
- ADR-0030, diagramas: [`0030-diagramas-tecnicos.md`](0030-diagramas-tecnicos.md).
- Jason Gregory, *Game Engine Architecture*, 3.ª edición, CRC Press, 2018, capítulo
  "Tools for Debugging and Development".
- Bartosz Taudul, *Tracy Profiler* (manual y código, v0.14.1, BSD-3):
  <https://github.com/wolfpld/tracy>
- Omar Cornut, *Dear ImGui* (v1.93.0, MIT): <https://github.com/ocornut/imgui>
- rxi, *microui* (MIT): <https://github.com/rxi/microui>
- Casey Muratori, *Immediate-Mode Graphical User Interfaces* (video, 2005):
  <https://caseymuratori.com/blog_0001>
- Perfetto, visor de trazas: <https://perfetto.dev/docs/>
- Google, *Trace Event Format*:
  <https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU/>
- Brendan Gregg, ejemplos de `perf`: <https://www.brendangregg.com/perf.html>
- Firefox Profiler, importar desde `perf`:
  <https://github.com/firefox-devtools/profiler/blob/main/docs-user/guide-perf-profiling.md>
- RenderDoc, API *in-app*: <https://renderdoc.org/docs/in_application_api.html>
- Khronos, `VK_EXT_present_timing`:
  <https://registry.khronos.org/vulkan/specs/latest/man/html/VK_EXT_present_timing.html>
- Mesa, variables de entorno (`INTEL_MEASURE`): <https://docs.mesa3d.org/envvars.html>
- Unreal Engine, variables de consola:
  <https://dev.epicgames.com/documentation/en-us/unreal-engine/console-variables-cplusplus-in-unreal-engine>
- Philip Orwig, *Replay Technology in 'Overwatch': Kill Cam, Gameplay, and Highlights*,
  GDC 2017: <https://gdcvault.com/play/1024053/Replay-Technology-in-Overwatch-Kill>
- S. Yang, S. J. Park y J. Ousterhout, *NanoLog: A Nanosecond Scale Logging System*,
  USENIX ATC 2018: <https://www.usenix.org/conference/atc18/presentation/yang-stephen>
- Glenn Fiedler, *Floating Point Determinism* (2010):
  <https://gafferongames.com/post/floating_point_determinism/>
- Bruce Dawson, *Floating-Point Determinism* (2013):
  <https://randomascii.wordpress.com/2013/07/16/floating-point-determinism/>
- glibc 2.41, funciones correctamente redondeadas de CORE-MATH (2025):
  <https://gcc.gnu.org/pipermail/libc-alpha/2025-January/164378.html>
