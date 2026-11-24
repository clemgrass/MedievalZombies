#pragma once

// instance: la conexión del juego con Vulkan. Es lo primero que se crea y lo último que se
// destruye: a partir de ella se ven las GPUs del sistema y se crea la superficie de la
// ventana.
//
//   auto created = instance::create(library->get_instance_proc_addr(),
//                                   library->instance_extensions());
//   if (!created) {
//       // created.error() dice qué falta: una versión, una layer, una extensión.
//   }
//
// Pide Vulkan 1.3. Con la validación activa (MZ_VULKAN_VALIDATION, encendida en los presets
// debug, debug-gcc y coverage), suma la layer
// de validación de Khronos y un messenger de VK_EXT_debug_utils que recibe sus mensajes:
//
//   error    aborta con el texto del mensaje: es un bug, como un assert.
//   warning  se escribe en stderr y se cuenta (validation_warnings()).
//
// Sin la validación, nada de eso existe: ni la layer ni el messenger cuestan nada.
//
// Solo se mueve, nunca se copia. Se destruye después de todo lo que se creó con ella (la
// superficie, el dispositivo): si no, la validación lo reporta como un error.

#include "mz/core/result.h"
#include "mz/gpu/functions.h"
#include "mz/gpu/init_error.h"

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <optional>
#include <span>
#include <utility>

namespace mz::gpu {

// La prende o la apaga cada preset (MZ_VULKAN_VALIDATION, 1 o 0). Sin la definición, el
// build no está armado con CMake y no compila.
inline constexpr bool validation_enabled = MZ_VULKAN_VALIDATION != 0;

inline constexpr const char *validation_layer_name = "VK_LAYER_KHRONOS_validation";

class instance {
public:
    // `get` es el vkGetInstanceProcAddr del loader: de él salen todas las funciones.
    // `extensions` son las que pide el sistema de ventanas para la superficie.
    //
    // Definida aquí, con el tipo de retorno deducido: dentro de la clase, instance todavía
    // es un tipo incompleto, y nombrar result<instance, init_error> en una declaración
    // comprobaría que se puede mover antes de que se sepa.
    [[nodiscard]] static auto create(PFN_vkGetInstanceProcAddr get,
                                     std::span<const char *const> extensions) {
        using created = core::result<instance, init_error>;
        instance made;
        if (const std::optional<init_error> failed = made.build(get, extensions)) {
            return created{core::unexpected{*failed}};
        }
        return created{std::move(made)};
    }

    instance(const instance &) = delete;
    instance &operator=(const instance &) = delete;
    // Mover deja el origen vacío: su destructor no destruye nada.
    instance(instance &&other) noexcept;
    instance &operator=(instance &&) = delete;
    ~instance();

    [[nodiscard]] VkInstance handle() const { return m_instance; }
    [[nodiscard]] const instance_functions &functions() const { return m_functions; }

private:
    instance() = default;

    // Crea todo en orden. Si algo falla, devuelve el error y deja creado lo de antes: el
    // destructor lo limpia.
    std::optional<init_error> build(PFN_vkGetInstanceProcAddr get,
                                    std::span<const char *const> extensions);

    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_messenger = VK_NULL_HANDLE;
    instance_functions m_functions;
};

// Cuántos warnings de la validación hubo desde que empezó el proceso. Los errores no se
// cuentan: abortan. Un juego limpio termina con 0; los tests lo comparan antes y después.
[[nodiscard]] std::uint32_t validation_warnings();

} // namespace mz::gpu
