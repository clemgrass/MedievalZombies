# Mediciones del pipeline de assets

Nota de investigación del [ADR-0017](../adr/0017-pipeline-de-assets.md). Guarda las
mediciones que sostienen la decisión, para poder repetirlas.

Fecha: 2026-10-20, en la máquina de referencia del ADR-0020 (Dell XPS 13 9360,
i7-8550U, Intel UHD 620). Son mediciones de laboratorio para decidir, no del protocolo
del ADR-0020: no hubo calentamiento térmico, había otras aplicaciones abiertas y los
archivos estaban en la caché del sistema operativo.

## El material de prueba

Los paquetes gratis de Quaternius, bajo CC0 (compatibles con el ADR-0019), descargados
de OpenGameArt:

- *Universal Animation Library 2* [Standard]: `universal_animation_library_2standard.zip`;
- *Universal Animation Library* [Standard]: `universal_animation_librarystandard.zip`.

Se inspeccionaron con un script de Python que lee el `.glb` (la cabecera, el JSON y el
bloque binario) sin bibliotecas externas.

### El esqueleto

| Qué | UAL2 (versión Unreal-Godot) | UAL1 (versión Godot) |
|---|---|---|
| Huesos | **65** | 53 |
| De esos, dedos | 40 (5 dedos × 4 huesos × 2 manos, contando las puntas `*_leaf`) | 30 (sin las puntas) |
| Sin dedos ni puntas | **23** | 23 |

Los nombres siguen el maniquí de Unreal (`root`, `pelvis`, `spine_01`…`spine_03`,
`neck_01`, `Head`, `clavicle_l`, `upperarm_l`, `lowerarm_l`, `hand_l`, `thigh_l`,
`calf_l`, `foot_l`, `ball_l`…). Los dos paquetes usan el mismo rig: UAL1 solo omite las
puntas de los dedos y de los pies. El `.blend` del maniquí femenino trae un rig de
control hecho con **Rigify** (el generador de rigs que viene con Blender), y el glTF
exporta solo los huesos que deforman.

### Las mallas

| Malla | Vértices | Triángulos | Vértices por triángulo |
|---|---|---|---|
| Maniquí masculino (UAL2), pieza 1 | 3389 | 5732 | 0,59 |
| Maniquí masculino (UAL2), pieza 2 | 5157 | 8012 | 0,64 |
| Maniquí femenino, pieza 1 | 10 070 | 6415 | **1,57** |
| Maniquí femenino, pieza 2 | 15 566 | 8197 | **1,90** |

En una malla cerrada y suave hay ~0,5 vértices por triángulo. El maniquí femenino tiene
3 o 4 veces más, porque **glTF duplica un vértice en cada costura**: si dos triángulos
comparten una posición pero no la normal o la UV, son dos vértices distintos. Los
atributos son `POSITION`, `NORMAL` y `TEXCOORD_0` en `float`, `JOINTS_0` en `uint8` y
`WEIGHTS_0` en `float`.

### Los clips

- **Todos los clips vienen muestreados a 30 Hz**, con interpolación `LINEAR`, y cada
  clip anima los 65 huesos en traslación, rotación y escala: 8385 canales en 43 clips.
- **El 85 % de los canales es constante:**

| Canal | Canales | Constantes |
|---|---|---|
| Traslación | 2795 | 2743 (98 %) |
| Rotación | 2795 | 1552 (56 %) |
| Escala | 2795 | 2795 (100 %) |
| **Total** | **8385** | **7090 (85 %)** |

Las salidas de los clips (los valores de cada clave, sin los tiempos) pesan 5,55 MB. Los
canales que cambian pesan **0,93 MB**: quitar los constantes deja los datos 6 veces más
chicos, sin perder nada. Cada canal constante se reemplaza por un solo valor.

### Qué clips son gratis

| Para qué (ADR-0015) | En los paquetes gratis |
|---|---|
| Zombie: caminar | `Zombie_Walk_Fwd_Loop` (UAL2) |
| Zombie: agarrar | No. Lo más cercano es `Zombie_Scratch` (arañar, UAL2) |
| Zombie: cojear, arrastrarse con dos brazos y con uno | **No** |
| Caballera: trotar | `Jog_Fwd_Loop` (UAL1) |
| Caballera: salto de esquive | No. Hay saltos genéricos (`Jump_*`, `NinjaJump_*`) |
| Caballera: paso del tajo, carga, sacudirse de un agarre | **No** |

Los paquetes de pago (*Source*, desde 14,99 USD) agregan la locomoción completa de
zombie y los `.blend` con las animaciones. Siguen siendo CC0, pero no son gratis.

## Leer glTF frente a leer un binario cocinado

### Qué se midió

Un programa en C++20 (Clang 22.1.8, `-O2`) con **cgltf 1.15**:

1. leer el `.glb` a memoria;
2. `cgltf_parse` y `cgltf_load_buffers`: interpretar el JSON y ubicar los datos;
3. `cgltf_accessor_unpack_floats` sobre todos los *accessors*: pasar todo a `float`,
   como haría un cargador en runtime;
4. leer un archivo con el mismo contenido ya desempaquetado (lo que leería un binario
   cocinado), con un solo `fread`.

Es el mejor tiempo de 50 repeticiones, en 3 ejecuciones.

### Resultados

| Archivo | Leer el `.glb` | Interpretar | Desempaquetar | **glTF, total** | **Cocinado** |
|---|---|---|---|---|---|
| UAL2 completo (8,06 MB: 1,9 MB de JSON y 43 clips) | 1,6 ms | 13,4 ms | 2,8 a 4,6 ms | **17,9 a 19,7 ms** | **1,3 ms** |
| Maniquí femenino (1,44 MB: 18 KB de JSON y sin clips) | 0,2 ms | 0,2 ms | 2,5 ms | **2,9 ms** | **0,3 ms** |

- **Lo caro es el JSON:** con 1,9 MB de JSON, interpretarlo cuesta 8 veces más que leer
  el archivo.
- **Desempaquetar elemento por elemento también cuesta:** en el maniquí, sin clips, es
  casi todo el tiempo.
- **El cocinado es de 10 a 14 veces más rápido,** pero en valores absolutos los dos son
  chicos: 18 ms para 8 MB. Con unos 50 MB de assets en el corte vertical, glTF tardaría
  del orden de 0,1 s.
- **No se midió la lectura en frío** (con el archivo fuera de la caché, desde el SSD).
  La lectura es la misma en los dos casos, así que no cambia la diferencia.

### El código

```cpp
// Solo para medir: no revisa los cgltf_result ni llama a cgltf_free.
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
// ...
// 2. Interpretar el JSON y ubicar los buffers.
cgltf_options opt{};
cgltf_data* data = nullptr;
cgltf_parse(&opt, file.data(), file.size(), &data);
cgltf_load_buffers(&opt, data, path);
// 3. Desempaquetar todos los accessors a float.
for (cgltf_size i = 0; i < data->accessors_count; ++i) {
    cgltf_accessor* a = &data->accessors[i];
    cgltf_size n = cgltf_accessor_unpack_floats(a, nullptr, 0);
    size_t base = out.size();
    out.resize(base + n);
    cgltf_accessor_unpack_floats(a, out.data() + base, n);
}
```

Cada paso se mide con `std::chrono::steady_clock`. El programa completo, y los scripts
de Python que inspeccionan el `.glb`, quedaron fuera del repo: no son parte del engine.

## Exportar desde Blender sin interfaz

Blender 5.2.1 LTS exporta glTF desde la línea de comandos, sin abrir la ventana:

```bash
blender -b --factory-startup -noaudio Mannequin_F.blend \
    --python export_glb.py -- salida.glb
```

```python
import sys, bpy
out = sys.argv[sys.argv.index("--") + 1]
bpy.ops.export_scene.gltf(filepath=out, export_format='GLB', export_def_bones=True)
```

| Qué | Resultado |
|---|---|
| Tiempo, con el arranque de Blender | **1,1 s** |
| Exportaciones repetidas (4) | **Idénticas bit a bit** (mismo SHA-256) |
| El `.blend` | 4,0 MB, que comprimido con gzip queda en 0,6 MB |

`--factory-startup` ignora la configuración del usuario, y Blender no ejecuta los scripts
que trae el `.blend` (avisa: `scripts disabled … skipping 'rig_ui.py'`). La exportación
es determinista **con la misma versión de Blender**: con otra versión del exportador, el
resultado puede cambiar.

## La máquina y Blender

Requisitos mínimos de Blender 5.x (blender.org), contra la máquina de referencia:

| Requisito | Mínimo de Blender | La máquina (ADR-0020) |
|---|---|---|
| CPU | 4 núcleos con SSE4.2 | i7-8550U: 4 núcleos y 8 hilos |
| RAM | 8 GB | 8 GB (7,5 GiB visibles), compartida con la GPU |
| GPU Intel | Kaby Lake o más nueva | UHD 620 (Kaby Lake GT2): **justo el límite** |
| GPU | 2 GB de VRAM, OpenGL 4.3 o Vulkan 1.3 | Memoria compartida, Vulkan 1.3 |
| Render de Cycles en la GPU | Solo Intel Arc | No disponible (no hace falta: el juego no usa render offline) |

La máquina cumple los mínimos sin margen. Modelar low poly no exige mucho, pero una
escena pesada o varias aplicaciones abiertas a la vez compiten por los mismos 8 GB.

## Texturas comprimidas en la GPU

`vulkaninfo` en la UHD 620 (Mesa):

| Formato | Soporte |
|---|---|
| `textureCompressionBC` | Sí |
| `textureCompressionETC2` | Sí |
| `textureCompressionASTC_LDR` | Sí |
| `textureCompressionASTC_HDR` | No |

**Cuánto pesan las texturas del estilo PSX,** sin comprimir (RGBA8):

| Textura | Bytes | Con mipmaps (+33 %) |
|---|---|---|
| 128 × 128 (un zombie) | 64 KiB | 85 KiB |
| 256 × 256 (un muro) | 256 KiB | 341 KiB |
| 20 tipos de personaje, más 30 texturas de escenario | ~8,8 MiB | ~11,7 MiB |

Frente a los 384 MiB de recursos de la GPU (`docs/performance/presupuestos.md`), es un
3 %.
