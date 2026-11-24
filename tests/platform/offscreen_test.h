#pragma once

// La base de los tests que inician SDL. Estos tests son de caja blanca: son los únicos,
// fuera de engine/platform, que incluyen SDL, para inyectar eventos de SDL y comprobar la
// traducción a los tipos propios.

#include "mz/platform/context.h"
#include "mz/platform/event.h"
#include "mz/platform/window.h"

#include <SDL3/SDL_events.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string_view>
#include <utility>

// Corre SDL con el driver de video `offscreen`: crea ventanas de verdad, también con
// Vulkan, sin pantalla. Así los tests corren igual en la CI y no abren ventanas en el
// escritorio de quien los corre.
//
// Y le pide al loader de Vulkan un solo driver, lavapipe (el de software de Mesa), el mismo
// en la CI y en cada máquina. Sin eso, el loader carga todos los drivers instalados, y el
// test depende de la GPU de quien lo corre: cuánto tarda en cargarlos (el de NVIDIA, más
// de un segundo con ASan) y qué hacen al cargarse.
//
// Las dos van como variables de entorno y no como pistas de SDL (SDL_SetHint): SDL_Quit,
// al destruirse cada context, borra las pistas, y un test que crea un segundo context
// quedaría sin driver. Un test que quiere otro valor usa SDL_SetHintWithPriority con
// SDL_HINT_OVERRIDE, la única prioridad que le gana al entorno.
class OffscreenTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_EQ(setenv("VK_LOADER_DRIVERS_SELECT", "*lvp*", 1), 0);
        ASSERT_EQ(setenv("SDL_VIDEO_DRIVER", "offscreen", 1), 0);
    }
};

// Los helpers de abajo son para los tests que no prueban la creación ni la cola vacía. Si
// lo que piden no existe, el test no puede seguir: give_up() deja el motivo y aborta (los
// helpers devuelven un valor, y FAIL() solo sirve en una función void). Va a stderr, sin
// buffer: lo que GoogleTest tuviera en el buffer de stdout se perdería con el abort.
[[noreturn]] inline void give_up(std::string_view why) {
    std::fprintf(stderr, "%.*s\n", static_cast<int>(why.size()), why.data());
    std::abort();
}

// Un context con la cola vacía: SDL deja sus propios eventos al iniciar.
inline mz::platform::context started() {
    std::optional<mz::platform::context> platform = mz::platform::context::create();
    if (!platform) {
        give_up(mz::platform::last_error());
    }
    mz::platform::context created = std::move(*platform);
    while (created.poll_event()) {
    }
    return created;
}

inline mz::platform::window opened(mz::platform::context &platform, std::int32_t width = 640,
                                   std::int32_t height = 360) {
    std::optional<mz::platform::window> created =
        mz::platform::window::create(platform, "test", width, height);
    if (!created) {
        give_up(mz::platform::last_error());
    }
    return std::move(*created);
}

// El siguiente evento de la cola.
inline mz::platform::event next(mz::platform::context &platform) {
    const std::optional<mz::platform::event> received = platform.poll_event();
    if (!received) {
        give_up("La cola está vacía.");
    }
    return *received;
}

// Mete un evento de SDL en la cola, como si viniera del sistema.
inline void push(SDL_Event sdl_event) { ASSERT_TRUE(SDL_PushEvent(&sdl_event)); }
