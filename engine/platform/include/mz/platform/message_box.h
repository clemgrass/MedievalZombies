#pragma once

// show_message: un cuadro de mensaje del sistema, para lo que quien juega tiene que leer
// aunque no haya abierto el juego desde una terminal: un error que no deja arrancar (sin
// Vulkan, sin una GPU que cumpla) o un aviso importante. En Windows, quien abre el juego con
// doble clic nunca ve stderr.
//
//   show_message(message_kind::error, "MedievalZombies", "La GPU ... no puede correr el juego");
//
// Espera a que quien juega lo cierre. No necesita el context ni una ventana: sirve también
// cuando lo que falló fue crearlos.

#include <cstdint>
#include <string_view>

namespace mz::platform {

enum class message_kind : std::uint8_t {
    error,
    warning,
};

// Muestra el cuadro y espera a que lo cierren. false si no se pudo mostrar (sin pantalla,
// por ejemplo en la CI); el motivo queda en last_error(). Un texto más largo que 1023 bytes
// se corta.
bool show_message(message_kind kind, std::string_view title, std::string_view text);

} // namespace mz::platform
