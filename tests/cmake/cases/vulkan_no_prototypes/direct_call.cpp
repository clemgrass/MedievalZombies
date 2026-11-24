#include <vulkan/vulkan_core.h>

// Llama a Vulkan directo, como si el juego enlazara libvulkan. Con VK_NO_PROTOTYPES, los
// headers no declaran la función: hay que pedirla al loader y llamarla por la tabla.
VkResult create(const VkInstanceCreateInfo *info, VkInstance *instance) {
    return vkCreateInstance(info, nullptr, instance);
}
