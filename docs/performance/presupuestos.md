# Presupuestos de rendimiento

Este es el **documento vivo** del ADR-0020. El ADR fija la máquina de referencia, los
requisitos R1 a R5 y el protocolo de medición. Aquí está el **reparto por sistema**, que
cambia a medida que se mide.

**Cómo se cambia:** solo con un PR que trae la medición hecha con el protocolo del
ADR-0020, y una línea nueva en el historial de abajo. Mientras un sistema no se haya
medido, su número es una **hipótesis**. Una nota que solo cita la decisión de un ADR
aceptado, sin cambiar ninguna meta, se actualiza en el PR de ese ADR, con su línea en el
historial y "Ninguna" en la columna de medición.

**La medición de un spike:** los spikes no se mergean (ADR-0014), así que su medición
queda en el issue del spike. Un PR `docs:` aparte, desde una rama normal, la pasa a este
documento y la cita en el historial. Si el spike no pudo seguir todo el protocolo (por
ejemplo, sin escenarios grabados), el número sigue marcado como hipótesis.

## Frame de CPU

Con un hilo (ADR-0012), los sistemas se suman. El hilo de audio del ADR-0024 corre en
otro núcleo y tiene su propia meta, más abajo. Hay dos tipos de costo:

- **por paso de simulación:** se paga una vez por cada paso del acumulador. Lo normal es
  uno por frame, pero un frame puede tener de cero a ocho (ADR-0012);
- **por frame:** se paga una vez, tenga el frame los pasos que tenga.

**Regla de coherencia:** un frame con **dos pasos** tiene que caber en 16,7 ms (el nivel
obligatorio de R1), y un frame con un paso, en la meta de 13,3 ms. La pantalla va a
59,98 Hz y la simulación a 60 Hz, así que un frame con dos pasos aparece cada ~50 s y
después de cualquier tirón. Si no cabe, el frame siguiente también tiene dos pasos y el
juego queda pegado a 30 fps. Después de un tirón largo puede haber más pasos, y el loop
se recupera solo mientras un paso cueste menos que su duración (6,0 ms contra 16,7 ms).

### Por paso de simulación

| Sistema | Meta | Estado |
|---|---|---|
| Gameplay e IA (50 zombies) | 1,3 ms | Hipótesis. La IA y la navegación del ADR-0023 (decidir, caminos en la navmesh y ORCA) midieron ~0,1 ms por paso con 50 zombies (p99 ~0,11 ms), en un prototipo 2D sin el gameplay real |
| Combate y corte (hasta 4 cortes en un paso) | 2,0 ms | Hipótesis. El gasto de la fuerza del ADR-0018 midió ~60 µs por zombie en todo el barrido, en un prototipo sin optimizar |
| Física (zombies y ~100 pedazos) | 1,5 ms | Hipótesis |
| Animación (50 esqueletos) | 1,2 ms | Hipótesis |
| **Total por paso** | **6,0 ms** | |

### Por frame

| Sistema | Meta | Estado |
|---|---|---|
| Plataforma e input | 0,3 ms | Hipótesis |
| Render en la CPU (culling, comandos y subida de geometría) | 2,5 ms | Hipótesis |
| Audio en el hilo principal (eventos, emisores, sonido 3D y comandos, ADR-0024) | 0,5 ms | Hipótesis |
| VFX: integrar, crear marcas, ordenar y escribir los registros de dibujo (ADR-0025) | 0,2 ms | Hipótesis. El prototipo del ADR-0025 midió ~0,005 ms con la demo de un corte (~200 partículas vivas) y 0,24 ms con 20 000; cada marca recortada, 2 µs. La integración y las marcas corren en cada paso, así que un frame con dos pasos las paga dos veces (ver abajo) |
| UI: armar el HUD, los subtítulos y los menús (ADR-0026) | 0,1 ms | Hipótesis, sobre la mediana. El prototipo del ADR-0026 midió, a 640×360, ~34 µs el HUD con subtítulos y de 23 a 51 µs el menú de pausa (mediana), con un p99 de 108 µs al navegar el menú. El menú solo aparece en la pausa, con 0 pasos de simulación, así que en esos frames sobra CPU |
| Reserva | 0,2 ms | — |
| **Total por frame** | **3,8 ms** | |

### Cómo quedan los frames

| Frame | Cálculo | Total | Límite |
|---|---|---|---|
| Con un paso | 6,0 + 3,8 | 9,8 ms | Meta: 13,3 ms |
| Con dos pasos | 2 × 6,0 + 3,8 + 0,2 | 16,0 ms | Obligatorio: 16,7 ms |

El 0,2 de más es la integración de los VFX del segundo paso (ADR-0025): está en la tabla
por frame, pero corre en cada paso.

### Hilo de audio

El callback del ADR-0024 mezcla en el hilo de audio de SDL3, en otro núcleo, así que no
se suma al frame. Su límite es otro: cada llamada tiene que terminar antes de que el
dispositivo pida la siguiente (10,7 ms con el buffer de 512), o el audio se corta.

| Qué | Meta | Estado |
|---|---|---|
| Mezcla (64 voces), música y limitador, por cada 16,7 ms de audio | 0,5 ms | Hipótesis. El prototipo del ADR-0024 midió 0,22 ms con 64 voces y ~0,05 ms para decodificar la música |

## Frame de GPU

Meta total: **13,3 ms** (R2), a la resolución interna de 640×360 más el escalado a la
salida. El reparto sigue las pasadas del ADR-0022.

| Pasada | Meta | Estado |
|---|---|---|
| Color: 50 zombies, escenario y ~100 pedazos (640×360) | 7,0 ms | Hipótesis. El prototipo del ADR-0022 midió 4,5 ms con la GPU fija en 700 MHz |
| VFX dentro de la pasada de color: partículas, marcas y estela (ADR-0025) | 0,5 ms | Hipótesis. El prototipo del ADR-0025 midió ~0,23 ms con la demo de un corte (partículas 0,03, marcas 0,2) con la GPU fija en 700 MHz |
| Dibujo de depuración y postproceso (640×360) | 0,5 ms | Hipótesis. Los *frames* de impacto del ADR-0025 midieron 0,14 ms, solo en los frames en que salen |
| Blit *nearest* a la pantalla | 2,0 ms | Hipótesis. El prototipo midió 1,3–1,6 ms a 700 MHz |
| UI a 640×360: HUD, subtítulos y menús (ADR-0026) | 0,2 ms | Hipótesis. El prototipo del ADR-0026 midió 0,02 ms el HUD con subtítulos y 0,14 ms el menú de pausa, con el reloj libre (~300 MHz en promedio) |
| ImGui (solo en desarrollo) | 0,5 ms | Hipótesis |
| Reserva | 2,6 ms | — |
| **Total** | **13,3 ms** | |

## Memoria

Meta total: **1 GiB** (R4). Como la GPU usa la misma RAM, la suma incluye las dos, y
cada byte se cuenta una sola vez: la RAM anónima del proceso más la memoria de Vulkan.

| Qué | Meta | Estado |
|---|---|---|
| Recursos de la GPU (mallas, texturas y buffers) | 384 MiB | Hipótesis |
| Assets en la CPU | 256 MiB | Hipótesis |
| Mundo y ECS | 64 MiB | Hipótesis |
| Pedazos, con un tope fijo | 32 MiB | Hipótesis. La cuenta del ADR-0010 (pico vivo simulado, lo que espera liberarse en 2 frames en vuelo y margen de fragmentación) da ~9 MiB |
| Arenas temporales: la del paso, la del frame y las *scratch* (ADR-0010) | 16 MiB | Hipótesis. El reparto entre las tres se fija midiendo |
| Subida de geometría dinámica, por frame en vuelo | N × 8 MiB | Hipótesis. N = 2 frames en vuelo (ADR-0007), así que son 2 × 8 MiB. Incluye las matrices de los huesos: 74 KB por frame con 50 zombies (ADR-0017). La cuenta del ADR-0010 para el peor caso (32 cortes, las matrices de los zombies, 10 ragdolls y 100 pedazos rígidos) da ~2,3 MB por frame |
| Audio | 64 MiB | Hipótesis |
| Herramientas de depuración (solo `debug` y `relwithdebinfo`, ADR-0021) | 8 MiB de región + ~14 MiB de Tracy | La región es una hipótesis: log (~0,5 MiB), dibujo de depuración (~1 MiB), ImGui (unos MiB) y los bloques del registro y del *replay* (2 × 64 KiB por flujo; se vuelcan a disco, así que no crecen con la sesión). Tracy (*on demand*, sin servidor) se midió en laboratorio, fuera del protocolo: ~14 MiB de RAM anónima. Las dos cuentan en R4 porque el protocolo mide en `relwithdebinfo`; en Release no existen |
| Ejecutable, driver y reserva | el resto, hasta 1 GiB | — |

Las filas de pedazos, arenas temporales y subida son regiones del ADR-0010 que usa el
frame: se reservan y se tocan enteras al iniciar, con un tope duro. Las demás regiones
del ADR-0010 (pedidos de corte, cuerpos y ragdolls, ECS y las partículas de los VFX,
~330 KB) caen en "Mundo y ECS", y también se reservan y se tocan al iniciar. Las marcas
de sangre (384 KB) y los campos de distancia de las tapas (512 KB) del ADR-0025 son
bloques de la GPU que viven todo el programa y caen en "Recursos de la GPU". La región
de UI del ADR-0026 (~200 KB, con las tablas de las fuentes y de los textos) cae en "Mundo y ECS", y el atlas de sus fuentes (~165 KB),
en "Recursos de la GPU"; sus vértices viajan por la subida (160 KB por frame). Con el
ADR-0027, la memoria que antes vivía "un nivel" vive **una zona**, y se dimensiona por el
tope de zonas cargadas a la vez (5, de partida): los recursos de la GPU de cada zona caen
en "Recursos de la GPU", y sus datos en la CPU (instancias, colisión, navmesh,
marcadores, fases y triggers; ~11 KB de ECS por zona en el prototipo, con mallas de
juguete), en "Mundo y ECS". Cómo se reparte y se carga (recursos de la GPU, assets y
audio) lo decide el ADR-0028.
Las cuentas del ADR-0010 son estimaciones, no mediciones del protocolo: las metas
cambian recién con una medición.

## Capacidad

| Qué | Obligatorio (R5) | Punto de quiebre medido |
|---|---|---|
| Zombies activos | 50 | Sin medir |
| Pedazos a la vez | 100 | Sin medir |
| Cortes en un mismo paso de simulación | 4 | Sin medir |
| Cortes por segundo | 8 | Sin medir |

## Historial

| Fecha | Ticket | Cambio | Medición |
|---|---|---|---|
| 2026-10-19 | #34 | Reparto inicial (hipótesis) | Ninguna: todavía no hay código |
| 2026-10-20 | #11 | N = 2 frames en vuelo, fijado por el ADR-0007, y las matrices de los huesos (74 KB por frame, ADR-0017) anotadas en la fila de subida. Ninguna meta cambia | Ninguna: es una decisión, no una medición |
| 2026-10-21 | #19 | Las filas de pedazos, arenas temporales y subida pasan a ser regiones del ADR-0010; la memoria que vive un nivel queda para el ADR-0028. La arena por frame se vuelve "arenas temporales" (paso, frame y *scratch*), y las cuentas del ADR-0010 quedan anotadas en las filas de pedazos y de subida. Ninguna meta cambia | Ninguna: son una decisión y cuentas, no una medición del protocolo |
| 2026-10-22 | #31 | El costo del combate del ADR-0018 (~60 µs por zombie y por tajo) queda anotado en la fila de combate y corte. Ninguna meta cambia | Ninguna: prototipo de laboratorio, fuera del protocolo |
| 2026-10-28 | #43 | Fila nueva: las herramientas de depuración del ADR-0021 (región de 8 MiB, hipótesis, más ~14 MiB de Tracy), que salen de "Ejecutable, driver y reserva". Ninguna otra meta cambia | Ninguna del protocolo: la región es una cuenta, y lo de Tracy, un prototipo de laboratorio |
| 2026-11-01 | #51 | La CI del ADR-0029 usa las filas de memoria por región como límite del *replay*, compilado en `relwithdebinfo`: falla si un pico las pasa. Las filas de capacidad no son un límite, porque R5 es un mínimo. Los tiempos no se miden en la CI. Ninguna meta cambia | Ninguna: es una decisión, no una medición |
| 2026-11-01 | #36 | Reparto del frame de GPU entre las pasadas del ADR-0022: color 7,0 ms, depuración y postproceso 0,5, blit 2,0, ImGui 0,5 y reserva 3,3. El total no cambia | Ninguna del protocolo: un prototipo de laboratorio con la GPU fija en 700 MHz |
| 2026-11-01 | #37 | El costo de la IA y la navegación del ADR-0023 (~0,1 ms por paso con 50 zombies) queda anotado en la fila de gameplay e IA. Ninguna meta cambia | Ninguna del protocolo: prototipo de laboratorio, fuera del protocolo |
| 2026-11-01 | #38 | La fila de audio por frame pasa a ser "Audio en el hilo principal", con la misma meta. Meta nueva: el hilo de audio del ADR-0024, 0,5 ms por cada 16,7 ms de audio, en otro núcleo (hipótesis). Ninguna otra meta cambia | Ninguna del protocolo: prototipo de laboratorio, fuera del protocolo |
| 2026-11-02 | #39 | Metas nuevas para los VFX del ADR-0025 (hipótesis): 0,2 ms de CPU por frame, que salen de la reserva del frame (de 0,5 a 0,3 ms), y 0,5 ms de GPU, que salen de la reserva de GPU (de 3,3 a 2,8 ms). Los *frames* de impacto entran en la fila de postproceso. Como la integración corre en cada paso, el frame con dos pasos pasa de 15,8 a 16,0 ms. La memoria de los VFX cae en "Mundo y ECS" (partículas, ~330 KB) y en "Recursos de la GPU" (marcas y campos de distancia, ~0,9 MB). Los totales no cambian | Ninguna del protocolo: prototipo de laboratorio con la GPU fija en 700 MHz |
| 2026-11-02 | #40 | Metas nuevas para la UI del ADR-0026 (hipótesis): 0,1 ms de CPU por frame, que salen de la reserva del frame (de 0,3 a 0,2 ms), y 0,2 ms de GPU, que salen de la reserva de GPU (de 2,8 a 2,6 ms). La región de UI (~200 KB) cae en "Mundo y ECS" y el atlas de las fuentes (~165 KB), en "Recursos de la GPU". Los totales no cambian | Ninguna del protocolo: prototipo de laboratorio con el reloj de la GPU libre |
| 2026-11-02 | #41 | Con el ADR-0027, la memoria que vivía "un nivel" vive una zona, dimensionada por el tope de zonas cargadas a la vez (5, de partida): los recursos de la GPU de cada zona caen en "Recursos de la GPU" y sus datos en la CPU, en "Mundo y ECS". Ninguna meta ni total cambia | Ninguna |
