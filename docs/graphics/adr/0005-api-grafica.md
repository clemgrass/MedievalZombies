# ADR-0005: API gráfica

- **Estado:** Aceptado
- **Fecha:** 2026-10-18
- **Ticket:** #9
- **Reemplaza / Reemplazado por:** —

## Contexto y problema

El renderer es propio (ver `CLAUDE.md`), pero tiene que hablar con la GPU a través de
una API gráfica. La elección define:

- cuánto del trabajo de la GPU (memoria, sincronización, compilación de pipelines)
  queda en manos del driver y cuánto en las del autor;
- la curva de aprendizaje y el tiempo hasta el primer triángulo;
- cómo llega a la GPU la geometría que genera el corte de mallas en tiempo real.

Las restricciones ya decididas:

- **C++20** (ADR-0003): las APIs gráficas son APIs en C y se usan directamente.
- **SDL3** como capa de plataforma (ADR-0004), que crea la *surface* de Vulkan o el
  contexto de OpenGL. `SDL_GPU` está prohibida.
- Linux como plataforma principal, **GPU de desarrollo Intel UHD 620 integrada** con
  Mesa 26.2.

La investigación completa está en
[`../research/api-grafica.md`](../research/api-grafica.md).

## Criterios

1. **Valor para la industria:** qué modelo de API se espera de un programador de
   gráficos o de engine.
2. **Valor académico:** qué tanto obliga a entender cómo funciona una GPU moderna.
3. **Control del rendimiento:** que el costo dependa de decisiones explícitas, no de
   heurísticas del driver.
4. **Aprendizaje y curva:** cuánto cuesta llegar a algo visible, y cuánto se entiende
   en el camino.
5. **Geometría dinámica:** qué tan bien soporta subir la geometría del corte sin
   tirones.
6. **Hardware disponible:** que funcione bien en la UHD 620.
7. **Depuración:** herramientas para entender qué está pasando.

## Opciones

### 1. OpenGL 4.6

- **A favor:**
  - Se llega rápido a dibujar.
  - DSA (4.5) y AZDO (4.6) modernizan mucho la API.
  - Hay muchísimo material de aprendizaje.
  - El driver compila GLSL directamente.
  - Funciona bien en la UHD 620.
- **En contra:**
  - El modelo de máquina de estados **esconde** la memoria, la sincronización y la
    compilación de pipelines.
  - El rendimiento depende de heurísticas del driver.
  - En la industria es una API en retirada, y su modelo transfiere poco a DirectX 12
    y Metal.

### 2. Vulkan 1.3+

- **A favor:**
  - Es explícita: memoria, sincronización y pipelines quedan en manos del autor.
  - Su modelo es el mismo que el de DirectX 12 y Metal.
  - Las validation layers son una herramienta de aprendizaje excelente.
  - Desde 1.3, *dynamic rendering* y *synchronization2* reducen mucho el código de
    inicio.
- **En contra:**
  - Es muy verbosa, y hay mucho trabajo antes del primer triángulo.
  - Los errores de sincronización son difíciles de depurar.
  - Necesita compilar los shaders a SPIR-V, otra herramienta del build.
  - Abre la decisión de usar o no VMA y vk-bootstrap.

### 3. OpenGL primero y Vulkan después

- **A favor:**
  - Resultados visibles temprano para aprender los conceptos de gráficos (pipeline,
    shaders, iluminación).
  - Después se migra a Vulkan con esos conceptos ya entendidos.
- **En contra:**
  - **Se escribe el renderer dos veces**, o hay que diseñar desde el principio una
    abstracción que sirva para los dos modelos, lo que es difícil porque son muy
    distintos.
  - La migración tiende a postergarse indefinidamente.

### 4. WebGPU (descartada)

Es una capa de abstracción más sobre Vulkan, parecida a `SDL_GPU`. Choca con "el
rendimiento depende de mis decisiones". Ver la nota de investigación.

## Investigación

Resumen de la nota [`../research/api-grafica.md`](../research/api-grafica.md):

- **Implícito frente a explícito.** OpenGL deja que el driver decida cuándo subir
  datos, cuándo esperar y cómo organizar la memoria. Vulkan obliga al autor a
  decidirlo todo. Esa es justamente la restricción del proyecto: el rendimiento tiene
  que depender de decisiones que el autor entienda y controle.
- **Docencia.** Un artículo de Eurographics Education 2022 (*The Road to Vulkan*)
  argumenta que Vulkan es especialmente adecuado para enseñar gráficos, porque obliga
  a entender la GPU moderna, y recomienda migrar los cursos introductorios desde
  OpenGL.
- **Vulkan 1.3** eliminó gran parte del código ceremonial (`VkRenderPass`,
  `VkFramebuffer`) con *dynamic rendering*, y simplificó la sincronización con
  *synchronization2*.
- **Corte de mallas.** En OpenGL, la subida de geometría dinámica puede causar esperas
  ocultas del driver. En Vulkan se diseña explícitamente, con un ring buffer de
  staging por frame en vuelo. Es más trabajo, pero el costo es predecible.
- **Memoria de la GPU en Vulkan.** Hay un límite de asignaciones, así que se
  subasigna. Es la misma idea de arenas y pools del ADR-0010, y podría compartir
  diseño con él.
- **Hardware, verificado con `vulkaninfo` el 2026-10-18.** La UHD 620 (ANV, Mesa
  26.2.2) expone:
  - **Vulkan 1.4.354.** El mínimo de este ADR es 1.3, para no exigir más de lo
    necesario.
  - `dynamicRendering` y `synchronization2` disponibles.
  - Un solo heap de 5,61 GiB compartido con la CPU, con tipos de memoria
    `DEVICE_LOCAL | HOST_VISIBLE | HOST_COHERENT`, con y sin `HOST_CACHED`. Eso
    habilita la ruta sin copia de GPU integrada.
  - `maxMemoryAllocationCount` sin límite práctico en este driver. Igual se
    subasigna, porque la especificación solo garantiza 4096 y muchos drivers se
    quedan en ese mínimo.
  - OpenGL 4.6 Core también está disponible, como referencia.

  Es una GPU modesta, así que el presupuesto de geometría y draw calls importa desde
  el principio.

| Criterio | OpenGL 4.6 | Vulkan 1.3+ | OpenGL → Vulkan |
|---|---|---|---|
| Valor para la industria | Medio | **Alto** | Alto (al final) |
| Valor académico | Medio | **Alto** | Alto (al final) |
| Control del rendimiento | Bajo | **Alto** | Bajo → Alto |
| Tiempo al primer triángulo | **Corto** | Largo | **Corto** |
| Aprendizaje total | Medio | **Alto** | Alto, pero duplicado |
| Geometría dinámica | Funciona, con esperas ocultas | **Explícita y predecible** | — |
| Depuración | RenderDoc, `KHR_debug` | **RenderDoc, validation layers** | — |
| Riesgo principal | Techo de aprendizaje | Curva inicial y sincronización | Doble trabajo |

## Decisión

Elijo **Vulkan 1.3**. Me da un mayor entendimiento de los procesos que ocurren por
detrás, y me permite controlar muy bien el rendimiento. Eso es crucial, porque quiero
que el juego pueda correr en casi cualquier computador, o por lo menos en este, que
tiene gráficos integrados y no es tan bueno para 3D.

Lo que sacrifico: hay mucha documentación que leer de una API que nunca he usado, y
mucho que aprender.

Sobre las bibliotecas de apoyo:

- **Sin vk-bootstrap:** la inicialización (instancia, GPU física, dispositivo lógico,
  colas, swapchain) la escribo yo.
- **VMA, opción C:** el engine define su propia interfaz de allocator de memoria de la
  GPU (`gpu::Allocator`). Empiezo con la versión más simple posible, escrita por mí, y
  la voy completando. VMA queda como plan B detrás de esa misma interfaz.

## Consecuencias

- **Features obligatorias de Vulkan 1.3:** *dynamic rendering* y *synchronization2*,
  confirmadas en la UHD 620. El engine tiene que verificarlas al iniciar y fallar con
  un mensaje claro si una GPU no las tiene.
- **Validation layers activas** en los builds de desarrollo. Es una dependencia solo
  de desarrollo, el paquete del sistema `vulkan-validation-layers`.
- **Shaders:** GLSL compilado a SPIR-V con glslang o shaderc. Es una herramienta más
  del build, y la elige el ADR-0006.
- **Inicialización propia:** se parte en tickets chicos con `owner:autor` (instancia →
  GPU física → dispositivo y colas → swapchain → limpiar la pantalla de un color →
  triángulo).
- **`gpu::Allocator`:** es la interfaz de memoria de la GPU. Su primera versión es
  simple y propia, y el diseño se comparte con el ADR-0010. VMA solo entra si el
  allocator propio bloquea el avance, y con su propia justificación.
- **Geometría dinámica del corte:** se sube con un ring buffer mapeado de forma
  persistente, una región por frame en vuelo, protegido con fences. El número de
  frames en vuelo y el presupuesto por frame se fijan junto con el ADR-0007 y el
  ADR-0010.
- **GPU dedicada frente a integrada:** hay una sola interfaz con dos implementaciones,
  elegidas al iniciar según los tipos de memoria que reporta Vulkan: copia de staging
  a *device-local*, o acceso directo en memoria compartida. Las dos rutas hay que
  probarlas, aunque la máquina de desarrollo solo tenga GPU integrada.
- **Depuración:** RenderDoc más validation layers.

## Cómo lo defiendo

**Vulkan es famoso por necesitar cientos de líneas para un triángulo. ¿Valió la pena
en un proyecto de una sola persona?**
Sí. Es hacia donde va la industria y lo que está usando, y aprender estos procesos me
sirve a futuro, incluso cuando no use Vulkan. Entender realmente qué ocurre es más
lento, pero al final me deja más cerca de poder crear mis propios sistemas.

**¿Qué hace el driver de OpenGL por ti que en Vulkan tienes que hacer tú?**
Esconde muchas partes que en Vulkan son explícitas:

- **la memoria:** dónde vive cada buffer y cómo se reparte;
- **la sincronización:** cuándo la CPU puede tocar datos que la GPU todavía está
  usando, que en Vulkan se maneja con fences y barreras;
- **la compilación de pipelines:** el driver de OpenGL compila variantes de shaders
  sobre la marcha, y eso puede causar tirones. En Vulkan los pipelines se compilan
  por adelantado.

**¿Cómo llega a la GPU la geometría nueva del corte sin que el juego se trabe?**
Con un *ring buffer* mapeado de forma persistente (*persistent mapped ring buffer*),
con una región por frame en vuelo. Hay que tener cuidado con la gestión de las fences:
si la CPU avanza muy rápido, puede dar la vuelta completa al buffer y empezar a
reescribir datos que la GPU todavía no terminó de usar. Para evitar el overhead, el
buffer se dimensiona para el peor caso posible: el máximo de geometría por frame
multiplicado por el número de frames en vuelo. Más no hace falta, porque la región de
cada frame se reutiliza apenas su fence indica que la GPU terminó con ella.

Además, el engine detecta el tipo de GPU y tiene dos rutas, para rendir bien en ambos
casos. En una GPU dedicada, la geometría se copia de staging a memoria *device-local*.
En una integrada, que comparte memoria con la CPU, la GPU la lee directo, sin copia.

**Tu GPU es integrada y modesta. ¿Por qué no elegiste la API más simple?**
Por esa misma razón: Vulkan me da más libertad para optimizar el rendimiento, en
especial en un hardware modesto.

## Referencias

Ver la lista completa en la [nota de investigación](../research/api-grafica.md#fuentes).
Las principales:

- Unterguggenberger et al., *The Road to Vulkan*, Eurographics Education 2022:
  <https://diglib.eg.org/handle/10.2312/eged20221043>
- Vulkan Tutorial (Khronos), *Dynamic Rendering*:
  <https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/02_Graphics_pipeline_basics/03_Dynamic_rendering.html>
- *Approaching Zero Driver Overhead*, GDC 2014:
  <https://gdcvault.com/play/1020791/Approaching-Zero-Driver-Overhead-in>
- GPUOpen, *Vulkan Memory Allocator*: <https://gpuopen.com/vulkan-memory-allocator/>
