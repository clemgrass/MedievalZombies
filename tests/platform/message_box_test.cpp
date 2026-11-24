#include "mz/platform/context.h"
#include "mz/platform/message_box.h"
#include "mz/test/offscreen.h"

#include <gtest/gtest.h>

#include <cstdlib>
#include <optional>
#include <string>

namespace {

using mz::platform::message_kind;
using mz::platform::show_message;

// Una variable de entorno borrada mientras vive el objeto, y restaurada después.
class unset_env {
public:
    explicit unset_env(const char *name) : m_name(name) {
        if (const char *value = std::getenv(name)) {
            m_saved = value;
        }
        unsetenv(name);
    }
    unset_env(const unset_env &) = delete;
    unset_env &operator=(const unset_env &) = delete;
    unset_env(unset_env &&) = delete;
    unset_env &operator=(unset_env &&) = delete;
    ~unset_env() {
        if (m_saved) {
            setenv(m_name, m_saved->c_str(), 1);
        }
    }

private:
    const char *m_name;
    std::optional<std::string> m_saved;
};

// Sin pantalla (sin Wayland ni X11), el cuadro no se puede mostrar: vuelve false con el
// motivo, sin quedarse esperando. Es lo que pasa en la CI. Que el cuadro se vea y espere el
// clic se prueba a mano (ver la descripción del PR).
class MessageBoxTest : public OffscreenTest {};

TEST_F(MessageBoxTest, WithoutADisplayFailsInsteadOfWaiting) {
    const unset_env no_wayland{"WAYLAND_DISPLAY"};
    const unset_env no_x11{"DISPLAY"};

    EXPECT_FALSE(show_message(message_kind::error, "test", "un error"));
    EXPECT_FALSE(mz::platform::last_error().empty());
    EXPECT_FALSE(show_message(message_kind::warning, "test", "un aviso"));
}

// Un texto más largo que el buffer se corta, sin escribir fuera de él (ASan lo vería).
TEST_F(MessageBoxTest, ALongTextIsCut) {
    const unset_env no_wayland{"WAYLAND_DISPLAY"};
    const unset_env no_x11{"DISPLAY"};
    const std::string long_text(4096, 'x');

    EXPECT_FALSE(show_message(message_kind::error, long_text, long_text));
}

} // namespace
