# ADR-0006: Build system y toolchain

- **Estado:** Aceptado
- **Fecha:** 2026-10-18
- **Ticket:** #10
- **Reemplaza / Reemplazado por:** —
- **Fe de erratas:** 2026-10-28 (#43): el ADR-0021 agrega dos cosas a los presets.
  RelWithDebInfo se compila con `-fno-omit-frame-pointer`, para que `perf` arme las pilas
  con *frame pointers*. Y cada preset fija en `CMakePresets.json` qué herramientas de
  depuración compila, con una macro por herramienta. Además, Dear ImGui y Tracy entran
  con `FetchContent` y una versión fijada, solo en Debug y RelWithDebInfo. Se corrige en
  las Decisiones 5 y 7.

## Contexto y problema

Ya está decidido que el engine se escribe en **C++20** (ADR-0003), con **SDL3**
(ADR-0004) y **Vulkan 1.3** con shaders en GLSL compilados a SPIR-V (ADR-0005). Falta
decidir **cómo se compila** todo eso. Este ADR agrupa varias subdecisiones chicas que
dependen unas de otras:

1. el build system;
2. el generador (el programa que realmente ejecuta la compilación);
3. el compilador y el linker;
4. módulos de C++20 o headers tradicionales;
5. cómo entran las dependencias (SDL3 y Vulkan) al build;
6. el compilador de shaders;
7. las configuraciones de build (debug, release) y los sanitizers.

**Herramientas ya instaladas en la máquina de desarrollo:**

- CMake 4.4, Ninja, Meson;
- GCC 16.2, Clang 22.1, `ld.lld`;
- `clang-format`, `clang-tidy`, `clangd`, GDB;
- `glslangValidator` (glslang 1.4.357), `glslc` (shaderc 2026.3), `spirv-val`.

No están instalados Premake, xmake, mold, ccache, LLDB, Valgrind, perf ni RenderDoc.
Tampoco `vulkan-headers` ni `vulkan-validation-layers`, que van a hacer falta antes del
primer código Vulkan.

## Criterios

1. **Estándar de la industria:** que lo que se aprende sirva en otros proyectos.
2. **Integración con herramientas:** `clangd` (autocompletado en el editor),
   `clang-tidy`, depuradores y, más adelante, CI.
3. **Reproducibilidad:** que el proyecto compile igual en otra máquina y en el futuro.
4. **Velocidad de compilación** en el ciclo de editar → compilar → probar.
5. **Simplicidad** para una sola persona.
6. **Detección temprana de errores de memoria** (ligado al ADR-0010).

## Opciones

### 1. Build system

| Opción | A favor | En contra |
|---|---|---|
| **CMake** | Es el estándar de facto en C++. SDL3, Vulkan y casi toda biblioteca lo soportan nativamente. Tiene *presets* y genera `compile_commands.json` para `clangd`. Funciona con Ninja | Su lenguaje de scripting es engorroso, y hay mucho CMake "antiguo" en internet que enseña malas prácticas |
| **Meson** | Su sintaxis es limpia y legible, es rápido con Ninja y lo usan Mesa y GNOME | Es menos común en la industria de videojuegos, y algunas bibliotecas solo lo soportan vía CMake |
| **Premake** | Históricamente usado en gamedev, se escribe en Lua y genera proyectos para Visual Studio | No está instalado, su versión 5 lleva años en beta, y en Linux genera Makefiles, más lentos que Ninja |

### 2. Generador

**Ninja** frente a Make. La propia documentación de Meson muestra que los builds con
Ninja son más rápidos que los basados en Make, más de un 20 % en su comparación con
CMake. Además, `import std` de C++20 solo funciona con Ninja.

### 3. Compilador y linker

| Opción | A favor | En contra |
|---|---|---|
| **Clang como principal, GCC como secundario** | Mensajes de error claros, y el mismo frontend que `clangd` y `clang-tidy`. Compilar con dos compiladores atrapa código no portable | Hay que mantener dos configuraciones |
| **Solo GCC** | Es el compilador por defecto del sistema | Las herramientas de Clang (`clang-tidy`, `clangd`) analizan con otro frontend |
| **Solo Clang** | Una sola configuración | No se detecta el código que solo compila por particularidades de Clang |

**Linker:** `ld.lld` ya está instalado y es mucho más rápido que el `ld` clásico.
`mold` es todavía más rápido, pero no está instalado.

### 4. Módulos de C++20 o headers

CMake escanea módulos con GCC 14+ y Clang 16+, pero `import std` sigue siendo
**experimental** (requiere una bandera experimental y solo funciona con Ninja). Ver el
ADR-0003.

- **Headers tradicionales (`#pragma once`):** son universales, funcionan con todas las
  herramientas, y es lo que se encuentra en la mayoría de los engines.
- **Módulos:** compilan más rápido a largo plazo y aíslan mejor, pero hoy dan
  problemas en `clangd`, en `clang-tidy` y en algunos sistemas.

### 5. Dependencias (SDL3 y Vulkan)

La documentación oficial de SDL3 (`README-cmake`) presenta las dos formas como
válidas, e incluso sugiere una opción de CMake para elegir entre ellas:

| Opción | A favor | En contra |
|---|---|---|
| **Paquete del sistema** (`find_package`) | Es lo más simple, no aumenta el tamaño del repo, y Arch tiene versiones recientes | Cada máquina puede tener otra versión. En Windows no hay "paquete del sistema" |
| **Copia dentro del repo** (submódulo de git o `FetchContent` con versión fijada) | Es reproducible: todos compilan exactamente la misma versión, y funciona igual en Windows | La primera compilación es más lenta y hay que actualizar la versión a mano |
| **Opción de CMake para elegir** | Lo mejor de las dos: el sistema en desarrollo y la versión fijada para reproducibilidad | Hay dos caminos que mantener |

**Vulkan** es distinto: el *loader* y los *headers* son parte del sistema o del SDK.
Lo normal es usar siempre `find_package(Vulkan)`.

### 6. Compilador de shaders

| Opción | A favor | En contra |
|---|---|---|
| **`glslc` (shaderc)** | Usa banderas al estilo GCC/Clang (`-O`, `-MD` para dependencias), fácil de integrar al build. Soporta `#include` | Es un wrapper de glslang, una capa más |
| **`glslangValidator`** | Es el compilador de referencia de Khronos y da control más fino | Su interfaz de línea de comandos es menos amigable para el build |
| **Slang** | Lenguaje moderno con módulos y genéricos. Khronos lo adoptó | No está instalado, y es otro lenguaje que aprender, encima de Vulkan |

Los dos primeros ya están instalados, igual que `spirv-val` para validar el SPIR-V
generado.

### 7. Configuraciones y sanitizers

- **Debug:** sin optimizar, con símbolos, **AddressSanitizer + UndefinedBehaviorSanitizer**
  y validation layers de Vulkan.
- **RelWithDebInfo:** optimizado, con símbolos, para perfilar.
- **Release:** optimizado, sin validation layers.
- **CMake Presets** (`CMakePresets.json`) para que cada configuración se active con un
  solo comando (`cmake --preset debug`).

## Investigación

- **CMake** es la opción que más se repite en el ecosistema: SDL3 documenta su
  integración por CMake en `README-cmake`, Vulkan tiene `find_package(Vulkan)`
  oficial, y `FetchContent` está en CMake desde la 3.11. La crítica más común es su
  lenguaje, que se mitiga usando solo **CMake moderno**, basado en *targets*
  (`target_link_libraries`, `target_compile_options`) en vez de variables globales.
- **Meson** documenta comparaciones de rendimiento donde los builds con Ninja (Meson y
  CMake+Ninja) son los más rápidos. La diferencia entre Meson y CMake+Ninja es mínima.
- **shaderc y glslang:** shaderc envuelve a glslang y a SPIRV-Tools, con una interfaz
  pensada para integrarse a build systems. glslang es el frontend de referencia de
  Khronos.
- **Módulos:** ver la investigación del ADR-0003. El soporte existe, pero `import std`
  todavía es experimental en CMake.

## Decisión

| # | Subdecisión | Elección |
|---|---|---|
| 1 | Build system | **CMake**, solo CMake moderno basado en *targets* |
| 2 | Generador | **Ninja** |
| 3 | Compilador y linker | **Clang** como principal, **GCC** como secundario, linker **`lld`** |
| 4 | Módulos o headers | **Headers con `#pragma once`.** Los módulos se reevalúan cuando `import std` deje de ser experimental |
| 5 | Dependencias | **SDL3:** opción de CMake para elegir entre el paquete del sistema (por defecto) y una copia con versión fijada vía `FetchContent`. **Vulkan:** siempre del sistema (`find_package(Vulkan)`). **Dear ImGui y Tracy** (ADR-0021): `FetchContent` con versión fijada, solo en Debug y RelWithDebInfo |
| 6 | Compilador de shaders | **`glslc`**, con validación con `spirv-val` en Debug |
| 7 | Configuraciones | **Debug** (ASan + UBSan + validation layers), **RelWithDebInfo** (con `-fno-omit-frame-pointer`, ADR-0021) y **Release**, definidas en `CMakePresets.json`. Cada preset fija qué herramientas de depuración compila (ADR-0021) |

## Consecuencias

- **Archivos de build** (`CMakeLists.txt`, `CMakePresets.json`): son periféricos y
  los puede escribir Claude, con revisión del autor.
- **Opción `MZ_VENDOR_SDL3`** (apagada por defecto): cuando se enciende, CMake
  descarga SDL3 en una versión fija con `FetchContent`.
- **Paquetes del sistema necesarios** antes del primer código Vulkan:
  `vulkan-headers` y `vulkan-validation-layers`.
- **Los shaders son parte del build:** cada `.vert`/`.frag`/`.comp` se compila a
  `.spv` con `glslc`, que registra las dependencias de `#include` para recompilar solo
  lo que cambió.
- **Dos compiladores:** el código tiene que compilar sin warnings con Clang y con GCC.
  La CI, cuando exista, compila con los dos.
- **`compile_commands.json`** se genera siempre, para que `clangd` y `clang-tidy`
  entiendan el proyecto.
- Las optimizaciones del ciclo de build (`mold`, `ccache`) quedan para cuando los
  tiempos de compilación lo justifiquen.

## Cómo lo defiendo

**CMake es famoso por ser horrible. ¿Por qué lo elegiste en vez de Meson, que es más
limpio?**
Es el estándar de la industria y, al final del día, funciona muy bien. Que sea
horrible o no depende más de cómo lo implemente yo.

**¿Por qué compilas con dos compiladores si con uno basta?**
Por rigurosidad. Compilar con dos compiladores atrapa mejor lo que se sale del
estándar completo. Con uno solo, esos problemas no aparecen hasta que se prueba el
juego en un sistema donde sí falla.

**¿Para qué sirven los sanitizers, y por qué solo en Debug?**
Ayudan a depurar en tiempo de ejecución: detectan errores que el compilador no puede
ver. Tienen un overhead alto, que el jugador no notaría como beneficio. No le son
útiles y solo le ocuparían recursos.

## Referencias

- SDL, *README-cmake*: <https://github.com/libsdl-org/SDL/blob/main/docs/README-cmake.md>
- CMake, *FetchContent*: <https://cmake.org/cmake/help/v4.3/module/FetchContent.html>
- CMake, *cmake-cxxmodules(7)*: <https://www.mankier.com/7/cmake-cxxmodules>
- Meson, *A simple comparison*: <https://mesonbuild.com/Simple-comparison.html>
- Meson, *Comparing Meson with other build systems*: <https://mesonbuild.com/Comparisons.html>
- Khronos, *What is SPIR-V*: <https://docs.vulkan.org/guide/latest/what_is_spirv.html>
- LunarG, *SPIR-V Toolchain*: <https://vulkan.lunarg.com/doc/view/latest/windows/spirv_toolchain.html>
