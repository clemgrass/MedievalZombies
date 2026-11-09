# Roadmap del código

Documento vivo del [ADR-0033](../adr/0033-organizacion-del-desarrollo-y-roadmap.md). Es
la **fuente única del orden** en que se construye el juego: qué entra en cada
milestone, qué depende de qué y qué riesgos se atacan en cada corte. Los datos que lo
sostienen están en la [nota de investigación](../research/organizacion-del-desarrollo-y-roadmap.md).

## Cómo se lee

- **Un milestone por corte vertical** (ADR-0014). Cada uno termina en algo que se juega
  y en un tag.
- **Cada fila es trabajo de un ADR, todavía sin partir.** Al abrir un milestone, Claude
  parte sus filas en átomos (las cuatro preguntas de `docs/process/tickets.md`) y
  propone el lote. Cuando el autor lo aprueba, se crean los issues y su número va a la
  columna **Issue**. Una fila puede dar uno o varios átomos.
- **Solo el milestone en curso va en átomos.** Los demás quedan como filas, y se
  refinan al abrirlos (*rolling wave planning*).
- **ADRs:** el primero es el dueño; los demás comparten el trabajo. El label `área:` sale
  del dueño, salvo en las filas de contenido, que llevan `área:contenido`.
- **Depende de:** filas que tienen que estar terminadas antes. Dentro de un milestone,
  las filas van en un orden que respeta esas dependencias. En GitHub, se marcan como
  *blocked by*.
- **TDD:** "estricto" cuando el ADR-0011 o el ADR de la fila lo pide (el núcleo, los
  cocinadores, el corte, las piezas puras); en el resto, tests después. En una fila que
  mezcla las dos cosas (por ejemplo, las cámaras fijas con las validaciones del
  cocinador), "estricto" vale para su parte de núcleo o de cocinador, que al partirla
  queda en su propio átomo.
- **Prefijos de las filas:** F (fundaciones), P (`platform::`), GL (game loop), EC
  (ECS), D (depuración), CI, G (GPU), AS (assets y cocinador), R (render), AN
  (animación), PH (física), CT (corte), SK (esqueleto), CB (combate), V (VFX), IA, AU
  (audio), UI, N (niveles), RE (recursos), CA (cámara), IN (input), K (cinemáticas), CD
  y C (contenido).

## Milestones

| Milestone | Tag | Qué se juega al cerrarlo | Filas | Estado |
|---|---|---|---|---|
| **M3, esqueleto andante** | `v0.1.0` | Una sala con una malla estática cocinada desde Blender. Una tecla la parte en dos por un plano fijo, y los pedazos se separan, sin física. La CI en verde, y la CD deja el borrador de Linux y Windows | 51 | En curso: por partir en átomos |
| **M4, "el tajo"** | `v0.2.0` | La caballera apunta, carga y suelta. El maniquí con esqueleto se parte por donde apuntó, y los pedazos caen con física | 50 | — |
| **M5, "todo pesa"** | `v0.3.0` | Carne y metal: rebote, tajo rápido, hendidura, hoja clavada, pausa de impacto, ragdoll, sangre y sonido del corte | 31 | — |
| **M6, "la horda"** | `v0.4.0` | Zombies que persiguen y agarran en la arena cocinada, con checkpoints y el look PSX completo | 37 | — |
| **M7, el corte vertical objetivo** | `v0.5.0` | Lo que pide la visión: la primera oleada, la horda y la pelea contra el alquimista, con la cámara del tajo, los menús, las opciones, los subtítulos y la accesibilidad, en 60 fps estables en la UHD 620 | 33 | — |
| **Producción** | — | Varias zonas, el castillo y las cinemáticas. Se planifica al cerrar M7, junto con lo que la historia pide y ningún ADR cubre | 14 | Sin planificar |

## El contenido, en resumen

Cada asset es una fila con prefijo C, en el corte que lo usa primero, y las filas que lo
consumen dependen de ella (ADR-0033). El origen sigue el ADR-0017 y el ADR-0019.

- **M3:** una malla estática simple y la sala (C1).
- **M4:** el maniquí CC0 de Quaternius, para la caballera y para el blanco, **arreglado
  en Blender** para que pase la validación *manifold* (hoy tiene 61 cáscaras encajadas
  y una arista con más de dos caras) (C2); los clips de la caballera (C3), la espada
  (C4) y la textura de detalle de la tapa (C7).
- **M5:** los tres tipos de la primera oleada, **provisionalmente con el mismo maniquí y
  su material por cápsula** (C5); los sonidos del corte, que graba el autor o salen de
  CC0 (C6, ADR-0024); los clips del zombie mutilado (C9).
- **M6:** la arena (C8), los clips de los zombies (C10), los demás sonidos y la música
  (C11).
- **M7:** el guardaespaldas y el alquimista, provisionalmente con el maniquí (C12); las
  fuentes y los textos (C13); la caballera, la espada y los zombies propios (C14).

Los zombies, el guardaespaldas y el alquimista definitivos son propios (ADR-0017). Su
diseño (cuántos modelos, y si el guardaespaldas y el alquimista son variantes) se decide
al abrir M5 y M7 (ADR-0033, Abierto).

## Riesgos

| # | Riesgo | Dónde se ve primero | Cómo se ataca | Estado |
|---|---|---|---|---|
| 1 | **El corte en mallas reales:** casos degenerados, LBS, determinismo | M3 (estático) y M4 (con esqueleto) | El corte va en la ruta crítica desde el esqueleto, con TDD estricto, pruebas de propiedades y fuzzing (CT10) | Abierto |
| 2 | **La sensación del tajo:** que pese y no sea flácido | M4 | Todo lo que da la sensación es una CVar (ADR-0031), para ajustarlo jugando el tag | Abierto |
| 3 | **El contenido:** mallas cerradas y *manifold* | M4 | El maniquí de terceros se arregla en el primer corte (C2), y el cocinador falla el build si una malla no pasa | Abierto |
| 4 | **El presupuesto de GPU en la UHD 620** con 50 zombies y 100 pedazos | M6 | El registro por frame (D4) desde M4, y el protocolo del ADR-0020 en cada tag | Abierto |
| 5 | **El determinismo del *replay*** | M3 (hash del ECS) y M4 (*replay* en la CI, CI6) | El hash guardado bloquea en la CI desde M4 | Abierto |
| 6 | **Windows:** `platform::` y llvm-mingw | M3 | El *spike* CD1 al principio del esqueleto | Abierto |
| 7 | **La IA de la horda:** el costo de ORCA y que los agarres sean justos | M6 | Los tests de *replay* de la horda (IA11) y los contadores (IA10) | Abierto |
| 8 | **El alcance y la revisión** para una persona | Siempre | Tareas átomo, un milestone en curso a la vez, y la revisión del plan al cerrar cada uno | Abierto |

## Filas por milestone

### M3, esqueleto andante

| Fila | Qué entrega | ADRs | Depende de | TDD | Issue |
|---|---|---|---|---|---|
| F1 | monorepo y CMake: un target por capa, presets, toolchain Clang C++20 | 0006 | — | — | — |
| F2 | clang-format, clang-tidy y chequeos del subconjunto (0009) | 0013 | F1 | — | — |
| F3 | framework de tests, CTest, sanitizers y cobertura | 0011 | F1 | — | — |
| P1 | platform: ventana SDL3, eventos crudos y reloj | 0004 | F1 | — | — |
| D3 | macro de zona y Tracy on-demand | 0021 | F1 | — | — |
| G3 | shaders: glslc en CMake | 0006 | F1 | — | — |
| CD1 | spike: engine real en sniper y llvm-mingw, tests bajo Wine | 0035 | F1 | — | — |
| F4 | assert con mensaje, result<T,E> y handle con generación | 0009 | F3 | estricto | — |
| F5 | matemática: vec, mat, quat, convención de ejes y reverse-Z | 0008 | F3 | estricto | — |
| CI1 | contenedor Arch con fecha, script por job, chequeos rápidos, ci-ok en el ruleset | 0029 | F1, F2 | — | — |
| G1 | Vulkan base: tabla propia de funciones, instancia, dispositivo, swapchain, validation, 2 frames en vuelo | 0005, 0035, 0022 | P1 | — | — |
| IN1 | piezas puras: zona muerta, curva, contadores, regla del paso por timestamp, buffer | 0032 | F3 | estricto | — |
| IN2 | platform: eventos con timestamp, teclas por posición, mouse relativo | 0032 | P1 | — | — |
| CD2 | platform en Windows y sección del paquete en PE | 0035, 0004, 0028 | P1, CD1 | — | — |
| CD3 | presets de la CD (toolchain file, estáticos, prefix-map, SOURCE_DATE_EPOCH, strip) | 0035 | CD1 | — | — |
| F6 | regiones de platform (mmap+POPULATE), arena, scratch con marcador, contadores | 0010 | F4 | estricto | — |
| F8 | allocator de offsets tipo TLSF | 0010 | F4 | estricto | — |
| D2 | CVars: registro, archivo de valores cambiados, constantes en Release | 0021 | F4 | — | — |
| CI2 | build y tests debug con Clang (ASan/UBSan/LSan) y GCC | 0029 | CI1, F3 | — | — |
| CI4 | Blender y D2 por tarball con sha256 y caché; verificadores de diagramas (b)(c)(d) | 0029 | CI1 | — | — |
| CI7 | seguridad: permisos, actions por SHA, Dependabot | 0029 | CI1 | — | — |
| AS1 | formato de asset en core: cabecera y secciones planas | 0017 | F4 | estricto | — |
| CD4 | recursos del .exe | 0035 | CD3 | — | — |
| F7 | pools con lista libre y contenedores de capacidad fija | 0010 | F6 | estricto | — |
| F9 | detección en debug: envenenamiento ASan, guardas, comprobación al cerrar | 0010 | F6 | estricto | — |
| GL1 | game loop: paso fijo 60 Hz, acumulador, tope 8 pasos y 250 ms, alfa de interpolación | 0012 | P1, F6 | estricto | — |
| D1 | logging: 5 niveles, categoría por módulo, ring buffer | 0021 | F6 | — | — |
| CI3 | cobertura 100 % y clang-tidy | 0029 | CI2 | — | — |
| CI5 | corrida nocturna con archlinux:latest | 0029 | CI2 | — | — |
| G2 | gpu::Allocator por vida, ring buffer y cola de liberación por frame | 0010 | G1, F8 | estricto | — |
| AS2 | cocinador mínimo: blender -b fijado, cgltf, malla estática a binario, regla CMake | 0017 | AS1 | estricto | — |
| RE1 | formato del paquete y cargador con validación | 0028, 0035 | AS1 | estricto | — |
| EC1 | ECS: sparse sets, handles, capacidad fija, iteración en orden de id | 0012 | F7 | estricto | — |
| AS9 | datos del corte: índice soldado, cáscaras y validación manifold (malla estática) | 0007, 0017 | AS2 | estricto | — |
| R6 | destino de 640x360 y blit al swapchain con factor entero | 0022, 0020 | G2, G3 | — | — |
| RE6 | paquete dentro del ejecutable en Release | 0028, 0035 | RE1 | — | — |
| C1 | contenido: malla estática simple y la sala en .blend, cerradas y manifold | 0017, 0019, 0007 | AS2 | — | — |
| EC2 | ECS: destrucción diferida, consultas y hash del estado | 0012 | EC1 | estricto | — |
| EC3 | colas de eventos entre sistemas en la arena del paso | 0012 | EC1, F6 | estricto | — |
| CT1 | clasificar y partir triángulos con el índice soldado | 0007 | F5, AS9 | estricto | — |
| R1 | malla con reverse-Z y test del front face | 0022, 0008 | R6, F5, AS2 | — | — |
| CD5 | modo de prueba de humo | 0035, 0028 | RE6 | — | — |
| CT2 | encadenar contornos (con el abierto que falla) | 0007 | CT1 | estricto | — |
| IN3 | capa de acciones: contextos, controles por defecto, disparadores | 0032, 0018 | IN1, IN2, EC3 | — | — |
| CD6 | workflow de la CD, con git-cliff y el CHANGELOG.md | 0035 | CD3, CD5, CI2 | — | — |
| CT3 | tapa: cortar orejas en la base del plano | 0007 | CT2 | estricto | — |
| CD7 | configuración del repo: releases inmutables, ruleset de tags, entorno itch.io | 0035 | CD6 | — | — |
| CD8 | job de itch.io con butler | 0035 | CD6 | — | — |
| CD9 | LEEME, LICENSES en el paquete, notas | 0035 | CD6 | — | — |
| CT5 | pedazos: fragmentos conexos (malla estática) | 0007 | CT3 | estricto | — |
| SK1 | corte mínimo en el juego: una tecla, plano fijo, pedazos como entidades que se separan, sin física | 0007, 0012, 0032, 0022 | CT5, EC3, IN3, R1, C1 | — | — |

### M4, "el tajo"

| Fila | Qué entrega | ADRs | Depende de | TDD | Issue |
|---|---|---|---|---|---|
| D4 | registro por frame (R1-R4) y memoria (statm + memory_budget) | 0021, 0020 | GL1, G2 | — | — |
| D5 | dibujo de depuración immediate mode | 0021 | R1 | — | — |
| D6 | replay: las acciones de cada paso, encabezado, hash cada 60 pasos | 0021, 0032 | EC2, IN3 | — | — |
| D7 | timestamps de GPU y captura de RenderDoc in-app | 0021 | G2 | — | — |
| AS3 | formato de vértice de 32 B: pesos unorm8 que suman 255, normal octaédrica | 0017 | AS2 | estricto | — |
| AS6 | texturas PNG a RGBA8 con stb_image y mipmaps | 0017 | AS2 | estricto | — |
| AS8 | colisión del escenario: prefijos UBX_/UCP_/USP_ con sus fallas | 0017 | AS2 | estricto | — |
| AN1 | esqueleto, pose, jerarquía local a global y paleta | 0015 | F5, EC1 | estricto | — |
| PH1 | formas y pruebas de superposición: esfera, cápsula, caja | 0016 | F5 | estricto | — |
| PH8 | geometría de recortar una cápsula (core) | 0016, 0007 | F5 | estricto | — |
| CB1 | la caballera: carga con su curva, energía y cancelación | 0018 | GL1, IN3 | — | — |
| CA1 | resorte críticamente amortiguado y barrido de esfera de la cámara | 0031 | F5 | estricto | — |
| IN4 | movimiento: tanque, paso lateral, esquivar, relativo a la pantalla | 0032, 0018, 0031 | IN3 | — | — |
| IN5 | guía de la espada: stick por posición, mouse por deltas | 0032, 0015, 0031 | IN3 | — | — |
| IN6 | modo D con present_wait2 y caída al modo A | 0032, 0022 | G1 | — | — |
| C4 | contenido: la espada | 0017, 0018, 0019 | AS2 | — | — |
| CI6 | replay en relwithdebinfo: hash guardado y métricas deterministas que bloquean | 0029, 0021, 0020 | D6, CI2 | — | — |
| AS4 | esqueletos: rotación 180°, recorte de huesos marcados, fundir pesos | 0017 | AS3 | estricto | — |
| AN5 | IK analítica de dos huesos | 0015 | AN1 | estricto | — |
| PH2 | rayo y barrido de esfera | 0016 | PH1 | estricto | — |
| PH3 | controlador de personaje: avanzar y sacarse de lo sólido | 0016 | PH1, AS8 | — | — |
| PH4 | cuerpos rígidos: impulsos secuenciales, fricción, dormir | 0016 | PH1 | — | — |
| PH7 | detección del tajo: sector de anillo contra cápsulas | 0016 | PH1 | estricto | — |
| CT4 | des-deformar y mezclar, recortar y cuantizar pesos | 0007, 0015 | CT1, AN1 | estricto | — |
| R2 | texturas nearest con mipmaps y bindless | 0022 | R1, AS6 | — | — |
| R3 | skinning en el vertex shader | 0022, 0015 | R1, AN1, AS3 | — | — |
| R11 | ImGui y dibujo de depuración en sus pasadas | 0022, 0021 | R1, D5 | — | — |
| C7 | contenido: textura de detalle de la carne de la tapa | 0022, 0019 | AS6 | — | — |
| AS5 | clips a 30 Hz y canales constantes | 0017 | AS4 | estricto | — |
| AS7 | cápsulas por hueso ajustadas y correcciones a mano | 0017 | AS4 | estricto | — |
| AS9B | datos del corte: marca de malla cortable y huesos dominantes por cáscara | 0007, 0017 | AS9, AS4 | estricto | — |
| AN3 | skinning en la CPU igual al del shader, con el test que los compara | 0015, 0022 | AN1, R3 | estricto | — |
| PH11 | fase amplia por fuerza bruta y test de dos ejecuciones | 0016 | PH4 | — | — |
| CA2 | tercera persona con colisión y screen-door | 0031 | CA1, PH2 | — | — |
| AN2 | muestreo de clips, mezcla nlerp, evaluación en el paso | 0015 | AN1, AS5 | estricto | — |
| CT5B | pedazos: unión por hueso dominante del mismo lado | 0007 | CT5, CT4, AS9B | estricto | — |
| CA3 | apuntado desde el pecho con la mira en el mundo | 0031, 0018 | CA2, IN5 | — | — |
| C2 | contenido: el maniquí CC0 arreglado en Blender (cerrado y manifold), rig recortado y cápsulas, para la caballera y el blanco | 0017, 0019, 0007, 0016 | AS9B, AS7 | — | — |
| C3 | contenido: clips de la caballera (quieta, caminar, postura de carga, paso del tajo) | 0017, 0015, 0019 | AS5 | — | — |
| AN4 | interpolación de la pose en el render | 0015 | AN2, GL1 | — | — |
| AN6 | el tajo: arco en el plano, IK de los brazos, torso, clip del paso, valores como CVars | 0015 | AN5, AN2, D2, C3, C4 | — | — |
| CT6 | corte completo de punta a punta: pedazos al ECS con máscara, cápsulas e índice extendido | 0007, 0016, 0015 | CT5B, SK1, EC2, PH8, C2 | estricto | — |
| CB2 | plano por el pecho y punto apuntado | 0018 | CB1, PH2, CA3 | — | — |
| AN7 | guía de la espada antes de soltar (sigue un objetivo, con peso) | 0015 | AN6, IN5 | — | — |
| PH9 | cuerpos compuestos de los pedazos | 0016, 0007 | PH4, CT6 | — | — |
| CT8 | cola de cortes: 4 por paso, 20 pendientes, pose guardada, mínimo de pedazo | 0007 | CT6 | estricto | — |
| CT10 | pruebas de propiedades y fuzzing del corte | 0007 | CT6 | — | — |
| CB3 | gastar la fuerza por área: material por cápsula, umbral, ángulo, juntas, slice_request | 0018, 0007, 0016 | PH7, CB2, CT6 | — | — |
| CB6 | vivo o muerto (R1/R2) | 0018 | CT6 | — | — |
| R8 | pedazos con dos llamadas y tapa procedural | 0022, 0007 | R3, CT6, C7 | — | — |

### M5, "todo pesa"

| Fila | Qué entrega | ADRs | Depende de | TDD | Issue |
|---|---|---|---|---|---|
| PH5 | Soft Step: subpasos y restricciones blandas | 0016 | PH4 | — | — |
| PH10 | contacto en un sentido con los pedazos e impulso del tajo rápido | 0016 | PH3, PH9 | — | — |
| CT7 | hendidura y cuña | 0007 | CT6 | estricto | — |
| CB7 | pausa de impacto | 0018, 0015 | CB3 | — | — |
| V1 | pool de partículas SoA, integración al final del paso, pcg | 0025 | GL1, F7 | — | — |
| V9 | campo de distancia y capas de la tapa | 0025, 0007, 0022 | CT6, R8 | estricto | — |
| AU1 | dispositivo de audio en platform, callback y reloj | 0024 | P1 | — | — |
| AU2 | cola SPSC | 0024 | F3 | estricto | — |
| AU4 | cocinador de audio y tabla de sonidos | 0024 | AS2 | estricto | — |
| CA6 | sacudida con trauma | 0031 | CA2 | — | — |
| C5 | contenido: los tres tipos de la primera oleada, con el maniquí y su material por cápsula (carne, cota, placa) | 0018, 0017, 0019 | C2 | — | — |
| C9 | contenido: clips del zombie mutilado (cojear, arrastrarse con dos brazos y con uno) | 0015, 0017, 0019 | AS5 | — | — |
| AN8 | huesos enmascarados y clips del zombie mutilado | 0015 | AN2, CT6, C9 | — | — |
| AN10 | congelar el reloj de animación por personaje | 0015, 0018 | AN2, CB7 | — | — |
| PH6 | rótulas con límites, ragdoll y su tope | 0016 | PH5 | — | — |
| CT9 | volver a cortar, incluida la herida abierta | 0007 | CT7 | estricto | — |
| CB4 | rebote y aturdimiento | 0018 | CB3, C5 | — | — |
| CB5 | tajo rápido | 0018 | CB3, PH10 | — | — |
| CB9 | hoja clavada: quick time | 0018 | CT7, IN3 | — | — |
| V2 | dibujo con instancing y tres mezclas, radix sort | 0025 | V1, R1 | — | — |
| AU3 | mezclador con render offline contra un hash | 0024 | AU2 | — | — |
| C6 | contenido: sonidos del corte por capa (carne, hueso, metal) | 0024, 0019 | AU4 | — | — |
| CB8 | pedazos rígidos o ragdoll según N huesos | 0018, 0016 | PH6, CT6 | — | — |
| V3 | salpicadura, chorros y plano por estallido | 0025, 0007, 0018 | V2, CB3 | — | — |
| V4 | marcas de malla con pool anillo | 0025 | V2, R1 | — | — |
| V6 | estela de la espada | 0025 | V2, AN6 | — | — |
| AU5 | sistema de audio en el hilo principal | 0024 | AU1, AU3, AU4, GL1 | — | — |
| V5 | tabla material a efecto y chispas | 0025 | V3 | — | — |
| AU6 | sonido 3D | 0024 | AU5 | — | — |
| AU9 | sonidos del corte | 0024 | AU5, CB3, C6 | — | — |
| AU10 | panel, contadores, Tracy y dibujo del audio | 0024 | AU5, R11 | — | — |

### M6, "la horda"

| Fila | Qué entrega | ADRs | Depende de | TDD | Issue |
|---|---|---|---|---|---|
| R4 | antorchas, ambiente y niebla | 0022 | R1 | — | — |
| R5 | dithering y vértices que tiemblan | 0022 | R1 | — | — |
| R9 | sombras blob | 0022 | R1 | — | — |
| R10 | screen-door de la caballera | 0022 | R1 | — | — |
| IA1 | navmesh: formato, A* y funnel, sobre una navmesh a mano | 0023 | F5 | — | — |
| AU7 | emisores | 0024 | AU6 | — | — |
| N1 | cocinador de una zona: instancias, ids, marcadores, luces, colisión | 0027 | AS8 | estricto | — |
| RE2 | hilo de I/O y sus dos colas | 0028 | P1, F6 | estricto | — |
| RE7 | arranque: VkPipelineCache y pantalla de carga | 0028 | G1 | — | — |
| C10 | contenido: clips de los zombies (caminar, trotar, atacar, agarrar, ensartado) | 0015, 0017, 0019 | AS5 | — | — |
| C11 | contenido: los demás sonidos y la música | 0024, 0019 | AU4 | — | — |
| R7 | culling por esfera hasta el final de la niebla (las zonas, en N11) | 0022, 0027 | R4 | — | — |
| V7 | polvo, fuego y partículas grandes | 0025 | V2, R4 | — | — |
| IA3 | seguir el camino, rayo en la navmesh, reparto de recálculos | 0023 | IA1, PH2 | — | — |
| AU8 | música: decodificadores, estados, crossfade | 0024 | AU5, C11 | — | — |
| N2 | textos de zona y de mundo con validaciones | 0027 | N1 | estricto | — |
| N4 | cargar una zona en el ECS en orden de id, con hash | 0027 | N1, EC2 | estricto | — |
| RE3 | slots en CPU y bloques en GPU, cola de liberación de zonas | 0028 | RE2, G2, F7 | estricto | — |
| RE5 | cocinador: binarios por zona, global, arranque, música, reparto | 0028 | RE1, N1 | estricto | — |
| RE8 | tarjeta de memoria y test que mata el proceso | 0028 | RE2 | estricto | — |
| C8 | contenido: la arena (ruinas, colisión con prefijos, antorchas, marcadores) | 0027, 0017, 0019 | N1 | — | — |
| IA2 | el cocinador genera la navmesh de la arena y la guarda en el binario de la zona | 0023, 0017, 0027 | IA1, N1, C8 | estricto | — |
| IA4 | ORCA entre agentes | 0023 | IA3, PH3 | — | — |
| IA10 | dibujo de depuración, panel y contadores de la IA | 0023, 0021 | IA3, D5 | — | — |
| N5 | fases y triggers con la arena | 0027 | N2, N4, C8 | — | — |
| N7 | antorchas como pieza, 16 más cercanas con fundido | 0027, 0022, 0024, 0025 | R4, N4 | — | — |
| N8 | dibujo de depuración de zonas y triggers | 0027, 0021 | N4, D5 | — | — |
| RE4 | subida con presupuesto por frame y rangos bindless | 0028 | RE3 | — | — |
| IA5 | BT plano y árbol del zombie común con su tabla | 0023 | IA4, AN2, C10 | — | — |
| N6 | checkpoints | 0027, 0028 | N5, RE8 | — | — |
| CB10 | agarres | 0018 | IA5 | — | — |
| IA6 | percepción: vista, ruido, contagio | 0023 | IA5 | — | — |
| IA7 | fichas y espera en anillo | 0023 | IA5 | — | — |
| IA8 | locomoción, escala de tiempo, zombie ensartado | 0023 | IA5, AN8 | — | — |
| AN9 | agarres procedurales | 0015 | AN5, CB10 | — | — |
| CB12 | energía como vida: muerte de la caballera | 0018 | CB1, CB10 | — | — |
| IA11 | tests de replay de la horda | 0023 | IA7, D6 | — | — |

### M7, el corte vertical objetivo

| Fila | Qué entrega | ADRs | Depende de | TDD | Issue |
|---|---|---|---|---|---|
| V8 | frames de impacto: máscara en alfa y postproceso | 0025 | R6 | — | — |
| V10 | contadores y CVars de los efectos | 0025 | V1, R11 | — | — |
| UI1 | cocinador de fuentes | 0026 | AS2 | estricto | — |
| UI2 | cocinador de textos y archivo de anchos | 0026 | AS2 | estricto | — |
| UI3 | lista de dibujo y pasada de UI a 640x360 | 0026 | R6 | — | — |
| UI10 | señales diegéticas | 0026, 0015, 0024, 0025 | AN6, AU5, V2 | — | — |
| RE9 | opciones y carpetas XDG | 0028 | RE2 | — | — |
| RE10 | recarga en caliente: inotify, región, corte de la grabación, incluida la tabla de sonidos | 0028, 0024 | RE3, AU4 | — | — |
| RE11 | panel, zonas de Tracy y metas de carga | 0028 | R11 | — | — |
| CA4 | cámara del tajo que sigue la hoja, cámara lenta | 0031, 0018 | CA3, CB3 | — | — |
| CA7 | primera persona | 0031 | CA2 | — | — |
| CA8 | fijas y rieles, con las validaciones del cocinador | 0031, 0017, 0027 | CA2, N1 | estricto | — |
| CA10 | cámara libre de depuración | 0031, 0021 | CA2, R11 | — | — |
| IN8 | dispositivos: cambio en caliente, pausa al desconectar | 0032, 0026 | IN3 | — | — |
| IN10 | vibración | 0032, 0024 | AU5 | — | — |
| IN11 | panel de input | 0032 | R11 | — | — |
| C12 | contenido: el guardaespaldas y el alquimista, con el maniquí y su material, y sus clips (invocar, aturdido) | 0023, 0018, 0017, 0019 | C5 | — | — |
| CB11 | corte especial: reglas y resolución al soltar | 0018 | CB3, CA4 | — | — |
| IA9 | guardaespaldas y FSM del alquimista | 0023 | IA7, C12 | — | — |
| UI12 | panel de ImGui de la UI | 0026 | UI3, R11 | — | — |
| CA5 | la herida que se abre | 0031, 0007, 0022 | CA4, R8 | — | — |
| C13 | contenido: fuentes Jersey 10 y Jacquard 12 y textos en español e inglés | 0026, 0019 | UI1, UI2 | — | — |
| C14 | contenido: la caballera, la espada y los zombies propios, donde reemplacen a los de terceros | 0017, 0019 | C2, C5, C12 | — | — |
| UI4 | texto: UTF-8, layout y corte de línea | 0026 | UI1, UI2, UI3, C13 | estricto | — |
| UI5 | controles immediate mode, foco y navegación | 0026 | UI4 | estricto | — |
| UI8 | subtítulos | 0026 | UI4, AU5 | — | — |
| UI9 | ayudas del HUD, avisos y línea del tajo | 0026, 0031 | UI4, CA3 | — | — |
| UI6 | pila de pantallas y pausa | 0026, 0012, 0024 | UI5, GL1 | — | — |
| UI7 | íconos por dispositivo | 0026, 0032 | UI5, IN8 | — | — |
| UI11 | accesibilidad: acento, x2, placas, lista viva | 0026 | UI8 | — | — |
| CA9 | opciones de cámara en el menú | 0031, 0026 | CA2, UI5 | — | — |
| IN7 | reasignación y opciones.txt | 0032, 0026, 0028 | IN3, UI5, RE9 | estricto | — |
| IN12 | pruebas con el DualShock: tipo de mando, giroscopio, deriva, vibración e íconos | 0032, 0026 | IN8, IN10, UI7 | — | — |

### Producción (sin planificar)

| Fila | Qué entrega | ADRs | Depende de | TDD | Issue |
|---|---|---|---|---|---|
| N9 | varias zonas: carga, simulación con espera, tope | 0027, 0028 | N5, RE3 | — | — |
| N12 | complemento de Blender, si hace falta | 0027 | N1 | — | — |
| K1 | secuenciador puro: pistas, reloj en pasos, salto, ramas | 0034 | GL1 | estricto | — |
| K2 | exportación de Blender de las cinemáticas | 0034, 0017, 0031 | AS5 | — | — |
| K3 | cocinador de cinemáticas | 0034, 0027, 0028 | N1 | estricto | — |
| K5 | cámara de la cinemática y salida a la del jugador | 0034, 0031 | CA2 | — | — |
| K6 | disparar la cinemática | 0034, 0027 | N5 | — | — |
| K7 | subtítulos, sonidos, música y bandas | 0034, 0024, 0026 | UI8, AU8 | — | — |
| K8 | saltar | 0034, 0021, 0026, 0032 | IN3, UI6, D6 | — | — |
| K9 | secuencia interactiva | 0034, 0018, 0032 | IN3, CB9 | — | — |
| K10 | panel de cinemáticas | 0034 | R11 | — | — |
| N10 | navmesh partida por zona con portales | 0027, 0023 | N9, IA1 | — | — |
| N11 | culling por niebla y zonas visibles | 0027, 0022 | N9, R7 | — | — |
| K4 | mundo detenido | 0034, 0012 | K1, EC2 | — | — |

## Historial

| Fecha | Cambio |
|---|---|
| 2026-11-09 | Primera versión (#75, ADR-0033): cinco milestones, 216 filas (14 de contenido y las pruebas del DualShock) y ocho riesgos. Se juntaron el trabajo repetido entre listas (la grabación de las acciones en el *replay*, la navmesh en el binario de la zona y la recarga de la tabla de sonidos) y se separó el culling por zonas del culling por la niebla. M3 queda en curso, por partir en átomos |
