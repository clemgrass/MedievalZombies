# ADR-0032: Input y controles

- **Estado:** Aceptado
- **Fecha:** 2026-11-06
- **Ticket:** #74
- **Reemplaza / Reemplazado por:** —

## Contexto y problema

Hay que decidir **cómo llega el input del jugador al juego**: desde que el mouse, el
teclado o el gamepad mandan un evento hasta que un paso de la simulación lo usa, y desde
ahí hasta que el jugador ve el resultado en la pantalla.

En este juego el input no es un trámite. **La espada se guía con el mouse o con el stick
derecho** (ADR-0031): al apuntar, mueven el punto; al cargar, giran la línea del tajo. La
precisión de ese gesto es la habilidad del jugador (visión: "apuntar es una habilidad, no
un trámite"), y el gesto tiene que sentirse pesado, nunca flácido (la idea del autor en
el ADR-0031). Además, **el input es lo único que entra a la simulación desde afuera:** si
el mismo gesto da resultados distintos según los FPS, el *replay* y la equidad entre
máquinas se rompen.

Varios ADRs ya aceptados dejaron preguntas para este:

| ADR | Qué dejó abierto |
|---|---|
| Visión | Los controles de teclado + mouse y de gamepad; todos reasignables; la sensibilidad, por separado para el mouse y el gamepad; E y Espacio como alternativa al clic para el trackpad; la alternativa de mantener a presionar rápido |
| ADR-0004 | SDL3 da los eventos (incluido el mouse relativo) detrás de `platform::`. Cómo se convierten en el estado de input del juego |
| ADR-0012 | El input se lee una vez por frame, antes de los pasos. Qué ve cada paso cuando un frame corre 0 u 8 |
| ADR-0018 | La alternativa de mantener para la hoja clavada y el agarre; el margen para soltar el tajo si la agarran; el tirón (mantener el ataque o tirar del stick hacia atrás) |
| ADR-0020 | Leer el input lo más tarde posible, como idea para bajar la latencia |
| ADR-0021 | La regla del input por paso ("qué parte del input del frame ve cada paso"); el *replay* graba el resultado. Las ventanas de input son CVars; la sensibilidad y la inversión de un eje, opciones del jugador |
| ADR-0026 | Las acciones abstractas, la reasignación, las curvas del stick y cómo se detecta el dispositivo. Los íconos siguen al último dispositivo usado. Las opciones que cambian la simulación (la sensibilidad, la alternativa de mantener) se graban como evento del paso |
| ADR-0028 | El remapeo de los controles; el formato de `opciones.txt` ya está |
| ADR-0031 | Las curvas, la zona muerta y la sensibilidad: la guía de la espada recibe "girar tantos grados". **Respecto de qué se mueve la caballera** sin una cámara del jugador |
| ADR-0015 | La guía de la espada es lenta y con peso; cómo se logra el peso es de ese ADR |

Hay que decidir:

1. **la capa de acciones:** si el juego lee teclas o acciones, de qué tipo es cada acción
   y los contextos;
2. **la reasignación:** qué se reasigna, los conflictos, posición o letra, y cómo se
   guarda;
3. **el mouse:** el movimiento relativo, crudo o con aceleración, y la unidad de la
   sensibilidad;
4. **el stick:** la zona muerta, la curva, y si marca una velocidad o una posición;
5. **el input en el paso fijo:** qué ve cada paso, cómo no se pierden los toques, qué
   graba el *replay* y respecto de qué se mueve la caballera;
6. **el *buffer* y las ventanas** del combate;
7. **la latencia:** cuándo se lee el input y cuántos frames esperan en la cola;
8. **los dispositivos:** el cambio en caliente, la desconexión y varios mandos;
9. **la vibración** (nadie la tenía asignada);
10. **la accesibilidad, los datos, las pruebas y la depuración.**

Lo que ya está decidido y limita este ADR:

- **ADR-0004:** SDL3 solo para ventana, input, tiempo y audio, detrás de `platform::`.
  **Ningún tipo de SDL sale de ahí.**
- **ADR-0009:** sin excepciones ni RTTI; en el código de cada frame no hay contenedores
  que asignen memoria.
- **ADR-0010:** regiones reservadas y tocadas al iniciar.
- **ADR-0011:** TDD estricto en el núcleo.
- **ADR-0012:** un hilo, paso fijo de 60 Hz con acumulador, **máximo 8 pasos por frame y
  frame limitado a 250 ms**. El orden del frame: input, pasos, render, audio.
- **ADR-0016 y ADR-0021:** la misma partida grabada da el mismo hash. El *replay* graba
  **el estado de input de cada paso, ya convertido**, no los eventos crudos.
- **ADR-0020:** 60 fps en la UHD 620, con 16,7 ms de CPU y 16,7 ms de GPU por frame.
- **ADR-0026:** Dear ImGui (solo de desarrollo) se queda con el input cuando tiene el
  foco. Los íconos siguen al último dispositivo, dibujados con primitivas.
- **ADR-0028:** las opciones van en `opciones.txt`, texto `clave = valor`: una clave
  desconocida se ignora, un valor fuera de rango se lleva al rango.
- **ADR-0031:** la cámara queda fuera de la simulación; un *replay* se mira con cualquier
  estilo. El apuntado es un rayo desde el pecho, dentro de un cono alrededor del rumbo.

**Fuera de alcance:**

| Qué | Lo decide |
|---|---|
| Cómo se siente la guía de la espada (el peso, la IK, los límites del cuerpo) | ADR-0015 |
| La navegación de los menús, el foco y el dibujo de los íconos | ADR-0026 |
| El formato de `opciones.txt` | ADR-0028 |
| El formato del *replay* | ADR-0021 |
| Qué hace cada acción en el combate (la fuerza, la carga, la hoja clavada) | ADR-0018 |

## Criterios

Los seis del ticket, más tres que salen de lo ya decidido:

1. **Respuesta inmediata:** la latencia del input a la pantalla.
2. **Precisión al apuntar,** con el mouse y con el stick.
3. **Accesibilidad:** las Xbox Accessibility Guidelines (XAG 107, input; XAG 110,
   vibración) y las Game Accessibility Guidelines, la meta que ya fijó el ADR-0026.
4. **Determinismo del *replay*** (ADR-0016, ADR-0021).
5. **Dependencias justificadas** (ADR-0004).
6. **Aprendizaje:** poder explicar cada pieza.
7. **Peso, nunca flácido:** el input no puede hacer que la espada se sienta liviana ni
   blanda (la idea del autor en el ADR-0031).
8. **Cabe en el frame y en el presupuesto** (ADR-0020).
9. **Práctica de la industria.**

## Opciones

Los números son de un prototipo medido en la máquina de referencia (ver Investigación).

### 1. La capa de acciones

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Teclas directas** | El gameplay pregunta por teclas y botones (`W`, `RT`) | No se puede reasignar sin tocar el gameplay. Cada mecánica se escribe dos veces (teclado y gamepad) |
| **Acciones con una tabla** (como el `InputMap` de Godot) | El gameplay pregunta por acciones ("apuntar", "mover"); una tabla dice qué entradas dan cada acción | Las mismas entradas significan cosas distintas en un menú y en el juego: hay que apagar la tabla del juego a mano |
| **Acciones con contextos** (como el *Enhanced Input* de Unreal) | Lo mismo, y además **contextos** apilados con prioridad (juego, secuencia de la hoja clavada, menú): el de arriba gana. Los **modificadores** (zona muerta, curva, sensibilidad, invertir) y los **disparadores** (mantener, tocar, presionar rápido) viven en la capa, no en el gameplay | Más piezas que diseñar y probar |

**Los tipos de acción** que pide la visión: **botón** (apuntar, cargar, esquivar,
interactuar, trotar, pausa), **eje 2D** (mover, guiar la espada). Los gatillos del
gamepad (LT, RT) son analógicos, pero el juego los usa como botones: necesitan un umbral,
y conviene que tenga histéresis (apretar a 0,30 y soltar a 0,20, por ejemplo) para que un
gatillo a medio camino no tiemble entre apretado y suelto.

**Guiar la espada** usa los mismos dos ejes al apuntar (mueven el punto) y al cargar
(giran la línea) (ADR-0031). Puede ser **una acción que el gameplay interpreta según la
fase** o **dos acciones en dos contextos**.

### 2. La reasignación

**Posición o letra.** Una tecla tiene dos identidades: el **scancode** (dónde está) y el
**keycode** (qué letra da con la distribución del jugador). P6:

| Posición | us | latam | es | fr | de | dvorak |
|---|---|---|---|---|---|---|
| W | w | w | w | **z** | w | **,** |
| A | a | a | a | **q** | a | a |
| Q | q | q | q | **a** | q | **'** |

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Guardar la letra** | `mover.arriba = w` | En un teclado francés, WASD queda repartido por todo el teclado |
| **Guardar la posición, mostrar la letra** | Se guarda el scancode; el menú muestra la letra que esa tecla da en la distribución del jugador (SDL3: `SDL_GetKeyFromScancode`, `SDL_GetKeyName`) | Si el jugador cambia de distribución, el menú cambia de letra, aunque la tecla sea la misma |

**El nombre en el archivo:** SDL3 avisa que `SDL_GetScancodeName` **no es estable** (en
Linux "Left GUI" y en Windows "Left Windows", y hay nombres repetidos). Hay que guardar el
**número** del scancode (es el código de uso del estándar USB HID) o **una tabla de nombres
propia**. El ADR-0004 ya pide un enum propio de teclas en `platform::`.

**Qué se reasigna y los conflictos:**

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Todo, por dispositivo** | Cada acción tiene sus entradas de teclado y mouse, y las suyas de gamepad, por separado | Es lo que pide la visión y la XAG 107 |
| **Hasta dos entradas por acción en el teclado** | La visión ya usa dos (clic derecho **o** E; clic izquierdo **o** Espacio) | La UI de reasignar es un poco más compleja |
| **Conflicto: intercambiar** | Si la entrada nueva ya la usa otra acción del mismo contexto, la otra recibe la entrada vieja | Lo hacen muchos juegos de PC. El jugador puede no notarlo |
| **Conflicto: rechazar** | Avisa y no deja | Obliga a liberar la tecla primero |
| **Teclas reservadas** | Esc y Start/Options siempre abren la pausa, aunque además estén asignadas | Ninguno: es lo que evita quedarse sin salida |

**Los menús** (ADR-0026: arriba, abajo, izquierda, derecha, aceptar, volver) pueden
reasignarse o quedar fijos con alternativas (flechas y WASD; Enter y Espacio; Esc; la
cruceta, el stick, A/Cruz y B/Círculo).

### 3. El mouse

**P1** (la sesión del autor, SDL3 en Hyprland): el mouse relativo funciona; los deltas
llegan **en cuentas enteras y sin la aceleración del sistema** (125 Hz con este mouse,
dos eventos por frame). **El mouse y el trackpad llegan como un solo puntero:** Wayland
los junta.

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Crudo, en grados por cuenta** | El giro es el delta por una sensibilidad en grados por cuenta. Sin aceleración y sin filtro | El mismo movimiento de la mano da el mismo giro siempre. Es lo que esperan los jugadores de PC |
| **Con la aceleración del sistema** | `SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE` | El giro depende de la velocidad de la mano: la memoria muscular no sirve |
| **Con un filtro** (suavizado) | Un promedio de los últimos deltas | Suma latencia y se siente blando. **El peso no va en el input:** lo da la espada, que sigue al objetivo con su velocidad y su aceleración (ADR-0015) |

**P4: el delta del mouse no se multiplica por el dt.** Ya es un desplazamiento. Con el
error clásico (sensibilidad × delta × dt), el mismo gesto llega a 40° a 30 FPS y a 11° a
144 FPS. Calculado por paso con la regla C (dimensión 5), da exactamente lo mismo a
cualquier FPS.

**El trackpad:** con un solo puntero, la sensibilidad del trackpad es la del mouse.
Separarlas pide preguntarle al sistema por otro camino (libinput), fuera de SDL3.

### 4. El stick

**La zona muerta** (P3, con 0,20 interior y 0,05 exterior):

| Método | Salto en el borde | Error de dirección máx. (largo 0,3) | Problema |
|---|---|---|---|
| **Axial** (cada eje por separado) | 0,20 | **41,8°** | Se pega a los ejes: un movimiento lento en diagonal pierde un componente |
| **Radial** (el largo del vector) | **0,20** | 0° | No hay salidas chicas: salta de 0 a 0,20 |
| **Radial escalada** (Sutphin, 2013) | 0,00 | 0° | Ninguno de los dos. Es la que usan los juegos de disparos |

**La curva** (el largo elevado a un exponente, sobre la radial escalada): con exponente
1, el 19 % del recorrido útil da salidas menores a 0,2; con 2, el 42 %; con 3, el 55 %.
Más recorrido para los movimientos chicos es más precisión, a cambio de girar más lento
con el stick a medias.

**Qué marca el stick al guiar la espada:**

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Velocidad** | Inclinar el stick hace girar el punto o la línea a una velocidad; soltarlo lo deja donde está. Es lo clásico para apuntar con gamepad | Precisión limitada por la curva. Sin una ayuda, cuesta frenar justo en un punto chico (una junta) |
| **Posición** | El cono tiene borde (ADR-0031): el stick a fondo es el borde del cono, y el centro es el rumbo. **Al cargar, el ángulo del stick es el ángulo de la línea** | Soltar el stick devuelve el punto al centro. El ruido del stick se ve en la mira, aunque la espada lo filtra con su peso (ADR-0015). El mouse sigue siendo relativo: los dos dispositivos se comportan distinto |
| **Mixta** | Posición al cargar (la línea apunta hacia donde apunta el stick) y velocidad al apuntar | Dos modelos que enseñar |
| **Giroscopio** (como opción) | El DualShock 4 y el DualSense lo tienen; SDL3 lo lee (`SDL_SetGamepadSensorEnabled`) | Solo algunos mandos. **Sin probar:** el mando del autor no estaba a mano |

**La ayuda de apuntado** (frenar la mira cerca de una junta, como hacen los juegos de
disparos con gamepad) es otra decisión: la visión dice que apuntar es una habilidad, y
el ADR-0031 descartó el *lock-on*.

### 5. El input en el paso fijo

**Qué ve cada paso** (P2: una traza real y una sintética, cinco horarios de frames, el
cono de 40°, con el error medido contra la traza entera después de quitar el mejor
retardo constante):

| Regla | Cómo funciona | Error RMS (traza real), 59,98 Hz / 60 FPS con tirones | ¿Igual a cualquier FPS? | Edad media del evento al usarse, 59,98 Hz |
|---|---|---|---|---|
| **A. Todo al primero** | El primer paso del frame recibe todo el movimiento | 0,91° / 2,84° | No (hasta 39° de diferencia) | 8,7 ms |
| **B. Repartido** | Partes iguales entre los pasos del frame | 0,90° / 1,88° | No (hasta 30°) | 8,7 ms |
| **C. Por *timestamp*** | Cada evento va al paso que cubre su *timestamp*; lo que cae después del último paso **espera al frame siguiente** | **0,07° / 0,07°** | **Sí, exacto** | 16,6 ms (+7,9) |
| **C'. Por *timestamp*, sin espera** | Como C, pero lo que sobra va al último paso | 0,91° / 0,96° | No (hasta 12°) | 8,7 ms |

- **C es la única que da el mismo juego a cualquier FPS.** El cono hace que importe:
  recortar es no lineal, así que el mismo movimiento partido de otra manera termina en
  otro lugar.
- **C cuesta medio paso de espera** (~8 ms a 60 Hz): lo que llegó después del último paso
  completo es de un paso que todavía no corrió. **El Input System de Unity hace lo mismo**
  en su modo `ProcessEventsInFixedUpdate`.
- **C necesita los *timestamps* de los eventos.** P1: en Hyprland, cada evento trae el
  suyo (100 % distintos dentro de un frame), con ~2 ms de error. Pero **las teclas de un
  teclado virtual de Wayland** (`wtype`, y quizás un teclado en pantalla) traen un
  *timestamp* inválido. Hay que acotarlos: uno del futuro se toma como el momento de la
  lectura, y uno de un paso que ya corrió va al primer paso del frame.

**Los toques cortos** (P2c, 269 toques de 15 a 120 ms; el más corto del autor en P1 duró
38 ms):

| Opción | Perdidos a 144 FPS | Perdidos a 20 FPS con un congelamiento |
|---|---|---|
| **Solo el estado** al leer | 0 % | **28,6 %** |
| **Estado + flanco del frame** | **54,3 %** (se pierde en los frames con 0 pasos) | 0 % |
| **Estado + contadores** de presiones y de sueltas, acumulados hasta que un paso los consume | **0 %** | **0 %** |

**Qué graba el *replay*:**

| Opción | Qué es | Problema |
|---|---|---|
| **El estado de los dispositivos** (teclas, ejes crudos) | Lo más cercano al hardware | El *replay* depende de la reasignación, la sensibilidad y las curvas: hay que grabarlas, y cada cambio, como evento del paso (lo que hoy dice el ADR-0026) |
| **Las acciones ya procesadas** | El movimiento, la guía y los botones con sus contadores | Lo que el jugador configura queda fuera de la simulación: la sensibilidad y la alternativa de mantener no se graban. Fe de erratas del ADR-0026. Un *replay* no muestra qué teclas tocó el jugador |

**Respecto de qué se mueve la caballera** (la pregunta del ADR-0031). El jugador no mueve
la cámara, y el apuntado es un cono alrededor del **rumbo** de la caballera, así que el
rumbo importa para apuntar:

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Relativo a la cámara** | Arriba en el stick es "hacia el fondo de la pantalla" | En la tercera persona, la cámara sigue el rumbo con un resorte: mantener "izquierda" hace que la caballera camine en círculos (como en *Super Mario 64*). En la fija, cambiar de toma cambia el sentido del stick |
| **Relativo a la cámara, manteniendo la dirección** | Como el anterior, pero mientras el stick no cambie, se mantiene la dirección del mundo de cuando se empezó a empujar, aunque la cámara corte | Lo habitual con cámaras fijas (*Devil May Cry*, el control moderno de *Resident Evil HD Remaster*) |
| **Tanque** | Arriba avanza, izquierda y derecha giran el rumbo | El jugador controla el rumbo, que es el centro del cono de apuntar. Se siente pesado (*Resident Evil 4*). En la fija y los rieles, a muchos jugadores les cuesta |
| **Por estilo** | Por ejemplo: tanque en la tercera y la primera persona; relativo a la pantalla, manteniendo la dirección, en la fija y los rieles | Dos esquemas que enseñar. **Cuál es el de partida es una decisión de diseño del autor** |

**Sea cual sea, la cámara sigue fuera de la simulación:** si el movimiento depende de la
pantalla, la capa de input lo convierte en una dirección del mundo **antes** del paso, con
la cámara del último frame, y el *replay* graba esa dirección, no la cámara.

**¿Camina mientras apunta?** La visión no lo dice. Si camina, el rumbo (y con él el cono)
se mueve mientras guía la espada. Es una pregunta de gameplay (ADR-0018) que este ADR
necesita saber.

### 6. El *buffer* y las ventanas

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Sin *buffer*** | Un botón solo cuenta si la acción se puede hacer en ese paso | Si se aprieta un poco antes (al terminar un salto, al recuperarse de un rebote), se pierde, y el juego se siente sordo |
| ***Buffer* corto por acción** | Una acción apretada hasta N pasos antes de poder hacerse sale en cuanto se puede. *Celeste* lo hace con el salto (Thorson, 2018) | Hay que elegir N por acción |
| ***Buffer* largo o cola de acciones** | Se recuerdan varias acciones seguidas | El jugador apretó dos veces y la caballera esquiva dos veces: pierde el control. Le quita peso a cada decisión |

**La unidad es el paso, no el milisegundo:** el *buffer* vive en la simulación y entra al
hash, igual que las ventanas del combate (el margen para soltar el tajo si la agarran,
ADR-0018). Todos son CVars (ADR-0021). **Soltar la carga no tiene *buffer*:** el momento de
soltar es la decisión.

### 7. La latencia

**P5:** del envío de una tecla hasta que se ve, con 6 ms de CPU por frame, FIFO y 3
imágenes en el *swapchain* (el mínimo de Mesa en Wayland):

| Opción | Cómo funciona | Total (media, p95) | Problema |
|---|---|---|---|
| **A. Leer al empezar el frame** | El *game loop* de libro, con 2 frames en vuelo | 82,2 ms, 90,5 | La CPU se adelanta hasta llenar la cola: el frame que leyó el input espera ~4 vsyncs |
| **B. Esperar el frame anterior** | Antes de leer, espera a que el frame anterior se muestre (`VK_KHR_present_wait`) | 55,2 ms, 72,1 | **La CPU y la GPU quedan en serie:** las dos juntas tienen que caber en 16,7 ms. Choca con el ADR-0020 |
| **C. B y leer tarde** | Además duerme hasta poco antes del vsync | 57,1 ms, 75,6 | No ganó: Hyprland compone justo después del vblank. Depende de cada compositor |
| **D. Esperar el frame de hace dos** | Antes de leer, espera a que se muestre el frame de hace dos | **43,3 ms, 50,6** | Un `VK_KHR_present_wait` por frame. La CPU de un frame sigue en paralelo con la GPU del anterior. Es la idea de NVIDIA Reflex: que no haya cola |

Ninguna mide el *polling* del dispositivo (8 ms de este mouse) ni la pantalla.

**La demo** (`ventana demo`: un cuadrado persigue al cursor del sistema, que el
compositor dibuja casi sin latencia) mostró algo que la medición no tenía: **con 6 ms de
CPU, B y C pierden frames** (B ~41 FPS; C de 28 a 39 FPS), porque si la espera del frame
anterior termina un poco después del vblank, el frame nuevo no llega al siguiente. A y D
se quedaron en 60 FPS.

### 8. Los dispositivos

**P6** (un DualShock 4 virtual de SDL3, porque el mando real no estaba): SDL lo reconoce
como PS4 por su vendor y su product, **y no filtra la deriva:** un stick quieto genera ~30
000 eventos por minuto.

| Pregunta | Opciones |
|---|---|
| **Qué cuenta como "usar" un dispositivo** (para los íconos del ADR-0026) | **Cualquier evento:** con el mando quieto en la mesa, los íconos cambian 292 veces por minuto. **Un umbral igual a la zona muerta:** 0 cambios hasta que la deriva la supera, y ahí la caballera también caminaría sola, así que es la misma CVar |
| **Cuántos dispositivos a la vez** | **Todos juntos** (el teclado y el mando funcionan siempre, sin elegir) o **solo el último usado** |
| **Varios mandos** | Se suman todos (un solo jugador) o solo el primero |
| **Desconexión** | **Pausa** si se desconecta el mando que se estaba usando (lo piden las certificaciones de las consolas, y es lo que espera un jugador) o seguir |
| **La base de datos de mandos** | La que trae SDL3, o además el archivo de la comunidad (`gamecontrollerdb.txt`, `SDL_AddGamepadMappingsFromFile`) |

### 9. La vibración

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Sin vibración** | — | Se pierde un canal: el corte también se siente en las manos |
| **Desde los eventos del paso** | Como el audio (ADR-0024) y los VFX (ADR-0025): cada paso deja sus eventos (el impacto con su fuerza, el corte con su material, el agarre, la hoja clavada) y, una vez por frame, la vibración los traduce a los dos motores (el grave y el agudo). **Es de la presentación:** no entra al hash | La XAG 110 pide que **nunca sea la única señal**, que se pueda apagar y que se regule la intensidad |
| **Gatillos adaptativos** (DualSense) | La resistencia del gatillo al cargar (`SDL_SendGamepadEffect`) | Solo el DualSense, y el autor no tiene uno |

El camino está probado (P7: `SDL_RumbleGamepad` llega al dispositivo). **Cómo se siente
queda para la prueba del autor con el DualShock 4.**

### 10. La accesibilidad, los datos, las pruebas y la depuración

| Qué | Lo que se puede decidir |
|---|---|
| **Mantener o alternar** | Apuntar y trotar se pueden configurar como "mantener" o como "tocar para alternar" (XAG 107) |
| **La alternativa de mantener del ADR-0018** | Un **disparador de la capa de acciones**: el gameplay recibe el mismo progreso, venga de presionar rápido o de mantener. Así la mecánica no cambia, como pide el ADR-0018 |
| **La sensibilidad** | La XAG 107 pide un ajuste para cada entrada analógica, de al menos ±50 % del valor por defecto |
| **La memoria** | Una región de input chica con topes fijos: los eventos de un frame (el autor tuvo 3 como máximo a 125 Hz; un mouse de 8000 Hz da ~133 por frame, y un tirón de 8 pasos, ~1100). **Si se llena,** se suman los deltas y nunca se pierde un botón |
| **Las pruebas** | Las piezas puras con TDD (ADR-0011): la zona muerta, la curva, la regla del paso, los contadores, la lectura de los controles en `opciones.txt` y los conflictos |
| **La depuración** | Un panel de ImGui: el dispositivo activo, el stick crudo y procesado, los eventos por frame, las acciones de cada paso y el modo de latencia |

## Investigación

### Prototipo medido

Código en C++20 con SDL3, Vulkan y libxkbcommon, medido en la máquina de referencia el
2026-11-06, con el mouse, el trackpad y el teclado del autor. El detalle y cómo
repetirlo están en
[`../research/input-y-controles.md`](../research/input-y-controles.md).

| # | Qué | Resultado |
|---|---|---|
| P1 | SDL3 en esta máquina | El mouse relativo funciona en Hyprland. **Cada evento trae su *timestamp*** (100 % distintos dentro de un frame, ~2 ms de error). Deltas crudos en cuentas enteras. Mouse a 125 Hz, trackpad a ~143 Hz, **los dos como un solo puntero**. El toque de tecla más corto: 38 ms |
| P2 | El input en el paso fijo | Con la traza real, error RMS a 59,98 Hz: A 0,91°, B 0,90°, **C 0,07°**, C' 0,91°. **Solo C da lo mismo a cualquier FPS** (0,000°), a cambio de +7,9 ms de espera media. Los contadores de presiones no pierden ningún toque; el flanco del frame pierde el 54 % a 144 FPS |
| P3 | Zona muerta y curva | La axial se desvía hasta 42°; la radial salta 0,20; la radial escalada, ni uno ni otro. Exponente 2: el 42 % del recorrido para salidas menores a 0,2 |
| P4 | La sensibilidad y los FPS | Con delta × dt, el mismo gesto da de 11° a 40° según los FPS. Por paso con C: idéntico |
| P5 | La latencia | A 82 ms, B 55 ms, C 57 ms, **D 43 ms** (media, del envío a la pantalla). En la demo del autor, **B y C pierden frames** con 6 ms de CPU (de 28 a 41 FPS); A y D se quedan en 60 |
| P6 | Teclas, íconos y dispositivos | En `fr`, la tecla de la W da "z". `SDL_GetScancodeName` no es estable. El DualShock 4 virtual se reconoce como PS4. SDL no filtra la deriva: cambiar de íconos con cualquier evento falla 292 veces por minuto; con el umbral de la zona muerta, 0 |
| P7 | La vibración | El camino funciona. **Sentirla queda para el autor** |

**Pendiente: la prueba del autor con un DualShock 4:** el tipo real, la deriva de su
stick, el giroscopio y la vibración.

Lo que mostró, además de los números:

- **El problema del input en un paso fijo es de tiempo, no de cantidad.** Las cuatro
  reglas entregan el mismo movimiento total. Lo que cambia es **en qué paso** cae cada
  parte, y como el cono recorta, eso cambia dónde termina la espada.
- **Un flanco por frame no alcanza** cuando el frame no corre ningún paso, que a 144 FPS
  es más de la mitad de las veces.
- **La mayor parte de la latencia es la cola de frames,** no la lectura del input. Leer
  "tarde" sirve de poco si el frame después espera tres vsyncs.
- **Un *timestamp* no siempre es confiable:** depende del protocolo que lo trajo.
- **Ni la deriva ni el ruido los filtra SDL.** La zona muerta es la única defensa, y sirve
  a la vez para el gameplay y para los íconos.

### Cómo lo hacen otros

- **Unreal Engine, *Enhanced Input*:** acciones (*Input Actions*), contextos con prioridad
  (*Input Mapping Contexts*), modificadores que transforman el valor (zona muerta,
  curvas, invertir) y disparadores que deciden cuándo dispara (mantener, tocar).
- **Unity, *Input System*:** acciones y *bindings*; en el modo `ProcessEventsInFixedUpdate`
  reparte los eventos entre los `FixedUpdate` **según su *timestamp***, y documenta el
  retardo que eso suma.
- **Godot, `InputMap`:** acciones con una zona muerta por acción, y consultas de estado y
  de flanco (`is_action_just_pressed`).
- ***Overwatch*, *High Precision Mouse Input*** (Blizzard, 2019): la simulación corre a
  62,5 Hz, pero al apretar o soltar el disparo usa la posición del mouse en el instante
  exacto del clic, entre dos pasos.
- **Fiedler, *Fix Your Timestep!*** (2004): el acumulador del ADR-0012.
- **Sutphin, "Doing Thumbstick Dead Zones Right"** (Third Helix, 2013): la axial, la
  radial y la radial escalada.
- **Thorson, "Celeste & Forgiveness"** (2018): el *buffer* del salto y el *coyote time*,
  ayudas que el jugador no ve.
- **Swink, *Game Feel*** (2008): la respuesta del control como parte de la sensación.
- **NVIDIA Reflex:** la CPU espera para que no haya una cola de frames listos esperando a
  la GPU. El modo D es la misma idea, con `VK_KHR_present_wait2` (el prototipo usó la
  primera generación, `VK_KHR_present_wait`).
- **Xbox Accessibility Guidelines:** XAG 107 (reasignar, mantener o alternar, la
  sensibilidad por cada entrada analógica) y XAG 110 (la vibración nunca es la única
  señal, se apaga y se regula).
- **SDL3:** el mouse relativo, `SDL_GetKeyFromScancode`, `SDL_GetGamepadType`,
  `SDL_RumbleGamepad`, los sensores y la base de datos de mandos.

## Decisión

Elijo que el juego hable con **acciones**, no con teclas: es lo estándar, y deja la
reasignación a un paso simple que cada jugador configura a su gusto. El input entra a la
simulación **como el gesto crudo**, sin aceleración ni filtro, y cada parte del gesto cae
en el paso en que pasó, para que el mismo gesto dé la misma espada a cualquier FPS. **El
peso no se pone en el input: lo da la espada.** El stick marca **dónde**, nunca qué tan
rápido; la rapidez del tajo sale de la carga. Grabar el input es una herramienta de
desarrollo: **Release no graba nada.**

| # | Dimensión | Elección |
|---|---|---|
| 1 | Acciones | **Acciones con contextos apilados**, como el *Enhanced Input* de Unreal pero mínimas: el juego, la secuencia (la hoja clavada y el agarre) y el menú; el de arriba gana, y ImGui, en desarrollo, está por encima de todos (ADR-0026). **Dos tipos:** botón (el estado y **contadores de presiones y de sueltas**) y eje 2D. **Los modificadores** (zona muerta, curva, sensibilidad, invertir) y **los disparadores** (mantener, alternar, presionar rápido) viven en la capa, no en el gameplay. **"Guiar" es una sola acción** que el gameplay interpreta según la fase (ADR-0031). Los gatillos son botones con un umbral con histéresis (de partida, apretar a 0,30 y soltar a 0,20; CVars) |
| 2 | Reasignación | **Todo se reasigna, por dispositivo:** hasta dos entradas por acción en el teclado y el mouse, una en el gamepad. **Se guarda la posición** (el scancode), con los nombres de una **tabla propia** de `platform::`, no los de SDL, y el menú muestra la letra que esa tecla da en la distribución del jugador. **Un conflicto en el mismo contexto se intercambia**, y el menú lo muestra. **Esc y Start/Options siempre abren la pausa.** Los menús son fijos, con alternativas (ADR-0026). Los controles por defecto son los de la visión; los cambiados van en `opciones.txt` (ADR-0028), una línea por acción y dispositivo (por ejemplo, `control.teclado.apuntar = mouse_derecho, e`) |
| 3 | Mouse | **Relativo y crudo:** cuentas sin la aceleración del sistema, **una sensibilidad en grados por cuenta**, sin filtro y **nunca multiplicada por el dt**. El mouse y el trackpad comparten la sensibilidad (Wayland los junta en un solo puntero). **En los menús, con el cursor libre, el cursor queda encerrado en la ventana mientras tenga el foco** (`SDL_SetWindowMouseGrab`): con un compositor donde el foco sigue al mouse, si el cursor sale, el teclado se va con él (lo que pasó en la demo) |
| 4 | Stick | **Zona muerta radial escalada** (de partida, 0,20 interior y 0,05 exterior) y **una curva de exponente 2**, como CVars; la zona muerta interior también es una opción del jugador, para los sticks gastados. **Solo posición, en las dos fases:** al apuntar, el stick a fondo es el borde del cono y soltarlo devuelve el punto al rumbo; al cargar, el ángulo del stick es el ángulo de la línea. La espada sigue a ese objetivo con su propio peso (ADR-0015). **La rapidez del tajo sale de la carga** (ADR-0018), no del stick. El mouse mueve el mismo objetivo, con sus deltas. **Si se usan los dos, manda el último dispositivo usado** (con el mismo umbral de la Decisión 8): si el último fue el stick, el objetivo es su posición; si fue el mouse, el objetivo sigue desde donde estaba y el mouse lo mueve, sin saltos. **Sin ayuda de apuntado.** El giroscopio queda abierto hasta la prueba del autor |
| 5 | El paso fijo | **Cada evento va al paso que cubre su *timestamp*** (la regla C): lo que cae después del último paso del frame espera al siguiente. **Un *timestamp* en el futuro** (P1: hasta 1,6 ms) se toma como el momento de la lectura, y **uno de un paso que ya corrió** (un evento que llegó tarde, o el *timestamp* inválido de un teclado virtual, P5) va al primer paso del frame. **La simulación recibe las acciones ya procesadas,** con estos campos por paso: **mover**, que es avance y giro (tanque) o una dirección del mundo (fija y rieles); **la guía**, que es la posición del stick normalizada y absoluta (0 es el rumbo; el cono la escala en la simulación) o el delta del mouse en grados, según el último dispositivo usado; y **los botones** con sus contadores. El recorte al cono se hace en la simulación. La sensibilidad, las curvas, la reasignación y la alternativa de mantener quedan fuera de la simulación. **El *replay* graba esas acciones solo en `debug` y `relwithdebinfo`** (ADR-0021); en Release no se compila. **Moverse:** **tanque en la tercera y la primera persona** (arriba avanza; izquierda y derecha giran el rumbo). El rumbo es estado de la simulación y cambia de un paso a otro, así que entran el avance y el giro, no una dirección. **En la fija y los rieles, relativo a la pantalla, manteniendo la dirección** mientras el stick no cambie, aunque la toma corte: la capa de input lo convierte en una dirección del mundo antes del paso, con la cámara del último frame, y la cámara sigue fuera de la simulación (ADR-0031). **Mientras apunta, la caballera puede caminar muy despacio**, bastante más lento que un zombie, sin girar el rumbo: adelante y atrás avanzan y retroceden, e **izquierda y derecha dan un paso lateral**. Solo para correcciones chicas. **Esquivar lee el stick respecto del rumbo:** salta adelante, atrás o a un lado, aunque caminar sea de tanque; en la fija y los rieles, hacia la dirección del mundo en que se mueve |
| 6 | *Buffer* y ventanas | **Un *buffer* corto por acción, en pasos** (CVars): de partida, 6 pasos (100 ms) para esquivar, el tajo rápido e interactuar. **Soltar la carga no tiene *buffer*.** Sin cola de acciones. Las ventanas del combate (ADR-0018) también se cuentan en pasos |
| 7 | Latencia | **El modo D:** antes de leer el input, el frame espera a que se muestre el frame de hace dos. La cola nunca tiene más de un frame esperando, y la CPU de un frame sigue en paralelo con la GPU del anterior (43 ms medidos, contra 82 del *game loop* de libro, a 60 FPS). **Una sola generación de extensiones en el engine:** `VK_KHR_present_wait2` con `VK_KHR_present_id2`, la misma que el ADR-0021 usa de respaldo para medir R3. El prototipo midió con la primera generación (`VK_KHR_present_wait` y `VK_KHR_present_id`); su ticket vuelve a medir con la segunda. **Si el dispositivo no las tiene, se cae al modo A.** El modo es una CVar, para medirlo con el protocolo del ADR-0020. En la demo, con D el cuadrado se queda pegado al cursor; con A funciona, pero se le escapa |
| 8 | Dispositivos | **Todos funcionan a la vez**, sin elegir. **Los íconos cambian** con un botón o con un eje que pasa la zona muerta, no con cualquier evento. Varios mandos se suman. **Pausa si se desconecta el mando en uso.** Solo la base de mandos que trae SDL3 |
| 9 | Vibración | **Desde los eventos del paso:** el impacto con su fuerza, el corte con su material, el agarre y la hoja clavada, traducidos una vez por frame, después del audio, a los dos motores. **Lee el arreglo de eventos del audio** (ADR-0024), que vive todo el frame: las colas del ADR-0012 mueren con el paso. Así no hace falta otra región, y los eventos ya traen la fuerza y el material. **Es de la presentación:** no entra al hash. **Nunca es la única señal**, se puede apagar y se regula la intensidad (XAG 110). Sin gatillos adaptativos. Cómo se siente queda para la prueba del autor |
| 10 | Accesibilidad, datos, pruebas y depuración | **Mantener o alternar** para apuntar y trotar, como opción. **La alternativa de mantener del ADR-0018 es un disparador de la capa:** el gameplay recibe el mismo progreso. **La sensibilidad de cada entrada analógica** (mouse, stick) va de 0,25× a 4×, más que el ±50 % de la XAG 107. **Una región de input con topes fijos** (ADR-0010): 2048 eventos por frame; **si se llena, se suman los deltas y ningún botón se pierde.** **TDD** (ADR-0011) para la zona muerta, la curva, la histéresis de los gatillos, la regla del paso, los contadores, el *buffer* en pasos, la lectura de los controles y los conflictos. **Un test de *replay* con otro horario de frames:** la misma traza **cruda** (los eventos con su *timestamp*, no las acciones grabadas) corrida con varios horarios de frames tiene que dar las mismas acciones por paso (desde el ticket de las piezas puras) y el mismo hash (desde el ticket de la guía, cuando existe la simulación que apunta). Es la prueba automática de que el mismo gesto da la misma espada a cualquier FPS; con las acciones grabadas pasaría siempre, aunque la regla estuviera mal. **Un panel de ImGui** con el dispositivo, el stick crudo y procesado, los eventos por frame y las acciones de cada paso |

## Consecuencias

### El camino de un evento

```
0. esperar a que se muestre el frame de hace dos        (modo D; sin present_wait2, nada)
1. platform: vaciar la cola de SDL → eventos propios con su timestamp
   input: contexto activo → controles → modificadores (zona muerta, curva,
          sensibilidad) → disparadores (mantener, alternar, presionar rápido)
          → las acciones de cada paso, según el timestamp de cada evento
          (lo que cae después del último paso espera al frame siguiente)
          (en debug y relwithdebinfo, el replay graba esas acciones)
2. cada paso: gameplay lee sus acciones (botones con contadores; mover: avance y
   giro, o una dirección del mundo; guiar: la posición del stick o el delta del mouse)
3. render ─→ 4. audio ─→ vibración: el arreglo de eventos del audio → los dos motores
```

**La capa de acciones** es un sistema encima de `engine/core`. `platform::` le da los
eventos con tipos propios (las teclas por posición, con la tabla de nombres propia), y
ella arma las acciones. El gameplay nunca ve una tecla ni un dispositivo.

### Lo que se gana

- **El mismo gesto da la misma espada a cualquier FPS** (P2: 0,000° de diferencia con la
  regla C), y el *replay* graba lo que la simulación vio de verdad.
- **Ningún toque se pierde,** ni en los frames con 0 pasos ni en los tirones.
- **La mitad de la latencia** del *game loop* de libro (43 contra 82 ms), sin perder el
  paralelismo de la CPU y la GPU.
- **El peso en un solo lugar:** la espada (ADR-0015). El input es crudo y lineal, así que
  el jugador siente su mano, y la espada le responde con su inercia.
- **Las opciones del jugador quedan fuera de la simulación:** cambiar la sensibilidad o la
  alternativa de mantener no toca el *replay* ni el hash.
- **Reasignar es cambiar una tabla:** el gameplay no sabe de teclas.

### Lo que se sacrifica

- **Medio paso de espera** (~8 ms a 60 Hz) para el movimiento que llega después del
  último paso completo.
- **Una dependencia de los *timestamps*** del sistema: si un protocolo los manda mal (un
  teclado virtual), el evento cae en el primer paso, como con la regla A.
- **El mouse y el trackpad comparten la sensibilidad** en Wayland.
- **Con el stick por posición, soltarlo devuelve el punto al rumbo.** Para dejar la mira
  quieta en un lugar hay que sostener el stick.
- **Un *replay* no dice qué teclas tocó el jugador,** solo qué acciones resultaron.
- **El modo D depende de `VK_KHR_present_wait2`.** Sin la extensión se cae al modo A, con
  el doble de latencia.
- **Con el mouse y el stick a la vez, manda el último usado:** no se suman. Pasar del
  stick al mouse deja el objetivo donde estaba; pasar del mouse al stick lo lleva a la
  posición del stick.

### Tickets, en orden

1. las piezas puras, en TDD: la zona muerta radial escalada, la curva, la histéresis de
   los gatillos, los contadores, la regla del paso por *timestamp* (con los *timestamps*
   del futuro y del pasado) y el *buffer* en pasos; y el test de la misma traza cruda con
   varios horarios de frames, que tiene que dar las mismas acciones por paso;
2. `platform::`: los eventos propios con su *timestamp*, la tabla de teclas por posición y
   el mouse relativo;
3. la capa de acciones: los contextos, los controles por defecto de la visión, los
   modificadores y los disparadores (junto al ADR-0018 para la alternativa de mantener);
4. el movimiento: tanque (avance y giro), el paso lateral al apuntar, esquivar respecto
   del rumbo, y relativo a la pantalla con la dirección mantenida, convertido a una
   dirección del mundo (junto a los ADR-0018 y ADR-0031);
5. la guía de la espada: el stick por posición y el mouse por deltas, hacia un objetivo,
   con el último dispositivo usado (junto a los ADR-0015 y ADR-0031); el test de la traza
   cruda suma aquí el mismo hash con varios horarios de frames;
6. el modo D con `VK_KHR_present_wait2` y `VK_KHR_present_id2`, medido otra vez con esta
   generación, y la caída al modo A (junto al ADR-0022);
7. la reasignación en las opciones y las líneas de `opciones.txt`, con los conflictos
   (junto a los ADR-0026 y ADR-0028);
8. los dispositivos: el cambio en caliente con el umbral, la pausa al desconectar y los
   íconos (junto al ADR-0026);
9. la grabación de las acciones en el *replay* (junto al ADR-0021);
10. la vibración desde el arreglo de eventos del audio, con su opción (junto al
    ADR-0024);
11. el panel de ImGui.

### Fe de erratas en otros ADRs

| ADR | Qué cambia |
|---|---|
| Visión | Versión 6: tanque en la tercera y la primera persona, relativo a la pantalla en la fija y los rieles; mientras apunta, camina muy despacio y da pasos laterales; esquivar respecto del rumbo; el stick marca una posición |
| ADR-0010 | La región de input (~70 KB). Llena, suma los deltas: la tercera excepción a abortar, y la única en Release |
| ADR-0012 | El orden del frame: esperar el frame de hace dos, leer los eventos con su *timestamp*, las acciones por paso y la vibración después del audio |
| ADR-0015 | La guía sigue un objetivo; el peso lo pone la espada, no el input |
| ADR-0018 | El stick por posición, caminar muy despacio al apuntar con pasos laterales, esquivar respecto del rumbo, los márgenes y el *buffer* en pasos, la alternativa de mantener como disparador |
| ADR-0024 | El arreglo de eventos del audio tiene un segundo lector: la vibración, después del audio y antes del fin del frame |
| ADR-0020 | Leer tarde no ganó; lo que baja la latencia es no dejar frames en la cola |
| ADR-0021 | La regla del input por paso; el *replay* graba las acciones procesadas, solo en desarrollo |
| ADR-0026 | La sensibilidad y la alternativa de mantener salen de la simulación; los íconos cambian con el umbral; los menús, fijos; el cursor, encerrado mientras la ventana tiene el foco |
| ADR-0031 | La guía recibe un objetivo; respecto de qué se mueve la caballera |
| `presupuestos.md` | "Plataforma e input" suma la capa de acciones y la vibración; la región de input cae en "Mundo y ECS" |

### Lo que queda abierto

- **La prueba del autor con un DualShock 4:** el tipo real, la deriva de su stick, el
  giroscopio (si se suma como opción) y cómo se siente la vibración.
- **Los valores de partida,** que se ajustan jugando (CVars): la zona muerta, la curva, la
  sensibilidad, el *buffer* de cada acción, la histéresis de los gatillos y la velocidad
  de caminar al apuntar.
- **El modo de latencia en otros compositores y en Windows:** el D se midió solo en
  Hyprland. Si otro compositor se comporta distinto, la CVar permite medirlo con el
  protocolo del ADR-0020.
- **La sensibilidad del trackpad aparte de la del mouse,** si algún jugador la pide: hace
  falta preguntarle al sistema por otro camino, fuera de SDL3.

## Cómo lo defiendo

**¿Por qué acciones y no teclas?**
Porque es lo estándar (Unreal, Unity y Godot lo hacen así), y deja la reasignación en un
paso simple: cambiar una tabla. El gameplay pregunta "¿apuntó?", no "¿apretó el clic
derecho?", así que cada mecánica se escribe una vez y vale para el teclado y para el
mando. Los contextos resuelven que la misma tecla haga una cosa en el juego y otra en el
menú, sin apagar nada a mano.

**Tu simulación corre a 60 Hz, pero el mouse manda eventos a 125 Hz y un frame puede
correr de 0 a 8 pasos. ¿Qué ve cada paso?**
Lo que pasó durante ese paso. Cada evento trae su *timestamp*, y lo mando al paso que
cubre ese instante; lo que llegó después del último paso completo espera al frame
siguiente. Probé cuatro reglas con una traza real de mi mouse, y esta fue la única que da
exactamente el mismo apunte a 30, 60 y 144 FPS. Las otras se desviaban hasta 39°, porque
el cono de apuntar recorta, y recortar no es lineal: el mismo movimiento partido de otra
manera termina en otro lugar. Unity hace lo mismo en su modo de input en `FixedUpdate`.

**¿Y qué te cuesta?**
Medio paso de espera, unos 8 ms a 60 Hz, para el movimiento que llega al final del
frame: pertenece a un paso que todavía no corrió. Lo acepto porque la otra opción es que
la espada termine en otro lugar según los FPS de cada jugador.

**¿Cómo sabes que el *timestamp* es confiable?**
Lo medí: en Hyprland, cada evento trae el suyo, separados 8 ms como reporta mi mouse, con
un error de unos 2 ms. Pero no siempre: un teclado virtual de Wayland mandó *timestamps*
inválidos. Por eso los acoto: uno en el futuro se toma como el momento de la lectura, y
uno de un paso que ya corrió va al primer paso del frame.

**¿Cómo te aseguras de no perder un toque rápido?**
Con contadores de presiones y de sueltas que se acumulan hasta que un paso los consume.
Mirar solo si el botón está apretado pierde los toques que empiezan y terminan entre dos
lecturas (el 29 % a 20 FPS con un congelamiento de 400 ms). Mirar el flanco del frame los pierde cuando el frame no
corre ningún paso, y a 144 FPS eso pasa en más de la mitad de los frames (el 54 %). Con
los contadores, ninguno.

**¿Por qué el mouse no tiene aceleración ni suavizado, si querías que la espada pese?**
Porque el peso no va en el input, va en la espada. Si filtro el mouse, sumo latencia y se
siente blando, y el jugador deja de sentir su mano. El input entrega el gesto crudo, en
grados por cuenta, y la espada lo sigue con su propia velocidad y su inercia (ADR-0015).
Y el delta del mouse nunca se multiplica por el dt: ya es un desplazamiento. Con ese
error, el mismo gesto daba 11° a 144 FPS y 40° a 30.

**¿Por qué el stick marca una posición y no una velocidad?**
Porque el stick dice **dónde**, no qué tan rápido: la rapidez del tajo sale de la carga.
El cono de apuntar tiene borde, así que el stick a fondo es justo ese borde, y al cargar
la línea apunta hacia donde apunta el stick. Soltarlo devuelve la mira al rumbo, pero
como la espada lo sigue con su peso, ese regreso es lento, no un salto.

**¿Por qué la zona muerta radial escalada?**
Porque es la única que no tiene defectos visibles. La axial se pega a los ejes: un
movimiento lento en diagonal se desvió hasta 42°. La radial salta de 0 a 0,2 al salir de
la zona, así que no hay movimientos chicos. La escalada reescala lo que queda a [0, 1], y
encima va una curva de exponente 2, que le da a los movimientos chicos el 42 % del
recorrido.

**¿Por qué guardas la posición de la tecla y no la letra?**
Porque en un teclado francés, la tecla donde yo tengo la W da una Z. Si guardo la letra,
WASD queda repartido por todo el teclado; si guardo la posición, es la misma forma bajo la
mano izquierda en cualquier distribución, y el menú muestra la letra que tiene esa tecla
para el jugador. Y no guardo el nombre que da SDL, porque su documentación avisa que no es
estable entre plataformas.

**¿De dónde sale la latencia, y cómo la bajaste?**
Casi toda sale de la cola de frames, no de leer el input. Con el *game loop* de libro, la
CPU se adelanta hasta llenar la cola, y el frame que leyó la tecla espera cuatro vsyncs:
82 ms del envío a la pantalla. Leer más tarde no ganó nada en mi compositor. Lo que
funciona es esperar, antes de leer el input, a que se muestre el frame de hace dos: 43
ms, la mitad. Es la idea de NVIDIA Reflex, con una extensión estándar de Vulkan
(`VK_KHR_present_wait2`; el prototipo midió con la primera generación). En una demo, con ese modo, el cuadrado se queda pegado al cursor
del sistema; con el *game loop* de libro, se le escapa.

**¿Por qué no esperar al frame anterior, que es todavía menos cola?**
Porque pone la CPU y la GPU en serie: las dos juntas tienen que caber en 16,7 ms, y mi
presupuesto le da 16,7 ms a cada una (ADR-0020). En la demo, ese modo perdía frames con
solo 6 ms de CPU: si la espera termina un poco después del vblank, el frame no llega al
siguiente. Esperar el de hace dos deja un frame de margen, y la CPU de un frame sigue en
paralelo con la GPU del anterior.

**¿Por qué el *replay* graba las acciones y no las teclas?**
Porque así lo que el jugador configura queda fuera de la simulación. La sensibilidad, las
curvas, la reasignación y la opción de mantener se aplican antes del paso; la simulación
recibe el movimiento (avance y giro, o una dirección del mundo), la guía (la posición del
stick o el delta del mouse) y los botones. Cambiar una opción
no toca el hash. Y el *replay* es una herramienta mía: solo existe en los builds de
desarrollo, nunca en Release.

**Si la cámara está fuera de la simulación, ¿cómo te mueves "hacia el fondo de la
pantalla" en una cámara fija?**
La capa de input convierte esa intención en una dirección del mundo **antes** del paso,
con la cámara del último frame, y eso es lo que entra a la simulación y lo que graba el
*replay*. La simulación nunca ve la cámara. En la tercera y la primera persona no hace
falta: son de tanque, y lo que entra es el avance y el giro, porque el rumbo es estado de
la simulación y cambia de un paso a otro. Así el jugador controla el rumbo, que es justo
el centro del cono de apuntar.

## Referencias

- Blizzard Entertainment (2019). *New Feature: High Precision Mouse Input (Gameplay
  Option)*. Foros de Overwatch.
  <https://us.forums.blizzard.com/en/overwatch/t/new-feature-high-precision-mouse-input-gameplay-option/422094>
- Epic Games. *Enhanced Input in Unreal Engine*. Documentación de Unreal Engine.
  <https://dev.epicgames.com/documentation/en-us/unreal-engine/enhanced-input-in-unreal-engine>
- Fiedler, G. (2004). *Fix Your Timestep!* Gaffer On Games.
  <https://gafferongames.com/post/fix_your_timestep/>
- Godot Engine. *InputMap* y *Input examples*. Documentación de Godot.
  <https://docs.godotengine.org/en/stable/tutorials/inputs/input_examples.html>
- Khronos Group. *VK_KHR_present_wait* y *VK_KHR_present_wait2*. Especificación de
  Vulkan. <https://registry.khronos.org/vulkan/specs/latest/man/html/VK_KHR_present_wait.html>
  y <https://registry.khronos.org/vulkan/specs/latest/man/html/VK_KHR_present_wait2.html>
- Microsoft. *Xbox Accessibility Guideline 107: Input*.
  <https://learn.microsoft.com/en-us/gaming/accessibility/xbox-accessibility-guidelines/107>
- Microsoft. *Xbox Accessibility Guideline 110: Haptic feedback*.
  <https://learn.microsoft.com/en-us/gaming/accessibility/xbox-accessibility-guidelines/110>
- NVIDIA. *NVIDIA Reflex*. <https://developer.nvidia.com/performance-rendering-tools/reflex>
- SDL. *SDL3 Wiki: CategoryKeyboard, CategoryMouse, CategoryGamepad*.
  <https://wiki.libsdl.org/SDL3/>
- Sutphin, J. (2013). *Doing Thumbstick Dead Zones Right*. Third Helix / Game Developer.
  <https://www.gamedeveloper.com/disciplines/doing-thumbstick-dead-zones-right>
- Swink, S. (2008). *Game Feel: A Game Designer's Guide to Virtual Sensation*. Morgan
  Kaufmann.
- Thorson, M. (2018). *Celeste & Forgiveness*.
  <https://maddymakesgames.com/articles/celeste_and_forgiveness/>
- Unity Technologies. *Input System: Input in Fixed Update mode*.
  <https://docs.unity3d.com/Packages/com.unity.inputsystem@1.15/manual/timing-optimize-fixed-update.html>
