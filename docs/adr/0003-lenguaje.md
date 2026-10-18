# ADR-0003: Lenguaje del engine y del juego

- **Estado:** Aceptado
- **Fecha:** 2026-10-18
- **Ticket:** #6
- **Reemplaza / Reemplazado por:** —

## Contexto y problema

Es la primera decisión técnica del proyecto, y casi todas las demás dependen de ella:
cómo se habla con la API gráfica (ADR-0005), qué build system se usa (ADR-0006), cómo
se escribe la matemática (ADR-0008) y cómo se maneja la memoria al cortar mallas
(ADR-0007).

La restricción del proyecto acota las opciones a **C, C++ o C#**. El resto de las
restricciones también pesa:

- El rendimiento tiene que depender de decisiones que el autor entienda y controle.
- La plataforma principal es Linux.
- El proyecto tiene que servir como portafolio técnico para la industria y para
  programas de posgrado en ingeniería de videojuegos.

**Toolchain disponible hoy en la máquina de desarrollo:** GCC 16.2, Clang 22.1,
CMake 4.4 y .NET SDK 10.0.

## Criterios

1. **Valor para la industria:** qué lenguaje se espera de un programador de engine,
   gráficos o gameplay en estudios grandes.
2. **Valor académico:** qué demuestra frente a programas orientados a gráficos,
   simulación y programación consciente del hardware.
3. **Control de memoria y rendimiento:** sobre todo en el corte de mallas, que crea
   geometría nueva en el peor momento posible, en pleno combate.
4. **Aprendizaje:** cuánto se entiende de lo que pasa "debajo".
5. **Acceso a APIs nativas:** Vulkan, OpenGL, SDL y GLFW son APIs en C.
6. **Ergonomía para código matemático:** vectores, matrices y cuaterniones están en
   todas partes del engine.
7. **Herramientas en Linux:** depuradores, sanitizers y profilers.

## Opciones

### 1. C (C11 o C23)

- **A favor:**
  - El lenguaje más chico y transparente: lo que escribes es casi lo que ejecuta la
    máquina.
  - Todas las APIs gráficas y de plataforma son nativas en C, sin capas intermedias.
  - Compila muy rápido.
  - C23 agrega `constexpr`, `nullptr`, `typeof`, `auto` para inferir tipos y `#embed`
    para incrustar recursos binarios.
- **En contra:**
  - **No tiene sobrecarga de operadores.** La matemática se vuelve verbosa:
    `vec3_add(vec3_scale(n, d), p)` en vez de `p + n * d`, y un engine con corte de
    mallas y física está lleno de eso.
  - Sin templates, los contenedores genéricos se hacen con macros o `void*`.
  - Las ofertas de programador de engine piden casi siempre C++, no C.

### 2. C++ (C++20 o C++23)

- **A favor:**
  - Es el estándar de la industria de engines.
  - Llama a las APIs en C directamente.
  - Tiene sobrecarga de operadores para la matemática.
  - Tiene templates para contenedores propios.
  - RAII permite liberar recursos de forma determinista.
  - Herramientas en Linux de primera: GDB, LLDB, ASan, UBSan, TSan, perf.
- **En contra:**
  - **Es enorme y tiene muchas formas de hacer lo mismo.** Sin disciplina, el código
    puede volverse lento y opaco: jerarquías de herencia profundas, asignaciones
    ocultas en la STL, excepciones.
  - Compila más lento que C.
  - Hay que decidir explícitamente **qué subconjunto** del lenguaje se usa.

### 3. C# (.NET 10, con NativeAOT opcional)

- **A favor:**
  - Es muy productivo y tiene un sistema de tipos seguro.
  - Tiene sobrecarga de operadores, `struct`, `Span<T>`, SIMD (`System.Numerics`,
    `Vector128`) y punteros en bloques `unsafe`.
  - Existen engines completos en C#, como Stride, y bindings a Vulkan, como Silk.NET.
- **En contra:**
  - **Tiene un garbage collector:** el runtime decide cuándo se libera la memoria, no
    el autor.
  - Hablar con Vulkan, OpenGL o SDL requiere una capa de bindings, que es otra
    dependencia más.
  - En los engines comerciales más usados, C# es el lenguaje de *scripting* (Unity),
    no el del núcleo.

## Investigación

### Qué usa la industria

- **Unreal Engine 5** está escrito en C++, y los grandes estudios contratan
  programadores de C++ para su núcleo técnico.
- **Unity** tiene el núcleo en C++ y expone C# como lenguaje de scripting.
- **Stride** (MIT, .NET Foundation) es la prueba de que un engine completo en C# es
  viable: tiene Vulkan y DirectX 12 y usa C# 14 con .NET 10. Es una excepción, no la
  norma.

### El garbage collector y los frames

A 60 FPS cada frame tiene unos 16,6 ms. Una pausa del GC en medio de un frame se ve
como un tirón.

La propia documentación de Unity lo explica. Su GC incremental **no hace más rápida la
recolección**, solo la reparte entre varios frames para reducir los picos. Y la
recomendación oficial sigue siendo **evitar las asignaciones** en el código que corre
cada frame.

Para este proyecto la consecuencia es directa. Cortar una malla genera vértices,
índices y objetos nuevos justo en el momento del golpe. En C#, para que eso no cause
tirones, igual habría que escribir código sin asignaciones (pools, `struct`, `Span`).
Es decir, disciplina manual de memoria, pero con un runtime de por medio que el autor
no controla del todo. En C y en C++ esa misma disciplina, con arenas o pools, es
explícita y no hay pausas posibles.

### C "puro" frente a C++ con estilo C

Hay una corriente fuerte en la industria que privilegia el diseño orientado a datos
sobre la orientación a objetos clásica:

- **Mike Acton** dio la keynote *Data-Oriented Design and C++* en CppCon 2014.
- **Casey Muratori** escribe *Handmade Hero* en un estilo C dentro de C++.

Lo importante es que **el estilo orientado a datos se puede aplicar en C o en C++**.
La pregunta real no es "C o C++", sino **qué subconjunto de C++** usar. Por ejemplo:
sobrecarga de operadores y templates simples sí; jerarquías de herencia profundas y
excepciones no. Si se elige C++, ese subconjunto puede ser su propio ADR.

### Estado de C++ moderno en Linux

CMake soporta *escanear* módulos de C++20 con GCC 14+ y Clang 16+. Pero `import std`
todavía es **experimental** en CMake: necesita una bandera experimental y solo
funciona con Ninja.

Si se elige C++, lo prudente es empezar con headers tradicionales y evaluar módulos más
adelante. Esa decisión le corresponde al ADR-0006 (build system).

### Resumen

| Criterio | C | C++ | C# |
|---|---|---|---|
| Valor para la industria | Medio | **Alto** (estándar) | Bajo para núcleo |
| Valor académico | Alto | **Alto** | Medio |
| Control de memoria | **Total** | **Total** | Parcial (GC) |
| Aprendizaje "debajo" | **Máximo** | Alto (según el subconjunto) | Medio |
| APIs nativas | **Directo** | **Directo** | Con bindings |
| Ergonomía matemática | Baja | **Alta** | Alta |
| Herramientas Linux | Alta | **Alta** | Media |
| Riesgo principal | Verbosidad | Complejidad del lenguaje | Pausas del GC y bindings |

## Decisión

Elijo **C++20**. Es el lenguaje clásico y el estándar de la industria, en especial en
programación de engines.

Lo que sacrifico: tengo que aprender un lenguaje nuevo para mí, conocido por ser
relativamente complejo por lo amplio que es.

Qué partes de C++ se usan y cuáles no, todavía no se decide. Eso queda para un ADR
futuro que acote el subconjunto del lenguaje.

## Consecuencias

- **Se necesita un ADR que acote el subconjunto de C++:** qué se usa y qué se evita
  (por ejemplo excepciones, RTTI, herencia profunda, la STL en el código de cada
  frame). Hasta que exista, cada uso de una característica "pesada" del lenguaje se
  discute en la revisión.
- **Las APIs en C** (Vulkan u OpenGL, SDL o GLFW) se usan directamente, sin capas de
  bindings.
- **La matemática** (ADR-0008) puede usar sobrecarga de operadores, lo que hace más
  legible el código del corte y de la física.
- **El manejo de memoria es responsabilidad del proyecto.** Hará falta diseñar
  allocators (arena por frame, pools con presupuesto) y decidir cómo se garantiza que
  la memoria reservada alcance. Probablemente en su propio ADR, ligado al ADR-0007.
- **Módulos de C++20:** dado el soporte experimental de `import std` en CMake, el
  ADR-0006 tendrá que decidir si se usan módulos o headers tradicionales.
- **Toolchain:** GCC y Clang ya están disponibles en Linux, con sanitizers (ASan,
  UBSan) para detectar errores de memoria temprano.

## Cómo lo defiendo

**C# moderno tiene `struct`, `Span` y SIMD. ¿Por qué no lo usaste?**
C# está más enfocado en Unity, un motor que no me interesa. Sé que con C# se pueden
hacer videojuegos y también motores, pero en este ámbito C++ es lo más clásico.

**Si valoras tanto la transparencia, ¿por qué no C puro?**
C++ puede comportarse de forma muy similar a C. Pero si en el futuro quiero agregar
alguna funcionalidad de C++ que C no tiene, puedo hacerlo, y no al revés. Además, C++
permite la orientación a objetos, que es lo más tradicional en videojuegos, y también
puedo cambiar al diseño orientado a datos si quiero. Eso es de lo mejor de C++: me
mantiene las opciones abiertas, y soy yo quien elige qué camino tomar.

**C++ es enorme. ¿Cómo evitas que el código se vuelva complejo e inmanejable?**
Con un ADR que acote el lenguaje, para usar realmente solo lo que nos interesa.

**¿Qué pasa con la memoria en el instante en que la espada corta al zombie?**
C++ me deja decidir de dónde sale esa memoria y cuándo se libera. Reservarla de
antemano es lo más consciente del rendimiento, aunque exige diseñar exactamente cómo
va a ocurrir y cómo aseguramos que siempre haya suficiente memoria reservada. Una
arena por frame sirve para el trabajo temporal del corte: descartar la geometría vieja
y armar la nueva. Un pool con presupuesto mantiene esa geometría nueva, en especial
cuando el zombie queda en dos partes separadas que caen con la física.

## Referencias

- Unity Manual, *Incremental garbage collection*:
  <https://docs.unity3d.com/Manual/performance-incremental-garbage-collection.html>
- Stride Game Engine: <https://stride3d.net/>
- Mike Acton, *Data-Oriented Design and C++*, CppCon 2014:
  <https://isocpp.org/blog/2015/01/cppcon-2014-data-oriented-design-and-c-mike-acton>
- cppreference, *C23*: <https://en.cppreference.com/c/23>
- CMake, *cmake-cxxmodules(7)*: <https://www.mankier.com/7/cmake-cxxmodules>
- Jason Gregory, *Game Engine Architecture*, 4.ª ed. Es la referencia general de
  arquitectura de engines, con ejemplos en C++.
- Panorama de lenguajes en la industria (fuente secundaria, de divulgación):
  <https://langpop.com/blog/programming-languages-game-development>
