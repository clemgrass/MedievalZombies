#pragma once

// surface: donde el swapchain presenta, la ventana vista desde Vulkan. La crea
// platform::window (cada sistema de ventanas tiene su propia función para crearla), y gpu
// se queda con ella para destruirla a tiempo:
//
//   surface target{*vulkan, main_window->create_vulkan_surface(vulkan->handle())};
//
// Se destruye antes que la instancia y que la ventana. Solo se mueve, nunca se copia.

#include "mz/gpu/instance.h"

#include <vulkan/vulkan_core.h>

namespace mz::gpu {

class surface {
public:
    // Adopta una superficie ya creada en `owner`. Una nula es un error del programador
    // (había que revisar lo que devolvió la ventana): aborta.
    surface(const instance &owner, VkSurfaceKHR handle);

    surface(const surface &) = delete;
    surface &operator=(const surface &) = delete;
    // Mover deja el origen vacío: su destructor no destruye nada.
    surface(surface &&other) noexcept;
    surface &operator=(surface &&) = delete;
    ~surface();

    [[nodiscard]] VkSurfaceKHR handle() const { return m_surface; }

private:
    // Lo justo para destruirla: la instancia puede moverse, así que no se guarda un
    // puntero a ella.
    VkInstance m_instance;
    PFN_vkDestroySurfaceKHR m_destroy;
    VkSurfaceKHR m_surface;
};

} // namespace mz::gpu
