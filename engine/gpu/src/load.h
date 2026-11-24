#pragma once

// La carga de las tablas de funciones (mz/gpu/functions.h) y las enumeraciones de Vulkan.

#include "mz/gpu/functions.h"

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <vector>

namespace mz::gpu::detail {

// Las funciones globales. vkEnumerateInstanceVersion puede quedar en nullptr: un loader de
// Vulkan 1.0 no la tiene, y eso es un error de la máquina, no del engine. Si falta otra,
// aborta con su nombre.
[[nodiscard]] global_functions load_global(PFN_vkGetInstanceProcAddr get);

// Las de la instancia, y las de VK_EXT_debug_utils si la validación está activa. Si falta
// una, aborta con su nombre: la instancia se creó con Vulkan 1.3 y sus extensiones, así
// que tienen que estar todas.
[[nodiscard]] instance_functions load_instance(PFN_vkGetInstanceProcAddr get, VkInstance instance);

// Las del dispositivo, pedidas a él. Si falta una, aborta con su nombre.
[[nodiscard]] device_functions load_device(PFN_vkGetDeviceProcAddr get, VkDevice device);

// Aborta con el nombre de la llamada si devolvió un error. Para las llamadas que solo fallan
// si el sistema se quedó sin memoria o la GPU se perdió (VK_ERROR_DEVICE_LOST): las
// enumeraciones, y las del swapchain y el frame, una vez verificados los requisitos.
void check_call(VkResult result, const char *call);

// Las dos llamadas de una enumeración de Vulkan: la primera pide cuántos hay, la segunda
// los llena. `call(count, items)` envuelve a la función de Vulkan. Es inicialización, fuera
// del frame: puede usar std::vector.
template <typename Item, typename Call>
[[nodiscard]] std::vector<Item> enumerate(const char *name, Call call) {
    std::uint32_t count = 0;
    check_call(call(&count, nullptr), name);
    std::vector<Item> items(count);
    check_call(call(&count, items.data()), name);
    // Si entre las dos llamadas hay menos (una GPU que se desconectó), count bajó.
    items.resize(count);
    return items;
}

} // namespace mz::gpu::detail
