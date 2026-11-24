#include "load.h"

#include "mz/core/assert.h"
#include "mz/gpu/functions.h"
#include "mz/gpu/instance.h"

#include <vulkan/vulkan_core.h>

namespace mz::gpu::detail {
namespace {

// Aborta si el loader no tiene la función. Una sola función, no un template por cada tipo
// de puntero: así hay un solo lugar que falla, y una sola rama que probar.
PFN_vkVoidFunction require(PFN_vkVoidFunction found, const char *name) {
    MZ_CHECK(found != nullptr, "El loader de Vulkan no tiene {}", name);
    return found;
}

} // namespace

// Cada lista de functions.h se expande en una línea por función. Para vkCreateDevice:
//
//   table.vkCreateDevice = reinterpret_cast<PFN_vkCreateDevice>(
//       require(get(instance, "vkCreateDevice"), "vkCreateDevice"));
//
// El reinterpret_cast es el del loader: entrega todas como PFN_vkVoidFunction, y el tipo
// real lo sabe quien la pidió.

global_functions load_global(PFN_vkGetInstanceProcAddr get) {
    global_functions table;
    table.vkEnumerateInstanceVersion = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
        get(VK_NULL_HANDLE, "vkEnumerateInstanceVersion"));
#define MZ_VK_LOAD(name)                                                                           \
    table.name = reinterpret_cast<PFN_##name>(require(get(VK_NULL_HANDLE, #name), #name));
    MZ_VK_GLOBAL_FUNCTIONS(MZ_VK_LOAD)
#undef MZ_VK_LOAD
    return table;
}

instance_functions load_instance(PFN_vkGetInstanceProcAddr get, VkInstance instance) {
    instance_functions table;
#define MZ_VK_LOAD(name)                                                                           \
    table.name = reinterpret_cast<PFN_##name>(require(get(instance, #name), #name));
    MZ_VK_INSTANCE_FUNCTIONS(MZ_VK_LOAD)
    if constexpr (validation_enabled) {
        MZ_VK_DEBUG_FUNCTIONS(MZ_VK_LOAD)
    }
#undef MZ_VK_LOAD
    return table;
}

device_functions load_device(PFN_vkGetDeviceProcAddr get, VkDevice device) {
    device_functions table;
#define MZ_VK_LOAD(name)                                                                           \
    table.name = reinterpret_cast<PFN_##name>(require(get(device, #name), #name));
    MZ_VK_DEVICE_FUNCTIONS(MZ_VK_LOAD)
#undef MZ_VK_LOAD
    return table;
}

void check_call(VkResult result, const char *call) {
    // Los códigos de éxito de Vulkan son >= 0 (VK_SUCCESS, VK_INCOMPLETE); los errores, < 0.
    MZ_CHECK(result >= 0, "{} devolvió el error {}", call, static_cast<int>(result));
}

} // namespace mz::gpu::detail
