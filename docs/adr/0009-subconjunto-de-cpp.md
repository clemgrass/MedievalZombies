# ADR-0009: Subconjunto de C++

- **Estado:** Aceptado
- **Fecha:** 2026-10-18
- **Ticket:** #18
- **Reemplaza / Reemplazado por:** —

## Contexto y problema

El ADR-0003 eligió C++20 y reconoció su mayor riesgo: es enorme y tiene muchas formas de
hacer lo mismo. Este ADR decide **qué partes del lenguaje se usan, cuáles se evitan y
cuáles solo se permiten en ciertos lugares**.

Lo condicionan las decisiones ya tomadas:

- **Rendimiento predecible** (restricción del proyecto), en especial en el corte de
  mallas.
- **Memoria reservada de antemano:** arena por frame y pools con presupuesto (ADR-0003,
  ADR-0010).
- **Warnings estrictos y `clang-tidy`** (ADR-0013): lo que se decida aquí se puede hacer
  cumplir con la herramienta.

La arquitectura del engine (ECS u orientación a objetos, módulos, flujo del frame) es
del ADR-0012. Este ADR solo decide **qué herramientas del lenguaje** están disponibles
para esa arquitectura.

## Criterios

1. **Rendimiento predecible:** que ninguna característica tenga un costo oculto en el
   código que corre cada frame.
2. **Control de memoria:** que nada asigne memoria a espaldas del autor.
3. **Legibilidad:** que el código se entienda sin conocer los rincones del lenguaje.
4. **Tiempo de compilación.**
5. **Defendible:** que cada característica usada tenga una razón explicable.
6. **Verificable:** que la regla se pueda revisar con el compilador o `clang-tidy`, y no
   dependa solo de la disciplina.

## Opciones

Hay 7 dimensiones que decidir, cada una con sus opciones.

### 1. Excepciones

| Opción | A favor | En contra |
|---|---|---|
| **Usarlas** | Separan el camino del error del camino normal. Es la forma "estándar" de C++ | Su costo es invisible en el código, aumentan el tamaño del binario y obligan a escribir todo el código *exception-safe* |
| **Desactivarlas (`-fno-exceptions`)** | Todo error es explícito en el tipo de retorno. Es lo habitual en engines | Hay que diseñar el manejo de errores. Partes de la STL que lanzan excepciones quedan inutilizables o abortan el programa |

Sin excepciones, los errores se dividen en dos clases:

- **Errores del programador** (un índice fuera de rango, un puntero nulo): `assert`, que
  aborta en Debug con un mensaje claro.
- **Errores esperables** (no se encontró un archivo, la GPU no tiene una feature):
  un tipo de retorno que lleva el resultado o el error. `std::expected` es de C++23,
  así que en C++20 sería un tipo propio y chico (`result<T, E>`).

### 2. RTTI (`dynamic_cast`, `typeid`)

| Opción | A favor | En contra |
|---|---|---|
| **Usarla** | `dynamic_cast` permite preguntar "¿este objeto es de tal tipo?" | Tiene costo en tiempo de ejecución y en tamaño. Suele señalar un diseño que debería resolverse de otra forma |
| **Desactivarla (`-fno-rtti`)** | Sin costo oculto. Es lo que hacen Unreal (por defecto) y LLVM | Si hace falta identificar tipos, se hace con un ID propio y explícito |

### 3. Biblioteca estándar (STL)

| Opción | Qué significa |
|---|---|
| **A. Libre** | Se usa cualquier cosa de la STL |
| **B. Solo lo que no asigna memoria** | `std::span`, `std::array`, `std::string_view`, `std::optional`, `<algorithm>`, `<type_traits>`, `<bit>`, `<numbers>`, `<concepts>`. Los contenedores dinámicos son propios |
| **C. Por zonas** | Lo de la opción B se usa en todas partes. Los contenedores que asignan (`std::vector`, `std::unordered_map`) se permiten **solo fuera del código que corre cada frame** (inicialización, herramientas, carga). En el código de cada frame, contenedores propios sobre las arenas y los pools del ADR-0010 |
| **D. STL con allocators propios (`std::pmr`)** | Contenedores estándar (`std::pmr::vector`) que piden memoria a un recurso propio, por ejemplo una arena (`monotonic_buffer_resource`). Existe desde C++17 |

### 4. Herencia y funciones virtuales

| Opción | Qué significa |
|---|---|
| **Libre** | Jerarquías de clases como en la orientación a objetos clásica |
| **Solo en las fronteras** | `virtual` para interfaces entre módulos con pocas llamadas por frame (por ejemplo los backends de plataforma o del allocator de la GPU). **Nunca** jerarquías profundas ni llamadas virtuales por entidad en el código de cada frame |
| **Prohibida** | Polimorfismo solo en tiempo de compilación (templates) o con tablas de funciones explícitas |

### 5. Templates

| Opción | Qué significa |
|---|---|
| **Libres** | Incluida la metaprogramación compleja |
| **Simples y con `concepts`** | Templates para contenedores y matemática genérica, con restricciones legibles (`template <std::floating_point T>`). Sin metaprogramación "inteligente" |

### 6. Punteros inteligentes y dueños de la memoria

| Opción | Qué significa |
|---|---|
| **`std::shared_ptr` permitido** | Varios dueños compartidos, con conteo atómico de referencias |
| **Solo `std::unique_ptr`**, y fuera del código de cada frame | Un solo dueño, RAII sin costo. Para objetos de larga vida que se crean al iniciar |
| **Handles** | Índices con generación (`handle<mesh_piece>`) en vez de punteros, para los objetos que viven en pools. Se pueden validar y no quedan colgando si el objeto se recicla |

### 7. Características de C++20

| Característica | Uso posible |
|---|---|
| `concepts` | Restringir templates con mensajes de error legibles |
| `std::span` | Pasar "un pedazo de un buffer" sin copiar y sin perder el tamaño |
| *Designated initializers* | Inicializar estructuras de Vulkan legibles (`.sType = …, .flags = …`) |
| `constexpr` / `consteval` | Cálculos en tiempo de compilación (tablas, constantes) |
| `std::bit_cast` | Reinterpretar bits de forma segura (por ejemplo, de `float` a `uint32_t`) |
| `<numbers>` | `std::numbers::pi_v<float>` |
| Corrutinas | Útiles para algunos sistemas de gameplay, pero asignan memoria por defecto |
| Módulos | Ya descartados por el ADR-0006 |

## Investigación

### Lo que hace la industria de videojuegos

- **Unreal Engine** compila por defecto **sin RTTI** (`-fno-rtti`, con `bUseRTTI`
  apagado) y sin excepciones. Su recomendación es activar RTTI solo en el módulo que la
  necesite, y nunca en todo el engine.
- **EASTL.** Electronic Arts reescribió parte de la biblioteca estándar y lo documentó
  ante el comité de C++ (paper **N2271**, 2007). El problema principal era **el diseño
  de los allocators de la STL**: no permitían controlar la alineación, el seguimiento ni
  de dónde sale la memoria. El problema secundario, que faltaban contenedores pensados
  para usar bien la memoria.
- **"Orthodox C++"** es una corriente que usa C++ como "un C mejorado". Sus reglas: sin
  excepciones, sin RTTI y **nada de la STL que asigne memoria**. Proyectos citados que
  siguen este enfoque: DOOM 3 BFG, Dear ImGui, bgfx y The Forge.
- **Google** tampoco usa excepciones en su guía de estilo de C++. Sus razones: el código
  *exception-safe* exige maquinaria y disciplina en todas partes, las excepciones
  aumentan el tamaño del binario, y facilitan usarlas donde no corresponde (por ejemplo,
  ante input inválido del usuario).

### Lo que hacen los sistemas críticos (militares, espaciales, automotrices)

Estos estándares buscan **verificabilidad y cero fallos**, no rendimiento. Pero llegan a
reglas sorprendentemente parecidas a las de los engines:

| Estándar | Origen | Reglas relevantes |
|---|---|---|
| **JSF AV C++** (2005) | Lockheed Martin, para el F-35. Publicado en el sitio de Bjarne Stroustrup | Restringe **excepciones**, **recursión** y **asignación dinámica de memoria**. Se escribió para evitar fallos como el del cohete Ariane 5 |
| **The Power of 10** (2006) | Gerard Holzmann, NASA/JPL, para código C crítico | Regla 1: sin recursión. Regla 2: todos los loops con un límite fijo. **Regla 3: no asignar memoria dinámica después de la inicialización.** Funciones cortas y alta densidad de *assertions* |
| **MISRA C++:2023** | Industria automotriz (fusión con AUTOSAR C++14) | Actualizado para C++17. Reglas pensadas para ser **decidibles por herramientas de análisis estático**, como exige ISO 26262 |

El razonamiento de la regla 3 de JPL es casi el mismo que el de este proyecto: los
allocators y los garbage collectors tienen un comportamiento impredecible, y vivir
dentro de memoria fija reservada de antemano elimina toda una clase de errores (usar
memoria liberada, pedir más de la que existe, escribirse fuera de un bloque) y permite
**probar** cuánta memoria usa el programa.

### La conclusión que cruza todo

Los engines y los sistemas críticos llegan a las **mismas restricciones** (sin
excepciones, sin RTTI, sin asignar memoria en el código crítico, polimorfismo
controlado) por razones distintas:

- los engines, por **rendimiento predecible**;
- los sistemas críticos, por **verificabilidad**.

Las dos son **predictibilidad**: poder saber, leyendo el código, qué va a pasar y cuánto
va a costar.

## Decisión

| # | Dimensión | Elección |
|---|---|---|
| 1 | Excepciones | **Desactivadas (`-fno-exceptions`).** `assert` para los errores del programador y un tipo `result<T, E>` propio para los errores esperables |
| 2 | RTTI | **Desactivada (`-fno-rtti`).** Si hace falta identificar tipos, con IDs propios y explícitos |
| 3 | STL | **Por zonas.** Lo que no asigna memoria se usa en todas partes. Los contenedores que asignan (`std::vector`, `std::unordered_map`) solo fuera del código que corre cada frame. En ese código, contenedores propios sobre las arenas y los pools; si se usa o no `std::pmr`, lo decide el ADR-0010 |
| 4 | Herencia y `virtual` | **Solo en las fronteras entre módulos.** Nunca jerarquías profundas ni llamadas virtuales por entidad en el código de cada frame |
| 5 | Templates | **Simples y con `concepts`.** Sin metaprogramación compleja |
| 6 | Punteros inteligentes | **`std::unique_ptr`** para objetos de larga vida creados al iniciar. **Handles** para lo que vive en pools. **`std::shared_ptr` prohibido** |
| 7 | C++20 | **Sí:** `concepts`, `std::span`, designated initializers, `constexpr`/`consteval`, `std::bit_cast`, `<numbers>`. **Corrutinas no** por ahora |
| 8 | Recursión | **Evitarla en el código de cada frame**, o usarla solo con profundidad acotada y conocida. Fuera de ahí, permitida |

## Consecuencias

- **Banderas globales:** `-fno-exceptions -fno-rtti` en todos los targets del engine,
  definidas en CMake (ADR-0006).
- **La STL que lanza excepciones aborta.** Con `-fno-exceptions`, funciones como
  `std::vector::at` terminan el programa en vez de lanzar una excepción. En la
  práctica, los accesos se validan con `assert` y no con `at()`.
- **Utilidades nuevas** que necesitan su ticket: `result<T, E>`, macros de `assert` con
  mensaje y ubicación, contenedores propios para el código de cada frame y el tipo
  `handle<T>` con contador de generación. Su diseño de memoria se cruza con el ADR-0010
  y su lugar en el engine con el ADR-0012.
- **"El código que corre cada frame"** tiene que quedar identificable (por módulo o por
  convención), porque varias reglas dependen de él. Lo define el ADR-0012.
- **Cómo se hacen cumplir las reglas:** las banderas del compilador hacen cumplir la 1
  y la 2. El resto (`std::shared_ptr` prohibido, STL por zonas, `virtual` solo en
  fronteras) se revisa en cada PR y, cuando exista CI, con verificaciones automáticas
  simples.
- **Dependencias:** SDL3 y Vulkan son APIs en C, así que no se ven afectadas por
  `-fno-exceptions` ni por `-fno-rtti`.

## Cómo lo defiendo

**Sin excepciones, ¿cómo te enteras de que algo falló? Por ejemplo, que no se pudo
cargar una textura.**
Analizando los valores de retorno o de estado, a través del flujo de control del
programa.

**¿Por qué prohibir `std::vector` en el código de cada frame, si es tan rápido?**
Por el peligro de las reasignaciones, el uso constante de `new` y `delete`, y la
pérdida de determinismo dentro del frame.

**¿Qué tiene que ver un estándar de aviones de combate con un videojuego?**
No es directo, y un videojuego no requiere tanta rigurosidad. Pero en esos casos
extremos se observa de verdad qué se necesita y qué no. Además, ahí también importa
mucho el rendimiento: una milésima de segundo puede ser la diferencia entre chocar y
detectar un problema a tiempo. En un videojuego no importa ese nivel de rendimiento,
pero la misma lógica sí importa, para que la experiencia sea fluida.

**¿Qué es un *handle* y por qué es mejor que un puntero para un pedazo de zombie que
vive en un pool?**
Evita los punteros colgantes y permite desfragmentar la memoria.

## Referencias

- Electronic Arts, *EASTL: Electronic Arts Standard Template Library*, N2271 (2007):
  <https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2271.html>
- *Orthodox C++*, resumen y proyectos que lo usan:
  <https://cppdepend.com/blog/orthodox-c-fans-are-they-a-minority-or-a-silent-majority/>
- Google C++ Style Guide, sección de excepciones:
  <https://chromium.googlesource.com/external/github.com/google/styleguide/+/HEAD/cppguide.html>
- Unreal Engine, RTTI desactivada por defecto (`bUseRTTI`):
  <https://answers.unrealengine.com/questions/877365/view.html>
- Lockheed Martin, *Joint Strike Fighter Air Vehicle C++ Coding Standards* (2005):
  <https://stroustrup.com/JSF-AV-rules.pdf>
- Gerard J. Holzmann, *The Power of 10: Rules for Developing Safety-Critical Code*
  (NASA/JPL, 2006): <https://en.wikipedia.org/wiki/The_Power_of_10:_Rules_for_Developing_Safety-Critical_Code>
- MISRA C++:2023, introducción: <https://www.perforce.com/blog/qac/misra-cpp-2023-intro>
