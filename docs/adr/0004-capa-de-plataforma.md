# ADR-0004: Capa de plataforma (ventana, input y tiempo)

- **Estado:** Aceptado
- **Fecha:** 2026-10-18
- **Ticket:** #8
- **Reemplaza / Reemplazado por:** —

## Contexto y problema

Antes de dibujar un solo triángulo, el engine necesita hablar con el sistema
operativo. Esta capa de plataforma tiene que:

- **abrir una ventana** y darle a la API gráfica dónde dibujar (una *surface* de
  Vulkan o un contexto de OpenGL);
- **leer el input:** teclado, mouse (incluido el **movimiento relativo** para una
  cámara en tercera persona) y gamepad;
- **medir el tiempo** con alta resolución, para el loop del juego y la física;
- más adelante, **audio**.

La restricción "desde cero con criterio" ya acepta una capa mínima de ventana e input
como dependencia externa. Este ADR decide **cuál**, o si se escribe propia.

**Entorno de desarrollo actual:**

- Linux con sesión **Wayland** y el compositor **Hyprland**.
- GPU integrada **Intel UHD 620** (Kaby Lake-R) con Mesa 26.2.
- Ya instalados: SDL3 3.4.16, GLFW 3.5.1, libdecor y libxkbcommon.

## Criterios

1. **Cuánto queda "desde cero":** que la dependencia no haga el trabajo que el
   proyecto quiere hacer por su cuenta (renderer, física, etc.).
2. **Aprendizaje:** qué se entiende del sistema operativo.
3. **Cobertura de lo que el juego necesita:** ventana, mouse relativo, gamepad,
   tiempo y audio.
4. **Wayland primero, X11 de respaldo:** que funcione bien en el entorno actual y en
   otros escritorios.
5. **Portabilidad futura:** poder llevar el juego a Windows sin reescribir la capa.
6. **Costo de mantenimiento** para una sola persona.
7. **Licencia** compatible con un repo público.

## Opciones

### 1. SDL3

Es una biblioteca en C, con licencia zlib, de la que se usaría **solo** una parte:
ventana, input, tiempo y, más adelante, audio.

- **A favor:**
  - Cubre todo lo que el juego necesita, incluidos **gamepads** con base de datos de
    mapeos y **audio** (con PipeWire como prioridad en Linux).
  - En SDL 3.2+, Wayland es el driver por defecto cuando el compositor soporta los
    protocolos necesarios, y X11 queda de respaldo.
  - Funciona en Linux, Windows y macOS.
  - Muy probada en la industria. Muchos ports de juegos a Linux la usan.
- **En contra:**
  - Es grande. Incluye un renderer 2D (`SDL_Renderer`) y una API gráfica propia
    (`SDL_GPU`) que **no se deben usar**, porque harían el trabajo del renderer
    propio. Hay que ponerse esa regla de forma explícita.
  - Esconde mucho del sistema operativo.

### 2. GLFW

Es una biblioteca en C, con licencia zlib, especializada en ventana, contexto e input.

- **A favor:**
  - Es **mucho más chica y enfocada** que SDL3: hace casi solo lo que este ADR
    necesita.
  - Desde la 3.4 elige la plataforma (Wayland o X11) en tiempo de ejecución, mejoró
    el soporte de Wayland y usa libdecor para las decoraciones.
  - Funciona en Linux, Windows y macOS.
- **En contra:**
  - **No tiene audio.** Haría falta otra dependencia o escribirlo propio.
  - Su soporte de gamepads es más básico que el de SDL3.
  - La capa de plataforma quedaría repartida entre varias bibliotecas: la ventana con
    GLFW y el audio con otra.

### 3. Capa propia sobre Wayland (y X11)

El engine habla directamente con `libwayland-client`, como hacen los toolkits.

- **A favor:**
  - Es **el máximo aprendizaje** sobre cómo funciona un sistema de ventanas moderno.
  - Cero dependencias de ventana.
  - En Hyprland es el caso más simple, porque el compositor no exige decoraciones.
- **En contra:**
  - **Es mucho trabajo antes del primer frame**, y casi nada de ese trabajo es lo que
    el proyecto quiere mostrar. Hay que implementar:
    - el registro de globals y el protocolo `xdg-shell` para tener una ventana;
    - **decoraciones del lado del cliente**: en GNOME, por ejemplo, la ventana no
      tiene barra de título si no se dibuja (o se usa `xdg-decoration` o libdecor);
    - el teclado con `libxkbcommon`, para traducir keycodes a símbolos según el
      layout;
    - el **mouse relativo** de la cámara, que en Wayland necesita dos protocolos
      extra (`relative-pointer` y `pointer-constraints`);
    - los gamepads con evdev/libinput;
    - el audio con PipeWire o ALSA;
    - y todo de nuevo para X11 y para Windows si algún día se quiere portar.
  - Es una superficie grande para los bugs, en un área que **no** es el foco del
    proyecto.

## Investigación

### Qué es "la capa de plataforma" en un engine

En la arquitectura clásica de un engine, la capa de plataforma (*platform layer* o
*platform abstraction layer*) aísla al resto del engine del sistema operativo. La regla
de diseño importante **no depende de la biblioteca elegida**: el engine solo habla con
**una interfaz propia** (por ejemplo `platform::Window`, `platform::Input`), y la
biblioteca queda escondida detrás. Así se puede cambiar de biblioteca, o escribir una
capa propia más adelante, sin tocar el renderer ni el gameplay.

### SDL3

- SDL 3.2 fue la primera versión estable de SDL3 (enero de 2025).
- Trae Wayland por defecto, PipeWire por defecto para el audio, alta densidad de
  píxeles (*high DPI*), tabletas gráficas y soporte inicial de HDR.
- Licencia zlib.
- Que incluya `SDL_GPU` no es un problema si no se usa. Pero la tentación existe, y
  eso es un argumento para dejar la regla escrita en este ADR.

### GLFW

- GLFW 3.4 agregó la selección de plataforma en tiempo de ejecución (`GLFW_PLATFORM`),
  Wayland y X11 activados por defecto, decoraciones vía libdecor y soporte para un
  allocator de memoria propio.
- Su alcance está limitado a propósito: ventana, contexto e input.

### Capa propia en Wayland: dónde está la complejidad

Wayland deja al cliente muchas responsabilidades que en X11 o en Windows hacía el
servidor. Por ejemplo:

- **Las decoraciones.** El cliente las dibuja, salvo que el compositor ofrezca
  `xdg-decoration` en modo servidor.
- **El mouse relativo.** Requiere protocolos opcionales, no el núcleo.

Escribir un cliente Wayland es un excelente ejercicio, pero en este proyecto es un
**desvío** con respecto a la feature estrella.

### Resumen

| Criterio | SDL3 | GLFW | Capa propia |
|---|---|---|---|
| "Desde cero" | Medio (con regla de no usar `SDL_GPU`/`SDL_Renderer`) | **Alto** | **Máximo** |
| Aprendizaje del SO | Bajo | Bajo | **Máximo** |
| Cobertura (ventana, mouse relativo, gamepad, tiempo, audio) | **Completa** | Parcial (sin audio) | Lo que se escriba |
| Wayland + X11 | **Sí** | **Sí** | Hay que escribir los dos |
| Portabilidad a Windows | **Sí** | **Sí** | Reescritura |
| Mantenimiento | **Bajo** | Bajo + otra dependencia de audio | **Alto** |
| Licencia | zlib | zlib | — |

## Decisión

Elijo **SDL3**. Es la opción más completa y además muy usada en la industria.

Se usa **solo** para ventana, input, tiempo y audio, y queda **escondida detrás de una
interfaz propia** del engine. `SDL_GPU` y `SDL_Renderer` **están prohibidos**. La idea
es que SDL3 ayude a no reinventar la rueda, pero el auto lo tengo que construir yo.

Lo que sacrifico: está la tentación de facilitarme muchos pasos con todas las
integraciones que trae SDL3, en vez de resolverlos yo.

## Consecuencias

- **Regla de uso:** prohibidos `SDL_GPU` y `SDL_Renderer`. Cualquier otro subsistema
  de SDL3 que se quiera usar fuera de ventana, input, tiempo y audio necesita
  justificarse en la revisión, o en un ADR si es grande.
- **Interfaz propia:** el engine habla con `platform::` (ventana, input, tiempo), y
  **ningún tipo de SDL sale del módulo de plataforma**. Por ejemplo, las teclas usan
  un enum propio y no `SDL_Scancode`.
- **Wayland por defecto**, con X11 de respaldo. Funciona en Hyprland y en otros
  escritorios, y deja abierto un port a Windows.
- **Cómo se integra SDL3** al build (paquete del sistema o copia dentro del repo) lo
  decide el ADR-0006.
- **El alcance de SDL3 en audio** (solo salida al dispositivo, o también mezcla) se
  decide cuando llegue el ADR de audio.
- Un backend propio sobre Wayland queda como posible proyecto aparte, fuera del
  alcance de este.

## Cómo lo defiendo

**El proyecto dice ser "desde cero". ¿Usar SDL no contradice eso?**
"Desde cero" es una hipérbole. Se refiere a trabajar a bajo nivel y a evitar los
motores de videojuegos tradicionales, como Unity, Unreal o Godot, no a un cero literal.

**¿Por qué no escribiste tu propia capa sobre Wayland, si querías aprender?**
Quiero aprender, pero enfocada en lo que se usa en la industria, y aprender a usar SDL
también es importante. Crear una capa realmente propia, desde cero, podría ser otro
proyecto. Este no lo abarca.

**Si mañana SDL deja de servirte, ¿cuánto del engine tendrías que reescribir?**
Solo la capa de plataforma. El uso de SDL queda separado y abstraído detrás de esa
capa, así que al cambiarlo solo cambia la plataforma y el resto del engine no se
entera.

**¿Cómo se entera tu engine de que el jugador movió el mouse? ¿Qué pasa entre el
movimiento físico y tu código?**
Los movimientos llegan a una cola de eventos, que se vacía una vez por frame. Al
vaciarla, los eventos se convierten en el estado de input que usa el juego, y ese
estado termina en acciones en pantalla.

## Referencias

- SDL 3.2, primera versión estable (Wayland por defecto, PipeWire, HDR):
  <https://blog.desdelinux.net/en/sdl-3-2-the-first-official-stable-version-arrives-with-improvements-for-wayland-new-apis-and-more/>
- Repositorio de SDL: <https://github.com/libsdl-org/SDL>
- GLFW 3.4, notas de versión: <https://www.glfw.org/docs/3.4/news.html>
- Phoronix, *GLFW 3.4 Brings Better Support For Wayland & Run-Time Platform Selection*:
  <https://www.phoronix.com/news/GLFW-3.4-Released>
- Ejemplo de decoraciones en Wayland (`xdg-decoration` y subsurfaces):
  <https://git.tdem.in/christianrauch/wayland_window_decoration_example>
- Wayland Book (Drew DeVault), introducción a la programación de clientes Wayland:
  <https://wayland-book.com/>
- Jason Gregory, *Game Engine Architecture*, 4.ª ed., capítulo de la capa de
  independencia de plataforma.
