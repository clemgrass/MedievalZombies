# ADR-0008: Biblioteca de matemática y convenciones de coordenadas

- **Estado:** Aceptado
- **Fecha:** 2026-10-18
- **Ticket:** #12
- **Reemplaza / Reemplazado por:** —

## Contexto y problema

Todos los sistemas del engine hablan en vectores, matrices y rotaciones: el renderer
(cámara, proyección), la física (posiciones, velocidades, colisiones), la animación
(huesos, interpolación) y, sobre todo, el **corte de mallas**, que es geometría pura:
planos, intersecciones de segmentos con planos y clasificación de vértices a un lado u
otro del tajo. El ADR-0012 ubicó la matemática en `engine/core`, la capa más baja, de la
que dependen todas las demás.

Hay que decidir dos cosas, y la segunda importa tanto como la primera:

1. **¿Biblioteca propia o externa** (por ejemplo GLM)?
2. **¿Qué convenciones usa?** Orientación de los ejes, eje "arriba", unidades, cómo se
   multiplican y guardan las matrices, cómo es la proyección para Vulkan, cómo se
   representan las rotaciones y cómo se comparan números de punto flotante.

Las convenciones son lo más caro de cambiar después: un error de signo o de orden de
multiplicación aparece como un modelo de espaldas, una cámara invertida o un corte que
sale espejado, y hay que revisarlo en todo el código. Por eso se fijan ahora, antes de
la primera línea de código.

## Criterios

1. **Aprendizaje:** entender cada operación lo suficiente para explicarla y depurarla.
2. **Corrección:** resultados correctos y verificables con tests.
3. **Rendimiento:** suficiente para skinning, física y corte de mallas a 60 Hz en la
   GPU integrada del autor (ADR-0005).
4. **Coherencia con el resto del stack:** Vulkan (ADR-0005), GLSL en los shaders y el
   formato de intercambio de assets que se decida en el ADR-0017.
5. **Tiempo de desarrollo:** cuánto hay que escribir antes de ver el primer triángulo.
6. **Encaje con el subconjunto de C++** (ADR-0009): sin excepciones, templates simples
   y sin metaprogramación compleja.

## Opciones

### 1. Biblioteca propia o externa

| Opción | Pros | Contras |
|---|---|---|
| **A. Propia** | Se aprende y se puede defender cada línea. Convenciones exactas a medida (Vulkan, reverse-Z). Solo lo necesario, compila rápido. Coherente con la regla del proyecto ("el núcleo es propio") | Hay que escribirla y testearla. Los errores de signo son propios. Retrasa el primer triángulo unos días |
| **B. GLM** | Estándar de facto en tutoriales de OpenGL y Vulkan. Imita los tipos de GLSL (`vec3`, `mat4`), así que CPU y shader se leen igual. Madura y muy usada. Header-only, licencia MIT | Pensada para OpenGL: para Vulkan hay que activar defines (`GLM_FORCE_DEPTH_ZERO_TO_ONE`) y voltear Y a mano. API enorme y templates pesados, que alargan la compilación. Su SIMD es opcional (`GLM_FORCE_INTRINSICS`) y parcial. Usarla no demuestra nada en un portafolio |
| **C. DirectXMath** | Diseñada para SIMD desde el principio (SSE, AVX, NEON). Licencia MIT. Usada en juegos comerciales | Pensada para Windows: en Linux necesita un `sal.h` aparte. Con GCC y Clang **no tiene operadores sobrecargados** para `XMVECTOR`, así que todo se escribe como funciones (`XMVectorAdd(a, b)`). Convención de vectores fila |
| **D. Propia, con GLM solo en los tests** | Todo lo de A, más un **oráculo**: los tests comparan cada función propia contra el resultado de GLM | Una dependencia más, aunque solo en `tests/`. Si GLM usa otra convención (OpenGL), hay que traducir antes de comparar |

Eigen se descartó: es excelente para álgebra lineal numérica, pero sus *expression
templates* chocan con el subconjunto de C++ (ADR-0009) y alargan mucho la compilación.

### 2. SIMD

| Opción | Qué significa |
|---|---|
| **Escalar primero** | `struct vec3 { float x, y, z; }` simple. El compilador vectoriza solo parte del código (ver Investigación). Se optimiza con SIMD donde el profiler lo pida |
| **`vec4` SIMD como tipo base** | Cada vector vive en un registro de 128 bits (`__m128`). `vec3` desperdicia 4 bytes y la API se complica (registro frente a memoria) |
| **SoA en los bucles calientes** | Los tipos siguen siendo escalares, pero los sistemas pesados (skinning, corte, partículas) guardan los datos como arreglos separados (`xs[]`, `ys[]`, `zs[]`) y procesan 4 u 8 elementos por instrucción |

### 3. Sistema de coordenadas

| Opción | Quién lo usa |
|---|---|
| **Mano derecha, Y arriba, −Z adelante** | glTF 2.0, Godot, OpenGL, Maya |
| **Mano derecha, Z arriba** | Blender, 3ds Max, la mayoría del software CAD |
| **Mano izquierda, Z arriba** | Unreal Engine |
| **Mano izquierda, Y arriba** | Unity, Direct3D clásico |

Unidades: **metros** (glTF y los motores de física suelen asumirlos) y ángulos en
**radianes**.

### 4. Matrices

| Opción | Qué significa |
|---|---|
| **Vectores columna, guardado por columnas** (*column-major*) | Se escribe `M * v`, y las transformaciones se leen de derecha a izquierda (`P * V * M * v`). Es la convención de los libros de matemática, de GLSL y de GLM. La matriz se sube a la GPU sin transponer |
| **Vectores fila, guardado por filas** | Se escribe `v * M`, y se lee de izquierda a derecha. Es la convención de DirectXMath y del Direct3D clásico |

### 5. Proyección para Vulkan

| Tema | Opciones |
|---|---|
| **Rango de profundidad** | Vulkan usa **[0, 1]**, no el [−1, 1] de OpenGL. La matriz se escribe directamente para [0, 1] |
| **Eje Y** | En Vulkan, Y de pantalla apunta **hacia abajo**. Se voltea en la matriz de proyección, o con una altura de *viewport* negativa (permitida desde Vulkan 1.1). Ambas invierten el orden de los vértices (*winding*), así que hay que ajustar `frontFace` |
| **Profundidad** | Estándar (cerca → 0, lejos → 1) con buffer entero, o **reverse-Z** (cerca → 1, lejos → 0) con buffer `D32_SFLOAT` |
| **Plano lejano** | Finito, o **infinito** (la matriz se simplifica y casi no cambia la precisión) |

### 6. Rotaciones

| Representación | Cuándo sirve |
|---|---|
| **Cuaterniones** | Guardar e interpolar rotaciones (animación, cámara). 4 floats, sin *gimbal lock*, interpolación suave (`slerp`, `nlerp`) |
| **Matrices 3×3 o 4×4** | Lo que consume la GPU y lo que transforma muchos puntos rápido |
| **Ángulos de Euler** | Solo para mostrarle valores a una persona (editor, debug). Sufren *gimbal lock* y dependen del orden de los ejes |

### 7. Punto flotante

| Tema | Opciones |
|---|---|
| **Precisión** | `float` (32 bits) en todo el engine, o `double` para las posiciones del mundo (como Unreal 5) |
| **Comparaciones** | `==` directo, epsilon absoluto, o epsilon relativo y ULPs según el caso |
| **Flags del compilador** | IEEE estricto, o `-ffast-math` (más rápido, pero rompe NaN, infinitos y el orden de las operaciones) |
| **Instrucciones de CPU** | x86-64 base (SSE2, cualquier PC de 64 bits), o `x86-64-v3` (AVX2 y FMA, CPUs desde 2013 a 2015 aprox.) |

## Investigación

### GLM, DirectXMath y bibliotecas propias

- **GLM** sigue a GLSL en nombres y comportamiento. Requiere C++17, es header-only y
  tiene licencia MIT (o la "Happy Bunny", una MIT modificada). Su SIMD se activa con
  `GLM_FORCE_INTRINSICS`. Para Vulkan necesita `GLM_FORCE_DEPTH_ZERO_TO_ONE` y voltear
  Y, porque fue diseñada para OpenGL.
- **DirectXMath** separa los tipos de registro (`XMVECTOR`) de los tipos de memoria
  (`XMFLOAT4`), y obliga a cargar y guardar explícitamente. Es el modelo más eficiente,
  pero el más verboso. En GCC y Clang define `_XM_NO_XMVECTOR_OVERLOADS_`, porque esos
  compiladores no permiten sobrecargar operadores para `XMVECTOR`.
- Los engines grandes tienen **biblioteca propia**: Unreal (`FVector`, `FMatrix`,
  `FQuat`), Godot (`Vector3`, `Basis`, `Transform3D`), id Tech. No es una rareza,
  sino lo normal en un engine.

### SIMD: el `vec4` no es donde está la ganancia

**Experimento** con Clang 22 a `-O2`, en la máquina del autor:

- Sumar dos `vec4` escalares (`{x, y, z, w}`) **ya compila a `addps`**: una sola
  instrucción SSE suma los 4 componentes. El compilador vectoriza esto solo (*SLP
  vectorizer*).
- En cambio, `dot(vec3, vec3)` compila **casi entero a instrucciones escalares**
  (`mulss`, `addss`) con *shuffles*. El producto punto suma componentes **dentro** de un
  mismo vector (una operación "horizontal"), y SIMD está hecho para operar **entre**
  vectores.

Esa es la idea de fondo de la charla *SIMD at Insomniac Games* (Andreas Fredriksson,
GDC 2015) y del diseño orientado a datos en general: **el SIMD rinde cuando se procesan
4 u 8 objetos a la vez** (por ejemplo, los productos punto de 8 vértices contra el plano
del tajo en una instrucción, con los datos en SoA), y no cuando se mete un solo vector
en un registro. glam (Rust) y DirectXMath reconocen el costo del otro camino: con SIMD,
`vec3` ocupa 16 bytes y desperdicia 4 de relleno.

La CPU del autor (Kaby Lake-R) tiene SSE4.2, AVX2 y FMA. SSE2 está garantizado en
cualquier x86-64.

### Convenciones

- **glTF 2.0**, el estándar abierto de Khronos para intercambiar modelos 3D, define un
  sistema **de mano derecha, Y arriba, en metros**. Las herramientas que exportan a
  glTF convierten a esa convención al exportar. El formato y las herramientas de assets
  del proyecto se deciden en el ADR-0017.
- **Godot** usa mano derecha, Y arriba y −Z adelante, igual que glTF.
- **Unreal** usa Z arriba y mano izquierda, y desde UE5 usa `double` en `FVector`
  (*Large World Coordinates*) para mundos enormes.
- **Vulkan:** el espacio NDC tiene Y hacia abajo y profundidad en [0, 1]. Las
  proyecciones de GLM ya tratan el cambio de orientación, pero el [0, 1] y el volteo de Y
  hay que pedirlos.
- **GLSL** guarda las matrices por columnas por defecto. Con la misma convención en la
  CPU, la matriz se copia tal cual al buffer.
- **Trampa de alineación en buffers:** en `std140` (uniform buffers), un `vec3` se
  alinea a 16 bytes. En `std430` ocupa 12 bytes, pero se sigue alineando a 16. Un
  `struct` de C++ con `vec3` seguidos **no coincide** con el del shader. La solución
  habitual es usar `vec4` en los buffers o rellenar a mano, y verificarlo con
  `static_assert` sobre `sizeof` y `offsetof`.

### Profundidad: reverse-Z

Nathan Reed (*Depth Precision Visualized*, 2015) midió los errores de profundidad de
cada combinación. Sus conclusiones:

- Con la proyección estándar, un buffer de `float` y uno entero rinden casi igual,
  porque "el error aritmético tapa al error de cuantización".
- **Reverse-Z con un buffer de `float` dio cero errores en su prueba.** Funciona porque
  los `float` tienen mucha más densidad cerca de 0, y eso compensa la distribución 1/z
  de la proyección perspectiva.
- **El plano lejano infinito hace una diferencia minúscula** en el error.
- Recomendación para el rango [0, 1] de D3D y Vulkan: "usar un buffer de profundidad en
  punto flotante con reverse-Z".

Para un juego de espadas, con zombies a centímetros de la cámara y muros a cientos de
metros, esto elimina el *z-fighting* sin pensar en dónde poner el plano cercano.

### Precisión de `float`

Un `float` tiene 24 bits de mantisa. La distancia entre dos valores representables
crece con la magnitud:

| Distancia al origen | Paso mínimo |
|---|---|
| 1 km | ~0,06 mm |
| 10 km | ~1 mm |
| 100 km | ~8 mm (empieza el *jitter* visible en animación y cámara) |

Un mapa medieval de unos pocos kilómetros queda muy por debajo del límite. `double` en
el mundo (como Unreal 5) resuelve un problema que este juego no tiene.

### Comparaciones y robustez

- Bruce Dawson (*Comparing Floating Point Numbers, 2012 Edition*) explica que **no hay
  un epsilon universal**: el epsilon absoluto falla con números grandes, el relativo
  falla cerca de cero, y cada caso necesita su tolerancia. Comparar con `==` solo sirve
  con valores exactos.
- **El corte de mallas es el punto más delicado.** Decidir de qué lado del plano queda
  un vértice es un *predicado geométrico*: cuando el vértice está casi sobre el plano,
  el redondeo puede dar respuestas contradictorias y producir agujeros o caras sueltas.
  Shewchuk (1997) publicó los predicados adaptativos (`orient3d`), que usan aritmética
  exacta solo cuando el resultado es dudoso, y son el estándar de la industria desde
  entonces. La técnica concreta para el corte se decide en el ADR-0007. Este ADR solo
  deja preparada la base (tipo plano, distancia con signo, tolerancias explícitas).
- **`-ffast-math`** permite al compilador reordenar operaciones y asumir que no hay NaN
  ni infinitos. Eso rompe justamente los chequeos que protegen el corte y la física, y
  hace que el resultado cambie según el nivel de optimización.

### Bibliografía para escribirla

- Dunn y Parberry, *3D Math Primer for Graphics and Game Development*: gratis en
  gamemath.com, el más accesible.
- Eric Lengyel, *Foundations of Game Engine Development, Vol. 1: Mathematics*:
  vectores, matrices, cuaterniones y álgebra de Grassmann, pensado justo para escribir
  la biblioteca de un engine.
- Christer Ericson, *Real-Time Collision Detection*: robustez numérica y geometría,
  para el corte y la física.

## Decisión

| # | Qué | Elección |
|---|---|---|
| 1 | Biblioteca | **Propia**, en `engine/core`. Sin GLM ni DirectXMath |
| 2 | Tipos | **Concretos y con `float`:** `vec2`, `vec3`, `vec4`, `mat3`, `mat4`, `quat`, `plane`. Sin templates genéricos (`vec<N, T>`) |
| 3 | SIMD y CPU | **Tipos escalares.** SIMD y datos en SoA solo en los bucles que el profiler marque. Objetivo **x86-64 base** (sin `-march`) y **sin `-ffast-math`** |
| 4 | Coordenadas | **Mano derecha, Y arriba, −Z adelante.** Metros y radianes |
| 5 | Matrices | **Vectores columna** (`P * V * M * v`), **guardados por columnas** |
| 6 | Proyección | Profundidad en **[0, 1]**, **Y volteada en la matriz de proyección**, **reverse-Z** con `D32_SFLOAT` y **plano lejano infinito** en la cámara principal |
| 7 | Rotaciones | **Cuaterniones** para guardar e interpolar, **matrices** para la GPU y para transformar puntos, **Euler** solo en debug y editor |
| 8 | Punto flotante | **`float` de 32 bits** en todo el engine. Nada de `==` salvo valores exactos: `nearly_equal(a, b, tol)` con tolerancia explícita. La robustez del corte se decide en el ADR-0007 |

## Consecuencias

- **La biblioteca es núcleo (`owner:autor`).** Va en su propio ticket, cuando empiece el
  código, con tests desde el primer día: valores calculados a mano y propiedades como
  `inverse(M) * M ≈ I`, `|q| = 1` o `dot(v, cross(v, w)) ≈ 0`. El framework se decide
  en el ADR-0011.
- **Reverse-Z cambia el pipeline:** el buffer de profundidad se limpia a `0.0` (no a
  `1.0`), y la comparación es `VK_COMPARE_OP_GREATER` (o `GREATER_OR_EQUAL`), no
  `LESS`. Cualquier tutorial con la convención clásica hay que traducirlo.
- **El volteo de Y invierte el orden de los vértices**, así que `frontFace` se fija de
  acuerdo con eso y se verifica con un triángulo de prueba antes de activar el *culling*.
- **Las sombras usan una proyección finita.** El plano lejano infinito es solo para la
  cámara principal.
- **Los `struct` que van a buffers de GPU** usan `vec4` o relleno explícito, con
  `static_assert` sobre `sizeof` y `offsetof`, por la trampa de alineación del `vec3`
  en `std140` y `std430`.
- **Flags:** el CMake no agrega `-ffast-math` ni `-march`. Si un día el profiler
  justifica AVX2, se decide en un ADR nuevo.
- **Los assets** se convierten a Y arriba y metros al importarlos, venga de la
  herramienta que decida el ADR-0017.
- **Lo que queda abierto:** los predicados robustos del corte (ADR-0007), el formato de
  assets (ADR-0017) y qué sistemas pasan a SoA, que se decide midiendo.

## Cómo lo defiendo

**¿Por qué escribiste tu propia matemática si GLM existe y está probada?**
Para poder controlar todos los procesos de la matemática. Además, no necesito todo GLM,
y escribirla es una mejor experiencia de programación.

**¿Por qué cuaterniones y no ángulos de Euler?**
Los cuaterniones son de lo más estándar que conozco en los motores de videojuegos.

**¿Qué es reverse-Z y qué problema resuelve?**
Resuelve la pérdida de precisión de la profundidad y el molesto parpadeo
(*z-fighting*). Para eso, invierte los valores de profundidad con los que trabaja la
GPU.

**Tus vectores no usan SIMD. ¿No es lento?**
SIMD se va a usar en las partes donde el profiler indique que conviene. Así, solo se
ocupa donde realmente se necesita.

**¿Cómo comparas dos `float`?**
Los aproximo a la igualdad según un umbral (*threshold*). Dos `float` nunca se comparan
directamente, porque dos números que en la práctica son el mismo pueden ser distintos a
nivel de programación.

## Referencias

- GLM: <https://github.com/g-truc/glm>
- DirectXMath: <https://github.com/microsoft/DirectXMath>
- Andreas Fredriksson, *SIMD at Insomniac Games: How We Do the Shuffle*, GDC 2015:
  <https://gdcvault.com/play/1022249/SIMD-at-Insomniac-Games-How>
- glam (Rust), notas de diseño sobre SIMD y `vec3`: <https://docs.rs/crate/glam/0.8.5>
- Nathan Reed, *Depth Precision Visualized*:
  <https://www.reedbeta.com/blog/depth-precision-visualized/>
- Khronos, especificación de glTF 2.0:
  <https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html>
- Godot, *Introduction to 3D* y clase `Basis`:
  <https://docs.godotengine.org/en/stable/tutorials/3d/introduction_to_3d.html>
- Epic Games, *Large World Coordinates in Unreal Engine 5*:
  <https://dev.epicgames.com/documentation/en-us/unreal-engine/large-world-coordinates-in-unreal-engine-5>
- Khronos Community, coordenadas de Vulkan y proyección con GLM:
  <https://community.khronos.org/t/confused-when-using-glm-for-projection/108548>
- Bruce Dawson, *Comparing Floating Point Numbers, 2012 Edition*:
  <https://www.gamedeveloper.com/programming/in-depth-comparing-floating-point-numbers-2012-edition>
- Jonathan R. Shewchuk, *Adaptive Precision Floating-Point Arithmetic and Fast Robust
  Geometric Predicates* (1997):
  <https://people.eecs.berkeley.edu/~jrs/papers/robust-predicates.abstract>
- Fletcher Dunn e Ian Parberry, *3D Math Primer for Graphics and Game Development*:
  <https://gamemath.com/>
- Eric Lengyel, *Foundations of Game Engine Development, Vol. 1: Mathematics* (2016).
- Christer Ericson, *Real-Time Collision Detection* (2004).
