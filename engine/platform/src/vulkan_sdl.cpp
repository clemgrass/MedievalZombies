#include "mz/platform/vulkan.h"

#include "dependents.h"

#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_vulkan.h>

#include <cstddef>
#include <optional>
#include <span>
#include <utility>

namespace mz::platform {

std::optional<vulkan_library> vulkan_library::load(context & /*platform*/) {
    // nullptr: el loader por defecto del sistema (libvulkan.so.1, vulkan-1.dll), o el que
    // diga la pista SDL_HINT_VULKAN_LIBRARY. SDL cuenta las cargas: una ventana creada para
    // Vulkan también lo carga, y el loader se descarga cuando se fue el último.
    if (!SDL_Vulkan_LoadLibrary(nullptr)) {
        return std::nullopt;
    }
    detail::vulkan_loaded();
    return vulkan_library{};
}

vulkan_library::vulkan_library(vulkan_library &&other) noexcept
    : m_loaded(std::exchange(other.m_loaded, false)) {}

vulkan_library::~vulkan_library() {
    if (!m_loaded) {
        return;
    }
    SDL_Vulkan_UnloadLibrary();
    detail::vulkan_unloaded();
}

PFN_vkGetInstanceProcAddr vulkan_library::get_instance_proc_addr() const {
    // SDL lo devuelve como un puntero a función genérico; su tipo real es este.
    return reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr());
}

std::span<const char *const> vulkan_library::instance_extensions() const {
    Uint32 count = 0;
    const char *const *names = SDL_Vulkan_GetInstanceExtensions(&count);
    return {names, static_cast<std::size_t>(count)};
}

} // namespace mz::platform
