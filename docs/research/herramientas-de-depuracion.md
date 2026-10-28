# Mediciones de las herramientas de depuración

Nota de investigación del [ADR-0021](../adr/0021-herramientas-de-depuracion.md). Guarda
las mediciones que sostienen la decisión, para poder repetirlas.

Fecha: 2026-10-23, en la máquina de referencia del ADR-0020 (Dell XPS 13 9360,
i7-8550U, UHD 620, 8 GB de RAM). Son mediciones de laboratorio para decidir, no del
protocolo del ADR-0020: no hubo calentamiento térmico y había otras aplicaciones
abiertas. Los programas son código descartable, fuera del repo.

| Qué | Versión |
|---|---|
| Linux / glibc | 7.2.8 / 2.44 |
| Clang / GCC | 23.1.3 / 16.2.1, con `-O2` salvo que se diga otra cosa |
| Mesa (ANV) | 26.2.5 |
| Sesión | Wayland (Hyprland), con XWayland disponible |
| SDL3 | 3.4.18 |
| Vulkan-Headers / validation layers | 1.4.363 / 1.4.363 |
| Tracy | 0.14.1 |
| Dear ImGui | 1.93.0 |
| RenderDoc | 1.46 |
| perf | 7.2.11 |

## 1. Costo de una zona de profiling (P1)

**Qué se midió:** un loop de 2 millones de iteraciones con un trabajo chico (~35 ns),
con y sin una zona alrededor. El costo de la zona es la diferencia, dividida por las
iteraciones. Cada variante corre 5 veces y se toma la mejor.

- **Zona propia con `rdtsc`:** lee el contador de ciclos al entrar y al salir, y escribe
  un evento de 24 B (id, inicio, fin) en un arreglo ya tocado.
- **Zona propia con `clock_gettime(CLOCK_MONOTONIC)`:** lo mismo, con el reloj del
  sistema (por vDSO, sin entrar al kernel).
- **Tracy** (`ZoneScoped`), en tres situaciones.

| Variante | Costo por zona | Memoria |
|---|---|---|
| Propia, `rdtsc` | **4–8 ns** | 24 B por zona, en el arreglo propio |
| Propia, `clock_gettime` | 26–33 ns | Igual |
| Tracy, con `TRACY_ON_DEMAND` y sin servidor | **4,6 ns** | No guarda nada |
| Tracy, con el servidor conectado (`tracy-capture`, 4 corridas) | **10–14 ns** | Cliente: 60 MB de pico. Traza en disco: 12 MB para 10 M de zonas (~1,2 B por zona) |
| Tracy, sin `TRACY_ON_DEMAND` y sin servidor | 40 ns | **640 MB para 10 M de zonas** (~64 B por zona), sin tope |

El manual de Tracy da 2,25 ns por zona en su benchmark, con otra CPU y otro trabajo.
Aquí costó entre 2 y 6 veces más, y aun así es poco: 500 zonas por frame a 14 ns son
7 µs, el 0,04 % de 16,7 ms.

**Lo que importa del caso sin servidor:** Tracy, por defecto, empieza a grabar al
arrancar el programa y guarda todo en memoria hasta que se conecta un servidor. Con
500 zonas por frame a 60 fps son ~1,9 MB por segundo, y ~115 MB por minuto. Una medición
del protocolo del ADR-0020 sin GUI, con Tracy compilado así, ensucia R4 y nunca libera
esa memoria. Con `TRACY_ON_DEMAND`, solo graba mientras hay un servidor conectado.

Sobre `rdtsc`: la CPU tiene `constant_tsc` y `nonstop_tsc`, así que el contador avanza a
frecuencia fija aunque la CPU cambie de frecuencia o duerma. Tracy usa el mismo
mecanismo.

**El servidor de Tracy:** no está en los repositorios oficiales de Arch. `tracy-capture`
(la captura sin GUI) se compiló desde el código con CMake en ~6 minutos; CMake descargó
sus dependencias durante la configuración. El cliente y el servidor tienen que ser de la
misma versión del protocolo.

**El formato de Chrome (*Trace Event Format*):** una zona propia exportada como evento
JSON completo (`{"name":…,"ph":"X","ts":…,"dur":…,"pid":…,"tid":…}`) ocupa unos 60–70 B.
Es una estimación, no se midió: 10 M de zonas serían ~650 MB de JSON, contra 12 MB de
la traza comprimida de Tracy. Perfetto también lee un formato binario (protobuf), más
chico.

## 2. Muestreo con `perf` (P2)

`kernel.perf_event_paranoid` vale 2 en esta máquina. Con ese valor, un usuario sin
privilegios puede medir sus propios procesos, pero solo en espacio de usuario (sin
muestras del kernel). Alcanzó para todo lo que sigue.

**Contadores** (`perf stat -e cycles:u,instructions:u,cache-misses:u,branch-misses:u`)
sobre la simulación de P5: funcionan sin privilegios.

**Muestreo** (`perf record`) sobre el programa de P4 (5,3 s sin `perf`), compilado con
`-g -fno-omit-frame-pointer`:

| Configuración | Tiempo | Archivo |
|---|---|---|
| Sin `perf` | 5,32–5,38 s | — |
| `-F 999 --call-graph fp` | 5,83 s (+9 %) | 656 KB |
| `-F 4000 --call-graph fp` | 5,88 s (+10 %) | 2,7 MB |
| `-F 999 --call-graph dwarf` | 6,17 s (+15 %) | 46 MB |

- **Pilas con *frame pointers*** (`fp`): baratas, pero solo sirven si todo el código se
  compila con `-fno-omit-frame-pointer`. Las de DWARF no lo necesitan, pero copian un
  trozo de la pila en cada muestra (de ahí los 46 MB).
- **Símbolos de las bibliotecas del sistema:** sin ellos, el 97 % del tiempo de P4 quedó
  como direcciones en `libc.so.6` y `libstdc++.so`. Arch los sirve por *debuginfod*
  (`DEBUGINFOD_URLS` ya está configurado), así que se descargan al resolverlos.
- **Las GUIs:** Hotspot no está en los repositorios oficiales de Arch. `perf report`
  trae una interfaz de terminal, y el Firefox Profiler importa la salida de
  `perf script`. Ninguna de las dos se probó aquí.

El muestreo no cuenta frames: dice dónde se va el tiempo en promedio, pero no por qué un
frame en particular tardó 30 ms. Para eso hacen falta zonas con tiempos (sección 1).

## 3. La GPU, la presentación y la memoria (P3)

**Qué se midió:** un programa con SDL3 y Vulkan 1.3 abre una ventana de 640×360 y
presenta 600 frames en FIFO. Cada frame solo limpia la imagen. Se descartan los
primeros 60 frames.

**Lo que expone el driver:**

| Qué | Valor |
|---|---|
| `timestampPeriod` | 83,33 ns (un contador de 12 MHz) |
| Bits válidos de timestamp en la cola | 36: el contador da la vuelta cada ~95 minutos |
| `VK_KHR_calibrated_timestamps` | Sí. Desviación entre el reloj de la GPU y `CLOCK_MONOTONIC`: p50 2,1–2,5 µs, p99 3,2–4,3 µs (100 lecturas) |
| `VK_EXT_present_timing` en Wayland | Sí. Etapas: fin de la cola y **primer píxel enviado** (`IMAGE_FIRST_PIXEL_OUT`). Presentar a un tiempo absoluto: sí; relativo: no |
| `VK_EXT_present_timing` en XWayland | Sí, pero sin la etapa de primer píxel: solo fin de la cola y pedido tomado (`REQUEST_DEQUEUED`). Presentar a un tiempo relativo: sí; absoluto: no |
| `VK_KHR_present_id2` y `VK_KHR_present_wait2` | Sí, en las dos |
| `VK_EXT_memory_budget` | Sí |

**Tiempos medidos en Wayland:**

| Qué | p50 | p99 | Máximo |
|---|---|---|---|
| Período de refresco que reporta el swapchain | 16,6717 ms (59,98 Hz, como dice el ADR-0020) | | |
| Entre primeros píxeles consecutivos (present timing) | 16,672–16,673 ms | 16,680 ms | 16,681 ms |
| Intervalo entre frames en la CPU | 16,670 ms | 17,4–24,5 ms | 20,2–42,0 ms |
| GPU: limpiar la imagen (timestamps) | 58–70 µs | 90 µs a 3,2 ms, según la corrida | hasta 3,9 ms |

- **La presentación es mucho más estable que el reloj de la CPU.** El primer píxel sale
  cada 16,672 ms con 8 µs de diferencia entre p50 y p99, mientras que la CPU ve
  intervalos de hasta 24 ms. Por eso el ADR-0020 mide R3 con los tiempos de
  presentación y no con el reloj de la CPU.
- **Huecos:** en una corrida hubo un hueco de 1,37 s entre dos presentaciones, y en otra
  uno de 33 ms en el frame 10. El primero coincide con algo externo a la ventana: la
  medición los muestra, pero no dice qué los causó.
- **El primer timestamp medía otra cosa.** La primera versión escribía el timestamp
  inicial en `TOP_OF_PIPE`, que no espera al semáforo de adquisición del swapchain, y
  dio p99 de 6,4 ms para limpiar una imagen. Escribirlo en la etapa que sí espera
  (`TRANSFER`) separa el trabajo de la espera. Aun así, el p99 del trabajo saltó entre
  90 µs y 3,2 ms en corridas iguales. **Hipótesis, no verificada:** la GPU se comparte
  con el compositor, o baja su frecuencia entre frames casi vacíos. Un timestamp mide
  tiempo de pared en la GPU, no "el trabajo de este programa".
- **Presupuesto de memoria:** un solo heap de 5740 MiB, compartido con la CPU. El
  presupuesto que reporta `VK_EXT_memory_budget` cambió entre 1677 y 1975 MiB entre
  corridas y dentro de una misma corrida: depende de la RAM libre del sistema. El uso del
  programa fue 2,8 MiB.

**RenderDoc:**

| Qué | Resultado |
|---|---|
| Programa en Wayland, dentro de RenderDoc | **Falla al crear la instancia** (`VK_ERROR_EXTENSION_NOT_PRESENT`): RenderDoc no expone `VK_KHR_wayland_surface` |
| Programa en XWayland (`SDL_VIDEO_DRIVER=x11`), dentro de RenderDoc | Funciona. La API *in-app* (`RENDERDOC_GetAPI` por `dlopen` con `RTLD_NOLOAD`) disparó una captura del frame 300 sin tocar el teclado |
| Costo de RenderDoc sin capturar | El trabajo de GPU subió de ~60 µs a ~120 µs (p50) |

Así, **la medición de R3 con primer píxel y RenderDoc no corren en la misma sesión:**
la primera necesita Wayland y RenderDoc necesita XWayland. SDL3 elige el driver al
arrancar (`SDL_VIDEO_DRIVER`), así que basta con una variable de entorno.

**Validation layers:** con `VK_LAYER_KHRONOS_validation`, P3 reportó un error real del
prototipo en cada presentación: `VUID-VkPresentTimingInfoEXT-timeDomainId-12400`, por
pedir tiempos con `timeDomainId = 0` en vez de un dominio devuelto por
`vkGetSwapchainTimeDomainPropertiesEXT`. El driver lo aceptaba igual y daba datos. Es
un error que solo se ve con las layers.

## 4. El costo de una línea de log (P4)

**Qué se midió:** 1 millón de líneas como `[corte] paso 123: plano n=(1.250, -0.500,
3.000)` (100 000 en las que escriben a disco, para no llenar `/tmp`, que es tmpfs).
Compilado con `-fno-exceptions -fno-rtti` (ADR-0009). Las asignaciones se contaron
reemplazando `operator new`.

| Variante | Clang | GCC | Asignaciones |
|---|---|---|---|
| `fprintf` a un archivo (con el buffer de `stdio`) | 397 ns | 410 ns | 0 |
| `fprintf` + `fflush` en cada línea | 1185 ns | 1186 ns | — |
| `snprintf` a un *ring buffer* en memoria (entradas de 128 B) | 389 ns | 411 ns | — |
| `std::format_to_n` al mismo *ring buffer* | 446 ns | 476 ns | **0** |
| Guardar los argumentos en binario (32 B, con timestamp) y formatear después | **14 ns** | 15 ns | — |

- **`std::format` funciona sin excepciones.** El formato se verifica al compilar (es un
  `consteval`), así que un error en el formato no compila. Con un formato en tiempo de
  ejecución (`std::vformat`), libstdc++ aborta en vez de lanzar.
- **Formatear cuesta lo mismo que escribir:** casi todo el costo es convertir los
  `float` a texto. Escribir a disco por el buffer de `stdio` casi no suma; lo caro es
  vaciarlo en cada línea.
- **Formatear después es ~30 veces más barato** en el momento del log, pero obliga a
  guardar el formato aparte, a copiar los argumentos crudos (con cuidado con las
  cadenas) y a tener un paso que los formatee. Es la idea de NanoLog.

## 5. El replay y el hash del estado (P5)

**Qué se midió:** una simulación chica parecida a la física del ADR-0016: 256
partículas con integración de Verlet, gravedad, un empuje con `sinf` y 4 iteraciones de
restricciones de distancia con `sqrtf`, durante 3600 pasos de 1/60 s (un minuto). El
input de cada paso sale de una secuencia determinista, como si viniera de un archivo
grabado. Se imprime el hash FNV-1a de las posiciones cada 600 pasos.

| Compilación | ¿Mismo hash que Clang `-O2`? |
|---|---|
| Clang y GCC, con `-O0`, `-O2` y `-O3` | **Sí, los seis** |
| Clang y GCC, `-O2 -march=native` | **No, desde el primer paso** |
| `-march=native -ffp-contract=off` (y `-O3`, y `-march=x86-64-v3`) | Sí |
| `-ffast-math` | No (y distinto entre Clang y GCC) |
| La misma build, 3 veces | Sí |

- Con las banderas del ADR-0008 (x86-64 base, sin `-march`, sin `-ffast-math`), **dos
  compiladores y tres niveles de optimización dan los mismos bits.** En x86-64 base no
  hay FMA, así que el compilador no puede fusionar una multiplicación y una suma.
- Con `-march=native` sí la hay, y fusionar `a*b + c` cambia el redondeo. El estado
  difiere desde el primer paso. Si un día se agrega `-march`, como permite el ADR-0008
  cuando lo pida el profiler, `-ffp-contract=off` conserva los replays grabados.

**La libm de otra máquina.** Un replay que se compara en otra máquina (por ejemplo, en
la CI) usa otra glibc. Si las funciones de `float` dan el resultado correctamente
redondeado, todas las implementaciones correctas dan los mismos bits; si no, cada
versión puede dar otros. Se comparó cada función con el resultado en `double`
redondeado a `float`, sobre ~20 millones de valores por función:

| Función (glibc 2.44) | Resultados distintos del correctamente redondeado |
|---|---|
| `sinf` | 208 885 (1,05 %) |
| `cosf` | 125 241 (0,63 %) |
| `expf` | 10 570 (0,05 %) |
| `atan2f`, `acosf` | 0 |
| `sqrtf` | 0 (IEEE 754 lo exige exacto) |

`atan2f` y `acosf` son correctamente redondeadas porque glibc 2.41 las reemplazó por las
del proyecto CORE-MATH. Esto tiene dos consecuencias. Una glibc anterior a la 2.41 da
otros bits en esas dos funciones. Y `sinf`, `cosf` y `expf` pueden cambiar en cualquier
versión futura. **El hash del estado no se puede comparar entre máquinas con
distinta glibc** sin controlar las funciones trascendentales.

**Cuánto ocupa el input** (cuenta, no medición): el estado de input de un paso, ya
convertido de los eventos de SDL (dos ejes de movimiento, el movimiento del mouse o del
stick derecho, la dirección del tajo y una máscara de botones), cabe en ~24 B. A 60
pasos por segundo son ~86 KB por minuto sin comprimir: un escenario de 10 minutos ocupa
menos de 1 MB.

## 6. Dear ImGui sin ventana (P6)

**Qué se midió:** solo la CPU: `NewFrame`, la interfaz y `Render`, que generan las
listas de dibujo, sin backend ni GPU. 3000 frames, después de 100 de calentamiento, con
el mouse moviéndose sobre la ventana. Compilado con `-fno-exceptions -fno-rtti`.

| Caso | p50 | p99 | Máximo | Geometría | Asignaciones en 3000 frames |
|---|---|---|---|---|---|
| Panel típico: 30 sliders, gráfica de 300 tiempos de frame, tabla de 10 regiones de memoria | **37 µs** | 93 µs | 673 µs | 1632 vértices, 2574 índices | **0** |
| Ventana demo (con sus secciones cerradas) | 12 µs | 31 µs | 383 µs | 760 vértices | 0 |

| Tamaño | Valor |
|---|---|
| Código del núcleo (`imgui`, `imgui_draw`, `imgui_tables`, `imgui_widgets`) | ~43 000 líneas, ~895 KB de código compilado |
| La demo (`imgui_demo`) | ~250 KB más; es opcional |
| Backends de SDL3 y de Vulkan | 890 y 2050 líneas |
| Atlas de la fuente por defecto | 512×128 RGBA, 256 KiB |

- **Cero asignaciones en régimen:** después de los primeros frames, ImGui reutiliza sus
  buffers. Sus asignaciones pasan por `ImGui::SetAllocatorFunctions`, así que se pueden
  mandar a una región propia (ADR-0010).
- **El backend de render es reemplazable:** ImGui entrega listas de vértices, índices y
  rectángulos de recorte. El backend de Vulkan que trae crea su propio pipeline y sus
  descriptores. Dibujarlas con el renderer propio es la otra opción.

## Cómo repetirlo

Los programas no están en el repo (son descartables, ADR-0014). Cada sección dice qué
hace su programa y con qué banderas se compiló. Lo que hay que instalar en Arch:
`perf`, `renderdoc` y `vulkan-validation-layers` (del repositorio `extra`). Tracy, Dear
ImGui y Vulkan-Headers se descargaron en las versiones de la tabla del principio.
