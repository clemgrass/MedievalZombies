# ADR-0020: Requisitos mínimos y presupuestos de rendimiento

- **Estado:** Aceptado
- **Fecha:** 2026-10-19
- **Ticket:** #34
- **Reemplaza / Reemplazado por:** —
- **Fe de erratas:** 2026-10-28 (#43): el ADR-0021 decide dos puntos abiertos. El input
  se graba por paso de simulación, como pedía este ADR. Y los frames de recuperación
  después de un tirón externo se marcan en el registro por frame y se reportan aparte:
  R1 se da con y sin ellos, con su cantidad, y nunca se descartan. Se corrige en el
  protocolo (punto 7) y en Abierto.

## Contexto y problema

"Que corra bien" no se puede verificar: es una sensación, y una sensación no sirve para
decidir si un cambio empeoró el juego ni para defender el rendimiento frente a un
evaluador. Este ADR convierte esa sensación en **requisitos con número, con una forma
fija de medirlos y con una regla para cuando no se cumplen**.

Hay que decidir:

1. cuál es la **máquina de referencia** (la mínima y la objetivo);
2. la **resolución** y la **tasa de frames** objetivo;
3. **cómo se expresa y se mide** el rendimiento: qué métricas, con qué protocolo y con
   qué estadística;
4. cómo se **reparte el frame** entre input, simulación, corte, física, animación y
   render;
5. qué **presupuesto de memoria** tienen la CPU y la GPU;
6. qué **capacidad** se soporta: zombies, pedazos y cortes a la vez;
7. qué pasa cuando **se supera un presupuesto**.

Lo que ya está decidido y limita este ADR:

- **La visión** (`docs/vision/README.md`) propone los números del corte vertical:
  50 zombies en el pico de la horda, hasta 4 partidos por un tajo, unos 8 cortes de
  malla por segundo, unos 100 pedazos en el suelo y 60 fps en la UHD 620. Aquí se
  fijan.
- **ADR-0005:** Vulkan 1.3 sobre la UHD 620, con memoria compartida entre la CPU y la
  GPU.
- **ADR-0012:** paso fijo de 60 Hz con acumulador, un solo hilo al principio, colas de
  eventos y una arena por frame.
- **ADR-0014:** se mide antes de optimizar, y cada PR de rendimiento trae la medición de
  antes y de después.

**Fuera de alcance:** las herramientas de profiling (ADR-0021), el diseño de los
allocators (ADR-0010), dónde corren los benchmarks en CI (ADR-0029) y las técnicas de
render (ADR-0022). Este ADR fija los **números y el método**. Esos ADRs deciden **con
qué** se cumplen.

## Criterios

1. **Medible:** cada requisito tiene una unidad y una forma de medirlo.
2. **Verificable con profiling:** se puede comprobar con datos, repetidamente, en la
   máquina de referencia.
3. **Estadísticamente honesto:** una diferencia entre dos mediciones solo cuenta si es
   mayor que el ruido.
4. **Coherente** con la visión y con los ADR-0005, 0010 y 0012.
5. **Realista** en hardware modesto, incluido el límite térmico de un portátil.
6. **Valor de aprendizaje y de portafolio:** usar el vocabulario y los métodos que usa
   la industria y la investigación.

## Opciones

### 1. Cómo se expresa "corre bien"

| Opción | Qué significa | Problema |
|---|---|---|
| **Sensación en el playtest** | "Se siente fluido" | No se puede repetir ni comparar. Dos personas, o la misma persona otro día, opinan distinto |
| **FPS promedio** | "Corre a 70 fps" | Esconde los tirones: 59 frames de 10 ms y uno de 200 ms dan un promedio de 60 fps, y el jugador ve un congelamiento. Además, promediar fps es un error estadístico (ver Investigación) |
| **Requisitos cuantificados sobre el tiempo de frame** | Cada requisito tiene una escala, un medidor, un nivel obligatorio y una meta. Se reportan percentiles del tiempo de frame en ms, medidos con un protocolo fijo | Más trabajo: obliga a definir escenarios de prueba y un protocolo de medición |

### 2. La máquina mínima

| Opción | Qué significa | Problema |
|---|---|---|
| **Esta máquina es la mínima y la objetivo** | Si corre aquí, cumple. Todo se mide aquí | No se sabe cómo corre en máquinas más débiles |
| **Un mínimo más bajo que esta** | Por ejemplo, una GPU integrada más vieja | No hay otra máquina para medirlo. Un requisito que no se puede medir no es un requisito |
| **Esta como recomendada, sin mínimo declarado** | Se publica solo la recomendada | Deja abierta la pregunta que el ticket pide cerrar |

### 3. Resolución

| Opción | Escalado a la pantalla de 3200×1800 | Problema |
|---|---|---|
| **Nativa (3200×1800)** | 1× | Unas 25 veces más píxeles que 640×360. Inviable a 60 fps en la UHD 620, y contradice el estilo PSX |
| **Interna de 640×360** | **5× entero.** También es entero a 1280×720 (2×), 1920×1080 (3×), 2560×1440 (4×) y 3840×2160 (6×) | Pocos píxeles para la línea del tajo y las juntas de la armadura. Hay que verificar que se lea bien |
| **Interna de 960×540** | 3,33×, **no entero**: los píxeles quedan de tamaños distintos | El estilo PSX pide píxeles iguales. Cuesta 2,25 veces más que 640×360 |
| **Interna de 320×240** (PSX original) | No es entera a 16:9, y obliga a franjas negras | Demasiado poca resolución para apuntar con precisión |

En todas las opciones de resolución interna, la imagen final sale a la resolución
nativa de la pantalla. Cómo se escala (por ejemplo, sin filtro) lo decide el ADR-0022.

### 4. Dónde se fija el reparto del frame y de la memoria

| Opción | Qué significa | Problema |
|---|---|---|
| **Una tabla fija en este ADR** | Los milisegundos y los MiB de cada sistema quedan en el ADR | Hoy no existe código: los números son hipótesis. Con el ADR inmutable, cada ajuste exigiría un ADR nuevo |
| **El ADR fija el total y el método, y el reparto vive en un documento vivo** | El ADR fija el frame, la máquina, los requisitos y el protocolo. El reparto por sistema vive en `docs/performance/`, y cada cambio llega en un PR con su medición | Hay que mantener el documento al día |
| **Sin reparto por sistema** | Solo el total del frame | Cuando el frame se pasa, no se sabe qué sistema culpar. La industria reparte el frame justamente para eso |

### 5. Qué pasa al superar un presupuesto

| Opción | Qué significa | Problema |
|---|---|---|
| **Solo avisar** | Se registra y se arregla después | En el juego no cambia nada: la horda puede seguir creciendo hasta romper el frame |
| **Límites duros y degradación diseñada** | Cada recurso tiene un tope fijo, y al llegar a él el juego hace algo previsto (recicla el pedazo más viejo, deja un corte para el paso siguiente) | Hay que diseñar qué se degrada y verificar que no se note |
| **Escalado dinámico** | El engine baja la resolución o el detalle cuando el frame se pasa | Más complejo. A 640×360 el costo por píxel ya es bajo, así que bajar la resolución ayuda poco |

## Investigación

### La máquina de referencia, medida

Medido en la máquina del autor el 2026-10-19. Los números de este ADR salen de aquí, no
de la ficha comercial.

| Componente | Valor | Cómo se obtuvo |
|---|---|---|
| Equipo | Dell XPS 13 9360 (portátil de 13", 2017) | `/sys/class/dmi/id` |
| CPU | Intel Core i7-8550U: 4 núcleos y 8 hilos, 1,8 GHz base y 4,0 GHz turbo, L3 de 8 MiB | `lscpu` |
| Límite de potencia | **17 W sostenidos** (PL1) y 51 W en ráfagas cortas (PL2), **compartidos entre la CPU y la GPU** | `intel-rapl` (powercap) |
| CPU bajo carga sostenida | Con los 8 hilos ocupados, la frecuencia media cae de 2,8 a **~2,1 GHz en 40 s**, con el paquete a 98 °C | 8 bucles ocupados y lectura de `scaling_cur_freq` cada pocos segundos |
| RAM | 8 GB LPDDR3-1867 soldada, en dos canales (7,5 GiB visibles), compartida con la GPU | `free` y `dmidecode -t memory` |
| Ancho de banda de memoria | **~16 GB/s** con un hilo (5 corridas, de 15,9 a 16,4 GB/s), un 54 % del máximo teórico de 29,9 GB/s (1867 MT/s × 8 B × 2 canales) | Kernel *triad* al estilo STREAM, con arrays de 64 MiB |
| GPU | Intel UHD 620 (Kaby Lake GT2), de 300 a 1150 MHz | `lspci` y `sysfs` |
| Driver | Mesa 26.2.2 (ANV), Vulkan 1.4.354 | `vulkaninfo` |
| Memoria de la GPU | Un solo heap de 5,61 GiB, que es la misma RAM del sistema | `vulkaninfo` (ADR-0005) |
| Medición en la GPU | *Timestamps* en gráficos y cómputo (resolución de 83 ns), `VK_EXT_memory_budget`, `VK_KHR_present_wait`, `VK_EXT_present_timing` y *calibrated timestamps* | `vulkaninfo` |
| Pantalla | 3200×1800 a 59,98 Hz (escala 2 en el escritorio) | `hyprctl monitors` |

**La medición de carga sostenida no siguió un protocolo:** había otras aplicaciones
abiertas y el portátil ya estaba caliente. Sirve como indicio, no como dato final. Es
justo el problema que tiene que resolver la forma de medir de este ADR.

Lo que dicen estos datos:

- **La pantalla es de 60 Hz.** Más de 60 fps no se ven en esta máquina: el frame dura
  **16,67 ms**, y el tiempo que sobra sirve para la estabilidad, no para más fps.
- **El turbo de 4,0 GHz no es el número de diseño.** Bajo carga sostenida, la máquina
  baja casi a la mitad, así que los presupuestos tienen que cumplirse **en estado
  térmico estable**, después de calentar.
- **La CPU y la GPU se reparten 17 W.** Cada milisegundo de CPU que sobra deja más
  potencia para la GPU, y al revés. En una GPU integrada, **ahorrar CPU también acelera
  el render**.
- **El ancho de banda es el recurso escaso.** Unos 16 GB/s, divididos por 60 frames,
  son ~270 MB por frame para la CPU y la GPU juntas. Una sola pasada a pantalla completa
  en RGBA8 (leer y escribir) mueve 46 MB a 3200×1800, pero solo 1,8 MB a 640×360. Esto
  respalda con un número la resolución interna baja de la visión.

### Marcos formales para el rendimiento

No existe un estándar único de "rendimiento de videojuegos", pero sí piezas formales que
se combinan bien:

- **ISO/IEC 25010:2023 (modelo de calidad de producto).** Define la *eficiencia de
  desempeño* con tres subcaracterísticas: **comportamiento temporal** (tiempo de
  respuesta y tasa), **uso de recursos** (CPU, memoria, energía) y **capacidad**
  (límites máximos). Son exactamente los tres tipos de presupuesto de este ADR: el
  frame, la memoria y la cantidad de zombies y pedazos.
- **Planguage (Tom Gilb, *Competitive Engineering*, 2005).** Es una forma de escribir
  requisitos de calidad sin ambigüedad. Cada requisito tiene una **escala** (la unidad y
  qué se cuenta), un **medidor** (cómo se mide), un nivel **obligatorio** (*Must*: bajo
  esto, falla), una **meta** (*Plan*) y un **deseo** (*Wish*). Le quita la ambigüedad a
  "corre a 60".
- **Software Performance Engineering (Connie Smith y Lloyd Williams, *Performance
  Solutions*, 2001).** La disciplina de fijar objetivos de rendimiento **antes** de
  escribir el código, repartirlos entre los componentes y validarlos con mediciones a lo
  largo del desarrollo. Es el origen de la idea de un presupuesto por sistema.
- **Raj Jain, *The Art of Computer Systems Performance Analysis* (1991).** El libro
  clásico. Propone un método sistemático: fijar los objetivos, elegir las métricas, la
  carga de trabajo y los factores que varían, diseñar el experimento, analizar y
  presentar. Incluye un catálogo de errores comunes, como los benchmarks engañosos y los
  promedios de razones.
- **Hoefler y Belli, *Scientific Benchmarking of Parallel Computing Systems* (SC15).**
  Doce reglas para reportar rendimiento. Las que aplican aquí:
  - regla 3: la media aritmética solo para **costos** (ms) y la armónica para **tasas**
    (fps). **El "fps promedio" de un reporte suele ser el error de promediar tasas**;
  - reglas 5 y 6: si los datos no son deterministas, reportar intervalos de confianza,
    sin suponer una distribución normal;
  - regla 7: comparar dos versiones con un método estadístico, por ejemplo intervalos
    de confianza que no se superponen;
  - regla 8: para el peor caso, usar **percentiles**, no la media;
  - regla 9: documentar todo el entorno (hardware, software, versiones) para que se
    pueda reproducir.
- **Mytkowicz y otros, *Producing Wrong Data Without Doing Anything Obviously Wrong!*
  (ASPLOS 2009).** Cambios inocentes, como el tamaño de las variables de entorno o el
  orden de enlazado, mueven el resultado lo suficiente para sacar conclusiones falsas.
  Una mejora de pocos puntos porcentuales no es real hasta que se demuestre por encima
  de ese ruido.
- **Kalibera y Jones, *Rigorous Benchmarking in Reasonable Time* (ISMM 2013).** Las
  repeticiones se ponen donde nace la variación: entre ejecuciones, entre iteraciones o
  entre builds. Antes de medir hay un calentamiento que se descarta.
- **El método USE (Brendan Gregg).** Para cada recurso (CPU, GPU, ancho de banda,
  memoria), revisar su **utilización**, su **saturación** y sus **errores**. Sirve para
  diagnosticar *por qué* no se cumple un presupuesto.
- **SLI, SLO y presupuesto de error (Google, *Site Reliability Engineering*, cap. 4).**
  Es el mismo patrón en otra industria: un **indicador** medible (SLI), un **objetivo**
  sobre ese indicador (SLO) y un margen de fallas permitido. Por ejemplo, "el 99 % de los
  frames bajo 16,7 ms" es un SLO, y el 1 % restante es el presupuesto de error.

### Cómo mide la industria de videojuegos

- **El tiempo de frame, no los fps.** Desde *Inside the second* (Scott Wasson, The Tech
  Report, 2011), la prensa técnica y los fabricantes miden el tiempo de cada frame y
  reportan el **percentil 99** y el **tiempo acumulado sobre el límite** (por ejemplo,
  cuántos ms se pasaron de 16,7). NVIDIA lo extendió a más percentiles para estudiar los
  tirones.
- **Un frame bien calculado puede verse mal presentado.** Alen Ladavac (Croteam, GDC
  2018, *The Elusive Frame Timing*) muestra que un juego a 60 fps puede tener
  micro-tirones si los frames no llegan a la pantalla a intervalos parejos. Por eso
  también se mide **cuándo se presentó** cada frame. Vulkan lo estandarizó en
  `VK_EXT_present_timing` (Vulkan 1.4.335, noviembre de 2025), que Mesa ya trae en esta
  máquina.
- **La CPU y la GPU trabajan en paralelo.** Con frames en vuelo, la CPU prepara el
  frame N+1 mientras la GPU dibuja el N. Por eso cada una tiene **su propio presupuesto
  de 16,7 ms**: no se suman.
- **Margen térmico.** Unity recomienda usar solo un ~65 % del frame en móviles, para
  que el chip no se caliente y baje su frecuencia. Un portátil de 15 W sin un buen
  ventilador está entre un móvil y un escritorio.
- **Las GPU integradas de Intel** comparten el límite de potencia del paquete con la
  CPU. El driver y el firmware reparten esa potencia según la carga (guía de
  rendimiento de Intel para las GPU Gen9).
- **Escenarios repetibles.** Los estudios miden con recorridos grabados o con partidas
  reproducidas, no jugando a mano, para que dos mediciones sean comparables. El paso
  fijo del ADR-0012 hace posible reproducir una partida exacta con el input grabado.
- **El frame repartido por sistema.** Es la práctica habitual para saber a quién culpar
  cuando el frame se pasa, y viene directamente de la Software Performance Engineering.

### Cuánto pesa la geometría (estimación)

Para dimensionar la memoria sin código, con zombies low poly de unos 3000 vértices de
32 bytes:

| Qué | Cálculo | Tamaño |
|---|---|---|
| Un zombie | 3000 × 32 B | ~96 KB |
| 50 zombies | 50 × 96 KB | ~4,8 MB |
| 100 pedazos | 100 × ~2000 vértices × 32 B | ~6,4 MB |
| Geometría nueva de un tajo con 4 cortes | 4 × ~200 KB | ~0,8 MB por frame |

La geometría es chica frente a la RAM. **El costo está en el tiempo de cálculo del corte
y en el ancho de banda, no en el espacio.**

## Decisión

Lo más importante es que el juego corra bien en esta máquina, y que "corre bien" salga
de datos y no de una sensación. Por eso cada requisito tiene un número y una forma fija
de medirlo, y la resolución la elegí comparándolas en un prototipo: 640×360 se ve bien,
escala entera a todas las pantallas comunes y es la más barata.

| # | Dimensión | Elección |
|---|---|---|
| 1 | Cómo se expresa | **Requisitos cuantificados** en las tres categorías de la ISO/IEC 25010 (tiempo, recursos y capacidad), escritos al estilo Planguage: escala, medidor, nivel obligatorio y meta. El tiempo de frame se reporta en ms y con percentiles, nunca en fps promedio |
| 2 | Máquina | **El Dell XPS 13 9360 de la Investigación es la mínima y la objetivo.** Todo se mide ahí |
| 3 | Tasa y resolución | **60 fps sincronizados con la pantalla.** La salida va a la resolución nativa, con una **resolución interna de 640×360** escalada por un factor entero (5× en esta pantalla). Cómo se escala lo decide el ADR-0022 |
| 4 | Requisitos | Los de la tabla de abajo |
| 5 | Reparto | El ADR fija los totales y el método. **El reparto por sistema vive en [`docs/performance/presupuestos.md`](../performance/presupuestos.md)**, y cada cambio llega en un PR con su medición |
| 6 | Al superarlo | **Límites duros y degradación diseñada** en el juego. En desarrollo, incumplir un nivel obligatorio es un bug y no cumplir una meta, un ticket |
| 7 | Protocolo | El de abajo, igual para todas las mediciones |

### Requisitos

**Medidor común:** el protocolo de medición, en los escenarios del corte vertical.

| # | Requisito | Escala | Obligatorio | Meta |
|---|---|---|---|---|
| R1 | Tiempo de CPU por frame | ms de trabajo de la CPU por frame, sin contar las esperas a la GPU ni a la presentación | p99 ≤ 16,7 ms, y ningún frame sobre 33,3 ms | p99 ≤ 13,3 ms (80 % del frame) |
| R2 | Tiempo de GPU por frame | ms entre los *timestamps* de inicio y fin del frame en la GPU | p99 ≤ 16,7 ms, y ningún frame sobre 33,3 ms | p99 ≤ 13,3 ms |
| R3 | Ritmo de presentación | % de frames presentados más de un refresco después del anterior | ≤ 1 % | ≤ 0,1 % |
| R4 | Memoria total | Pico del proceso: RAM anónima residente (`RssAnon`) más el `heapUsage` de `VK_EXT_memory_budget`. En la GPU integrada, la memoria de Vulkan mapeada también aparece en el RSS completo, así que sumar los dos la contaría dos veces | ≤ 1,5 GiB | ≤ 1 GiB |
| R5 | Capacidad | Zombies activos, pedazos y cortes, cumpliendo R1 a R3 | 50 zombies, 100 pedazos, 4 cortes en un paso de simulación y 8 cortes por segundo | Conocer el punto de quiebre: subir la carga hasta que falle R1 o R2, y anotarlo |

El 20 % de margen de la meta absorbe la caída térmica, el sistema operativo y los
picos que todavía no se conocen.

### Al superar un presupuesto

- **En el juego, límites duros diseñados:**
  - los pedazos tienen un tope fijo, y al llegar a él desaparece el más viejo;
  - los cortes tienen una cuota **por paso de simulación**, y los que sobran se procesan
    en los pasos siguientes. La cuota cuenta pasos y no frames, porque un frame puede
    tener de cero a ocho pasos según el reloj, y así la misma partida grabada da siempre
    los mismos cortes;
  - la simulación ya tiene su tope de 8 pasos por frame (ADR-0012).
- **En desarrollo:**
  - un frame que se pasa se ve en pantalla;
  - **incumplir un nivel obligatorio es un bug de rendimiento** en el Project #2. Sigue
    el camino de los bugs *sin test automático* del ADR-0011: en "por qué no tiene
    test" se nombran el escenario y el requisito incumplido, y se adjunta la medición.
    La severidad sale de la escala del ADR-0011, y el bug se cierra como `FIXED` solo
    con una medición nueva que cumple;
  - no cumplir una meta es un ticket o `deuda-técnica`.
- **Nunca se aborta el juego por tiempo.**

### Protocolo de medición

1. **Build:** el preset `relwithdebinfo` del ADR-0006 (optimizado y con símbolos),
   **sin las validation layers** de Vulkan. Debug, con ASan, UBSan y validation layers,
   no sirve para medir.
2. **Condiciones fijas:** enchufado, perfil de energía en rendimiento, pantalla completa
   a la resolución nativa y nada pesado abierto.
3. **Entorno registrado** en cada medición: commit, preset, compilador, kernel, Mesa,
   temperatura y frecuencias.
4. **Escenarios deterministas:** partidas grabadas con su input y versionadas en el
   repo, como "pico de la horda", "tajo cuádruple" o "100 pedazos".
5. **Calentamiento:** unos 2 minutos para llegar al estado térmico estable. Esos frames
   se descartan.
6. **Repeticiones:** al menos 5 ejecuciones por escenario.
7. **Qué se reporta:** p50, p99, p99,9 y el máximo de R1 y R2, más R3 y R4, por
   ejecución. El resumen es la mediana entre ejecuciones, con un intervalo de confianza
   del 95 % calculado con *bootstrap*, sin suponer normalidad. R1 se reporta con y sin
   los frames de recuperación de un tirón externo, y se dice cuántos hubo (ADR-0021).
8. **Cómo se compara:** un cambio mejora o empeora solo si los intervalos no se
   superponen.
9. **Dónde:** en la máquina de referencia.

## Consecuencias

- **"Corre bien" deja de ser una opinión.** Un PR de rendimiento (ADR-0014) trae las
  dos mediciones con el protocolo, y la comparación se decide con los intervalos.
- **Hacen falta escenarios grabados desde temprano.** El paso fijo del ADR-0012 los
  permite, pero hay que construir la grabación y la reproducción del input. Es el mismo
  mecanismo que los tests de *replay* del ADR-0011, así que se diseña una sola vez y
  sirve a los dos. Cómo se construye lo decide el ADR-0021.
- **El reparto distingue lo que se paga por paso de lo que se paga por frame.** La
  pantalla va a 59,98 Hz y la simulación a 60 Hz, así que el acumulador del ADR-0012
  corre dos pasos en un frame cada ~50 s, y también después de cualquier tirón. Si un
  frame con dos pasos no cabe en 16,7 ms, el siguiente también tiene dos, y el juego
  queda pegado a 30 fps. Por eso el documento vivo exige que **dos pasos de simulación
  más el resto del frame quepan en 16,7 ms**. Después de un tirón puede haber más pasos
  (hasta 8, ADR-0012). El loop se recupera solo mientras **un paso cueste menos que su
  duración** (6,0 ms contra 16,7 ms): 8 pasos, después 4, después 2, y vuelve a 1.
- **Hay que instrumentar el engine desde el principio:** tiempo de CPU por sistema,
  *timestamps* de la GPU, tiempos de presentación (`VK_EXT_present_timing` o
  `VK_KHR_present_wait`) y memoria (`VK_EXT_memory_budget`). Cómo se instrumenta lo
  decide el ADR-0021.
- **Los números del documento vivo son hipótesis** hasta que se midan. Se corrigen con
  el primer spike de cada sistema. Como los spikes no se mergean (ADR-0014), su medición
  queda en el issue, y un PR `docs:` aparte la pasa al documento vivo.
- **Lo que se sacrifica:**
  - no se sabe cómo corre en máquinas más débiles que esta;
  - más de 60 fps no es un objetivo;
  - la resolución interna limita el detalle de lejos. El campo de visión al apuntar
    (30°) compensa en parte, porque agranda lo que se apunta.
- **El escalado final cuesta más ancho de banda que la escena:** escribir 3200×1800
  son unos 23 MB por frame, contra 1,8 MB de una pasada a 640×360. Cómo reducirlo lo
  decide el ADR-0022.
- **Los cortes en cola** abren una opción para el ADR-0007 y el ADR-0018: la pausa de
  impacto (de 50 a 80 ms, de 3 a 5 pasos de simulación) es una ventana natural para
  repartir varios cortes sin que se note.
- **Los cortes pendientes no pueden vivir en la arena por frame,** porque se borra al
  terminar el frame (ADR-0012). Necesitan un almacenamiento propio del sistema de corte,
  con un tope, que se diseña con el ADR-0010.
- **Abierto:**
  - **la grabación guarda el input por paso de simulación, no por frame** (decidido en
    el ADR-0021).
    El ADR-0012 lee el input una vez por frame, antes de los pasos. Si se grabara por
    frame, la cantidad de pasos de cada frame cambiaría entre ejecuciones, y la
    reproducción dejaría de dar los mismos cortes;
  - cómo cuentan para R1 los frames de recuperación después de un tirón externo: con 5
    pasos o más pasan de 33,3 ms (decidido en el ADR-0021: se marcan y se reportan
    aparte, nunca se descartan);
  - el ruido del vsync cerca de 59,98 Hz, que puede hacer alternar frames de 0 y 2 pasos
    durante un rato. Tyler Glaiel describe cómo evitarlo ajustando el `dt` al período
    del refresco (ticket del game loop);
  - qué se puede automatizar en CI, donde los runners compartidos no sirven para medir
    tiempo (ADR-0029);
  - cómo se implementan las arenas, los pools y los ring buffers del presupuesto de
    memoria (ADR-0010).

## Cómo lo defiendo

**¿Por qué no reportas el "fps promedio", que es lo que reporta todo el mundo?**
Porque el promedio puede esconder los problemas que el jugador sí ve, como los
congelamientos o los picos de rendimiento. El rendimiento se reporta con percentiles y
porcentajes, no con promedios.

**¿Por qué el percentil 99 y no la media? ¿Y para qué exiges además que ningún frame
pase de 33,3 ms?**
La media tiene el mismo problema que el promedio de fps. Lo que importa no es el valor
medio, sino que no haya picos y que el juego se sienta fluido. El percentil 99 mide esa
fluidez y esa estabilidad, porque deja a la vista los congelamientos y los tirones.

Pero el p99 deja pasar un 1 % de frames malos (unos 36 por minuto a 60 fps), y no dice
qué tan malos son: un congelamiento de medio segundo cada pocos minutos casi no lo
mueve. El tope de 33,3 ms atrapa justo eso. Un frame más largo pierde dos refrescos
seguidos, como si por un instante el juego corriera a menos de 30 fps, y el jugador lo
nota como un tirón molesto.

**Tu frame dura 16,7 ms. ¿Por qué la meta es 13,3 ms y no 16,7?**
Para tener un margen de seguridad. Medimos que esta máquina baja casi a la mitad su
frecuencia con el calor, y además están el sistema operativo y los picos que todavía no
conocemos. Ese margen también se podría usar para bajar el input lag, por ejemplo
leyendo el input lo más tarde posible.

**Tienes un solo frame de 16,7 ms. ¿Cómo es que la CPU y la GPU tienen 16,7 ms cada una,
en vez de repartírselo?**
Porque trabajan en paralelo: mientras la CPU prepara un frame, la GPU dibuja el
anterior. Pero en esta máquina no son del todo independientes: comparten los 17 W y el
ancho de banda, así que si la CPU trabaja más, la GPU baja su frecuencia.

**Optimizaste algo: antes, el p99 era 12,1 ms y después es 11,8 ms. ¿Es una mejora?**
No necesariamente. Una sola medición podría ser ruido: solo es una mejora si se observa
repetidamente y se mantiene constante. Además, las dos versiones ya están bajo lo que
necesitamos, y la diferencia es de solo 0,3 ms.

**¿Por qué 640×360 si 800×450 también escala entero en tu pantalla?**
Porque 640×360 escala bien en todas las pantallas 16:9 tradicionales, y me gustó su
estilo visual.

**¿No es arriesgado que la máquina mínima sea tu propio portátil?**
No. Es una máquina con gráficos integrados y casi 9 años, así que es un buen mínimo de
rendimiento. Además, es el mínimo que puedo medir: no sé cómo corre en algo más débil,
pero un requisito que no se puede medir no es un requisito.

## Referencias

- ISO/IEC 25010:2023, *Systems and software Quality Requirements and Evaluation
  (SQuaRE): Product quality model*. Resumen de la eficiencia de desempeño:
  <https://iso25000.com/index.php/en/iso-25000-standards/iso-25010/59-performance-efficiency>
- Tom Gilb, *Competitive Engineering* (2005). Introducción a Planguage:
  <https://www.modernanalyst.com/Resources/Articles/tabid/115/ID/2926/Specifying-Quality-Requirements-With-Planguage.aspx>
- Connie U. Smith y Lloyd G. Williams, *Performance Solutions: A Practical Guide to
  Creating Responsive, Scalable Software*, Addison-Wesley (2001).
- Raj Jain, *The Art of Computer Systems Performance Analysis*, Wiley (1991).
- Torsten Hoefler y Roberto Belli, *Scientific Benchmarking of Parallel Computing
  Systems: Twelve Ways to Tell the Masses when Reporting Performance Results*, SC15:
  <https://htor.inf.ethz.ch/publications/img/hoefler-scientific-benchmarking.pdf>
- Todd Mytkowicz, Amer Diwan, Matthias Hauswirth y Peter F. Sweeney, *Producing Wrong
  Data Without Doing Anything Obviously Wrong!*, ASPLOS 2009:
  <https://dl.acm.org/doi/10.1145/1508244.1508275>
- Tomas Kalibera y Richard Jones, *Rigorous Benchmarking in Reasonable Time*, ISMM 2013:
  <https://kar.kent.ac.uk/33611/>
- Brendan Gregg, *The USE Method*: <https://www.brendangregg.com/usemethod.html>
- Betsy Beyer y otros, *Site Reliability Engineering*, cap. 4, *Service Level
  Objectives*: <https://sre.google/sre-book/service-level-objectives/>
- Scott Wasson, *Inside the second: A new look at game benchmarking*, The Tech Report
  (2011).
- NVIDIA, *Analysing Stutter – Mining More from Percentiles*:
  <https://developer.nvidia.com/content/analysing-stutter-%E2%80%93-mining-more-percentiles-0>
- Alen Ladavac, *The Elusive Frame Timing: A Case Study for Smoothness Over Speed*,
  GDC 2018: <https://gdcvault.com/browse/gdc-18/play/1025031>
- Khronos, *VK_EXT_present_timing: the Journey to State-of-the-Art Frame Pacing in
  Vulkan*:
  <https://www.khronos.org/blog/vk-ext-present-timing-the-journey-to-state-of-the-art-frame-pacing-in-vulkan>
- Unity, *Optimize your mobile game performance* (presupuesto del 65 %):
  <https://unity.com/blog/games/optimize-your-mobile-game-performance-tips-on-profiling-memory-and-code-architecture-from>
- Intel, *Graphics API Performance Guide for Intel Processor Graphics Gen9*:
  <https://www.intel.com/content/www/us/en/content-details/671201/graphics-api-performance-guide-for-intel-processor-graphics-gen9.html>
- John McCalpin, *STREAM: Sustainable Memory Bandwidth in High Performance Computers*:
  <https://www.cs.virginia.edu/stream/>
- Tyler Glaiel, *How to make your game run at 60fps* (2018):
  <https://medium.com/@tglaiel/how-to-make-your-game-run-at-60fps-24c61210fe75>
