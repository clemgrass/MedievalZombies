# ADR-0036: Nombres de las tablas de funciones de Vulkan

- **Estado:** Aceptado
- **Fecha:** 2026-11-24
- **Ticket:** #103
- **Reemplaza / Reemplazado por:** reemplaza una parte de la Decisión 2 del ADR-0013 (el
  prefijo `m_` y el `snake_case` de los miembros), solo para los punteros de las tablas de
  funciones de Vulkan en `engine/gpu`

## Contexto y problema

El ADR-0035 decidió que `engine/gpu` cargue cada función de Vulkan en una tabla propia:
con `VK_NO_PROTOTYPES`, los headers no declaran `vkCreateDevice`, y el engine pide su
dirección con `vkGetInstanceProcAddr` o `vkGetDeviceProcAddr` y la guarda en un struct.
G1.1 (#103) escribió esas tablas: `global_functions`, `instance_functions` y
`device_functions`, en `engine/gpu/include/mz/gpu/functions.h`.

Cada puntero es un miembro de un struct, así que la Decisión 2 del ADR-0013 le pide el
prefijo `m_` y `snake_case`: `functions.m_create_device(...)`. Pero esos miembros no son
datos del engine. Son las funciones de una API de C que ya tienen nombre, el de la
especificación, y ese nombre es el que aparece en la especificación, en los mensajes de
la validación, en los *VUID* (`VUID-vkGetDeviceQueue-queueIndex-00385`) y en cualquier
ejemplo o charla sobre Vulkan.

Desde el primer PR con código, un cambio de decisión pide un ADR nuevo (ADR-0033,
Decisión 9). Por eso esta excepción no puede quedar solo en el `.clang-tidy` de
`engine/gpu`.

## Criterios

- **Buscar la llamada tal cual:** que el nombre del código sea el de la especificación,
  para copiarlo de un mensaje de la validación y encontrarlo en el código, y al revés.
- **Una excepción chica:** que valga solo donde hace falta, y que la herramienta la
  aplique, no la memoria de quien escribe.
- **Lo que hace la industria:** cómo nombran estas tablas los cargadores conocidos.

## Opciones

| Opción | Ejemplo | A favor | En contra |
|---|---|---|---|
| **La Decisión 2 tal cual** | `fns.m_create_device(...)` | Sin excepciones | El nombre no se encuentra en la especificación ni en los mensajes de la validación; hay que traducir cada vez |
| **`m_` más el nombre de la especificación** | `fns.m_vkCreateDevice(...)` | Conserva el `m_` | Igual rompe el `snake_case`, así que igual pide una excepción, y no se busca tal cual |
| **El nombre de la especificación** | `fns.vkCreateDevice(...)` | Se busca tal cual; es lo que hacen volk y el *dispatcher* de Vulkan-Hpp | Una excepción a la regla de nombres |
| **El nombre sin el prefijo `vk`** | `fns.DestroyDevice(...)` | Es lo que usa la tabla de *dispatch* del loader de Khronos | Tampoco se busca tal cual, y rompe el estilo igual |

## Investigación

- **volk**, un cargador muy usado para no enlazar el loader, guarda las funciones de un
  dispositivo en `VolkDeviceTable`, con un miembro por función y el nombre de la
  especificación: `PFN_vkCreateBuffer vkCreateBuffer;`.
- **Vulkan-Hpp**, los headers de C++ de Khronos, hacen lo mismo en
  `vk::DispatchLoaderDynamic`: `PFN_vkCreateInstance vkCreateInstance = 0;`.
- **El loader de Khronos** usa, para la tabla que comparten las capas
  (`VkLayerDispatchTable`), el nombre sin el prefijo: `PFN_vkDestroyDevice DestroyDevice;`.
- `clang-tidy` permite una excepción por carpeta: un `.clang-tidy` con
  `InheritParentConfig: true` hereda todo y cambia solo lo que nombra. Con
  `readability-identifier-naming.MemberIgnoredRegexp: '^vk[A-Z][A-Za-z0-9]*$'`, solo los
  miembros que empiezan con `vk` y una mayúscula quedan fuera de la regla, y solo en
  `engine/gpu`. Un miembro como `m_handle` en el mismo módulo sigue con la regla.

## Decisión

Los punteros de las tablas de funciones de Vulkan se llaman **exactamente como la
función en la especificación** (`vkCreateDevice`), sin `m_` y sin `snake_case`, como en
volk y en Vulkan-Hpp. Lo que busco es poder copiar el nombre de un mensaje de la
validación y encontrarlo en el código, y al revés, sin traducir.

La excepción vale **solo en `engine/gpu`** y **solo para nombres con la forma de una
función de Vulkan** (`vk` y una mayúscula). La aplica `clang-tidy`, con un `.clang-tidy`
propio del módulo que hereda las reglas de la raíz. El resto de la Decisión 2 del
ADR-0013 sigue igual, también dentro de `engine/gpu`.

## Consecuencias

- `engine/gpu/.clang-tidy` lleva la excepción, con su razón al lado.
- Las tablas se escriben con *X-macros*: cada función se nombra una sola vez, y el
  nombre del miembro y el de la cadena que se pide al loader salen del mismo token. Así
  no pueden quedar distintos.
- Si otro módulo carga otra API de C del mismo modo (por ejemplo, una tabla de funciones
  de la API de RenderDoc), no entra en esta excepción: necesita la suya.
- La regla no distingue un puntero de una tabla de un miembro común que empiece con `vk`
  y una mayúscula. En `engine/gpu`, un miembro así es casi seguro un puntero a una
  función de Vulkan; si apareciera otro caso, se ve en la revisión.

## Cómo lo defiendo

**¿Por qué no seguiste tu propia regla de nombres?**
Porque estos miembros no son nombres míos: son las funciones de Vulkan. Si los
renombro, cada mensaje de la validación (`vkGetDeviceQueue: queueIndex …`) me obliga a
traducir el nombre para encontrar la llamada. Con el nombre de la especificación, lo
busco tal cual. volk y Vulkan-Hpp hacen lo mismo.

**¿Una excepción no abre la puerta a otras?**
Está acotada por la herramienta: solo en `engine/gpu` y solo para nombres con la forma
`vk` más una mayúscula. Cualquier otro miembro del módulo sigue con `m_` y
`snake_case`, y `clang-tidy` lo revisa.

## Referencias

- volk, `volk.h` (`VolkDeviceTable`): <https://github.com/zeux/volk>
- Vulkan-Hpp, `vulkan.hpp` (`DispatchLoaderDynamic`):
  <https://github.com/KhronosGroup/Vulkan-Hpp>
- Vulkan-Loader, `vk_layer_dispatch_table.h` (`VkLayerDispatchTable`):
  <https://github.com/KhronosGroup/Vulkan-Loader>
- Especificación de Vulkan, "Command Function Pointers":
  <https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#initialization-functionpointers>
- `readability-identifier-naming` de clang-tidy:
  <https://clang.llvm.org/extra/clang-tidy/checks/readability/identifier-naming.html>
