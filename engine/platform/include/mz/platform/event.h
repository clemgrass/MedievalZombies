#pragma once

// Los eventos del sistema operativo que le llegan al engine, con tipos propios: el resto
// del engine nunca ve un evento de SDL. platform traduce los que el engine usa y descarta
// los demás.

#include <cstdint>

namespace mz::platform {

// Un tamaño en píxeles reales de la pantalla: lo que mide la imagen que se presenta. En
// una pantalla con escala 2, una ventana de 1280×720 lógicos mide 2560×1440 píxeles.
struct pixel_size {
    std::uint32_t m_width = 0;
    std::uint32_t m_height = 0;
};

// Una tecla por su posición en el teclado, no por la letra que da: la tecla de la W es la
// misma también en un teclado francés, donde da "z". Solo están las que el engine usa.
enum class key : std::uint8_t {
    // Cualquier tecla que el engine todavía no nombra.
    unknown,
    escape,
};

enum class event_type : std::uint8_t {
    // Pidieron cerrar el juego: el botón o el atajo del compositor, o una señal (Ctrl+C).
    quit_requested,
    // La ventana cambió de tamaño en píxeles: m_size trae el nuevo.
    resized,
    focus_gained,
    focus_lost,
    // Una tecla bajó o subió: m_key dice cuál, y m_repeat si es la repetición automática
    // del sistema con la tecla mantenida.
    key_down,
    key_up,
};

// Un evento. Es un dato plano: cada campo vale solo para los tipos que lo nombran.
struct event {
    event_type m_type = event_type::quit_requested;
    pixel_size m_size;
    key m_key = key::unknown;
    bool m_repeat = false;
};

} // namespace mz::platform
