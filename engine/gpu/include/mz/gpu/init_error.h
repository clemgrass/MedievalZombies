#pragma once

// Por qué no se pudo iniciar Vulkan. Son errores esperables: dependen de la máquina de
// quien juega (su driver, su GPU), no de un bug del engine. El mensaje (describe) nombra lo
// que falta con su nombre en la especificación, para que se pueda buscar, y dice para qué
// lo usa el juego.

#include <vulkan/vulkan_core.h>

#include <array>
#include <cstdint>
#include <span>
#include <string_view>

namespace mz::gpu {

// El nombre de una GPU, como lo da el driver, terminado en '\0'.
using gpu_name = std::array<char, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE>;

enum class init_failure : std::uint8_t {
    // El loader es de una versión de Vulkan anterior a 1.3. m_name: "Vulkan 1.3".
    old_vulkan,
    // Falta la layer de validación, en un build con validación. m_name: la layer.
    missing_layer,
    // Falta una extensión de instancia. m_name: la extensión.
    missing_extension,
    // Ninguna GPU cumple todo lo que exige el engine. m_name: lo primero que le falta a la
    // primera GPU de la lista (su versión, una extensión, la cola o una feature), y m_gpu, su
    // nombre. Sin ninguna GPU, m_name es "una GPU con Vulkan" y m_gpu queda vacío.
    no_suitable_gpu,
    // Una llamada que crea un objeto devolvió un error. m_name: la función; m_result, lo que
    // devolvió.
    call_failed,
};

struct init_error {
    init_failure m_failure;
    // Un literal, o un texto que vive al menos lo que vive lo que se le pasó a quien
    // devolvió el error (los nombres de extensiones que entrega platform).
    std::string_view m_name;
    VkResult m_result = VK_SUCCESS;
    gpu_name m_gpu{};
};

// El mensaje completo para quien juega, escrito en `buffer` (sin pedir memoria): qué falta,
// dónde, y para qué lo usa el juego. Si no cabe, se corta. Devuelve lo escrito.
[[nodiscard]] std::string_view describe(const init_error &error, std::span<char> buffer);

} // namespace mz::gpu
