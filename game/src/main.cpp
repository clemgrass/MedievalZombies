#include "mz/core/core.h"
#include "mz/gpu/gpu.h"
#include "mz/platform/context.h"
#include "mz/platform/event.h"
#include "mz/platform/window.h"

#include <cstdio>
#include <optional>
#include <string_view>

namespace {

using mz::platform::event;
using mz::platform::event_type;

// Lo que tarda en volver wait_event si no llega nada. Todavía no hay frames: el juego
// duerme hasta el próximo evento.
constexpr int event_timeout_ms = 100;

int report(const char *what) {
    const std::string_view reason = mz::platform::last_error();
    std::fprintf(stderr, "%s: %.*s\n", what, static_cast<int>(reason.size()), reason.data());
    return 1;
}

} // namespace

int main() {
    // core y gpu todavía no tienen nada que el juego use. Nombrarlas obliga al linker a
    // resolverlas, y así el build comprueba que las capas enlazan.
    if (mz::core::module_name().empty() || mz::gpu::module_name().empty()) {
        return 1;
    }

    std::optional<mz::platform::context> platform = mz::platform::context::create();
    if (!platform) {
        return report("No se pudo iniciar la plataforma");
    }
    // Se declara después del context: se destruye antes.
    const std::optional<mz::platform::window> main_window =
        mz::platform::window::create(*platform, "MedievalZombies", 1280, 720);
    if (!main_window) {
        return report("No se pudo crear la ventana");
    }

    // Sale solo con el aviso de cierre: el botón o el atajo del compositor, o Ctrl+C. Esc no
    // cierra el juego: va a abrir la pausa.
    while (true) {
        const std::optional<event> received = platform->wait_event(event_timeout_ms);
        if (received && received->m_type == event_type::quit_requested) {
            return 0;
        }
    }
}
