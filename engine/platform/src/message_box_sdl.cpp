#include "mz/platform/message_box.h"

#include <SDL3/SDL_messagebox.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <string_view>

namespace mz::platform {
namespace {

// SDL pide textos terminados en '\0', y un string_view no lo garantiza: se copia a un buffer
// en la pila, cortado si no cabe. Sin pedir memoria, por si el error fue justamente que no
// había.
class terminated {
public:
    explicit terminated(std::string_view text) {
        const std::size_t size = std::min(text.size(), m_buffer.size() - 1);
        std::ranges::copy(text.substr(0, size), m_buffer.begin());
        m_buffer[size] = '\0';
    }

    [[nodiscard]] const char *c_str() const { return m_buffer.data(); }

private:
    std::array<char, 1024> m_buffer{};
};

} // namespace

bool show_message(message_kind kind, std::string_view title, std::string_view text) {
    const SDL_MessageBoxFlags flags =
        kind == message_kind::error ? SDL_MESSAGEBOX_ERROR : SDL_MESSAGEBOX_WARNING;
    const terminated title_text{title};
    const terminated body{text};
    // Sin ventana madre: el cuadro tiene que poder salir aunque no haya ventana.
    return SDL_ShowSimpleMessageBox(flags, title_text.c_str(), body.c_str(), nullptr);
}

} // namespace mz::platform
