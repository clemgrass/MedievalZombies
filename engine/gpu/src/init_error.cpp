#include "mz/gpu/init_error.h"

#include "mz/gpu/requirements.h"

#include <cstddef>
#include <format>
#include <span>
#include <string_view>
#include <utility>

namespace mz::gpu {
namespace {

// Escribe en el buffer lo que quepa y devuelve lo escrito.
template <typename... Args>
std::string_view write(std::span<char> buffer, std::format_string<Args...> format, Args &&...args) {
    const auto written = std::format_to_n(buffer.data(), static_cast<std::ptrdiff_t>(buffer.size()),
                                          format, std::forward<Args>(args)...);
    return {buffer.data(), written.out};
}

} // namespace

std::string_view describe(const init_error &error, std::span<char> buffer) {
    const std::string_view why = purpose_of(error.m_name);
    switch (error.m_failure) {
    case init_failure::old_vulkan:
        return write(buffer,
                     "El driver de Vulkan es demasiado viejo: el juego necesita {}, para {}. "
                     "Actualiza el driver de la GPU.",
                     error.m_name, why);
    case init_failure::missing_layer:
        return write(buffer,
                     "Falta la layer {}. Los builds de desarrollo la usan para revisar cada "
                     "llamada a Vulkan; viene en el paquete vulkan-validation-layers.",
                     error.m_name);
    case init_failure::missing_extension:
        return write(buffer,
                     "El driver de Vulkan no tiene la extensión {}, que el juego necesita para "
                     "dibujar en la ventana.",
                     error.m_name);
    case init_failure::no_suitable_gpu:
        if (error.m_gpu[0] == '\0') {
            return write(buffer,
                         "No se encontró {}. Revisa que el driver de la GPU esté instalado.",
                         error.m_name);
        }
        return write(buffer,
                     "La GPU \"{}\" no puede correr el juego: le falta {}, que el juego "
                     "usa para {}.",
                     error.m_gpu.data(), error.m_name, why);
    case init_failure::call_failed:
        return write(buffer, "Falló la llamada de Vulkan {}, con el código {}.", error.m_name,
                     static_cast<int>(error.m_result));
    }
    // Todos los casos del enum vuelven arriba: -Wswitch avisa si se agrega uno sin su
    // mensaje. Esto es para un valor fuera del enum.
    return write(buffer, "Error desconocido al iniciar Vulkan.");
}

} // namespace mz::gpu
