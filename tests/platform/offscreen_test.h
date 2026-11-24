#pragma once

// Los helpers de los tests de platform. Estos tests son de caja blanca: son los únicos,
// fuera de engine/platform, que incluyen SDL, para inyectar eventos de SDL y comprobar la
// traducción a los tipos propios. La base (OffscreenTest y give_up) está en
// tests/support, compartida con los tests de gpu.

#include "mz/platform/context.h"
#include "mz/platform/event.h"
#include "mz/platform/window.h"
#include "mz/test/offscreen.h"

#include <SDL3/SDL_events.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <utility>

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
