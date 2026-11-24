#pragma once

// window: la ventana del juego, creada para Vulkan. No dibuja nada por su cuenta: la imagen
// la presenta el renderer con su swapchain. En Wayland, la ventana recién se muestra al
// recibir su primera imagen.
//
//   auto main_window = window::create(*platform, "MedievalZombies", 1280, 720);
//
// Pide los píxeles reales de la pantalla (alta densidad): en una pantalla con escala 2,
// 1280×720 lógicos son 2560×1440 píxeles, y la imagen se escala por un factor entero. Sin
// eso, el compositor estiraría una imagen de la mitad de tamaño.
//
// Solo se mueve, nunca se copia. Se destruye antes que su context.

#include "mz/platform/context.h"
#include "mz/platform/event.h"

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <optional>

namespace mz::platform {

class window {
public:
    // Crea la ventana con un tamaño en unidades lógicas (las del escritorio). Pide el
    // context como prueba de que SDL está iniciada. Vacío si el sistema no puede crearla,
    // por ejemplo sin un driver de Vulkan instalado; el motivo queda en last_error().
    [[nodiscard]] static std::optional<window> create(context &platform, const char *title,
                                                      std::int32_t width, std::int32_t height);

    window(const window &) = delete;
    window &operator=(const window &) = delete;
    // Mover deja la ventana de origen vacía: su destructor no hace nada.
    window(window &&other) noexcept;
    window &operator=(window &&) = delete;
    ~window();

    // El tamaño actual en píxeles reales: el que va a tener el swapchain.
    [[nodiscard]] pixel_size size_in_pixels() const;

    // Crea la superficie de Vulkan de esta ventana: donde el swapchain presenta. La
    // instancia tiene que haberse creado con vulkan_library::instance_extensions().
    // VK_NULL_HANDLE si no se puede; el motivo queda en last_error(). La superficie es de
    // quien la pide: la destruye con vkDestroySurfaceKHR antes que la instancia y que esta
    // ventana.
    [[nodiscard]] VkSurfaceKHR create_vulkan_surface(VkInstance instance) const;

private:
    explicit window(void *native) : m_native(native) {}

    // La ventana de SDL. Va como void* para que este header no nombre ningún tipo de SDL;
    // solo window_sdl.cpp sabe qué es.
    void *m_native;
};

} // namespace mz::platform
