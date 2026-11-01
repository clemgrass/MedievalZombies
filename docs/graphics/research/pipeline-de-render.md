# Mediciones del pipeline de render

Nota de investigación del [ADR-0022](../adr/0022-pipeline-de-render.md). Guarda las
mediciones que sostienen la decisión, para poder repetirlas.

Fecha: 2026-11-01, en la máquina de referencia del ADR-0020 (Dell XPS 13 9360,
i7-8550U, UHD 620, 8 GB de RAM). Son mediciones de laboratorio para decidir, no del
protocolo del ADR-0020. El prototipo es código descartable, fuera del repo.

| Qué | Versión |
|---|---|
| Linux / glibc | 7.2.8 / 2.44 |
| GCC | 16.2.1, con `-O2` |
| Mesa (ANV) | 26.2.5, Vulkan 1.4.354 |
| glslc (shaderc) | 2026.4, `--target-env=vulkan1.3 -O` |
| Vulkan-Headers / validation layers | 1.4.363 / 1.4.363 |
| SDL3 | 3.4.18 |
| Sesión | Wayland (Hyprland 0.56.2), *direct scanout* apagado |
| Blender | 5.2.2, para exportar el maniquí y cortarlo |

## El prototipo

Un programa en C++20 con Vulkan 1.3 (*dynamic rendering* y *synchronization2*, como
pide el ADR-0005) que dibuja una escena del corte vertical. Corre **sin ventana**: dibuja
a una imagen de 640×360 y la escala a otra de 3200×1800, la resolución de la pantalla,
con un ritmo de 60 Hz simulado. Para la prueba del compositor (sección 9) abre una
ventana real con SDL3.

**La escena:**

- **50 zombies** con skinning LBS en el vertex shader (ADR-0015). Son el maniquí
  masculino de Quaternius (*Universal Animation Library 2*, CC0), diezmado en Blender a
  **3269 vértices y 4534 triángulos**: el tamaño de un zombie del juego según el
  ADR-0007, que estima ~3 veces menos vértices que el maniquí completo (8546). Cada uno
  camina con `Zombie_Walk_Fwd_Loop` (32 poses), con su propia fase.
- **100 pedazos** en el piso, armados desde la misma malla con un plano al azar: los
  triángulos de un lado, los vértices nuevos del borde y una tapa en abanico. Suman
  9575 vértices nuevos. No es el corte real del ADR-0007: sirve para medir cómo se
  dibujan, no para verse bien.
- Un piso de 40×40 m, 8 columnas, hasta 16 antorchas puntuales con parpadeo, una luz de
  luna direccional y niebla.
- Texturas procedurales de 64×64 en `R8G8B8A8_SRGB` (ADR-0017).
- Profundidad `D32_SFLOAT` con reverse-Z y plano lejano infinito (ADR-0008). 2 frames
  en vuelo (ADR-0007).

**Qué mide:** timestamps de la GPU (83 ns) antes y después de cada pasada, con
`VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT`, y el tiempo de CPU que toma grabar los
comandos. 120 frames de calentamiento y 600 medidos (10 s). Cada configuración corre
**dos veces**, en dos vueltas completas de la matriz, para ver el ruido.

**Las validation layers** pasaron sin errores en todas las configuraciones antes de
medir.

## 1. La frecuencia de la GPU (el hallazgo que obligó a repetir todo)

La primera vuelta de la matriz se midió con la frecuencia de la GPU libre, como la deja
el driver. **No sirvió para comparar.** A 60 Hz, a la GPU le sobra tiempo, así que el
driver la baja para ahorrar energía, entre 300 y 857 MHz según la corrida. La misma
configuración dio tiempos muy distintos:

| Configuración | Vuelta 1 | Vuelta 2 | Frecuencia promedio (1 / 2) |
|---|---|---|---|
| Forward, malla completa | 11,35 ms | 17,28 ms | 320 / 612 MHz |
| Prepass | 12,21 ms | 12,18 ms, con p99 de 25,3 ms | 517 / 568 MHz |
| Sin pedazos | 6,33 ms | 7,60 ms | 333 / 322 MHz |

La frecuencia que se lee en `gt_act_freq_mhz` tampoco sirve para corregir: es una muestra
cada 15 frames, y entre ráfagas cortas la GPU sube y baja.

**La solución:** fijar la frecuencia con `gt_min_freq_mhz = gt_max_freq_mhz = 700`, en
`/sys/class/drm/card1/` (requiere root). Con el reloj fijo, **las dos vueltas coinciden
dentro del 1 %**. Todos los números de esta nota, salvo los de esta sección, son con el
reloj fijo en 700 MHz. El máximo de la UHD 620 es 1150 MHz, así que el juego real corre
más rápido en lo que depende del reloj. Para comparar técnicas solo importan las
diferencias.

Es la misma práctica que el *stable power state* de los profilers de GPU de otros
fabricantes: fijar el reloj para comparar, y medir con el reloj libre para saber cómo
corre de verdad.

## 2. La arquitectura (P1)

Mediana del tiempo de GPU por frame, en ms. "Principal" es la pasada de color (en
deferred, la del G-buffer); "pedazos" está dentro de ella. Escalado con una pasada
propia.

| Arquitectura | Prepass | Principal | Pedazos | Luz | Escalado | **Total** | p99 |
|---|---|---|---|---|---|---|---|
| Forward | — | 4,55 | 2,04 | — | 2,40 | **6,97** | 7,15–8,01 |
| Forward + *depth prepass* | 3,04–3,08 | 4,38 | 2,02 | — | 2,40 | **9,84–9,90** | 10,2–10,9 |
| Deferred (G-buffer `RGBA8_SRGB` + `A2B10G10R10`) | — | 3,80–3,82 | 1,89 | 0,48 | 2,40 | **6,68–6,72** | 6,96–6,99 |
| Forward, malla de 8546 vértices | — | 10,42–10,46 | 5,02 | — | 2,41 | **12,83–12,88** | 13,2–13,3 |
| Deferred, malla de 8546 vértices | — | 9,30 | 4,65 | 0,50 | 2,40 | **12,21–12,22** | 12,6–12,7 |

Lo que muestra:

- **La escena está limitada por los vértices.** Con 2,6 veces más vértices por zombie,
  la pasada principal cuesta 2,3 veces más. Los píxeles casi no pesan: 640×360 son
  230 400 píxeles.
- **El *depth prepass* es el peor:** dibuja dos veces la geometría (y hace dos veces el
  skinning) para ahorrar sombreado de píxeles, que aquí es barato.
- **Deferred gana por 0,25 a 0,30 ms** con 8 luces: su pasada principal no calcula luz,
  y la pasada de luz cuesta 0,48 ms porque hay pocos píxeles. Pero pierde la luz por
  vértice (sección 4) y complica las transparencias (no hay luz en el G-buffer para lo
  semitransparente).
- **El G-buffer no pesa a esta resolución:** 2 imágenes de 0,9 MB.

## 3. El escalado y la salida (P2)

| Configuración | Escalado | Total |
|---|---|---|
| Pasada propia a pantalla completa (`texelFetch` del píxel que toca) | 2,39–2,44 | 6,97–7,00 |
| `vkCmdBlitImage2` con `VK_FILTER_NEAREST` | **1,62–1,63** | 6,14–6,18 |
| Sin escalar | 0 | 4,54 |
| Escena en `R16G16B16A16_SFLOAT` (HDR), con pasada propia | 2,42–2,44 | 6,99–7,00 |

- **El escalado es más de un tercio del frame.** Escribir 3200×1800×4 B son 23 MB por
  frame; la escena entera escribe 0,9 MB de color. Lo anticipó el ADR-0020.
- **El blit es 0,77 ms más barato** que la pasada propia. Usa el hardware de copia, y la
  pasada propia paga un fragment shader por cada uno de los 5,76 M de píxeles.
- **La escena en HDR no cambia nada:** a 640×360, pasar de 4 a 8 B por píxel no se nota.
  Con HDR, el tonemapping va en la pasada de escalado (un blit no puede hacerlo).

La prueba con ventana real, y la del compositor, están en la sección 9.

## 4. La luz y las sombras (P3)

| Configuración | Principal | Total |
|---|---|---|
| 4 antorchas, por píxel | 4,29–4,31 | 6,71–6,72 |
| 8 antorchas, por píxel | 4,55 | 6,97 |
| 16 antorchas, por píxel | 5,14–5,16 | 7,57 |
| 8 antorchas, **por vértice** (Gouraud) | 4,20 | 6,62–6,63 |
| 16 antorchas, **por vértice** | 5,40–5,41 | 7,81–7,82 |
| 16 antorchas, deferred | 3,81 + luz 0,77–0,78 | 6,98 |
| Shadow map 512² de la luna (pasada aparte) | 4,58–4,59 + sombra 3,02–3,04 | 9,99–10,04 |
| Shadow map 1024² de la luna | 4,58 + sombra 3,12–3,13 | 10,11–10,13 |

- **Cada antorcha cuesta ~0,07 ms** por píxel. Con 16, forward y deferred empatan.
- **La luz por vértice no es más barata.** Con 8 antorchas ahorra 0,35 ms, y con 16
  cuesta 0,25 ms más: hay más vértices que píxeles con geometría. Elegirla es una
  decisión de estética (el sombreado facetado de la PS1), no de rendimiento.
- **El shadow map cuesta 3 ms con cualquier tamaño:** el costo es volver a hacer el
  skinning de los 50 zombies y los 100 pedazos, no rasterizar. Son 23 % del presupuesto
  de GPU del ADR-0020 (13,3 ms).

## 5. El look PSX (P4)

| Configuración | Principal | Total |
|---|---|---|
| Base: *nearest*, sin mipmaps | 4,55 | 6,97 |
| Niebla + *dithering* a 15 bits + *snapping* de vértices a 320×180 | 4,40 | 6,80–6,81 |
| Lo anterior + texturas afines (`noperspective`) | 4,40 | 6,80–6,81 |
| Filtro lineal + mipmaps (el contraste moderno) | 4,54 | 6,95–6,96 |

**El look PSX es gratis:** las diferencias son ruido. Se elige solo por cómo se ve. Las
capturas y el video comparan cada efecto (sección 10).

Cómo se hizo cada efecto en el prototipo:

| Efecto | Implementación |
|---|---|
| Vértices que tiemblan | En el vertex shader, la posición en NDC se redondea a una grilla (320×180 en el prototipo) y se vuelve a multiplicar por `w` |
| Texturas afines | El calificador `noperspective` en la coordenada de textura: se interpola en pantalla, sin corregir la perspectiva, como en la PS1. Se nota en el piso, con triángulos grandes |
| *Dithering* a 15 bits | Al final del fragment shader: un umbral de una matriz de Bayer de 4×4 y cuantización a 5 bits por canal |
| Niebla | Lineal por distancia a la cámara, hacia un color frío |
| Mipmaps | Generados en la CPU promediando en espacio lineal (ADR-0017). Sin mipmaps, el piso lejano "brilla" con ruido (*aliasing*) |

## 6. Dibujar un pedazo (P6)

100 pedazos sobre 50 zombies, forward. "Pedazos" es el tiempo de GPU de sus llamadas;
"CPU" es grabar todos los comandos del frame.

| Estrategia | Llamadas por pedazo | Pedazos (GPU) | CPU |
|---|---|---|---|
| Sin pedazos | — | — (principal 2,46) | 0,134–0,135 |
| **Mismo `VkBuffer`**, índices absolutos de 32 bits | 1 | 2,04–2,05 | 0,173–0,176 |
| **Dos llamadas**: el buffer del modelo + uno chico con copias de los vértices del borde | 2 | 2,00–2,01 | 0,204–0,206 |
| ***Vertex pulling***: el vertex shader lee dos buffers por índice (bit 31 = vértice nuevo) | 1 | **2,90** | 0,174–0,182 |

- **"Mismo buffer" y "dos llamadas" empatan en la GPU.** Dos llamadas solo agregan
  ~0,03 ms de CPU por cada 100 pedazos.
- **El *vertex pulling* es 42 % más lento en esta GPU:** se salta el hardware que lee los
  atributos de vértice y los saca de un buffer genérico con una estructura de 44 B.
- **100 pedazos cuestan lo mismo que 50 zombies** (2,04 ms contra 2,09 ms del resto de
  la pasada). A los pedazos rígidos se les sigue haciendo skinning con la pose
  congelada.

## 7. Datos a los shaders (P7)

Lo que soporta la UHD 620 con Mesa 26.2.5 (`vulkaninfo`):

| Capacidad | Soporte |
|---|---|
| *Descriptor indexing* (bindless): `runtimeDescriptorArray`, `descriptorBindingPartiallyBound`, índices no uniformes, *update after bind* | Sí. Hasta 201 M de imágenes por etapa |
| *Buffer device address* | Sí |
| *Push descriptors* (`VK_KHR_push_descriptor`, núcleo en 1.4) | Sí, hasta 32 |
| `VK_EXT_descriptor_buffer` | Sí |
| *Dynamic rendering local read* | Sí |
| *Push constants* | 256 B |
| *Multi draw indirect*, `drawIndirectFirstInstance`, `shaderDrawParameters` | Sí |
| Memoria | Toda compartida: hay tipos `DEVICE_LOCAL` + `HOST_VISIBLE` + `HOST_COHERENT` |

Tres formas de que cada llamada sepa su objeto (matriz, paleta de huesos, textura),
con dos llamadas por pedazo: **259 llamadas** por frame.

| Modelo | Por llamada | Principal (GPU) | CPU del frame | CPU de las llamadas |
|---|---|---|---|---|
| **Clásico:** un *descriptor set* por textura, con un UBO dinámico por objeto | `vkCmdBindDescriptorSets` con un *offset* | **4,16–4,19** | 0,253–0,262 | 0,169–0,171 |
| **Bindless:** un arreglo de objetos en un SSBO y un arreglo de texturas; el índice va en un *push constant* | `vkCmdPushConstants` (4 B) | 4,53–4,54 | 0,201–0,207 | 0,116–0,118 |
| **BDA:** la dirección del objeto va en un *push constant* (`buffer_reference`); texturas bindless | `vkCmdPushConstants` (8 B) | 4,28–4,29 | 0,206–0,209 | 0,120–0,123 |

- **En la GPU gana el clásico, por 0,35 ms** sobre bindless. En la GPU de Intel, un UBO
  se lee por un camino de constantes más rápido que un SSBO indexado.
- **En la CPU gana bindless**, pero los tres están lejos del presupuesto: grabar el frame
  entero cuesta 0,2–0,26 ms de los 2,5 ms del ADR-0020 para el render en la CPU.
- **BDA queda en el medio en la GPU** y es el más simple de grabar.

## 8. La tapa del corte (P5)

El maniquí completo, en la pose 8 de `Zombie_Walk_Fwd_Loop`, cortado en Blender por un
plano oblicuo a la altura de la cintura (`bmesh.ops.bisect_plane` y relleno de los
contornos). Cada mitad lleva su tapa (193 triángulos) con el material 2. Tres técnicas
en el fragment shader, con la cámara cerca:

| Técnica | Cómo funciona | Cómo se ve |
|---|---|---|
| **Textura con UV plana** | La UV es la posición proyectada en la base `(u, v)` del plano, la del ADR-0007, ×4 (una repetición cada 25 cm) | Carne roja con fibras. Sin hueso ni capas: la textura no sabe qué hay debajo |
| **Procedural según la distancia al hueso** | Para cada píxel de la tapa, la distancia al segmento de hueso más cercano (65 segmentos, de la pose): médula, hueso, borde del hueso o carne con ruido | Se ve el hueso **en su lugar**: la columna en la cintura, el radio en el antebrazo. Lee 65 segmentos por píxel de tapa; las tapas son pocos píxeles |
| **Triplanar** | Tres proyecciones por los ejes, mezcladas según la normal | Igual a la UV plana, con algo de deformación: **una tapa es plana**, así que la triplanar no aporta nada |

Lo que mostró, además:

- **Las mallas tienen que ser *manifold*.** La única arista no *manifold* del torso del
  maniquí (la misma del ADR-0007) dejó el contorno de la cintura sin cerrar, y la tapa
  no se rellenó. Hubo que soldar vértices y rellenar a mano. Lo confirma la regla del
  cocinador del ADR-0007.
- **Las capas por profundidad** (piel, grasa, carne; metal y carne en los zombies con
  armadura) **necesitan un dato que la tapa no tiene:** la distancia de cada punto al
  contorno. La tapa que arma el corte solo tiene vértices en el borde. Opciones: que el
  corte agregue puntos interiores con esa distancia, o calcular un campo de distancia 2D
  en la tapa. Ninguna se probó.

## 9. El escalado del compositor (P2, con ventana)

Prototipo con ventana SDL3 a pantalla completa, swapchain FIFO con 3 imágenes. Dos
modos:

- **`native`:** swapchain de 3200×1800 (`SDL_WINDOW_HIGH_PIXEL_DENSITY`); escala el
  engine con un blit *nearest*.
- **`small`:** swapchain de 640×360; Wayland deja que la aplicación elija el tamaño del
  buffer (`currentExtent` = `0xFFFFFFFF`) y el compositor lo escala a la pantalla.

SDL3 también emula cambios de modo en Wayland con `wp_viewporter`, pero su lista no
incluye 640×360: el más cercano que ofreció fue 864×486, que no es un factor entero.

La GPU entera, compositor incluido, se midió con el tiempo fuera de RC6 (el estado de
sueño de la GPU) en `/sys/class/drm/card1/gt/gt0/rc6_residency_ms`, con el reloj fijo en
700 MHz:

| Modo | Escalado (nuestro) | Total (nuestro) | GPU entera ocupada |
|---|---|---|---|
| `native` | 1,27–1,28 ms | 5,83–5,84 ms | 77,0–77,5 % (12,8–12,9 ms por frame) |
| `small` | 0,09 ms (blit 1:1) | 4,66–4,67 ms | 70,5–70,7 % (11,7–11,8 ms por frame) |
| Escritorio sin el prototipo | — | — | 23,8 % |

- **Que escale el compositor ahorra ~1,1 ms**, lo mismo que cuesta nuestro blit. El
  compositor compone igual una imagen de 3200×1800 en los dos casos, porque el *direct
  scanout* está apagado.
- **La imagen sale borrosa.** Una captura de pantalla (`grim`) muestra que Hyprland
  escala con filtro lineal. `wp_viewporter` define a qué tamaño se muestra el buffer, no
  con qué filtro, así que la aplicación no lo puede pedir.
- **Con presentación real, el blit costó 1,28 ms** en lugar de 1,63 ms sin ventana.
  Probablemente la imagen del swapchain usa una disposición interna más eficiente; no se
  investigó.
- **"GPU ocupada" exagera:** con el escritorio quieto ya marca 24 %, porque entre ráfagas
  cortas la GPU no alcanza a dormirse. Sirve para comparar dos modos, no como costo
  absoluto.

## 10. Capturas y video

El prototipo guarda capturas de la imagen de 640×360 y secuencias de frames. Con ellas se
armó un video de 56 s con 14 segmentos rotulados (el look PSX efecto por efecto, la luz
por vértice y por píxel, las sombras y las tres tapas, con la cámara oscilando). Las
capturas y el video no están en el repo: se regeneran con el prototipo.

## 11. Lo que el prototipo encontró en otros ADRs

- **El *front face* con la Y volteada.** El ADR-0008 voltea la Y en la proyección. Con
  eso, Vulkan ve cada triángulo con el mismo sentido que en pantalla, así que un
  triángulo antihorario visto desde afuera es `VK_FRONT_FACE_COUNTER_CLOCKWISE`. El
  prototipo empezó con `CLOCKWISE` y dibujó los zombies **de adentro hacia afuera sin que
  se notara en la silueta**, porque el piso y las columnas generados a mano tenían el
  orden al revés y lo disimulaban. Lo destapó la tapa del corte, que se veía solo sin
  *culling*. Un test que dibuje un cubo y compruebe qué caras se ven lo atraparía.
- **El protocolo del ADR-0020 y el reloj de la GPU.** Para comparar dos versiones de una
  pasada, el reloj tiene que estar fijo (sección 1); para saber si se cumple R2, se mide
  como corre de verdad.

## Cómo repetirlo

1. Descargar *Universal Animation Library 2* [Standard] de Quaternius (CC0) desde
   OpenGameArt, como en el ADR-0017.
2. Con Blender 5.2 en modo headless, exportar a un binario simple: la malla completa y
   una diezmada a 0,33 (modificador *Decimate*), con sus pesos (los 4 mayores, en
   `unorm8` que suman 255) y las 32 poses de `Zombie_Walk_Fwd_Loop` como matrices de
   skinning, con los ejes del engine (Y arriba, ADR-0008). Para la tapa, cortar la pose 8
   con `bmesh.ops.bisect_plane`, soldar los vértices duplicados, rellenar los contornos y
   marcar la tapa con el material 2.
3. Compilar los shaders con `glslc` (todas las variantes por `-D`) y el prototipo con
   `g++ -std=c++20 -O2 -lvulkan -lSDL3`.
4. Fijar la frecuencia de la GPU (`gt_min_freq_mhz = gt_max_freq_mhz = 700`) y correr la
   matriz dos veces; restaurar el reloj al terminar (300 y 1150 MHz).
5. Para el compositor, correr con `present=native` y `present=small` y medir
   `rc6_residency_ms` antes y después.
