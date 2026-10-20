# ADR-0017: Pipeline de assets

- **Estado:** Aceptado
- **Fecha:** 2026-10-20
- **Ticket:** #30
- **Reemplaza / Reemplazado por:** —

## Contexto y problema

El engine necesita modelos, esqueletos, clips de animación, texturas y formas físicas.
Alguien los crea en una herramienta, y algo los lleva hasta la memoria del juego en el
formato que esperan la GPU, la animación, la física y el corte. Ese camino es el
*pipeline* de assets.

Hay que decidir:

1. con qué **herramientas** se crean los assets: modelos, rigs, clips y texturas;
2. el **formato de intercambio** entre la herramienta y el engine;
3. **cómo se lee** ese formato: con un parser propio o con una biblioteca;
4. el **formato de runtime** y **cuándo se convierte** al formato que carga el juego;
5. el formato de las **texturas**;
6. las reglas de los **esqueletos y los clips**: los ejes, el máximo de huesos, un rig
   común y cómo se guardan los clips;
7. cómo viajan las **formas físicas** dentro de los assets: las cápsulas por hueso y las
   primitivas del escenario;
8. de dónde salen los assets y **cómo se guardan** en el repo.

Lo que ya está decidido y limita este ADR:

- **`CLAUDE.md`:** todo gratis. Cada dependencia externa necesita un ADR que la
  justifique. Lo propio es el renderer, la física, la animación, el corte y el gameplay.
- **ADR-0005:** Vulkan 1.3.
- **ADR-0006:** CMake y Ninja. Los shaders ya se compilan como parte del build, con
  `glslc`.
- **ADR-0008:** el engine usa Y arriba, metros y el frente hacia −Z. Los assets se
  convierten a esa convención al importarlos. Si se elige glTF, cuyos modelos miran hacia
  +Z, el importador los rota 180° sobre Y en toda la jerarquía. **El detalle lo define
  este ADR.**
- **ADR-0009:** los contenedores que asignan memoria (`std::vector`) se permiten en las
  herramientas y en la carga, no en el código que corre cada frame.
- **ADR-0012:** `tools/` guarda las herramientas de assets, y los módulos van en capas.
- **ADR-0015:** LBS con hasta 4 huesos por vértice. Este ADR decide cómo se importan los
  pesos, el máximo de huesos por esqueleto y de dónde salen los clips. Los clips mínimos
  del corte vertical son:
  - zombie: caminar, cojear, arrastrarse con dos brazos y con uno, y agarrar;
  - caballera: trotar, el salto de esquive, el paso del tajo, la carga y sacudirse de un
    agarre.
- **ADR-0016:** cada hueso que mueve algún vértice tiene una cápsula (fe de erratas del
  ADR-0016), que detecta el tajo, forma el ragdoll y da la forma de los pedazos. Este ADR
  decide cómo se generan. El escenario se describe
  **solo con cajas, cápsulas y esferas**.
- **ADR-0019:** los assets de terceros solo pueden ser CC0 o CC BY. `REUSE.toml` ya pone
  `assets/**` bajo CC BY 4.0.
- **ADR-0020:** la máquina de referencia (i7-8550U, UHD 620, 8 GB compartidos con la
  GPU) y la memoria: 384 MiB para los recursos de la GPU y 256 MiB para los assets en la
  CPU (hipótesis). **No hay un requisito de tiempo de carga.**
- **La visión:** estilo PSX, low poly pero orgánico, con texturas de pocos píxeles sin
  filtrar.

**Fuera de alcance:**

| Qué | Lo decide |
|---|---|
| Cómo se cargan y administran los recursos en runtime, el *hot reload* y el guardado | ADR-0028 |
| Cómo se arma un nivel y qué formato tiene una escena | ADR-0027 |
| El filtrado, los mipmaps como parte del aspecto y cómo se dibuja la cara del corte | ADR-0022 |
| Las invariantes de malla que necesita el corte | ADR-0007 |
| El audio | ADR-0024 |

## Criterios

Los del ticket:

1. **Gratis y open source.**
2. **Un flujo simple para una sola persona.**
3. **Tiempos de carga.**
4. **Compatibilidad con la animación y el corte.**

Y dos más:

5. **Aprendizaje.**
6. **Reproducibilidad:** que el build dé los mismos assets en cualquier máquina y en la
   CI.

## Opciones

### 1. Herramientas de creación

**Modelar, hacer el rig y animar:**

| Opción | Qué es | Problema |
|---|---|---|
| **Blender** (GPL) | Todo en uno: modelado, UV, rig (con Rigify, que viene incluido), pesos automáticos, animación y exportación a glTF. Es el estándar abierto | Una interfaz enorme. La máquina cumple sus requisitos mínimos justo en el límite (ver Investigación) |
| **Blockbench** (GPL-3.0) | Un editor pensado para modelos low poly con texturas de pixel art. Tiene huesos, pesos y animación, y exporta glTF | Nació para modelos de cubos (Minecraft). Las siluetas orgánicas y los rigs humanos le cuestan más |
| **Wings 3D** (BSD) para modelar, y Blender para el rig y la animación | Un modelador de subdivisión simple y rápido | No tiene rig ni animación: obliga a usar dos herramientas y a pasar los modelos de una a otra |
| **Un editor propio** | Todas las herramientas, escritas en el engine | Es otro proyecto del tamaño del engine (ver Investigación) |

Maya, 3ds Max y Houdini (completo) son de pago. Houdini Apprentice es gratis, pero solo
para uso no comercial.

**Texturas de pocos píxeles:**

| Opción | Licencia | Para qué sirve |
|---|---|---|
| **Pintar en Blender** | GPL | Pintar directo sobre el modelo |
| **Krita** o **GIMP** | GPL | Edición de imagen general |
| **LibreSprite** | GPL-2.0 | Pixel art: el fork libre de Aseprite |
| **Pixelorama** | MIT | Pixel art, con paletas, capas y animación |

Aseprite es de pago (su código es visible, pero no es open source).

### 2. Formato de intercambio

| Opción | Qué es | Problema |
|---|---|---|
| **glTF 2.0** (`.glb`) | El estándar abierto de Khronos (ISO/IEC 12113:2022): mallas, esqueletos, pesos, clips y materiales, con especificación pública. Blender lo exporta de serie | Es JSON más un bloque binario: hay que interpretarlo. Duplica los vértices en las costuras |
| **FBX** | El formato de facto de la industria, de Autodesk | Propietario y sin especificación pública. El SDK oficial no es libre, y el soporte de Blender es ingeniería inversa de una parte del formato |
| **USD** | El formato de escenas de Pixar, abierto | Pensado para escenas enormes de cine. La biblioteca OpenUSD es muy grande para mallas de un juego |
| **`.blend` directo** | Leer el archivo de Blender | No está documentado: describe sus propias estructuras internas (el "DNA" de Blender), que cambian entre versiones. Ata el engine a Blender |
| **Un exportador propio** (Python en Blender) | Un script que escribe directamente el formato del engine | Se aprende la API de Blender en vez de un estándar, y hay que mantener el script con cada versión |

Quedan descartados OBJ (no tiene esqueleto ni animación), COLLADA (Blender 5.0 lo quitó)
e IQM, un formato abierto y simple de los motores tipo Quake, que en Blender depende de
un exportador de terceros.

### 3. Cómo se lee el formato de intercambio

| Opción | Qué es | Problema |
|---|---|---|
| **Parser propio** | Un lector de JSON y de las reglas de glTF | Interpretar JSON no es lo que este proyecto quiere aprender, y un parser de archivos es una fuente clásica de errores de memoria |
| **cgltf** (MIT) | Un solo header en C99, sin dependencias, ~7000 líneas. Tiene utilidades para leer los *accessors* y las transformaciones | Hay que recorrer a mano sus estructuras, al estilo C |
| **fastgltf** (MIT) | C++20 moderno, con SIMD. Según sus pruebas, de 1,4 a 24 veces más rápido que las otras | Depende de simdjson, una dependencia más |
| **tinygltf** (MIT) | C++, popular y simple | Depende de nlohmann/json, y sus utilidades para los *accessors* son limitadas |
| **Assimp** (BSD-3) | Lee más de 40 formatos y los convierte a una escena común | Grande, con problemas conocidos en el skinning y vulnerabilidades reportadas. Si se usa un solo formato, sus 40 formatos no aportan nada |

### 4. Formato de runtime y cuándo se convierte

| Opción | Cómo funciona | Problema |
|---|---|---|
| **glTF en el juego** | El juego carga el `.glb` y lo convierte al cargarlo | El parser va dentro del juego, y cada carga paga el JSON y las conversiones (la rotación de ejes, recortar huesos…) |
| **Binario propio, cocinado en el build** | Una herramienta de `tools/` (el "cocinador") lee el glTF, lo valida, lo convierte y escribe un binario listo para la memoria. CMake la corre como a `glslc` con los shaders, y solo recocina lo que cambió | Hay que diseñar el formato y su versión, y escribir la herramienta |
| **Cocinado al arrancar, con caché** | El juego cocina el glTF la primera vez y guarda el resultado | Dos caminos de carga dentro del juego, y el parser también va en el ejecutable |

Si se cocina, el binario puede ser:

- **cargado en el lugar** (*load in place*): el archivo se lee de una vez y se usa tal
  cual, con offsets relativos en vez de punteros;
- **leído por secciones:** una cabecera con la versión, y después bloques que se copian
  a sus destinos (la memoria de la GPU, los componentes del ECS).

### 5. Texturas

| Opción | Qué es | Problema |
|---|---|---|
| **RGBA8 sin comprimir** | 4 bytes por píxel, sin pérdida | 4 veces más memoria que BC7, y más ancho de banda (pero las texturas PSX son chicas: ver Investigación) |
| **BC7 o BC1** | Compresión por bloques de 4 × 4 que la GPU lee directamente. BC7 ocupa 1 byte por píxel, casi sin pérdida. BC1 ocupa la mitad, con artefactos visibles | En texturas de pocos píxeles sin filtrar, cada bloque de 4 × 4 se nota. Hay que cocinar con un compresor (Compressonator, bc7enc o las herramientas de KTX) |
| **Indexada con paleta** (como la PS1) | 1 byte por píxel (un índice) más una paleta de 256 colores. El shader lee el índice con `texelFetch` y busca el color en la paleta | Vulkan no tiene un formato con paleta: la búsqueda se hace en el shader, y los mipmaps hay que generarlos aparte. A cambio, obliga a usar la paleta de la dirección de arte |
| **KTX2 con Basis Universal** | Un formato que se convierte en el momento al que soporte cada GPU | Está pensado para distribuir a muchas GPUs distintas. Aquí hay una sola plataforma |

**El contenedor** puede ser KTX2 (el estándar de Khronos, que guarda el `VkFormat` y los
mipmaps) o una sección del binario propio. **El formato de origen** suele ser PNG, que
el cocinador decodifica con una biblioteca chica (stb_image o lodepng) o con un
decodificador propio de PNG, que incluye *inflate* (el algoritmo de descompresión de
zlib).

### 6. Esqueletos y clips

**La rotación de ejes (ADR-0008).** El exportador de glTF de Blender pasa de Z arriba a
Y arriba, y un modelo que mira hacia −Y en Blender (su vista frontal) queda mirando a +Z
en glTF.

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Rotar en el cocinador** | El cocinador aplica 180° sobre Y a toda la jerarquía | Una conversión más que probar |
| **Modelar mirando hacia +Y en Blender** | El modelo sale de glTF mirando ya hacia −Z, sin rotar nada | Va contra la vista frontal de Blender y de los assets de terceros, que habría que girar a mano. Además, contradice el ADR-0008, que pide rotar en el importador |

**El máximo de huesos y el rig:**

| Opción | Qué significa | Problema |
|---|---|---|
| **El rig completo de Quaternius** (65 huesos) | Se usan tal cual los esqueletos de los paquetes CC0 | 40 son dedos. Con 50 zombies, triplica las matrices que se suben por frame |
| **El rig de Quaternius, recortado** (~23 huesos) | El cocinador quita los dedos y las puntas, y suma su influencia al hueso padre (la mano o el pie) | Las manos quedan rígidas. Con guanteletes y en estilo PSX, no se nota |
| **Un rig propio por personaje** | Cada modelo trae su esqueleto | Los clips no se comparten: cada personaje necesita los suyos |

**Cómo se guardan los clips:**

| Opción | Qué significa | Problema |
|---|---|---|
| **Las claves de glTF tal cual** | Tiempos variables, con interpolación `STEP`, `LINEAR` o `CUBICSPLINE` | Para muestrear, hay que buscar la clave en cada canal. Se guardan todos los canales, aunque no cambien |
| **Muestreados a paso fijo, sin canales constantes** | El cocinador muestrea cada clip a una frecuencia fija y reemplaza cada canal constante por un solo valor | Hay que elegir la frecuencia: 30 Hz, como vienen los de Quaternius, o 60 Hz, como el paso de la simulación |
| **Comprimidos** (cuantización, como ACL) | Se guardan con menos bits por valor | Más complejo, y no hace falta: sin los canales constantes, los clips ya pesan 6 veces menos |

### 7. Formas físicas dentro de los assets

**Las cápsulas por hueso (ADR-0016):**

| Opción | Cómo funciona | Problema |
|---|---|---|
| **A mano en Blender** | Una cápsula por hueso, como un objeto hijo del hueso o como propiedades del hueso, exportadas en los `extras` de glTF | Trabajo manual en cada personaje, y hay que mantenerlas al día si cambia el modelo |
| **Ajustadas en el cocinador** | El cocinador toma los vértices que pertenecen a cada hueso (por su peso dominante) y calcula un segmento sobre el eje del hueso y un radio que los cubre. Es lo que hace la opción *Generate Bodies* del editor de físicas de Unreal | Un hueso con pocos vértices, o con ropa suelta, puede dar una cápsula mala |
| **Ajustadas, con correcciones a mano** | El cocinador ajusta todas, y un archivo de correcciones cambia las que salen mal | Dos fuentes para el mismo dato |

**Las primitivas del escenario:**

| Opción | Cómo funciona | Problema |
|---|---|---|
| **Convención de nombres** | Un objeto llamado `UBX_…` (caja), `UCP_…` (cápsula) o `USP_…` (esfera) es solo colisión, como en el importador de FBX de Unreal. Godot usa sufijos parecidos (`-colonly`) | Un nombre mal escrito falla en silencio, salvo que el cocinador avise |
| **Propiedades en los `extras` de glTF** | Cada objeto de Blender lleva una propiedad que dice qué primitiva es | Es menos visible en Blender que un nombre |
| **La extensión `KHR_physics_rigid_bodies`** | La extensión de física de Khronos para glTF | Todavía es un borrador en revisión, y Blender no la exporta de serie |

### 8. Origen y almacenamiento

**De dónde salen los assets:**

| Opción | Qué significa | Problema |
|---|---|---|
| **Todos propios** | El autor modela y anima todo | Mucho trabajo de arte, que no es el foco del portafolio |
| **Terceros CC0 o CC BY** | Quaternius, Kenney, Poly Haven, ambientCG (ADR-0019) | Los paquetes gratis no cubren los clips que pide el ADR-0015, y los maniquíes no son low poly (ver Investigación) |
| **Mixto** | Terceros para empezar y para lo genérico, y propios para lo que define al juego | Hay que acreditar los de CC BY y mantener el estilo parejo |

**Qué se guarda en el repo:**

| Opción | Qué se versiona | Problema |
|---|---|---|
| **Solo la fuente** (`.blend`), y Blender exporta en el build | El `.blend`. El build corre `blender -b` para exportar el glTF | El build y la CI necesitan Blender, con su versión fijada |
| **La fuente y lo exportado** | El `.blend` y el `.glb`, que el autor exporta a mano | El `.glb` puede quedar desactualizado respecto al `.blend` sin que nadie lo note |
| **Solo lo exportado** | El `.glb` | Se pierde la fuente: no se puede volver a editar bien |

**Los binarios en git:** cada versión de un `.blend` es un archivo completo nuevo en la
historia. **Git LFS** los guarda fuera del repo, a cambio de una cuota.

## Investigación

### Datos medidos

Medido en la máquina de referencia (ADR-0020) el 2026-10-20, con los paquetes CC0 de
Quaternius. Son mediciones de laboratorio para decidir, fuera del protocolo del
ADR-0020. El detalle y cómo repetirlas están en
[`../research/pipeline-de-assets.md`](../research/pipeline-de-assets.md).

| Qué | Resultado |
|---|---|
| Huesos del rig de Quaternius | **65**, de los que 40 son dedos. Sin dedos ni puntas: **23** |
| Leer el `.glb` de UAL2 (8 MB, 43 clips) con cgltf y pasarlo a `float` | **18 a 20 ms** |
| Leer el mismo contenido ya cocinado | **1,3 ms** |
| Canales de los clips que no cambian | **85 %**. Sin ellos, los clips pesan 6 veces menos |
| Muestreo de los clips exportados | 30 Hz, `LINEAR`, los 65 huesos en traslación, rotación y escala |
| Vértices por triángulo del maniquí femenino | 1,6 a 1,9: glTF duplica un vértice en cada costura de normal o UV |
| Triángulos de los maniquíes | 5700 a 8200 por pieza: **no son low poly** |
| Exportar un `.blend` a glTF con `blender -b` | **1,1 s**, e idéntico bit a bit en 4 ejecuciones |
| Clips gratis que pide el ADR-0015 | Caminar (zombie) y trotar (caballera). **Faltan** cojear, arrastrarse, agarrar, el esquive, el paso del tajo, la carga y sacudirse |
| Texturas de 20 personajes y 30 de escenario, en RGBA8 con mipmaps | ~11,7 MiB: un **3 %** de los 384 MiB de la GPU |
| Texturas comprimidas en la UHD 620 | BC, ETC2 y ASTC LDR |

**Lo que dicen los números:**

- **El tiempo de carga no decide nada a esta escala.** El cocinado es de 10 a 14 veces
  más rápido, pero glTF tardaría del orden de 0,1 s para todo el corte vertical. Lo que
  pesa del cocinado es **sacar del juego el parser y las conversiones**, y **validar los
  assets antes de que lleguen al juego**.
- **La memoria de texturas tampoco decide:** sin comprimir, ocupan un 3 % del
  presupuesto. La compresión solo ahorraría ancho de banda, y a 640×360 la GPU lee muy
  pocos texeles por frame.
- **Los paquetes gratis no alcanzan:** faltan casi todos los clips que definen al juego,
  y los maniquíes tienen 10 veces más triángulos que un personaje low poly. Sirven como
  base y para aprender.

### El cocinado en la industria

- **El *asset conditioning pipeline*.** Jason Gregory (*Game Engine Architecture*,
  capítulo *Resources and the File System*) describe el camino estándar: la herramienta
  de creación exporta a un formato intermedio, y una herramienta offline lo convierte al
  formato de cada plataforma. El juego solo carga lo cocinado.
- **Unreal** "cocina" el contenido antes de empaquetar el juego (*cooking*). **Godot**
  importa cada glTF a sus propios recursos, en una carpeta de caché, y el juego
  exportado lleva los recursos importados, no el glTF.
- **Bitsquid/Stingray** (Niklas Frykholm): las herramientas escriben los recursos en un
  formato de texto, y un compilador de datos los convierte a un blob binario por
  plataforma. Se cargan en el lugar: el blob se lee de una vez, y lo que tiene que ser un
  puntero se corrige al cargar. Con offsets relativos ni siquiera hace falta corregir.

### Herramientas

- **Blender** exporta glTF de serie, con opciones para exportar solo los huesos que
  deforman y para muestrear los clips. Corre sin interfaz (`blender -b`) y acepta un
  script de Python, así que se puede llamar desde CMake. La exportación es determinista
  **con la misma versión del exportador**.
- **Los pesos automáticos** de Blender (*bone heat*) salen del paper de Baran y Popović
  (SIGGRAPH 2007), que calcula los pesos con una ecuación de difusión del calor sobre la
  superficie. Rigify genera el rig de control, y el exportador saca solo los huesos que
  deforman.
- **Los requisitos mínimos de Blender 5.x** son 4 núcleos, 8 GB de RAM y una GPU Intel
  Kaby Lake o más nueva. La máquina de referencia los cumple **justo en el límite**: la
  UHD 620 es Kaby Lake y los 8 GB se comparten con la GPU. Para modelar low poly no es un
  problema. El render de Cycles en la GPU no está disponible, pero el juego no lo
  necesita.
- **Blockbench** (GPL-3.0) es un editor de modelos low poly con texturas de pixel art,
  con huesos, pesos y exportación de clips a glTF. Es popular en Minecraft y en motores
  chicos (Luanti, Defold).
- **Wings 3D** (BSD) es un modelador de subdivisión sin animación. Se suele usar junto
  con Blender.

### ¿Y un editor propio?

Para tener todo el pipeline propio, un editor necesitaría:

| Pieza | Qué exige | Referencia |
|---|---|---|
| Modelado poligonal | Una estructura de malla editable (como la *half-edge* o la BMesh de Blender), operaciones como extruir, cortar en anillo o unir, más la selección, los gizmos y deshacer | Botsch y otros, *Polygon Mesh Processing* (2010) |
| Desplegar UV | Aplanar la malla con poca distorsión: LSCM es el método clásico | Lévy, Petitjean, Ray y Maillot, SIGGRAPH 2002 |
| Pintar texturas | Un editor de imagen con paletas, o pintar sobre el modelo | — |
| Rig y pesos | Ubicar los huesos, calcular pesos automáticos y pintarlos a mano | Baran y Popović, SIGGRAPH 2007 |
| Animación | Línea de tiempo, claves, curvas e IK para posar | — |
| Exportar | Escribir el formato del engine | — |

- **El problema es el alcance, no la máquina.** Lo low poly abarata los datos, pero no
  la interfaz: extruir una malla de 500 triángulos necesita el mismo código que extruir
  una de 500 000. Blender se desarrolla desde mediados de los 90, es open source desde
  2002 y tiene un equipo de tiempo completo. Un editor propio sería un segundo proyecto
  del tamaño del engine, antes de tener el primer zombie.
- **La máquina sí alcanzaría:** el editor usaría el renderer del engine, y dibujar
  modelos PSX es trivial para la UHD 620. Además, consumiría menos memoria que Blender.
- **La industria hace las dos cosas:** usa herramientas externas para modelar y animar
  (Maya, 3ds Max, Blender, Houdini), y escribe sus propias herramientas para los datos
  del engine. Algunas son editores de niveles, otras son validadores y exportadores. El
  editor de físicas de Unreal (*Physics Asset Editor*), por ejemplo, es justo una
  herramienta para ajustar las cápsulas de cada hueso sobre el modelo.
- **El punto medio:** herramientas chicas dentro del engine, para lo que es propio de
  este juego: ver un asset cocinado, ajustar y ver las cápsulas, o probar un corte sobre
  un modelo. Necesitan una interfaz (como Dear ImGui, que sería otra dependencia con su
  ADR), y caben en el ADR-0021 (herramientas de depuración) y en el ADR-0027 (niveles).

### Formatos e importación

- **glTF 2.0** (Khronos; ISO/IEC 12113:2022) define Y arriba, metros y el frente del
  asset hacia +Z. Cada vértice tiene hasta 4 huesos por juego de atributos (`JOINTS_0` y
  `WEIGHTS_0`). Los índices de hueso pueden ser `uint8` o `uint16`. Las extensiones
  ratificadas incluyen `KHR_mesh_quantization` (atributos con menos bits) y
  `KHR_texture_basisu` (texturas KTX2 con Basis Universal). Las propiedades
  personalizadas de Blender viajan en los `extras` de cada nodo.
- **glTF duplica los vértices en las costuras.** Para el render es lo correcto: cada
  vértice tiene una sola normal y una sola UV. Pero dos triángulos vecinos a lo largo de
  una costura no comparten índices. Si un algoritmo necesita la conectividad por
  posición, el cocinador puede calcular un segundo índice que une los vértices con la
  misma posición: es el *shadow index buffer* de meshoptimizer (Arseny Kapoulkine).
  Si el corte lo necesita, lo decide el ADR-0007.
- **FBX** es propietario y no está documentado. El SDK de Autodesk tiene una licencia
  restrictiva, incompatible con la GPL de Blender. Por eso el importador de Blender es
  una reimplementación por ingeniería inversa de una parte del formato.
- **El formato `.blend`** se describe a sí mismo: al final del archivo guarda el "DNA",
  la descripción de todas sus estructuras. Blender lo usa para convertir archivos de
  otras versiones. Un lector externo es posible, pero tiene que seguir las estructuras
  internas de Blender, que cambian.
- **Blender 5.0 quitó COLLADA**, y recomienda glTF, USD o FBX.
- **Las bibliotecas de glTF:** cgltf (C99, un header) y fastgltf (C++20, con simdjson)
  son las más usadas en motores propios. fastgltf publica pruebas en las que es de 1,4 a
  24 veces más rápido que cgltf y tinygltf. Assimp lee muchos formatos, y tiene problemas
  conocidos con las animaciones esqueléticas.
- **El parser va solo en el cocinador, si se cocina:** la dependencia no viaja dentro del
  juego, y la zona de herramientas permite la STL que asigna memoria (ADR-0009).

### La rotación de 180° sobre Y

Girar toda la escena con `R` (180° sobre Y) equivale a conjugar la transformación local
de cada nodo: `L' = R · L · R⁻¹`. Las `R⁻¹ · R` de un padre y su hijo se cancelan, así que
la transformación de mundo de cada nodo queda `R · M · R⁻¹`. Como los vértices también
se giran (`p' = R · p`), cada punto termina en `R · M · p`: la misma escena, girada. Para
este giro en particular:

- una traslación `(x, y, z)` pasa a `(−x, y, −z)`, también en las claves de los clips;
- un cuaternión `(x, y, z, w)` pasa a `(−x, y, −z, w)`, también en las claves. El
  producto punto entre dos claves no cambia, así que el *nlerp* y el camino corto
  tampoco;
- una escala no cambia;
- las posiciones, las normales y la parte `xyz` de las tangentes pasan a `(−x, y, −z)`.
  La `w` de la tangente no cambia;
- los *morph targets* giran sus diferencias de posición, de normal y de tangente;
- un nodo que trae una `matrix` en vez de traslación, rotación y escala se conjuga
  entera, igual que las matrices de *bind* inversas;
- un vector que viaje en los `extras` (por ejemplo, la corrección de una cápsula) no lo
  convierte el exportador de Blender: el cocinador le aplica primero el paso de Z arriba
  a Y arriba del exportador, `(x, y, z) → (x, z, −y)` (−90° sobre X), y después `R`. Los
  dos juntos dan `(x, y, z) → (−x, z, y)`, también con determinante +1: `(1, 2, 3)` de
  Blender termina en `(−1, 3, 2)` en el engine. Intercambiar y con z sin el signo,
  `(x, z, y)`, sería un espejo.

El determinante de `R` es +1: no espeja, y el orden de los vértices de cada triángulo
(el *winding*) no cambia. **Rotar dos veces y volver al original no lo prueba,** porque
un espejo también vuelve al original si se aplica dos veces (y la identidad también). Lo
que distingue una rotación de un espejo son los valores calculados a mano, con las tres
componentes distintas de cero: `(1, 2, 3)` pasa a `(−1, 2, −3)`, y un espejo en Z lo
dejaría en `(1, 2, −3)`. Y la normal de un triángulo: `cross(b − a, c − a)` después de
girar tiene que coincidir con `R` aplicada a la normal original. "Un punto en +Z termina
en −Z" solo prueba el frente: el espejo en Z también lo cumple.

### Texturas

- **La PS1** usaba texturas de 4 u 8 bits con una tabla de colores (CLUT) de 16 o 256
  colores, en 1 MB de VRAM. La mayoría medían 128 × 128 o menos, y 256 × 256 era el
  máximo.
- **BC1** guarda cada bloque de 4 × 4 con dos colores y una interpolación entre ellos:
  en texturas chicas, los artefactos se notan. **BC7** es casi sin pérdida, con la misma
  tasa que BC3. Algunas guías de motores recomiendan texturas sin comprimir para el pixel
  art y para las de 128 × 128 o menos.
- **KTX2** es el contenedor de Khronos: guarda el `VkFormat`, los mipmaps y,
  opcionalmente, Basis Universal. Las herramientas de KTX-Software (`ktx create`) son
  Apache-2.0.

### Clips y animación

- **El exportador de glTF de Blender muestrea los clips** por defecto, una clave por
  frame. Por eso todos los clips de Quaternius vienen a 30 Hz, con todos los huesos en
  todos los canales.
- **ACL** (*Animation Compression Library*, Nicholas Frechette) es la referencia de la
  compresión de clips: viene con Unreal desde la 5.3. Este juego no la necesita: con
  50 esqueletos de ~23 huesos y clips cortos, quitar los canales constantes alcanza.
- **Los paquetes gratis de Quaternius** comparten un rig con nombres al estilo del
  maniquí de Unreal, que ellos llaman "universal" para reutilizar los clips en Unreal,
  Godot y Unity.

### Colisiones dentro de los assets

- **Unreal** lee la colisión de un modelo FBX por el prefijo del nombre: `UBX_` (caja),
  `UCP_` (cápsula), `USP_` (esfera) y `UCX_` (casco convexo).
- **Godot** usa sufijos: `-col` agrega una colisión con la misma malla, `-colonly`
  reemplaza la malla por una colisión, y `-rigid` crea un cuerpo rígido.
- **`KHR_physics_rigid_bodies`** está en estado *Review Draft*: todavía puede cambiar.
- **El editor de físicas de Unreal** genera los cuerpos de cada hueso a partir de los
  vértices: con cápsulas por defecto, con un tamaño mínimo de hueso (*Min Bone Size*) y
  con los vértices de peso dominante o de cualquier peso (*Vertex Weighting Type*).

### Almacenamiento

- **GitHub** avisa con archivos de más de 50 MiB, bloquea los de más de 100 MiB, y
  recomienda repos de menos de 1 GB (con menos de 5 GB como límite fuerte).
- **Git LFS en GitHub, gratis:** 10 GiB de almacenamiento y 10 GiB de ancho de banda por
  mes. Si se pasa, sin un medio de pago, LFS se desactiva hasta el mes siguiente. Las
  descargas de la CI también cuentan.
- **Un `.blend` del maniquí femenino** pesa 4 MB, y comprimido 0,6 MB (git comprime los
  objetos). Los modelos low poly son más chicos.

## Decisión

Elijo Blender porque es el estándar abierto de la industria y trae todo lo necesario. Si
mi computador se queda corto, trabajo los modelos 3D y los rigs en otro. El intercambio
es glTF 2.0, y el juego carga un binario propio, convertido en el build: con el alcance
del juego, tiene sentido. Prefiero RGBA8 a BC7: las texturas no pesan tanto, y así no
pierden calidad. El rig puede no venir de Quaternius, así que el paso de recortar huesos
tiene que funcionar con cualquier rig. Y los assets se guardan con git directo, nunca con
LFS.

| # | Dimensión | Elección |
|---|---|---|
| 1 | Herramientas | **Blender** para los modelos, los rigs y los clips. Las texturas, con **cualquier herramienta que exporte PNG** (Blender, Pixelorama, LibreSprite, Krita…) |
| 2 | Formato de intercambio | **glTF 2.0** (`.glb`) |
| 3 | Lectura | **cgltf**, solo dentro del cocinador. El juego no lo enlaza |
| 4 | Formato de runtime | **Binario propio, cocinado en el build.** Una herramienta de `tools/` lee el glTF, lo valida y lo convierte. CMake la corre como a `glslc`, y solo recocina lo que cambió. El binario tiene secciones planas con offsets, sin punteros |
| 5 | Texturas | **RGBA8 sin comprimir**, desde PNG decodificado con **stb_image** en el cocinador. Decodificar PNG queda fuera del aprendizaje, igual que interpretar JSON, y stb_image es un solo header muy usado en motores propios |
| 6 | Esqueletos y clips | La **rotación de 180° sobre Y se hace en el cocinador**, en toda la jerarquía. El cocinador **recorta los huesos que un rig marca como sobrantes** (en el de Quaternius, los dedos y las puntas: quedan ~23) y funde sus pesos en el hueso que se conserva. **Este paso no depende de un rig en particular.** Hasta **256 huesos por esqueleto** (índices de 8 bits). Los clips se **muestrean a la frecuencia de la escena de Blender** (30 Hz en los actuales), y cada canal constante se guarda como un solo valor |
| 7 | Formas físicas | **Cápsulas por hueso ajustadas en el cocinador**, por el peso dominante de los vértices (o con cualquier peso, si el hueso no domina en ninguno), **con correcciones a mano** donde salgan mal. Un hueso que no mueve ningún vértice no tiene cápsula. En el escenario, **prefijos `UBX_` (caja), `UCP_` (cápsula) y `USP_` (esfera)**, y el cocinador **falla el build** si un prefijo está mal escrito o una primitiva no calza |
| 8 | Origen y repo | **Mixto:** assets de terceros CC0 o CC BY para empezar y para lo genérico, y propios para lo que define al juego. **En el repo, solo la fuente** (`.blend` y PNG). El build exporta con `blender -b`, con la versión de Blender fijada. **Git directo, sin LFS** |

## Consecuencias

- **El camino de un asset:**

  ```
  assets/…/zombie.blend ──blender -b──▶ build/…/zombie.glb ──cocinador──▶ build/…/zombie.<bin>
  ```

  Lo exportado y lo cocinado viven en el directorio de build y no se versionan. CMake
  sabe qué depende de qué, así que tocar un `.blend` re-exporta y recocina solo ese
  asset.
- **El build necesita Blender, con su versión fijada** (hoy, 5.2 LTS), y la verifica al
  configurar:
  - la exportación es idéntica bit a bit con la misma versión. Cambiar de versión es un
    cambio deliberado: se re-exporta todo y se revisa la diferencia;
  - **el otro computador** donde se modele usa la misma versión. Un `.blend` guardado con
    una versión más nueva puede no abrirse bien en una más vieja;
  - la CI también necesita Blender. Cómo se instala lo decide el ADR-0029.
- **Dependencias nuevas, solo en el cocinador:** cgltf (MIT) y stb_image (dominio público
  o MIT; se usa bajo MIT). Son headers sueltos, **copiados en el repo en
  `tools/third_party/`**, con su versión a la vista: el build no necesita red. Están en
  `CREDITS.md`, y cuando entren los archivos, `REUSE.toml` suma una anotación con el
  copyright de sus autores (ADR-0019).
- **Qué enlaza qué:** el cocinador enlaza solo `engine/core`, para usar la misma
  matemática (ADR-0008) y la misma definición del formato que el cargador. Por eso **la
  definición del formato** (la cabecera y las structs de cada sección) **vive en
  `engine/core`** (por ejemplo, `engine/core/asset_format`), sin lógica de carga. Nada
  de `engine/` ni de `game/` enlaza el cocinador ni sus dependencias.
- **El cocinador sigue el subconjunto de C++ del ADR-0009:** sin excepciones ni RTTI.
  Cada error sube como un `result`, y `main` lo traduce en un mensaje y en un código de
  salida distinto de cero.
- **El cocinador falla el build, no avisa:** un error en un asset se ve al compilar, con
  el nombre del asset y del objeto, nunca en el juego. Valida:
  - en una malla con esqueleto, primero se funden los pesos de los huesos que se quitan,
    y después un vértice con más de 4 huesos conserva los 4 de mayor peso y se
    renormaliza para que sumen 1. En ese orden, los huesos quitados no hacen perder
    peso. Un vértice sin pesos en
    una malla con esqueleto es un error (las mallas del escenario no tienen esqueleto);
  - hasta 256 huesos por esqueleto. Es el límite del formato (índices de 8 bits), no un
    presupuesto: el costo de cada esqueleto lo acota el presupuesto de animación del
    ADR-0020;
  - los nombres con prefijo de colisión, y que cada primitiva calce con su objeto. Un
    `UCX_` (casco convexo, en Unreal) o cualquier otro prefijo desconocido es un error,
    porque la física no tiene cascos convexos (ADR-0016);
  - lo que fije el ADR-0007 sobre las mallas que se cortan.
- **El formato del binario:** una cabecera con una firma y una versión, y secciones
  planas con offsets relativos. Como lo cocinado no se versiona, **no hay compatibilidad
  hacia atrás**: si el formato cambia, sube la versión y todo se recocina. El cargador
  revisa la firma, la versión y que cada offset caiga dentro del archivo antes de usarlo.
  Cómo se carga y se administra en runtime lo decide el ADR-0028.
- **El orden del cocinador:** primero la rotación de ejes, así todo lo demás (el
  recorte, el ajuste de cápsulas y de primitivas) ya trabaja en el espacio del engine.
- **El rig queda abstraído:** el cocinador no conoce los nombres de Quaternius. Cada rig
  dice qué huesos se conservan; cada hueso que se quita funde sus pesos en el ancestro
  conservado más cercano. **Solo se quitan subárboles completos:** si un hueso que se
  conserva cuelga de uno que se quita, el cocinador falla. Si no, los clips del hijo
  quedarían con el padre equivocado. Los clips se comparten entre los modelos que usan
  el mismo rig. Pasar un clip de un rig a otro distinto (*retargeting*) queda fuera del
  cocinador: si hace falta, se hace en Blender.
- **La rotación de 180° sobre Y** cambia el signo de las componentes x y z de las
  traslaciones, de los cuaterniones, de los vértices, de las normales y de las tangentes,
  en el modelo y en las claves de los clips, y conjuga las matrices de *bind* inversas.
  La lista completa está en la Investigación. No espeja, así que el orden de los
  vértices de cada triángulo no cambia.
- **Los clips:** el cocinador guarda las claves a la frecuencia de la escena, y la
  animación interpola entre ellas en cada paso de 60 Hz (con *nlerp* para las rotaciones,
  ADR-0015). El cocinador deja cada clave en el mismo hemisferio que la anterior
  (`dot ≥ 0`), y el muestreo en runtime igual revisa el hemisferio, como el render. Sin
  los canales constantes, los clips de Quaternius pesan 6 veces menos.
- **Las matrices por frame:** con ~23 huesos y 50 zombies son 74 KB por frame, no los
  ~64 KB que estimó el ADR-0015 con 20 huesos. Ese es el número que va a
  `presupuestos.md` cuando se fijen los frames en vuelo.
- **Las cápsulas:**
  - el ajuste corre **después** del recorte de huesos, así la cápsula de la mano cubre
    los dedos fundidos;
  - **tiene cápsula todo hueso que mueve algún vértice** (con un peso distinto de
    cero), porque el skinning lee su matriz y el ragdoll tiene que dársela. El ajuste
    usa los vértices en los que ese hueso es el dominante y, si no tiene ninguno (un
    hueso de torsión, por ejemplo), los vértices con cualquier peso, como la opción
    *Any Weight* de Unreal;
  - **un hueso que no mueve ningún vértice (como `root`) no tiene cápsula:** no hay nada
    que ajustar, y una cápsula entre los pies chocaría dentro del ragdoll. Si queda en
    medio de una cadena, la rótula une a sus hijos con el ancestro más cercano que tenga
    cápsula, y el hueso sin cápsula se mueve rígido junto a ese ancestro. El ADR-0016 lo
    registra como fe de erratas;
  - el ajuste automático cubre casi todos los huesos, y las correcciones se guardan
    junto al asset. Si van como propiedades dentro del `.blend` o en un archivo al lado
    se decide al implementarlo, probando qué exporta Blender a los `extras`. Un editor
    visual de cápsulas, como el de Unreal, es una herramienta posible del ADR-0021.
- **El escenario:** el cocinador ajusta cada primitiva a los vértices de su objeto, y
  falla si no calza (por ejemplo, una esfera con escala no uniforme). Cómo se arma un
  nivel con esas piezas lo decide el ADR-0027.
- **Las texturas** ocupan ~3 % de la memoria de la GPU sin comprimir. La sección de
  texturas del binario guarda su `VkFormat`: `R8G8B8A8_SRGB` para el color y
  `R8G8B8A8_UNORM` para los datos, según el material glTF. Si hay mipmaps (y cómo se
  filtran) lo decide el ADR-0022; si los pide, el cocinador los genera, promediando el
  color en espacio lineal.
- **El repo:** solo git, nunca LFS.
  - Ningún archivo pasa de 50 MiB (el aviso de GitHub). Los `.blend` low poly pesan
    mucho menos.
  - Si el repo se acerca a 1 GB (la recomendación de GitHub), la salida se decide en un
    ADR nuevo, sin LFS.
  - Los assets propios van en `assets/`, que `REUSE.toml` ya cubre con CC BY 4.0. Cada
    asset de terceros suma su fila en `CREDITS.md` y su anotación en `REUSE.toml`
    (ADR-0019).
  - **Un asset de terceros que solo existe como `.glb`** (como los clips gratis de
    Quaternius) se importa a Blender y se versiona como `.blend`, igual que los propios.
    `CREDITS.md` anota el archivo original del que salió.
- **El trabajo de arte que queda:** los paquetes gratis cubren caminar (zombie) y trotar
  (caballera). Los demás clips del ADR-0015 los anima el autor en Blender: cojear,
  arrastrarse con dos brazos y con uno, agarrar, el esquive, el paso del tajo, la carga y
  sacudirse. Los maniquíes de Quaternius (5700 a 8200 triángulos por pieza) sirven como
  referencia de proporciones, no como modelos del juego: los personajes son low poly y
  propios.
- **Tests (ADR-0011):**
  - **TDD y 100 % de cobertura** en las conversiones puras del cocinador. Viven en una
    biblioteca que enlazan el cocinador y su ejecutable de tests (`tests/tools/`), y
    corren en el preset Debug con ASan y UBSan. Lo que este ADR no fija (la tolerancia
    con que un canal cuenta como constante y los casos a mano del ajuste de cápsulas) se
    acuerda con el autor en el ticket que las implemente, antes de escribir los tests.
    Cada propiedad va con casos calculados a mano, porque una propiedad sola puede
    cumplirla una función equivocada:
    - **la rotación:** `(1, 2, 3)` pasa a `(−1, 2, −3)`, un cuaternión `(x, y, z, w)` pasa
      a `(−x, y, −z, w)`, un modelo que mira hacia +Z termina mirando hacia −Z, un
      vector de los `extras` `(1, 2, 3)` termina en `(−1, 3, 2)`, una matriz de *bind*
      inversa con traslación `(1, 2, 3)` termina con `(−1, 2, −3)` (y en la pose de
      *bind*, el mundo por la *bind* inversa sigue dando la identidad), y la normal de un
      triángulo después de girar coincide con la normal girada (eso detecta un espejo;
      rotar dos veces y volver al original no lo detecta);
    - **fundir huesos y pesos:** en una cadena de 3 huesos, quitar el subárbol del medio
      (el segundo y el tercero) deja sus pesos en el primero, y siguen sumando 1. Quitar
      solo el del medio hace fallar al cocinador;
    - **más de 4 huesos por vértice:** un vértice con 5 influencias conserva las 4 de
      mayor peso, y suman 1. Y el orden: un vértice con 5 influencias, 2 de ellas de
      dedos que se funden en la mano, termina con 4 huesos sin descartar nada (con el
      orden invertido, el test falla);
    - **quitar canales constantes:** muestrear antes y después da lo mismo, y además un
      canal con todas las claves iguales queda como un solo valor, y uno con una sola
      clave distinta se conserva entero;
    - **el hemisferio de las claves:** una clave con `dot < 0` respecto de la anterior se
      niega, y la pose no cambia;
    - **el ajuste de cápsulas y de primitivas,** incluido que un hueso que no mueve
      ningún vértice no tiene cápsula;
  - **assets de prueba chicos**, cocinados en los tests, para el formato y sus
    validaciones. Son `.gltf` escritos a mano o generados, en `tests/tools/fixtures/`, y
    son **la única excepción a "en el repo, solo la fuente"**: prueban al cocinador, no
    son contenido del juego. Así los tests no necesitan Blender, y se pueden armar los
    casos inválidos (un vértice sin pesos, un prefijo desconocido como `UCX_`, una
    primitiva que no calza, un hueso conservado que cuelga de uno que se quita, un
    esqueleto de 257 huesos);
  - **determinismo del cocinado:** cocinar dos veces el mismo asset da los mismos bytes;
  - **fuzzing del cargador** del binario: un archivo truncado o con offsets falsos se
    rechaza, nunca se lee fuera del buffer.
- **Lo que se sacrifica:**
  - la compresión de texturas (más ancho de banda). Se reevalúa si el profiler lo pide;
  - cargar glTF directo: cambiar un asset exige recocinar, aunque CMake lo hace solo;
  - un build sin Blender;
  - las manos del rig recortado son rígidas.
- **Abierto:**
  - el formato de vértice exacto y la precisión de los pesos, que el corte necesita en la
    CPU (ADR-0007);
  - si el corte necesita un índice que una los vértices duplicados en las costuras
    (ADR-0007);
  - la carga en runtime y el *hot reload* de assets (ADR-0028). Las texturas no se
    pueden usar directo desde el archivo: en las dos rutas del ADR-0005 (incluida la GPU
    integrada) se copian a una imagen con *tiling* óptimo;
  - el formato de las escenas y los niveles (ADR-0027);
  - Blender en la CI (ADR-0029);
  - las herramientas visuales dentro del engine (ADR-0021).

## Cómo lo defiendo

**¿Por qué no cargas el glTF directo en el juego, si es un estándar pensado para eso?**
Porque el binario es más liviano y pensado para el rendimiento de este proyecto: llega
al juego ya en el formato del engine, y el juego no tiene que interpretar JSON ni
convertir nada al cargar.

**Mediste que glTF carga todo en ~0,1 s. Si el tiempo de carga no importa, ¿para qué
cocinar?**
Porque al cocinar antes, en el build y no en runtime, podemos identificar los problemas
de los glTF antes de que lleguen al juego.

**¿Por qué no escribiste tu propio parser de glTF, si la física y la animación son
propias?**
Porque un parser de glTF está fuera del interés del motor mismo: se escapa del
aprendizaje de este proyecto. Además, cgltf solo vive en el cocinador, no dentro del
juego.

**¿Por qué tus texturas no están comprimidas, si la industria usa BC7?**
Porque no ocupan tanta memoria como para necesitar comprimirlas, y así mantenemos la
mejor calidad en un juego que ya compromete harto la calidad visual.

**¿Qué pasa con un modelo al pasar de los ejes de Blender a los del engine, y cómo sabes
que la rotación no lo espeja?**
Blender usa Z arriba, pero eso ya lo corrige su exportador de glTF, que deja Y arriba. Lo
que queda es el frente: en glTF el modelo mira hacia +Z, y en nuestro engine "adelante"
es −Z. Sin corregirlo, la caballera caminaría de espaldas. El cocinador gira todo 180°
sobre Y, lo que cambia el signo de x y de z. No espeja porque es una rotación, con
determinante +1. Un espejo tiene −1: pasaría la espada a la otra mano y daría vuelta los
triángulos. Un test lo comprueba con valores calculados a mano: el punto `(1, 2, 3)`
termina en `(−1, 2, −3)`, mientras que un espejo en Z lo dejaría en `(1, 2, −3)`, y la
normal de un triángulo sigue apuntando hacia afuera. Rotar dos veces no alcanza, porque
un espejo también vuelve al original.

**¿Por qué quitas los dedos del esqueleto, y qué pasa con sus pesos?**
Para el rendimiento cuando hay muchos zombies: el esqueleto pasa de 65 huesos a 23, y con
50 zombies se suben 74 KB de matrices por frame en vez de 208 KB, con casi 3 veces menos
trabajo de animación. Los pesos de los dedos no se pierden: se suman a la mano, para que
cada vértice siga sumando 1, y los dedos se mueven rígidos junto con ella. Con
guanteletes y estilo PSX, no se nota.

**¿Por qué no escribiste tu propio editor de modelos?**
Por ahora no es necesario para el aprendizaje de este engine. Además, el límite no es mi
máquina, es el alcance: un editor propio sería un proyecto del tamaño del engine. Se
podría necesitar a futuro, si editar externamente se vuelve realmente una molestia.

## Referencias

- Khronos, *glTF 2.0 Specification*:
  <https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html>
- Khronos, extensiones de glTF y su estado:
  <https://github.com/KhronosGroup/glTF/blob/main/extensions/README.md>
- Khronos, KTX 2.0 y KTX-Software: <https://github.khronos.org/KTX-Software/>
- Jason Gregory, *Game Engine Architecture*, 3.ª ed., CRC Press (2018), capítulo
  *Resources and the File System*.
- Niklas Frykholm, el compilador de datos y la carga en el lugar de Bitsquid:
  <https://www.gamedeveloper.com/programming/in-depth-read-my-lips---no-more-loading-screens>
- Ilya Baran y Jovan Popović, *Automatic Rigging and Animation of 3D Characters*,
  SIGGRAPH 2007: <https://people.csail.mit.edu/ibaran/papers/2007-SIGGRAPH-Pinocchio.pdf>
- Bruno Lévy, Sylvain Petitjean, Nicolas Ray y Jérome Maillot, *Least Squares Conformal
  Maps for Automatic Texture Atlas Generation*, SIGGRAPH 2002.
- Mario Botsch, Leif Kobbelt, Mark Pauly, Pierre Alliez y Bruno Lévy, *Polygon Mesh
  Processing*, A K Peters (2010).
- Blender, requisitos del sistema: <https://www.blender.org/download/requirements/>
- Blender, la salida de COLLADA:
  <https://devtalk.blender.org/t/moving-collada-i-o-to-legacy-status/34621>
- Blender, el formato `.blend` y su DNA:
  <https://formats.kaitai.io/blender_blend/cpp_stl_98.html>
- cgltf: <https://github.com/jkuhlmann/cgltf>
- fastgltf, comparación con cgltf y tinygltf:
  <https://fastgltf.readthedocs.io/latest/overview.html>
- Assimp: <https://assimp-docs.readthedocs.io/en/master/about/introduction.html>
- meshoptimizer (Arseny Kapoulkine): <https://github.com/zeux/meshoptimizer>
- Nicholas Frechette, *Animation Compression Library*: <https://github.com/nfrechette/acl>
- Epic Games, *Animation Compression Library in Unreal Engine*:
  <https://dev.epicgames.com/documentation/unreal-engine/animation-compression-library-in-unreal-engine>
- Epic Games, *Physics Asset Editor: Tools and Profiles*:
  <https://dev.epicgames.com/documentation/unreal-engine/physics-asset-editor-in-unreal-engine---tools-and-profiles>
- Godot, *Node type customization using name suffixes*:
  <https://docs.godotengine.org/en/stable/tutorials/assets_pipeline/importing_3d_scenes/node_type_customization.html>
- Quaternius, *Universal Animation Library* 1 y 2 (CC0):
  <https://quaternius.com/packs/universalanimationlibrary.html>
- Blockbench: <https://github.com/JannisX11/blockbench>
- PlayStation, especificaciones técnicas:
  <https://en.wikipedia.org/wiki/PlayStation_technical_specifications>
- GitHub, *About large files on GitHub*:
  <https://docs.github.com/en/repositories/working-with-files/managing-large-files/about-large-files-on-github>
- GitHub, *Git LFS billing*: <https://docs.github.com/en/billing/concepts/product-billing/git-lfs>
