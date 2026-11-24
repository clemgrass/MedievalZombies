#include "mz/platform/window.h"

#include "dependents.h"

#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>

#include <cstdint>
#include <utility>

namespace mz::platform {
namespace {

SDL_Window *native_of(void *native) { return static_cast<SDL_Window *>(native); }

} // namespace

std::optional<window> window::create(context & /*platform*/, const char *title, std::int32_t width,
                                     std::int32_t height) {
    // VULKAN: SDL carga el loader de Vulkan y prepara la ventana para una superficie de
    // Vulkan; sin esa bandera, SDL_Vulkan_CreateSurface falla. HIGH_PIXEL_DENSITY: el
    // tamaño en píxeles reales de la pantalla, no en unidades lógicas.
    constexpr SDL_WindowFlags flags =
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    SDL_Window *native = SDL_CreateWindow(title, width, height, flags);
    if (native == nullptr) {
        return std::nullopt;
    }
    detail::window_opened();
    return window{native};
}

window::window(window &&other) noexcept : m_native(std::exchange(other.m_native, nullptr)) {}

window::~window() {
    if (m_native == nullptr) {
        return;
    }
    SDL_DestroyWindow(native_of(m_native));
    detail::window_closed();
}

pixel_size window::size_in_pixels() const {
    int width = 0;
    int height = 0;
    // Solo falla con una ventana que no existe, y la ventana vive mientras viva este objeto.
    SDL_GetWindowSizeInPixels(native_of(m_native), &width, &height);
    return {.m_width = static_cast<std::uint32_t>(width),
            .m_height = static_cast<std::uint32_t>(height)};
}

VkSurfaceKHR window::create_vulkan_surface(VkInstance instance) const {
    // SDL elige la función del sistema de ventanas (vkCreateWaylandSurfaceKHR,
    // vkCreateXlibSurfaceKHR...) y la pide a la instancia. Falla si la instancia no tiene
    // esa extensión.
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (!SDL_Vulkan_CreateSurface(native_of(m_native), instance, nullptr, &surface)) {
        return VK_NULL_HANDLE;
    }
    return surface;
}

} // namespace mz::platform
