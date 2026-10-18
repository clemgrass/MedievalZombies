# Investigación: API gráfica (OpenGL frente a Vulkan)

Nota de apoyo del [ADR-0005](../adr/0005-api-grafica.md). El ADR resume y decide; esta
nota explica.

## 1. Qué hace una API gráfica

El engine no le habla a la GPU directamente: le habla a un **driver**, a través de una
API. La API define cómo el engine:

- **sube datos** a la memoria de la GPU (vértices, índices, texturas);
- **describe cómo dibujar** (shaders, estado de rasterización, blending, profundidad);
- **envía trabajo** (draw calls, compute dispatches);
- **sincroniza** a la CPU y la GPU, que trabajan en paralelo y a destiempo.

La gran diferencia entre las APIs es **cuánto de eso decide el driver por ti** y cuánto
decides tú.

## 2. Dos modelos: implícito frente a explícito

### OpenGL: una máquina de estados que el driver administra

- Hay un **contexto** global con estado ("textura activa", "buffer activo",
  "shader activo"). Cada llamada lo modifica o lo usa.
- El driver **adivina** cuándo subir datos, cuándo esperar a la GPU, cuándo compilar
  variantes de shaders y cómo organizar la memoria.
- **OpenGL 4.5** agregó **Direct State Access (DSA)**: modificar objetos sin
  "bindearlos", lo que reduce mucho los errores del modelo de estado global.
- **OpenGL 4.6** incorporó buena parte de las técnicas *AZDO* (*Approaching Zero
  Driver Overhead*): draws indirectos múltiples, parámetros de draw en shaders,
  `KHR_no_error` y la posibilidad de cargar shaders en SPIR-V.
- **Ventaja:** se llega rápido a dibujar. **Costo:** lo que hace el driver queda
  escondido, y el rendimiento depende de heurísticas que no controlas.

### Vulkan: tú eres el driver

- No hay estado global. Todo es un objeto explícito: `VkInstance`, `VkDevice`,
  `VkQueue`, `VkCommandBuffer`, `VkPipeline`, `VkBuffer`, `VkDeviceMemory`, etc.
- **La memoria la manejas tú:** pides bloques de memoria de la GPU y los repartes
  entre buffers e imágenes. Hay un límite (`maxMemoryAllocationCount`) de cuántos
  bloques se pueden pedir, así que hay que **subasignar**. Es la misma idea de arenas
  y pools del ADR-0010, pero en la GPU.
- **La sincronización la haces tú:** barreras, semáforos y fences. Si te equivocas,
  hay corrupción visual o la aplicación se cuelga.
- **Los pipelines se compilan antes:** los cambios de estado se agrupan en objetos
  precompilados, lo que evita tirones por compilación en el medio del juego.
- **Ventaja:** el rendimiento depende de decisiones que tú tomas y entiendes.
  **Costo:** mucho código antes del primer triángulo, y errores difíciles.

### Vulkan 1.3 bajó mucho la barrera de entrada

- **Dynamic rendering:** ya no hacen falta `VkRenderPass` ni `VkFramebuffer` para
  dibujar. Los attachments se indican al grabar el command buffer.
- **Synchronization2:** una API de sincronización más clara y granular.
- La documentación oficial de Khronos ya tiene un *Hello Triangle* y un tutorial
  completo escritos con estas features.

## 3. Lo que cada opción enseña

| Tema | OpenGL 4.6 | Vulkan 1.3+ |
|---|---|---|
| Pipeline gráfico, shaders, transformaciones, iluminación | ✓ | ✓ |
| Cómo se organiza la memoria de la GPU | Escondido | **Explícito** |
| Sincronización CPU/GPU | Escondido | **Explícito** |
| Command buffers, colas, trabajo en paralelo | No existe | **Explícito** |
| Compilación de pipelines | Escondido | **Explícito** |
| Transferible a DirectX 12 y Metal | Poco | **Mucho** (mismo modelo explícito) |

Un artículo de Eurographics 2022 sobre docencia de gráficos (*The Road to Vulkan*)
argumenta que Vulkan, aunque es verboso y difícil al principio, es **especialmente
adecuado para enseñar**, porque obliga a entender cómo funciona una GPU moderna. Sus
autores recomiendan migrar los cursos introductorios de OpenGL a Vulkan.

## 4. Relevancia para el corte de mallas

El corte genera geometría nueva en tiempo real, y esa geometría tiene que llegar a la
GPU sin causar tirones.

- **En OpenGL:** se actualiza un buffer (`glNamedBufferSubData`) o se usan *persistent
  mapped buffers* (OpenGL 4.4+). Funciona, pero el driver decide si espera a la GPU o
  copia por detrás, y eso puede causar picos que no se ven en el código.
- **En Vulkan:** se escribe en un buffer de *staging* (o directo, si la memoria lo
  permite) y se programa la copia. Típicamente se usa un **ring buffer** por frame en
  vuelo. El costo es explícito y predecible, pero hay que diseñarlo.

Las dos permiten **compute shaders** (OpenGL desde 4.3), por si en el futuro parte del
corte se hace en la GPU.

## 5. Dependencias que trae cada opción

| | OpenGL 4.6 | Vulkan 1.3+ |
|---|---|---|
| Driver | Mesa (iris) | Mesa (ANV) |
| Carga de funciones | Un loader (glad o `SDL_GL_GetProcAddress`) | El loader de Vulkan (paquete del sistema) |
| Shaders | El driver compila GLSL | Hay que compilar GLSL a **SPIR-V** con glslang o shaderc |
| Opcionales comunes | — | **VMA** (allocator de memoria) y **vk-bootstrap** (inicialización) |

**VMA** (Vulkan Memory Allocator, de AMD) resuelve justo la subasignación de memoria.
Usarla ahorraría mucho trabajo, pero escribir un allocator propio es un gran
aprendizaje y conecta con el ADR-0010. Es una decisión aparte, si se elige Vulkan.

## 6. Herramientas de depuración (gratis)

- **RenderDoc:** captura un frame y muestra cada draw call, buffer y textura. Sirve
  con OpenGL y con Vulkan.
- **Validation layers de Vulkan:** avisan en tiempo de ejecución de cada uso incorrecto
  de la API. Es probablemente la mejor herramienta de aprendizaje de Vulkan.
- **`KHR_debug` en OpenGL:** mensajes del driver, con menos detalle.

## 7. El hardware de desarrollo

- **GPU:** Intel UHD 620 (Kaby Lake-R, Gen9.5), integrada, con **memoria compartida**
  con la CPU.
- **Drivers:** Mesa 26.2. ANV para Vulkan e iris para OpenGL.
- **Verificado con `vulkaninfo` y `eglinfo` (2026-10-18):**

  | Dato | Valor |
  |---|---|
  | Vulkan (`apiVersion`) | 1.4.354 |
  | `dynamicRendering` / `synchronization2` | `true` / `true` |
  | Heaps de memoria | 1 heap de 5,61 GiB, compartido con la CPU |
  | Tipos de memoria | `DEVICE_LOCAL`, `DEVICE_LOCAL + HOST_VISIBLE + HOST_COHERENT`, y lo mismo con `HOST_CACHED` |
  | `maxMemoryAllocationCount` | 4294967295 (la especificación solo garantiza 4096) |
  | OpenGL | 4.6 Core Profile |

  Que exista memoria `DEVICE_LOCAL + HOST_VISIBLE` es lo que permite que la GPU lea
  directo lo que escribe la CPU, sin copia de staging.
- **Consecuencia de diseño:** es una GPU modesta. Que el juego corra bien aquí es una
  buena restricción para un engine que dice "el rendimiento depende de mis
  decisiones". Pero obliga a ser cuidadoso con el presupuesto de draw calls, de
  ancho de banda y de geometría generada por el corte.

## 8. Opciones descartadas rápido

- **WebGPU** (wgpu-native o Dawn): es una API moderna y portable, pero en C++ nativo es
  otra capa de abstracción por encima de Vulkan, parecida a `SDL_GPU`. Se aleja del
  "el rendimiento depende de mis decisiones".
- **DirectX 12 y Metal:** no existen en Linux.

## Fuentes

- Khronos, lanzamiento de Vulkan 1.3 y roadmap:
  <https://www.khronos.org/news/permalink/https-www.khronos.org-news-press-vulkan-reduces-fragmentation-and-provides-roadmap-visibility-for-developers>
- Vulkan Samples, *Hello Triangle with Vulkan 1.3 Features*:
  <https://docs.vulkan.org/samples/latest/samples/api/hello_triangle_1_3/README.html>
- Vulkan Tutorial, *Dynamic Rendering*:
  <https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/02_Graphics_pipeline_basics/03_Dynamic_rendering.html>
- Vulkan Tutorial, *Synchronization 2*:
  <https://docs.vulkan.org/tutorial/latest/Synchronization/introduction.html>
- Unterguggenberger et al., *The Road to Vulkan: Teaching Modern Low-Level APIs in
  Introductory Graphics Courses*, Eurographics Education 2022:
  <https://diglib.eg.org/handle/10.2312/eged20221043>
- *Approaching Zero Driver Overhead*, GDC 2014: <https://gdcvault.com/play/1020791/Approaching-Zero-Driver-Overhead-in>
- Phoronix, *OpenGL 4.6 Released With Vulkan/SPIR-V Ingestion*:
  <https://www.phoronix.com/review/gl46-spirv-parallel-shad>
- InformIT, *OpenGL Is Alive and Well* (DSA en 4.5):
  <https://informit.com/articles/article.aspx?p=2437085>
- GPUOpen, *Vulkan Memory Allocator*: <https://gpuopen.com/vulkan-memory-allocator/>
- LWN, *Mesa 25.0 con Vulkan 1.4*: <https://lwn.net/Articles/1010898>
- RenderDoc: <https://renderdoc.org/>
