#pragma once

// swapchain: las imágenes en las que el juego dibuja y que la ventana muestra, por turnos.
// Mientras la pantalla muestra una, la GPU dibuja en otra.
//
//   swapchain output = swapchain::create(*vulkan, *gpu, target, window_size);
//   ...
//   output.recreate(new_size);   // la ventana cambió de tamaño
//
// Cómo se arma:
//
//   presentación  FIFO: cada imagen espera al siguiente refresco de la pantalla (vsync), en
//                 orden, sin descartar ninguna. Es el único modo que la especificación
//                 garantiza en toda GPU.
//   alfa          OPAQUE: el compositor ignora el alfa de la imagen. El juego guarda en el
//                 alfa la máscara de los personajes, no una transparencia.
//   formato       sRGB (choose_surface_format): la GPU convierte el color lineal del shader
//                 a sRGB al escribirlo.
//   imágenes      choose_image_count; cada una con su vista y su semáforo de "terminé de
//                 dibujarla" (el que espera la presentación).
//   tamaño        choose_extent.
//
// Recrearlo espera a que la GPU termine todo lo que tiene en curso (vkDeviceWaitIdle): pasa
// solo cuando la ventana cambia, y así ninguna imagen vieja sigue en uso.
//
// Solo se mueve, nunca se copia. Se destruye antes que el dispositivo y la superficie.

#include "mz/gpu/device.h"
#include "mz/gpu/functions.h"
#include "mz/gpu/instance.h"
#include "mz/gpu/surface.h"

#include <vulkan/vulkan_core.h>

#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace mz::gpu {

// El formato de las imágenes, entre los que ofrece la superficie: B8G8R8A8_SRGB o, si no
// está, R8G8B8A8_SRGB, los dos en el espacio de color sRGB. Vacío si no hay ninguno: la GPU
// no cumple los requisitos (device lo verifica antes).
[[nodiscard]] std::optional<VkSurfaceFormatKHR>
choose_surface_format(std::span<const VkSurfaceFormatKHR> formats);

// Cuántas imágenes: al menos 3, o el mínimo de la superficie si pide más, y nunca más que
// su máximo (0 es sin máximo). Con 3, mientras la pantalla muestra una, la GPU puede tener
// otra lista y dibujar una tercera.
[[nodiscard]] std::uint32_t choose_image_count(const VkSurfaceCapabilitiesKHR &capabilities);

// El tamaño de las imágenes: el de la superficie, si lo tiene. En Wayland no lo tiene
// (currentExtent vale 0xFFFFFFFF): entonces es el de la ventana en píxeles, dentro de los
// límites de la superficie.
[[nodiscard]] VkExtent2D choose_extent(const VkSurfaceCapabilitiesKHR &capabilities,
                                       VkExtent2D window);

// El tope de imágenes que guarda el swapchain, sin pedir memoria. El driver puede crear más
// de las que se piden, pero ninguno conocido pasa de 5.
inline constexpr std::uint32_t max_swapchain_images = 8;

class swapchain {
public:
    // Una imagen del swapchain: la imagen, su vista (lo que nombra dynamic rendering) y el
    // semáforo que se señala al terminar de dibujarla, el que espera su presentación.
    struct image {
        VkImage m_image = VK_NULL_HANDLE;
        VkImageView m_view = VK_NULL_HANDLE;
        VkSemaphore m_rendered = VK_NULL_HANDLE;
    };

    // Crea el swapchain en `target` para una ventana de `window` píxeles. Los requisitos ya
    // se verificaron al elegir la GPU: si una llamada falla, es falta de memoria o una GPU
    // perdida, y aborta con su nombre.
    [[nodiscard]] static swapchain create(const instance &owner, const device &gpu,
                                          const surface &target, VkExtent2D window);

    swapchain(const swapchain &) = delete;
    swapchain &operator=(const swapchain &) = delete;
    // Mover deja el origen vacío: su destructor no destruye nada.
    swapchain(swapchain &&other) noexcept;
    swapchain &operator=(swapchain &&) = delete;
    ~swapchain();

    // Lo rehace para una ventana de `window` píxeles: cuando la ventana cambió de tamaño o
    // cuando la presentación dijo que el swapchain ya no sirve (OUT_OF_DATE, SUBOPTIMAL).
    void recreate(VkExtent2D window);

    [[nodiscard]] VkSwapchainKHR handle() const { return m_swapchain; }
    [[nodiscard]] VkSurfaceFormatKHR format() const { return m_format; }
    [[nodiscard]] VkExtent2D extent() const { return m_extent; }
    [[nodiscard]] std::uint32_t image_count() const { return m_image_count; }
    // La imagen `index`, el que devolvió vkAcquireNextImageKHR. Fuera de rango, aborta.
    [[nodiscard]] const image &at(std::uint32_t index) const;

private:
    swapchain() = default;

    // Crea el swapchain nuevo (pasándole el viejo, si hay) y sus imágenes.
    void build(VkExtent2D window);
    // Destruye las vistas y los semáforos de las imágenes, no el swapchain.
    void destroy_images();

    // Lo justo para recrearlo: el dispositivo y la instancia pueden moverse, así que no se
    // guardan punteros a ellos.
    VkPhysicalDevice m_physical = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR m_get_capabilities = nullptr;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR m_get_formats = nullptr;
    VkDevice m_device = VK_NULL_HANDLE;
    device_functions m_functions;

    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkSurfaceFormatKHR m_format{};
    VkExtent2D m_extent{};
    std::uint32_t m_image_count = 0;
    std::array<image, max_swapchain_images> m_images{};
};

} // namespace mz::gpu
