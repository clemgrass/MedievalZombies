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

Con un hilo (ADR-0012), los sistemas se suman. Hay dos tipos de costo:

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
| Gameplay e IA (50 zombies) | 1,3 ms | Hipótesis |
| Combate y corte (hasta 4 cortes en un paso) | 2,0 ms | Hipótesis. El gasto de la fuerza del ADR-0018 midió ~60 µs por zombie en todo el barrido, en un prototipo sin optimizar |
| Física (zombies y ~100 pedazos) | 1,5 ms | Hipótesis |
| Animación (50 esqueletos) | 1,2 ms | Hipótesis |
| **Total por paso** | **6,0 ms** | |

### Por frame

| Sistema | Meta | Estado |
|---|---|---|
| Plataforma e input | 0,3 ms | Hipótesis |
| Render en la CPU (culling, comandos y subida de geometría) | 2,5 ms | Hipótesis |
| Audio | 0,5 ms | Hipótesis |
| Reserva | 0,5 ms | — |
| **Total por frame** | **3,8 ms** | |

### Cómo quedan los frames

| Frame | Cálculo | Total | Límite |
|---|---|---|---|
| Con un paso | 6,0 + 3,8 | 9,8 ms | Meta: 13,3 ms |
| Con dos pasos | 2 × 6,0 + 3,8 | 15,8 ms | Obligatorio: 16,7 ms |

## Frame de GPU

Meta total: **13,3 ms** (R2), a la resolución interna de 640×360 más el escalado a la
salida. El reparto entre pasadas se agrega cuando el ADR-0022 defina el pipeline.

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
| Ejecutable, driver y reserva | el resto, hasta 1 GiB | — |

Las filas de pedazos, arenas temporales y subida son regiones del ADR-0010 que usa el
frame: se reservan y se tocan enteras al iniciar, con un tope duro. Las demás regiones
del ADR-0010 (pedidos de corte, cuerpos y ragdolls, ECS) caen en "Mundo y ECS", y
también se reservan y se tocan al iniciar. La
memoria que vive un nivel (recursos de la GPU, assets y audio) la decide el ADR-0028.
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
| 2026-10-22 | #31 | El costo del combate del ADR-0018 (~60 µs por zombie y por tajo) queda anotado en la fila de combate y corte. Ninguna meta cambia | Prototipo de laboratorio, fuera del protocolo |
