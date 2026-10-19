# Presupuestos de rendimiento

Este es el **documento vivo** del ADR-0020. El ADR fija la máquina de referencia, los
requisitos R1 a R5 y el protocolo de medición. Aquí está el **reparto por sistema**, que
cambia a medida que se mide.

**Cómo se cambia:** solo con un PR que trae la medición hecha con el protocolo del
ADR-0020, y una línea nueva en el historial de abajo. Mientras un sistema no se haya
medido, su número es una **hipótesis**.

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
| Combate y corte (hasta 4 cortes en un paso) | 2,0 ms | Hipótesis |
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
| Pedazos, con un tope fijo | 32 MiB | Hipótesis |
| Arena por frame | 16 MiB | Hipótesis |
| Subida de geometría dinámica, por frame en vuelo | N × 8 MiB | Hipótesis. N, los frames en vuelo, lo fijan el ADR-0007 y el ADR-0010 (ADR-0005). Se supone N = 2 |
| Audio | 64 MiB | Hipótesis |
| Ejecutable, driver y reserva | el resto, hasta 1 GiB | — |

Cómo se implementan las arenas, los pools y los ring buffers lo decide el ADR-0010.

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
