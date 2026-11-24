#pragma once

// vulkan_library: el loader de Vulkan del sistema, cargado en runtime. El juego no enlaza
// libvulkan: lo carga SDL al pedirlo, y de aquí sale el único puntero que engine/gpu
// necesita para pedir todas las demás funciones.
//
//   auto library = vulkan_library::load(*platform);
//   if (!library) {
//       // No hay un driver de Vulkan instalado: el motivo, en last_error().
//   }
//   PFN_vkGetInstanceProcAddr get = library->get_instance_proc_addr();
//
// No necesita una ventana: el modo de prueba de humo pregunta si hay Vulkan sin abrir
// una. La superficie donde se dibuja la crea la ventana (window::create_vulkan_surface).
//
// Solo se mueve, nunca se copia. Se destruye antes que su context.

#include "mz/platform/context.h"

#include <vulkan/vulkan_core.h>

#include <optional>
#include <span>

namespace mz::platform {

class vulkan_library {
public:
    // Carga el loader de Vulkan del sistema. Pide el context como prueba de que SDL está
    // iniciada. Vacío si no hay loader; el motivo queda en last_error().
    [[nodiscard]] static std::optional<vulkan_library> load(context &platform);

    vulkan_library(const vulkan_library &) = delete;
    vulkan_library &operator=(const vulkan_library &) = delete;
    // Mover deja el origen vacío: su destructor no descarga nada.
    vulkan_library(vulkan_library &&other) noexcept;
    vulkan_library &operator=(vulkan_library &&) = delete;
    ~vulkan_library();

    // vkGetInstanceProcAddr del loader: con él se piden las funciones globales (con la
    // instancia VK_NULL_HANDLE) y las de una instancia.
    [[nodiscard]] PFN_vkGetInstanceProcAddr get_instance_proc_addr() const;

    // Las extensiones de instancia que hacen falta para crear una superficie en el sistema
    // de ventanas actual (Wayland, X11, Win32...). Los textos son de SDL y viven mientras
    // viva el loader.
    [[nodiscard]] std::span<const char *const> instance_extensions() const;

private:
    vulkan_library() = default;

    bool m_loaded = true;
};

} // namespace mz::platform
