# ADR-0022: Pipeline de render

- **Estado:** Aceptado
- **Fecha:** 2026-11-01
- **Ticket:** #36
- **Reemplaza / Reemplazado por:** —
- **Fe de erratas:** 2026-11-02 (#39): el ADR-0025 dibuja los VFX dentro de la pasada
  de color, después de lo opaco, sin pasadas nuevas, y usa el canal alfa del color como
  máscara de personajes para los *frames* de impacto (siluetas, en el postproceso). Con
  `--target-env=vulkan1.3`, glslc compila `discard` con `DemoteToHelperInvocation`, así
  que el *screen-door* necesita la feature `shaderDemoteToHelperInvocation`. Los VFX son
  la excepción a tres decisiones: usan *alpha blending* también en las marcas y la estela
  (Decisión 10), *instancing* (Decisión 7) y luz por vértice (Decisión 2). Como el alfa
  lleva la máscara, el swapchain usa `VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR`. Se corrige en
  Consecuencias y se cierran en Abierto las capas de la tapa y los *frames* de impacto.
  2026-11-02 (#40): el ADR-0026 suma **una pasada de UI a 640×360** (HUD, subtítulos y
  menús), después del postproceso y antes del blit: una llamada, con alfa premultiplicado
  y sin escribir el alfa del destino. Se corrige el orden del frame en Consecuencias.
  2026-11-02 (#41): el ADR-0027 parte el mundo en zonas que se cargan mientras se camina.
  **El culling suma dos reglas** a la esfera contra el frustum: solo se dibuja lo que
  está antes del final de la niebla (lo que está más lejos no se ve) y solo la zona
  actual y sus vecinas. **Las 16 antorchas del frame son las más cercanas a la cámara**,
  entre las zonas cargadas, y una antorcha que entra o sale se desvanece. Se corrige en
  Consecuencias (lo que se sacrifica y los tickets).

## Contexto y problema

El ADR-0005 eligió la API (Vulkan 1.3). Falta decidir **cómo se dibuja un frame**: qué
pasadas hay, cómo se ilumina, cómo se consigue el estilo PSX de la visión, cómo se ve el
interior del corte y cómo llega la imagen de 640×360 a la pantalla. Es la base del
renderer: todo lo que el juego dibuja pasa por estas decisiones.

Varios ADRs ya aceptados dejaron preguntas para este:

| ADR | Qué dejó abierto |
|---|---|
| ADR-0007 | Cómo se ve la tapa por dentro (materiales, capas). Cómo se dibuja un pedazo desde dos buffers: qué lleva el buffer chico de vértices nuevos |
| ADR-0010 | Índices de 16 o de 32 bits en los pedazos. Con 16, la subida y el pool bajan casi a la mitad |
| ADR-0017 | El filtrado de las texturas y si hay mipmaps (si los hay, los genera el cocinador) |
| ADR-0018 | Cómo se ve la cámara del tajo |
| ADR-0020 | Cómo se escala la imagen interna de 640×360 a la pantalla, por un factor entero. El escalado escribe más memoria que la escena. El reparto del frame de GPU entre pasadas |
| Visión | La luz de las antorchas contra un ambiente frío, "sombras simples o ninguna", la caballera semitransparente cuando la cámara se acerca y el estilo PSX con el rojo como único color saturado |

Hay que decidir:

1. la **arquitectura**: forward, deferred o una variante;
2. la **iluminación**: el modelo, dónde se calcula y cuántas antorchas afectan a cada
   objeto;
3. las **sombras**;
4. qué efectos del **look PSX** se usan y cómo;
5. cómo se ve **el interior del corte** y qué muestra la cámara del tajo;
6. cómo se **dibuja un pedazo** y el tamaño de sus índices;
7. cómo llegan **los datos a los shaders** y cómo se envían las llamadas;
8. cómo se organizan **las pasadas y la sincronización**;
9. el **escalado, el postproceso y la salida**;
10. las **transparencias**.

Lo que ya está decidido y limita este ADR:

- **ADR-0005:** Vulkan 1.3 con *dynamic rendering* y *synchronization2*. Un *ring buffer*
  mapeado para la geometría dinámica, protegido con fences. `gpu::Allocator` propio. Dos
  rutas de memoria: GPU dedicada (copia de *staging*) e integrada (acceso directo).
- **ADR-0008:** profundidad en [0, 1], **Y volteada en la matriz de proyección**,
  **reverse-Z** con `D32_SFLOAT` y plano lejano infinito. Matrices *column-major*.
- **ADR-0010:** regiones de memoria por sistema, con límites duros. Los pedazos viven en
  un pool de 32 MiB.
- **ADR-0012:** un hilo, paso fijo de 60 Hz e interpolación en el render.
- **ADR-0015:** **skinning LBS en el vertex shader**, hasta 4 huesos por vértice. La CPU
  deforma solo el zombie que se corta.
- **ADR-0007:** 2 frames en vuelo. El buffer de vértices del modelo vive una vez en la
  GPU; cada pedazo suma sus índices y un buffer chico con sus vértices nuevos. La tapa
  trae coordenadas de textura por proyección plana, en la base `(u, v)` del plano.
- **ADR-0017:** texturas `R8G8B8A8_SRGB` (color) y `R8G8B8A8_UNORM` (datos).
- **ADR-0020:** 60 fps sincronizados, **resolución interna de 640×360 escalada por un
  factor entero** (5× en la pantalla de referencia), 13,3 ms de GPU (R2) y 2,5 ms de
  render en la CPU. GPU integrada Intel UHD 620.
- **ADR-0021:** Dear ImGui y el dibujo de depuración los dibuja el renderer propio.
  Timestamps propios por pasada, RenderDoc y validation layers.

**Fuera de alcance:**

| Qué | Lo decide |
|---|---|
| Partículas, salpicadura de sangre y los *frames* de impacto estilo anime | ADR-0025 |
| HUD y menús | ADR-0026 |
| La colisión de la cámara con el escenario | ADR-0016 y la visión |
| Cómo se colocan las luces y los objetos en un nivel | ADR-0027 |
| La carga de shaders y texturas en runtime, y su recarga en caliente | ADR-0028 |
| La compilación de GLSL a SPIR-V en el build | ADR-0006 |

## Criterios

Los cinco del ticket, más tres que salen de cruzar lo ya decidido:

1. **Presupuesto de la GPU integrada** (ADR-0020): 13,3 ms de GPU y 2,5 ms de CPU.
2. **Dirección de arte** (visión): PSX oscuro, con el rojo legible.
3. **Ancho de banda de la memoria compartida:** la GPU integrada comparte la RAM con la
   CPU.
4. **Aprendizaje.**
5. **Valor para la industria.**
6. **El corte se lee a 640×360:** la línea del tajo, la tapa y el rojo (pilares 1 y 4 de
   la visión).
7. **Se construye por partes:** cada opción crece desde el "triángulo" del ADR-0005, sin
   reescribir.
8. **Depurable:** cada pasada se ve en RenderDoc y se mide con timestamps (ADR-0021).

## Opciones

Los tiempos son de un prototipo medido en la máquina de referencia, con la GPU fija en
700 MHz para poder comparar (ver Investigación). Son tiempos de GPU por frame, con 50
zombies y 100 pedazos.

### 1. La arquitectura

| Opción | Cómo funciona | Medido | Problema |
|---|---|---|---|
| **Forward** | Cada objeto se dibuja una vez, y su fragment shader calcula la luz | **6,97 ms** en total; 4,55 ms la pasada principal | Cada píxel tapado por otro también calcula su luz (*overdraw*) |
| **Forward con *depth prepass*** | Primero solo la profundidad, después el color con `EQUAL`: la luz se calcula una vez por píxel | **9,90 ms**: el prepass solo cuesta 3,1 ms | Dibuja la geometría dos veces y repite el skinning, y la escena está limitada por los vértices |
| **Deferred** | Un G-buffer (color y normal) y una pasada de luz a pantalla completa | **6,72 ms** (G-buffer 3,82 + luz 0,48) | Sin luz por vértice. Lo semitransparente no entra en el G-buffer: necesita su propia pasada forward. A 640×360 el G-buffer pesa solo 1,8 MB |
| **Forward+ / clustered** | Una pasada previa reparte las luces en tiles o *clusters* de la pantalla, y cada píxel solo recorre las suyas | No medido | Paga una pasada de cómputo y listas de luces. Sirve con cientos de luces; aquí hay hasta 16, y cada una cuesta ~0,07 ms |

**La escena está limitada por los vértices:** con 2,6 veces más vértices por zombie, la
pasada principal cuesta 2,3 veces más. A 640×360 los píxeles casi no pesan, y las
técnicas pensadas para ahorrar sombreado de píxeles (prepass, clustered) no tienen qué
ahorrar.

### 2. La iluminación

**El modelo:**

| Opción | Cómo es | Problema |
|---|---|---|
| **Estilizado** | Lambert con atenuación, un ambiente frío y, si hace falta, un especular simple | No reacciona a los materiales como un modelo físico |
| **PBR** (GGX, metal y rugosidad) | El estándar de la industria. glTF ya trae los parámetros | Con texturas de 64×64 sin filtrar y 640×360, sus matices no se ven. Más costo por píxel y más trabajo de assets |

**Dónde se calcula:**

| Opción | Medido (8 / 16 antorchas) | Cómo se ve |
|---|---|---|
| **Por vértice** (Gouraud, como la PS1) | 6,62 / 7,82 ms | Facetado, con la luz "corrida" entre vértices. Imposible en deferred |
| **Por píxel** | 6,97 / 7,57 ms | Suave. La luz de una antorcha se dibuja bien aunque el triángulo sea grande |

Con tantos vértices, **la luz por vértice no es más barata**: es una decisión de estética.

**Cuántas antorchas por objeto:** un arreglo global con todas (hasta 16, recorridas por
cada píxel), o las N más cercanas a cada objeto, elegidas en la CPU.

### 3. Las sombras

| Opción | Cómo funciona | Medido | Problema |
|---|---|---|---|
| **Ninguna** | — | 0 ms | Los personajes "flotan" sobre el piso |
| **Sombras *blob*** | Un disco oscuro bajo cada personaje, proyectado en el piso | No medido; es un quad por personaje | No sigue la forma del cuerpo |
| **Un shadow map de la luna** (direccional) | Se dibuja la escena desde la luna en una imagen de profundidad, y cada píxel compara su distancia | **+3,0 ms con 512² y con 1024²** | El costo es repetir el skinning de toda la escena, no rasterizar: 23 % del presupuesto |
| **Sombras de las antorchas** | Un *cube map* por antorcha: 6 pasadas cada una | No medido | Seis veces el costo anterior por cada antorcha |

**Una alternativa que cambia este costo:** hacer el skinning una sola vez por frame en
un *compute shader*, a un buffer que reutilizan todas las pasadas (el *GPU skin cache* de
Unreal). El ADR-0015 la descartó porque "solo conviene con varias pasadas". Si este ADR
elige sombras o un prepass, esa razón cambia.

### 4. El look PSX

Cada efecto es una elección aparte. **Ninguno cambia el costo:** con todos activos, la
pasada principal mide 4,40 ms contra 4,55 sin ninguno.

| Efecto | Qué hace | Implementación |
|---|---|---|
| **Vértices que tiemblan** | La PS1 no tenía precisión subpíxel: los vértices saltan de píxel en píxel al moverse | Redondear la posición a una grilla en el vertex shader |
| **Texturas afines** | La PS1 no corregía la perspectiva: las texturas se deforman en los triángulos grandes | El calificador `noperspective` en la coordenada de textura |
| **Filtrado *nearest*** | Los texeles se ven como cuadrados nítidos | El sampler |
| **Mipmaps** | Sin ellos, las texturas lejanas "brillan" con ruido al moverse | Generados por el cocinador (ADR-0017) |
| ***Dithering* a 15 bits** | Color de 5 bits por canal con un patrón ordenado, como la PS1 | Al final del fragment shader o en la pasada de escalado |
| **Niebla** | Esconde el fondo y da profundidad | Por distancia, hacia un color frío |

Las capturas y el video del prototipo comparan cada efecto, de a uno.

### 5. El interior del corte

**La tapa:**

| Opción | Cómo funciona | Cómo se ve |
|---|---|---|
| **Textura con la UV plana** del ADR-0007 | Una textura de carne repetida sobre la tapa | Carne sin estructura: no hay hueso |
| **Procedural según la distancia al hueso** | Para cada píxel de la tapa, la distancia al hueso más cercano decide: médula, hueso o carne | **El hueso aparece en su lugar**: la columna en la cintura, el radio en el antebrazo. Necesita los segmentos de los huesos de la pose |
| **Triplanar** | Tres proyecciones mezcladas por la normal | Lo mismo que la UV plana: la tapa es plana |
| **Mixta** | La procedural decide carne o hueso, y una textura le da detalle | Combina las dos |

**Las capas por profundidad** (piel, grasa y carne; metal y carne en los zombies con
armadura) necesitan saber a qué distancia del contorno está cada punto. La tapa del
ADR-0007 solo tiene vértices en el borde. Si se quieren, el corte tendría que agregar
puntos interiores con esa distancia, o la tapa tendría que calcular un campo de
distancia 2D.

**La cámara del tajo** (visión: "se ve el interior, carne y hueso"):

| Opción | Qué muestra | Problema |
|---|---|---|
| **Solo la tapa**, con la cámara cerca y en cámara lenta | El corte mientras la hoja atraviesa | Mientras la hoja está adentro, la tapa todavía no existe |
| **Rayos X**, como *Sniper Elite* | La piel semitransparente y los huesos por dentro | Necesita mallas de hueso en cada zombie (assets) y una pasada propia |

### 6. Dibujar un pedazo

| Opción | Cómo funciona | Medido (100 pedazos) | Problema |
|---|---|---|---|
| **Mismo `VkBuffer`** | El modelo y los vértices nuevos de los pedazos viven en el mismo buffer; un pedazo es una llamada con índices absolutos | 2,04 ms en la GPU | Índices de 32 bits: el ADR-0010 calculó casi el doble de subida y pool que con 16 |
| **Dos llamadas** | Una con los triángulos que solo usan vértices del modelo; otra con un buffer chico que lleva los nuevos y copias de los del borde | 2,00 ms en la GPU; +0,03 ms de CPU | Copia algunos vértices del borde (~20 KB por corte, ADR-0010). Permite índices de 16 bits en las dos llamadas |
| ***Vertex pulling*** | El vertex shader lee los vértices de dos buffers por índice | **2,90 ms**: 42 % más lento en esta GPU | Se salta el hardware que lee los atributos de vértice |

**100 pedazos cuestan lo mismo que 50 zombies,** porque se les sigue haciendo skinning
aunque estén quietos. Los pedazos rígidos (ADR-0016) se podrían dibujar sin skinning, con
sus vértices ya transformados una vez en la pose del corte.

### 7. Los datos a los shaders y el envío

**Cómo sabe cada llamada su objeto** (su matriz, su paleta de huesos y su textura):

| Opción | Por llamada | Medido (259 llamadas) | Problema |
|---|---|---|---|
| **Clásico**: un *descriptor set* por textura, con un UBO dinámico por objeto | Un `vkCmdBindDescriptorSets` con un *offset* | **4,16 ms** de GPU; 0,26 ms de CPU por frame | Hay que crear y mantener los sets. Más llamadas a la API |
| **Bindless**: un arreglo de objetos y uno de texturas; el índice va en un *push constant* | Un `vkCmdPushConstants` de 4 B | 4,53 ms; **0,20 ms** de CPU | Un error de índice no lo detecta la API. Los SSBO indexados se leen más lento que un UBO en esta GPU |
| ***Buffer device address***: la dirección del objeto va en un *push constant* | Un `vkCmdPushConstants` de 8 B | 4,28 ms; 0,21 ms de CPU | Punteros en los shaders: un error es un acceso inválido en la GPU |
| ***Push descriptors*** (núcleo en Vulkan 1.4, extensión en 1.3) | Los descriptores se escriben en el comando mismo | No medido | Una extensión sobre el Vulkan 1.3 del ADR-0005 |

En la CPU, los cuatro quedan muy por debajo de los 2,5 ms del ADR-0020. La UHD 620 con
Mesa soporta todos.

**Cómo se envían:** una llamada por objeto; *instancing* (los zombies comparten malla:
50 zombies en una llamada, cada instancia con su paleta); o *indirect* (la GPU lee las
llamadas de un buffer). **El culling:** por objeto en la CPU, con una esfera contra el
frustum.

### 8. Las pasadas y la sincronización

| Opción | Cómo es | Problema |
|---|---|---|
| **Lista fija escrita a mano** | Cada pasada, sus barreras y sus imágenes, en orden, en el código del frame | Agregar una pasada obliga a revisar las barreras a mano |
| **Un *render graph* chico** | Cada pasada declara qué lee y qué escribe; un grafo ordena las pasadas, calcula las barreras y reutiliza memoria (*FrameGraph*, Frostbite, 2017) | Más código antes de tener algo en pantalla. Con 4 o 5 pasadas, el beneficio es chico |

Además:

- **Cuándo se compilan los pipelines:** todos al arrancar, con un `VkPipelineCache` en
  disco, o la primera vez que se usan (puede causar tirones).
- **En qué resolución se dibujan ImGui y el dibujo de depuración** (ADR-0021): ImGui a la
  resolución nativa, después del escalado, para que el texto se lea; el dibujo de
  depuración a 640×360, antes del escalado, para usar la profundidad de la escena.

### 9. El escalado, el postproceso y la salida

**Quién escala:**

| Opción | Medido | Cómo se ve |
|---|---|---|
| **El engine**, con *nearest* y factor entero | Escalado de 1,3 a 2,4 ms (ver abajo) | **Píxeles nítidos e iguales** |
| **El compositor** (swapchain de 640×360) | Ahorra ~1,1 ms de GPU, compositor incluido | **Borroso:** Hyprland escala con filtro lineal, y Wayland no deja pedir otro |

**Cómo escala el engine:**

| Opción | Medido sin ventana / con ventana | Problema |
|---|---|---|
| **Blit** (`vkCmdBlitImage2`, `VK_FILTER_NEAREST`) | 1,63 / 1,28 ms | Solo copia: no puede hacer tonemapping, *dithering* ni otros efectos en el mismo paso |
| **Pasada propia** (un triángulo a pantalla completa) | 2,40 ms sin ventana | 0,8 ms más. Pero en la misma pasada caben el tonemapping, el *dithering*, los *frames* de impacto estilo anime (si se usan) e ImGui |

**El rango de color:** LDR directo a sRGB, o la escena en HDR (`R16G16B16A16_SFLOAT`) con
tonemapping. Medido: **mismo costo** a 640×360.

**El antialiasing:** ninguno (como la PS1), MSAA a 640×360 o un filtro de postproceso.
No medido.

**Pantallas sin factor entero** (por ejemplo, 1366×768): bordes negros alrededor de la
imagen escalada por el mayor factor entero que cabe, o escalar a un factor no entero.

### 10. Las transparencias

| Opción | Cómo funciona | Problema |
|---|---|---|
| ***Alpha blending* ordenado** | Lo semitransparente se dibuja al final, de atrás hacia adelante, mezclando con lo que ya está | Hay que ordenar. En deferred necesita una pasada forward aparte |
| ***Screen-door*** (*dithering*) | Se descartan píxeles con un patrón ordenado: un 50 % de transparencia descarta la mitad | Se ve la trama. A 640×360 escalado, la trama es visible, aunque encaja con el *dithering* del estilo PSX |

No se midieron: el costo de los dos es despreciable comparado con el resto.

## Investigación

### Prototipo medido

Un programa en C++20 con Vulkan 1.3, medido en la máquina de referencia (ADR-0020) el
2026-11-01, fuera del protocolo. Dibuja 50 zombies (el maniquí de Quaternius diezmado a
3269 vértices, CC0) con skinning, 100 pedazos, un piso, columnas, hasta 16 antorchas y
niebla, a 640×360, y lo escala a 3200×1800. El detalle y cómo repetirlo están en
[`../research/pipeline-de-render.md`](../research/pipeline-de-render.md).

| # | Qué | Resultado |
|---|---|---|
| P1 | Arquitectura | Forward 6,97 ms; prepass 9,90; deferred 6,72. Con la malla de 8546 vértices: 12,88 y 12,22. **Limitado por los vértices** |
| P2 | Escalado a 3200×1800 | Pasada propia 2,40 ms; blit 1,63 (1,28 con presentación real); el compositor lo hace por ~1,1 ms menos, pero borroso |
| P3 | Luces y sombras | ~0,07 ms por antorcha. Por vértice no ahorra (16 antorchas: 7,82 contra 7,57). Shadow map +3,0 ms con cualquier tamaño |
| P4 | Look PSX | Gratis: todos los efectos juntos, 6,80 ms contra 6,97 |
| P5 | La tapa | La procedural muestra el hueso en su lugar; la triplanar no aporta en una tapa plana. Las capas necesitan la distancia al contorno |
| P6 | Pedazos | Mismo buffer 2,04 ms; dos llamadas 2,00; *vertex pulling* 2,90 |
| P7 | Descriptores | GPU: clásico 4,16, BDA 4,28, bindless 4,53. CPU del frame: 0,20–0,26 ms. La UHD 620 soporta todo |

Lo que mostró, además de los números:

- **A 640×360 manda la geometría, no los píxeles.** Lo que cuesta es procesar vértices, y
  sobre todo repetir el skinning: el prepass y las sombras lo hacen de nuevo para toda
  la escena.
- **Comparar tiempos de la GPU integrada exige fijar su reloj.** Con el reloj libre, el
  driver lo baja a 60 Hz, entre 300 y 857 MHz, y la misma configuración dio 11,4 y
  17,3 ms. Con el reloj fijo en 700 MHz, las dos vueltas coinciden dentro del 1 %.
- **El *front face* correcto con la Y volteada del ADR-0008 es
  `VK_FRONT_FACE_COUNTER_CLOCKWISE`.** El prototipo empezó al revés y dibujó los zombies
  de adentro hacia afuera sin que se notara en la silueta. Lo destapó la tapa del corte.
- **La arista no *manifold* del maniquí rompió la tapa de la cintura**, como en el
  ADR-0007: el contorno no cerró.

**Lo que toca a otros ADRs:**

- **ADR-0008:** dice que voltear la Y "invierte el orden de los vértices, así que hay que
  ajustar `frontFace`", sin decir a qué valor. El valor es `COUNTER_CLOCKWISE`, y conviene
  un test que lo verifique.
- **ADR-0020:** el protocolo mide con el reloj de la GPU libre, que es lo correcto para
  saber si se cumple R2. Pero para comparar dos versiones de una pasada (el "antes y
  después" de un PR de rendimiento del ADR-0014) hay que fijarlo.
- **ADR-0015:** si se eligen sombras o un prepass, el *skin cache* en un *compute shader*
  deja de ser innecesario.

### Cómo lo hacen otros

- **La PS1** (Sony, 1994) no tenía buffer de profundidad (ordenaba los polígonos), no
  corregía la perspectiva de las texturas, usaba coordenadas de vértice enteras en
  pantalla y dibujaba en color de 15 bits con *dithering*. De ahí salen las texturas
  afines, los vértices que tiemblan y la trama de color del estilo (Copetti, *PlayStation
  Architecture*; Korth, *psx-spx*).
- **Forward+** (Harada, McKee y Yang, 2012) y **clustered shading** (Olsson, Billeter y
  Assarsson, 2012) reparten las luces por tiles o *clusters*. *DOOM* (2016) usa un
  forward clustered (Sousa y Geffroy, SIGGRAPH 2016).
- ***Real-Time Rendering*** (4.ª ed., capítulo 20) compara forward, deferred y sus
  variantes según la cantidad de luces, el ancho de banda y las transparencias.
- **FrameGraph** de Frostbite (O'Donnell, GDC 2017): el *render graph* que popularizó
  declarar las pasadas con sus lecturas y escrituras.
- **El *GPU skin cache* de Unreal Engine** deforma las mallas en un *compute shader* una
  vez por frame, para que todas las pasadas lo reutilicen.
- ***Programmable vertex pulling*** (Rákos, *OpenGL Insights*, 2012): leer los vértices
  desde buffers en el shader, sin la entrada fija de atributos.
- **Fijar el reloj para medir:** Direct3D 12 tiene `SetStablePowerState` para que los
  profilers comparen con el reloj fijo.

## Decisión

Elijo un renderer forward, simple, con el estilo PSX hecho a 640×360 y escalado por el
propio engine, para que los píxeles se vean nítidos. El prototipo mostró que a esta
resolución lo que pesa es la geometría, no los píxeles, así que las técnicas pensadas
para ahorrar sombreado (deferred, prepass, clustered) no me dan nada que necesite, y sí
me quitan cosas: la luz por vértice o las transparencias simples. Prefiero construir
desde el triángulo algo que entiendo entero, y dejar escritas las alternativas por si una
medición las pide más adelante.

El look PSX es gratis, así que lo elijo por cómo se ve: texturas nítidas, *dithering*,
niebla y vértices que tiemblan un poco, pero sin las texturas afines, que deforman el piso
y van contra que el corte se lea. El interior del corte tiene que mostrar el hueso en su
lugar: es lo que se recuerda del tajo. Que escale el compositor queda descartado: se ve
borroso.

| # | Dimensión | Elección |
|---|---|---|
| 1 | Arquitectura | **Forward:** una pasada de color y profundidad a 640×360, sin *depth prepass* |
| 2 | Iluminación | **Estilizada:** Lambert con atenuación cuadrática hasta el radio de cada luz y un ambiente frío, sin especular. **Por píxel.** Un **arreglo global de hasta 16 antorchas** en el uniform del frame, que recorre cada píxel |
| 3 | Sombras | **Sombras *blob*:** un disco oscuro bajo cada personaje, proyectado en el piso. Se dibuja con ***screen-door*** (la matriz de Bayer de la Decisión 10, que trama su borde suave), en la pasada de color justo después del piso, con un *depth bias* para que no parpadee contra él. **Sin shadow map.** El skinning sigue en el vertex shader (ADR-0015) |
| 4 | Look PSX | **Filtrado *nearest*, mipmaps** (también *nearest* entre niveles), ***dithering* a 15 bits** con una matriz de Bayer de 4×4 al final del fragment shader, y **niebla lineal** por distancia hacia un color frío. **Vértices que tiemblan, suaves:** la grilla es una CVar (ADR-0021), por defecto la resolución interna. **Texturas afines:** una CVar, apagada por defecto. En Release, las dos son constantes |
| 5 | Interior del corte | **Procedural según la distancia al hueso,** con una **textura de detalle** en la UV plana del ADR-0007 encima de la carne. **El hueso sale de las cápsulas del ADR-0016:** su eje es el segmento de la cápsula de cada hueso del pedazo, en su pose, y su radio es una fracción del radio de la cápsula (una CVar). **La cámara del tajo muestra la tapa**, de cerca y en cámara lenta. Las capas por profundidad y los rayos X quedan abiertos |
| 6 | Pedazos | **Dos llamadas por pedazo:** una con los triángulos que solo usan vértices del modelo, y otra con el buffer chico, que lleva los vértices nuevos y **copias de los vértices del borde**. **Índices de 16 bits** en las dos. Los pedazos se dibujan con el mismo vertex shader de skinning, con su paleta congelada |
| 7 | Datos y envío | **Bindless:** un *descriptor set* global con el arreglo de objetos (un SSBO por frame en vuelo) y el arreglo de texturas (*descriptor indexing*, *partially bound*). Cada llamada recibe el índice de su objeto en un *push constant*. **Una llamada por objeto**, sin *instancing*. **Culling por objeto en la CPU**, con una esfera contra el frustum |
| 8 | Pasadas | **Lista fija escrita a mano**, con barreras de *synchronization2*. **Todos los pipelines se compilan al arrancar**, con un `VkPipelineCache` en disco. **El dibujo de depuración va a 640×360** con la profundidad de la escena, e **ImGui a la resolución nativa**, después del escalado |
| 9 | Escalado y salida | **Escala el engine, con un blit *nearest* y factor entero** (`vkCmdBlitImage2`). **Todo el postproceso, si hay, va a 640×360, antes del blit.** Color **LDR** en `R8G8B8A8_SRGB`, **sin antialiasing**. En una pantalla sin factor entero, el **mayor factor entero que cabe, con bordes negros**. Presentación FIFO |
| 10 | Transparencias | ***Screen-door*** con la misma matriz de Bayer de 4×4, para la caballera cuando la cámara se acerca. El ***alpha blending*** queda para las partículas del ADR-0025 |

## Consecuencias

- **El frame tiene cinco pasos, en este orden:**

  ```
  640×360:  color y profundidad (opacos, screen-door y los VFX del ADR-0025)
            → dibujo de depuración → postproceso (si hay)
            → UI del ADR-0026 (HUD, subtítulos y menús)
  nativa:   blit nearest al swapchain → ImGui
  ```

  Cada paso es un `vkCmdBeginRendering` (o el blit) con sus barreras escritas a mano y un
  par de timestamps (ADR-0021). Dentro de la pasada de color, los VFX van después de lo
  opaco, en el orden del ADR-0025. La UI va después del postproceso, así que los *frames*
  de impacto no la invierten (fe de erratas, ADR-0026).
- **El canal alfa del color es la máscara de personajes** (ADR-0025): el shader de los
  personajes escribe 0 y el resto escribe 1. Las mezclas de los VFX conservan el alfa del
  destino, y el postproceso de los *frames* de impacto lo lee. **El swapchain usa
  `VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR`**: con `PRE_MULTIPLIED` o `INHERIT`, el compositor
  dibujaría transparentes a los personajes de los frames sin postproceso.
- **Los VFX del ADR-0025 son la excepción a tres decisiones:** usan *alpha blending* en
  las partículas, las marcas y la estela (la Decisión 10 lo dejaba solo para las
  partículas), *instancing* en las partículas (la Decisión 7 es una llamada por objeto)
  y luz por vértice, sin normal (la Decisión 2 es por píxel). El resto del frame sigue
  las Decisiones como están.
- **Reparto del frame de GPU** (hipótesis para `docs/performance/presupuestos.md`,
  medidas con el reloj fijo en 700 MHz, que es el 61 % del máximo):

  | Pasada | Medido a 700 MHz | Meta |
  |---|---|---|
  | Color: 50 zombies, escenario y ~100 pedazos | 4,5 ms | 7,0 ms |
  | Dibujo de depuración y postproceso | — | 0,5 ms |
  | Blit a la pantalla | 1,3–1,6 ms | 2,0 ms |
  | ImGui (solo en desarrollo) | — | 0,5 ms |
  | Reserva | — | 3,3 ms |
  | **Total** | | **13,3 ms** (R2) |

- **Lo que se gana:** un renderer chico, que crece pasada por pasada desde el triángulo
  del ADR-0005, y que gasta menos de la mitad del presupuesto de GPU, aun medido a
  700 MHz. El temblor de los vértices y las texturas afines se ajustan sin recompilar.
- **Lo que se sacrifica:**
  - las sombras con la forma del cuerpo: el blob no sigue la silueta;
  - más de 16 antorchas a la vez: con más, hay que elegir las N más cercanas a cada
    objeto, o pasar a clustered (decidido en el ADR-0027: las 16 más cercanas a la
    cámara, entre las zonas cargadas, con un fundido al entrar o salir);
  - ~0,35 ms de GPU de bindless frente al modelo clásico, a cambio de una CPU más simple;
  - la imagen no llena las pantallas sin factor entero.
- **Features que el engine verifica al iniciar,** junto a las del ADR-0005:
  `descriptorIndexing`, `runtimeDescriptorArray`, `descriptorBindingPartiallyBound` y
  `shaderSampledImageArrayNonUniformIndexing` (Vulkan 1.2), y
  `shaderDemoteToHelperInvocation` (Vulkan 1.3), que usa el `discard` del *screen-door*
  (fe de erratas, ADR-0025). La UHD 620 las tiene.
- **El *front face* es `VK_FRONT_FACE_COUNTER_CLOCKWISE`**, por la Y volteada del
  ADR-0008. Un test dibuja un cubo y verifica que se vean sus caras de afuera: el error
  no se nota en la silueta.
- **Para comparar dos versiones de una pasada, el reloj de la GPU se fija** (por ejemplo,
  en 700 MHz con `gt_min_freq_mhz` y `gt_max_freq_mhz`). Con el reloj libre, la misma
  configuración varió hasta un 50 %. Saber si se cumple R2 se sigue midiendo con el reloj
  libre, como pide el ADR-0020.
- **El cocinador (ADR-0017)** genera los mipmaps de las texturas de color.
- **La tapa lee las cápsulas del asset, no las del pedazo.** El ADR-0016 recorta las
  cápsulas de un pedazo para que terminen a un radio del plano del corte: con ellas, el
  hueso no llegaría a la tapa. La tapa usa las cápsulas tal como salen del cocinador, en
  el espacio de cada hueso, y las mueve con la paleta congelada del pedazo. Así no
  necesita datos nuevos. Como la cápsula se ajusta sobre el eje del hueso (ADR-0017),
  una corrección a mano que la saque del eje mueve también el hueso de la tapa.
- **Assets nuevos:** la textura del blob y la textura de detalle de la carne.
- **Tickets, en orden** (cada uno crece desde el anterior):
  1. una malla con profundidad reverse-Z y el test del *front face*;
  2. texturas *nearest* con mipmaps y el *descriptor set* bindless;
  3. skinning en el vertex shader con la paleta de huesos;
  4. antorchas, ambiente y niebla;
  5. *dithering* y vértices que tiemblan, con la CVar de la grilla y la de las texturas
     afines;
  6. el blit al swapchain, con el factor entero y los bordes negros;
  7. el culling por esfera, hasta el final de la niebla y solo en la zona actual y sus
     vecinas (ADR-0027);
  8. los pedazos con dos llamadas y la tapa procedural (junto al ADR-0007);
  9. las sombras blob;
  10. el *screen-door* de la caballera;
  11. ImGui y el dibujo de depuración en sus pasadas (junto al ADR-0021).
- **Abierto:**
  - las capas por profundidad en la tapa (piel, grasa, carne; metal y carne), que
    necesitan la distancia al contorno (decidido en el ADR-0025: un campo de distancia
    de 64² por corte, que llena el corte);
  - la cámara de rayos X. De ella depende lo que pide la visión para la cámara del tajo:
    ver el interior mientras la hoja atraviesa el cuerpo. Con esta Decisión, el interior
    se ve en la tapa, después de que pasa la hoja;
  - los *frames* de impacto estilo anime, si se usan: irían en el postproceso a 640×360
    (visión y ADR-0025; decidido en el ADR-0025: siluetas en blanco, negro y rojo, en el
    postproceso, solo en los cortes cargados y en la cámara del tajo);
  - el *instancing* de los zombies y el *skin cache*, si una medición los pide;
  - dónde se guarda el `VkPipelineCache` (ADR-0028);
  - el tamaño de la grilla del temblor, la fracción del radio de la cápsula que es hueso
    y la textura de detalle de la carne, que se ajustan mirando el juego.

**Fe de erratas en otros ADRs.** Al aceptar este ADR se corrigieron, como fe de erratas
(ADR-0014, regla 11):

| ADR | Qué se corrige |
|---|---|
| ADR-0007 | El buffer chico de un pedazo lleva los vértices nuevos y copias de los del borde (dos llamadas). Se cierra en Abierto |
| ADR-0008 | Con la Y volteada en la proyección, el *front face* es `COUNTER_CLOCKWISE` |
| ADR-0010 | Los índices de los pedazos son de 16 bits. Se cierra en Abierto |
| ADR-0017 | El cocinador genera mipmaps (*nearest* entre niveles) y falla si una malla que se corta pasa de 65 536 vértices |
| ADR-0020 | El escalado: blit *nearest* entero hecho por el engine. Para comparar versiones de una pasada, el reloj de la GPU se fija |
| `docs/performance/presupuestos.md` | El reparto del frame de GPU por pasada |

## Cómo lo defiendo

**¿Por qué forward, si deferred midió un poco menos?**
Deferred ganó por 0,25 a 0,30 ms con 8 luces, ~4 %. Deferred ahorra calcular la luz en
píxeles que después se tapan, pero a 640×360 hay muy pocos píxeles: lo que pesa son los
vértices (con 2,6 veces más vértices, el frame costó 2,3 veces más). Con 16 antorchas,
los dos empatan. A cambio de ese poco, deferred necesita un G-buffer y una pasada más, y
complica las transparencias. Forward es una sola pasada que crece desde el triángulo.

**¿Por qué no un *depth prepass*, que usan muchos engines?**
El prepass dibuja la geometría dos veces para no sombrear píxeles de más. Aquí la
geometría es lo caro y los píxeles son baratos, y además el skinning se hace dos veces.
Medido, el frame pasó de 6,97 ms a 9,90 ms.

**¿Por qué sombras *blob* y no un shadow map?**
El shadow map costó 3 ms con cualquier tamaño, el 23 % del presupuesto de GPU. El costo
no es la resolución, sino volver a hacer el skinning de los 50 zombies y los 100 pedazos
desde la luz. Un disco oscuro bajo cada personaje es lo que usaban los juegos de PS1 y
calza con el estilo.

**¿Por qué escala el engine, si dejar que escale el compositor ahorra ~1,1 ms?**
Porque el compositor escala con filtro lineal y se ve borroso. El protocolo de Wayland
(`wp_viewporter`) no deja elegir el filtro. El estilo PSX necesita píxeles nítidos, así
que escalo yo con un blit *nearest* y un factor entero: cada píxel de 640×360 se vuelve
un cuadrado exacto. Si el factor no es entero, uso el mayor que cabe y dejo bordes
negros, porque un factor no entero deja píxeles de distinto tamaño.

**¿Por qué bindless, si el modelo clásico fue 0,35 ms más rápido en la GPU?**
Es un intercambio que acepto. En la GPU de Intel, un UBO se lee por un camino más rápido
que un SSBO indexado. Pero con bindless se enlaza un solo *descriptor set* por frame, y
cada llamada recibe un índice de 4 bytes: grabar el frame es más simple, y agregar una
textura no obliga a crear un *set* nuevo. Es como trabajan los engines modernos, y los
0,35 ms caben de sobra en la reserva.

**¿Qué es el *front face* y por qué es `COUNTER_CLOCKWISE`?**
Es el orden de los vértices que dice qué cara de un triángulo mira a la cámara. La
proyección del ADR-0008 voltea la Y, y eso invierte el orden en la pantalla. El
prototipo lo tenía mal y los zombies se veían del revés por dentro, con la silueta
correcta. Por eso un test dibuja un cubo y verifica que se vean sus caras de afuera.

**¿Cómo se ve el hueso dentro del corte?**
Cada píxel de la tapa mide su distancia al eje de las cápsulas de los huesos del
pedazo. Cerca del eje es médula, un poco más afuera es hueso, y el resto es carne con
una textura de detalle. Uso las cápsulas del asset, no las del pedazo: el ADR-0016
recorta las del pedazo para que terminen a un radio del plano del corte, y el hueso no
llegaría a la tapa. Así la tapa no necesita datos nuevos del cocinador.

**¿Cuánto cuesta el look PSX?**
Nada: las diferencias que medí son ruido. Lo elijo por cómo se ve. Uso mipmaps con
filtro *nearest*, porque sin ellos el piso lejano brilla con ruido, y sigo viendo los
texels nítidos. Dejo las texturas afines apagadas: deforman el piso, y el corte tiene
que leerse bien.

**¿Por qué mides con el reloj de la GPU fijo?**
Porque la GPU cambia su frecuencia sola, y con el reloj libre la misma configuración
varió hasta un 50 %. Así no se pueden comparar dos versiones. Con el reloj fijo en
700 MHz, las mediciones repetidas coincidieron en ~1 %. Para saber si se cumple el
presupuesto mido con el reloj libre, como pide el ADR-0020, porque así juega el
jugador.

## Referencias

- Tomas Akenine-Möller, Eric Haines, Naty Hoffman y otros, *Real-Time Rendering*, 4.ª
  ed., CRC Press, 2018. Capítulo 20, "Efficient Shading".
- Takahiro Harada, Jay McKee y Jason C. Yang, "Forward+: Bringing Deferred Lighting to
  the Next Level", Eurographics 2012 (*short papers*).
- Ola Olsson, Markus Billeter y Ulf Assarsson, "Clustered Deferred and Forward Shading",
  High Performance Graphics 2012.
- Tiago Sousa y Jean Geffroy, "The devil is in the details: idTech 666", SIGGRAPH 2016,
  *Advances in Real-Time Rendering*.
- Yuriy O'Donnell, "FrameGraph: Extensible Rendering Architecture in Frostbite", GDC
  2017.
- Daniel Rákos, "Programmable Vertex Pulling", en *OpenGL Insights*, CRC Press, 2012.
- Lance Williams, "Casting Curved Shadows on Curved Surfaces", SIGGRAPH 1978.
- Henri Gouraud, "Continuous Shading of Curved Surfaces", *IEEE Transactions on
  Computers*, 1971.
- Bryce E. Bayer, "An optimum method for two-level rendition of continuous-tone
  pictures", IEEE International Conference on Communications, 1973.
- Rodrigo Copetti, *PlayStation Architecture: A Practical Analysis*,
  <https://www.copetti.org/writings/consoles/playstation/>.
- Martin Korth, *psx-spx: PlayStation Specifications*,
  <https://psx-spx.consoledev.net/>.
- Epic Games, documentación de Unreal Engine: *Skin Cache*.
- Microsoft, documentación de Direct3D 12: `ID3D12Device::SetStablePowerState`.
- Khronos, especificación de Vulkan 1.3 y *Vulkan Guide* (descriptor indexing, buffer
  device address, *dynamic rendering*).
- Wayland, protocolo `wp_viewporter` (wayland-protocols, estable).
