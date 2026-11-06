# Prototipo de input y controles

Nota de investigación del [ADR-0032](../adr/0032-input-y-controles.md).
Guarda las mediciones que sostienen la decisión, para poder repetirlas.

Fecha: 2026-11-06, en la máquina de referencia del ADR-0020 (Dell XPS 13 9360,
i7-8550U, Intel UHD 620, pantalla de 1600×900 a 59,98 Hz). Son mediciones de
laboratorio para decidir, no del protocolo del ADR-0020. La CPU y la GPU corrieron con
el reloj libre. El prototipo es código descartable, fuera del repo.

| Qué | Versión |
|---|---|
| Linux | 7.2.15 |
| Compositor | Hyprland 0.56.2 (Wayland) |
| GCC | 16.2.1, con `-O2`, sin `-march` (ADR-0008) |
| SDL3 | 3.4.18 |
| Mesa (ANV) | 26.2.5, Vulkan 1.4 |
| libxkbcommon | 1.13.2 |
| wtype | 0.4 |

**Los dispositivos:** un mouse Logitech USB óptico, el trackpad de la XPS (Synaptics) y
el teclado de la XPS con la distribución `latam`. **No hubo gamepad:** el DualShock 4 del
autor no estaba a mano. Lo que depende del mando real (el tipo, la deriva, el giroscopio
y la vibración) queda pendiente de su prueba, como en el ADR-0026.

## El prototipo

- **`paso.cpp`:** P2 y P4. El game loop del ADR-0012 (paso de 60 Hz, acumulador, tope de
  8 pasos y de 250 ms) con una traza de mouse, varios horarios de frames y tres reglas
  para repartir el input del frame entre sus pasos. Sin ventana.
- **`ventana.cpp`:** P1 y P5. SDL3 y Vulkan con un *swapchain* FIFO que dibuja un color
  liso. `sesion` graba cada evento real del autor; `latencia` inyecta teclas con `wtype`
  y mide hasta que el frame se muestra (`VK_KHR_present_wait`).
- **`zona.cpp`:** P3. Las zonas muertas y las curvas del stick, en números y en una
  imagen.
- **`teclas.c`:** P6. Qué letra da cada tecla física en varias distribuciones, con
  libxkbcommon (lo mismo que usa SDL3 en Wayland).
- **`mando.cpp`:** P6 y P7 sin el mando real: un DualShock 4 virtual de SDL3
  (`SDL_AttachVirtualJoystick`).
- **`globales.c`:** lista los protocolos que ofrece el compositor (no está
  `wayland-info`).
- **`tools/analizar_sesion.py`:** resume la sesión del autor y saca las trazas reales.

## P1. SDL3 en esta máquina

**El compositor** ofrece `zwp_relative_pointer_manager_v1`,
`zwp_pointer_constraints_v1`, `wp_presentation`, `wp_fifo_manager_v1` y
`wp_tearing_control_manager_v1`. **No ofrece** `zwp_input_timestamps_manager_v1`.

**La sesión del autor** (45 s guiados: mouse, trackpad y teclado) con el mouse relativo
de SDL3 (`SDL_SetWindowRelativeMouseMode`):

| Qué | Mouse | Trackpad |
|---|---|---|
| Eventos | 1569 | 1360 |
| Intervalo entre eventos | 8,00 ms (125 Hz), sin variación | 7 a 8 ms (~143 Hz) |
| Edad al leer (lectura − *timestamp*) | mediana 8,5 ms, p95 16,1, máx 17,1, **mín −1,6** | mediana 9,1 ms, p95 16,4, máx 18,5 |
| Eventos por frame | mediana 2, máx 3 | mediana 2, máx 3 |
| *Timestamps* distintos dentro de un frame | **100 %** | **100 %** |
| Deltas enteros | 100 % (cuentas crudas, sin la aceleración del sistema) | 100 % |

Teclado: 165 eventos, edad al leer con mediana de 8,6 ms y máximo de 18,2. **El toque más
corto duró 38 ms** (mediana 136 ms). 19 frames tuvieron más de un evento de teclado.

Lo que muestra:

- **Cada evento trae su propio *timestamp***, y no el del momento en que SDL lo leyó:
  dentro de un frame son todos distintos y van separados lo que reporta el dispositivo
  (8 ms). Así se puede saber a qué paso pertenece cada evento (P2, regla C).
- **El *timestamp* es aproximado:** un evento llegó con un *timestamp* 1,6 ms **posterior**
  a su lectura. SDL convierte el tiempo del compositor (en ms) a su reloj, y la
  conversión tiene ese error. Hay que acotarlo: un *timestamp* futuro cuenta como "ahora".
- **El mouse y el trackpad llegan como un solo puntero** (el mismo `which`): Wayland los
  junta en el asiento. El juego no puede tener una sensibilidad para el mouse y otra para
  el trackpad sin preguntarle al sistema por otro camino.
- **Los deltas son cuentas crudas:** SDL3 usa por defecto el movimiento sin aceleración de
  `relative-pointer` (`SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE` apagado).

## P2. El input en el paso fijo

Una traza de mouse pasa por el game loop con cinco horarios de frames y cuatro reglas:

| Regla | Qué hace |
|---|---|
| **A. Todo al primero** | El primer paso del frame recibe todo el movimiento del frame; los demás, nada |
| **B. Repartido** | El movimiento del frame se divide en partes iguales entre sus pasos |
| **C. Por *timestamp*** | Cada evento va al paso que cubre su *timestamp*. Lo que cae después del último paso del frame (el resto del acumulador) **espera al frame siguiente** |
| **C'. Por *timestamp*, sin espera** | Como C, pero lo que cae después del último paso va al último |

El apunte es un ángulo 2D (guiñada y cabeceo), recortado a un cono de 40° (como el del
ADR-0031), con 0,05° por cuenta. **La verdad** es la traza entera, recortada evento por
evento, en el instante que representa el final de cada paso. El error se mide **después de
quitar el mejor retardo constante**: un retardo fijo se compensa, el temblor no. Cada
escenario se corrió con 40 fases distintas del frame.

**Traza sintética** (1000 Hz, gestos de jerk mínimo: apuntar a un cuello, corregir, un
golpe contra el borde del cono y la vuelta, la línea que gira al cargar, y temblor de
~10 Hz):

| Escenario | A | B | C | C' |
|---|---|---|---|---|
| 30 FPS | RMS 0,61°, máx 6,6° | 0,32°, 4,2° | **0,12°, 0,35°** | 0,36°, 4,3° |
| 59,98 Hz con ruido | 0,28°, 4,5° | 0,27°, 4,5° | **0,12°, 0,35°** | 0,28°, 4,5° |
| 144 FPS | 0,18°, 1,5° | 0,18°, 1,5° | **0,12°, 0,35°** | 0,18°, 1,5° |
| 60 FPS con un tirón de 133 ms por segundo | 1,47°, 12,0° | 0,42°, 4,4° | **0,12°, 0,35°** | 0,33°, 4,3° |
| 20 FPS con un congelamiento de 400 ms | 1,05°, 9,3° | 1,77°, 15,3° | **0,12°, 0,35°** | 0,35°, 5,1° |

**Traza real del autor** (el mouse de P1: 1569 eventos en 15,7 s, con golpes rápidos de
hasta 126 cuentas en 8 ms, unos 790°/s con 0,05° por cuenta, mucho más rápido de lo que
se moverá la espada):

| Escenario | A | B | C | C' |
|---|---|---|---|---|
| 30 FPS | RMS 1,65°, máx 14,9° | 0,99°, 9,8° | **0,07°, 0,66°** | 1,05°, 12,1° |
| 59,98 Hz con ruido | 0,91°, 10,6° | 0,90°, 10,6° | **0,07°, 0,66°** | 0,91°, 10,6° |
| 144 FPS | 0,62°, 5,8° | 0,62°, 5,8° | **0,07°, 0,66°** | 0,62°, 5,8° |
| 60 FPS con tirones | 2,84°, 39,0° | 1,88°, 32,0° | **0,07°, 0,66°** | 0,96°, 11,1° |
| 20 FPS congelado | 3,59°, 40,3° | 3,20°, 34,6° | **0,07°, 0,66°** | 0,83°, 11,0° |

El error de C no es 0 porque el cono recorta por paso y la verdad, por evento: dentro de
un paso, un golpe que pasa el borde y vuelve se recorta distinto.

**¿Da lo mismo a cualquier FPS?** Máxima diferencia del apunte en el mismo paso contra 60
FPS limpio, traza real:

| Regla | 30 FPS | 59,98 Hz | 144 FPS | 60 FPS con tirones |
|---|---|---|---|---|
| A | 16,0° | 8,9° | 9,8° | 38,8° |
| B | 6,0° | 8,9° | 9,8° | 30,4° |
| **C** | **0,000°** | **0,000°** | **0,000°** | **0,000°** |
| C' | 12,2° | 8,9° | 9,8° | 10,8° |

**El costo de C es la espera.** Edad media del evento cuando un paso lo usa (traza real):

| Escenario | A, B y C' | C |
|---|---|---|
| 30 FPS | 16,7 ms | 25,1 ms (+8,4) |
| 59,98 Hz | 8,7 ms | 16,6 ms (+7,9) |
| 144 FPS | 8,7 ms | 11,8 ms (+3,1) |

Lo que muestra:

- **Solo C da el mismo juego a cualquier FPS.** Con A, B y C', cuántos pasos corre cada
  frame cambia qué movimiento ve cada paso. El cono hace que esa diferencia no se
  compense: recortar es no lineal, así que mover +10° y −10° en un paso no da lo mismo
  que en dos.
- **A "ve el futuro":** su retardo óptimo es negativo (de −4 a −20 ms). El primer paso del
  frame representa un instante anterior a parte del movimiento que recibe. A 59,98 Hz,
  los frames con 0 y 2 pasos (el ruido del vsync del ADR-0020) hacen que la mira se
  quede quieta un paso y salte el doble en el siguiente.
- **B es peor que A después de un congelamiento:** reparte 400 ms de movimiento en los 8
  pasos del tope.
- **C espera de media medio paso** (~8 ms a 60 Hz): lo que llegó después del último paso
  completo pertenece a un paso que todavía no corrió. Es exactamente lo que documenta el
  modo `ProcessEventsInFixedUpdate` del Input System de Unity.

### Los toques cortos

269 toques de 15 a 120 ms en 120 s. ¿Cuántos ve la simulación?

| Escenario | Solo el estado al leer | Estado + flanco del frame | Estado + **contador de presiones** |
|---|---|---|---|
| 30 FPS | 4,8 % perdidos | 0 % | **0 %** |
| 59,98 Hz con ruido | 0 % | 3,0 % | **0 %** |
| 144 FPS | 0 % | **54,3 %** | **0 %** |
| 60 FPS con tirones | 5,2 % | 1,9 % | **0 %** |
| 20 FPS congelado | **28,6 %** | 0 % | **0 %** |

- **Solo el estado** pierde los toques que empiezan y terminan entre dos lecturas: a FPS
  bajos y en los tirones.
- **El flanco del frame** se pierde cuando el frame corre 0 pasos: a 144 FPS, más de la
  mitad de los frames no corren ningún paso.
- **Un contador de presiones** (y otro de sueltas) que se acumula hasta que un paso lo
  consume no pierde ninguno, a ningún FPS.

## P3. Las zonas muertas y las curvas del stick

Zona muerta interior de 0,20 y exterior de 0,05. Sin el mando real, el ruido en reposo
es un supuesto (σ = 0,006, con el centro corrido 0,15).

| Método | Salto en el borde | Error de dirección máx., largo 0,3 / 0,6 | En reposo con deriva |
|---|---|---|---|
| Sin zona muerta | — | 0° / 0° | **siempre activo** |
| Axial (cada eje por separado) | 0,20 | **41,8° / 19,4°** | 0 % |
| Radial (el largo del vector) | 0,20 | 0° / 0° | 0 % |
| **Radial escalada** (Sutphin, 2013) | **0,00** | 0° / 0° | 0 % |

- **La axial se pega a los ejes:** un movimiento lento cerca de un eje pierde el otro
  componente, y la dirección salta hasta 42°.
- **La radial salta** de 0 a 0,20 al salir de la zona: no hay salidas chicas.
- **La radial escalada** reescala lo que queda entre las dos zonas a [0, 1]: sin salto y
  sin pegarse.

**Las curvas** (el largo de la salida elevado a un exponente, sobre la radial escalada):
recorrido del stick que da una salida menor a 0,2, para los movimientos chicos:

| Exponente | Recorrido | Del recorrido útil |
|---|---|---|
| 1 (lineal) | 0,20 a 0,35 | 19 % |
| 1,5 | 0,20 a 0,46 | 32 % |
| 2 | 0,20 a 0,54 | 42 % |
| 3 | 0,20 a 0,64 | 55 % |

## P4. La sensibilidad del mouse y los FPS

La misma traza real, con el apunte calculado por paso (regla C) o por frame, y la
sensibilidad multiplicada o no por el dt del frame (el error clásico, calibrado para
dar lo mismo a 60 FPS). Guiñada final y máxima del gesto:

| Escenario | Por paso (C) | Por frame, delta | Por frame, delta·dt |
|---|---|---|---|
| 30 FPS | 2,29° / 24,95° | 1,95° / 24,90° | **33,30° / 40,00°** |
| 59,98 Hz | 2,29° / 24,95° | 1,91° / 24,90° | 5,04° / 32,46° |
| 144 FPS | 2,29° / 24,95° | 2,26° / 24,95° | **−0,51° / 11,19°** |
| 60 FPS con tirones | 2,29° / 24,95° | 1,91° / 24,95° | 23,06° / 39,88° |

- **El delta del mouse ya es un desplazamiento:** multiplicarlo por el dt cambia la
  sensibilidad con los FPS (a 144 FPS el mismo gesto llega a 11° en vez de 25°).
- **En el modelo de velocidad, el stick sí se integra con el dt** (en el paso fijo, ese dt
  es siempre 1/60 s). El ADR eligió el modelo de posición, en el que el stick marca un
  objetivo y no se integra.
- **Por frame, sin el error, casi da:** las diferencias chicas son del recorte del cono.
  Por paso con C, es exacto.

## P5. La latencia, del input a la pantalla

`ventana latencia`: un hilo lanza `wtype -k space` de 180 a 420 ms al azar, 50 veces por
modo. El juego cambia el color de la pantalla en el frame que lee la tecla. Se mide
desde el envío hasta que ese frame se muestra (`vkWaitForPresentKHR` en otro hilo, que
el estándar permite). Cada frame gasta 6 ms de CPU; la GPU solo borra la imagen. FIFO,
3 imágenes en el *swapchain* (el mínimo de Mesa en Wayland).

| Modo | Cómo lee | Envío → lectura | Lectura → pantalla | **Total** (media, p95) |
|---|---|---|---|---|
| **A** | Al empezar el frame, 2 frames en vuelo (el *game loop* de libro) | 15,1 ms | 67,1 ms | **82,2 ms**, 90,5 |
| **B** | Espera a que se muestre el frame anterior y lee (1 en vuelo) | 20,5 ms | 34,7 ms | **55,2 ms**, 72,1 |
| **C** | Como B, y además duerme hasta poco antes del vsync y lee tarde | 27,5 ms | 29,7 ms | **57,1 ms**, 75,6 |
| **D** | Espera a que se muestre el frame de **hace dos** y lee | 10,9 ms | 32,4 ms | **43,3 ms**, 50,6 |

- **La cola de frames es la mayor parte de la latencia.** Con A, la CPU se adelanta hasta
  llenar el *swapchain*, y el frame que leyó el input espera ~4 vsyncs para salir.
- **D gana:** 39 ms menos que A, y la CPU de un frame sigue en paralelo con la GPU del
  anterior. **B y C ponen la CPU y la GPU en serie:** las dos juntas tienen que caber en
  16,7 ms, y eso choca con el presupuesto del ADR-0020, que le da 16,7 ms a cada una.
- **Leer tarde (C) no ganó en este compositor:** Hyprland compone justo después del
  vblank, así que despertar más tarde no adelanta la imagen.
- **Lo que no mide:** el *polling* del dispositivo (8 ms de este mouse), la pantalla, y
  una GPU con trabajo de verdad. `wtype` suma el tiempo de lanzar un proceso (igual en
  los cuatro modos). Son números de esta máquina y este compositor.

### La demo del autor

`ventana demo`: un cuadrado blanco sigue al mouse, y el cursor del sistema queda visible.
El compositor dibuja el cursor en un plano propio, casi sin latencia, así que **la
distancia entre los dos al mover rápido es la latencia del modo**. El fondo cambia de
color con el modo (A rojo, B verde, C azul, D gris), y la CPU por frame se cambia en vivo.

Lo que vio el autor: **con D, el cuadrado se queda pegado al cursor; con A funciona,
pero se le escapa.** Y los FPS (promedios de 2 s; algunas ventanas mezclan dos modos):

| Modo | FPS con 6 ms de CPU |
|---|---|
| A | 60 a 61 |
| B | ~41 |
| C | de 28 a 39 |
| D | 59 a 60 |

**B y C pierden frames** aunque la CPU sobre: esperan a que el frame anterior ya se haya
mostrado, y si la espera termina un poco después del vblank, el frame nuevo no alcanza el
siguiente. D deja un frame de margen y mantiene el ritmo.

**El foco sigue al mouse en Hyprland.** En la primera versión de la demo, con el cursor
libre, las teclas de cambio de modo nunca llegaron: al hacer círculos, el cursor salía de
la ventana y el teclado se iba con él. En P1 no pasó, porque el mouse relativo encierra el
cursor. La demo final encierra el cursor (`SDL_SetWindowMouseGrab`) y cambia de modo con
el clic.

**Las teclas de `wtype` traen un *timestamp* inválido:** el teclado virtual de Wayland
manda el tiempo que elige el cliente, y SDL lo copia. Lo mismo puede pasar con un
teclado en pantalla o cualquier herramienta de accesibilidad que use ese protocolo. **Hay
que acotarlos** (ADR-0032, Decisión 5): uno del futuro se toma como el momento de la
lectura, y uno de un paso que ya corrió va al primer paso del frame.

## P6. Las teclas, los íconos y el cambio de dispositivo

**Posición contra letra** (`teclas.c`): qué da cada tecla física de la visión en varias
distribuciones.

| Posición (scancode) | us | latam | es | fr | de | dvorak |
|---|---|---|---|---|---|---|
| W | w | w | w | **z** | w | **,** |
| A | a | a | a | **q** | a | a |
| S | s | s | s | s | s | **o** |
| D | d | d | d | d | d | **e** |
| Q | q | q | q | **a** | q | **'** |
| E | e | e | e | e | e | **.** |
| F | f | f | f | f | f | **u** |
| Espacio | space | space | space | space | space | space |

- **Si los controles se guardan por letra,** en un teclado francés WASD queda repartido
  por todo el teclado. **Por posición,** WASD es siempre la misma forma bajo la mano
  izquierda, y el menú muestra la letra que tiene esa tecla en la distribución del
  jugador (SDL3: `SDL_GetKeyFromScancode` y `SDL_GetKeyName`).
- **El nombre del scancode no sirve para guardarlo:** la documentación de SDL3 avisa que
  `SDL_GetScancodeName` no es estable (en Linux "Left GUI" y en Windows "Left Windows", y
  hay nombres repetidos).

**Un DualShock 4 virtual** (`mando.cpp`, vendor 0x054C, product 0x09CC):

- SDL3 lo reconoce como **PS4** por el vendor y el product: los íconos del ADR-0026
  salen de ahí.
- `SDL_RumbleGamepad` llega al dispositivo con los dos motores (grave y agudo).
- **SDL no filtra la deriva:** un stick quieto con ruido genera ~30 000 eventos de eje
  por minuto.

**La deriva contra el cambio de íconos:** un mando quieto en la mesa, con el centro
corrido, mientras el jugador toca una tecla cada 200 ms durante 60 s. Cambios de íconos
al mando que no debieron pasar:

| Centro corrido | Cualquier evento del mando | Eje > 0,25 | Eje > 0,5 |
|---|---|---|---|
| 0,02 | 292 | 0 | 0 |
| 0,08 | 292 | 0 | 0 |
| 0,15 | 292 | 0 | 0 |
| 0,28 | 292 | **292** | 0 |

Con la regla "cualquier evento", los íconos cambian con cada tecla. Con un umbral igual a
la zona muerta, solo fallan cuando la deriva la supera, y en ese punto la caballera
también caminaría sola: es la misma CVar.

## P7. La vibración

El camino está probado (P6: `SDL_RumbleGamepad` llega al dispositivo). **Cómo se siente
queda para la prueba del autor con el DualShock 4.**

## Pendiente: la prueba del autor con un DualShock 4

1. el tipo que da SDL3 con el mando real, por USB y por Bluetooth;
2. la deriva real de su stick en reposo (con `mando.cpp` adaptado a leer el mando);
3. el giroscopio (`SDL_SetGamepadSensorEnabled`);
4. la vibración en un corte: si se siente bien y con qué fuerza.

## Cómo repetirlo

```bash
g++ -std=c++20 -O2 src/paso.cpp -o paso && ./paso && ./paso traza_mouse.csv
g++ -std=c++20 -O2 -I<cabeceras de Vulkan> src/ventana.cpp -o ventana -lvulkan -lSDL3
./ventana sesion                 # el autor; después: python3 -I tools/analizar_sesion.py sesion.csv
./ventana latencia A 6 50        # y B, C, D: no tocar el teclado mientras corre
./ventana demo                   # clic: cambia de modo; rueda: CPU por frame; clic del medio: salir
g++ -std=c++20 -O2 src/zona.cpp -o zona && ./zona
gcc src/teclas.c -o teclas $(pkg-config --cflags --libs xkbcommon) && ./teclas
g++ -std=c++20 -O2 src/mando.cpp -o mando -lSDL3 && SDL_VIDEO_DRIVER=dummy ./mando
```
