#include "mz/platform/context.h"

#include "dependents.h"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_timer.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <utility>

namespace mz::platform {
namespace {

// SDL es un solo estado por proceso, así que el context también. Variables estáticas de
// función y no globales: se inicializan la primera vez que se usan, sin depender del
// orden en que arrancan los archivos.
bool &context_alive() {
    static bool alive = false;
    return alive;
}

int &open_windows() {
    static int count = 0;
    return count;
}

int &loaded_vulkan_libraries() {
    static int count = 0;
    return count;
}

// Un error del programador. platform está debajo de engine/core, así que no tiene
// MZ_CHECK: escribe el motivo en stderr y aborta.
[[noreturn]] void fail(const char *reason) {
    std::fprintf(stderr, "platform::context: %s\n", reason);
    std::abort();
}

key translate_key(SDL_Scancode scancode) {
    switch (scancode) {
    case SDL_SCANCODE_ESCAPE:
        return key::escape;
    default:
        return key::unknown;
    }
}

event of_type(event_type type) {
    event translated;
    translated.m_type = type;
    return translated;
}

// El evento propio que corresponde a uno de SDL, o vacío si el engine no lo usa. Hay una
// sola ventana, así que no se mira a cuál va dirigido.
std::optional<event> translate(const SDL_Event &sdl_event) {
    switch (sdl_event.type) {
    // SDL manda los dos al cerrar la única ventana, y QUIT también con SIGINT o SIGTERM.
    case SDL_EVENT_QUIT:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        return of_type(event_type::quit_requested);
    // En píxeles reales, no el aviso en unidades lógicas (SDL_EVENT_WINDOW_RESIZED): el
    // swapchain se mide en píxeles, y en una pantalla con escala cambian los dos números.
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
        event resized = of_type(event_type::resized);
        resized.m_size = {.m_width = static_cast<std::uint32_t>(sdl_event.window.data1),
                          .m_height = static_cast<std::uint32_t>(sdl_event.window.data2)};
        return resized;
    }
    case SDL_EVENT_WINDOW_FOCUS_GAINED:
        return of_type(event_type::focus_gained);
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        return of_type(event_type::focus_lost);
    // Por el scancode (la posición), no por el keycode (la letra que da según la
    // distribución del teclado).
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP: {
        event pressed = of_type(sdl_event.type == SDL_EVENT_KEY_DOWN ? event_type::key_down
                                                                     : event_type::key_up);
        pressed.m_key = translate_key(sdl_event.key.scancode);
        pressed.m_repeat = sdl_event.key.repeat;
        return pressed;
    }
    default:
        return std::nullopt;
    }
}

} // namespace

std::optional<context> context::create() {
    if (context_alive()) {
        fail("ya hay un context vivo, y SDL es una sola por proceso");
    }
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return std::nullopt;
    }
    context_alive() = true;
    return context{};
}

context::context(context &&other) noexcept : m_owns_sdl(std::exchange(other.m_owns_sdl, false)) {}

context::~context() {
    if (!m_owns_sdl) {
        return;
    }
    if (open_windows() != 0) {
        fail("se destruye con una ventana viva; la ventana se destruye antes que su context");
    }
    if (loaded_vulkan_libraries() != 0) {
        fail("se destruye con el loader de Vulkan cargado; vulkan_library se destruye antes "
             "que su context");
    }
    // Cierra todo SDL, y con él el origen del reloj.
    SDL_Quit();
    context_alive() = false;
}

std::optional<event> context::poll_event() {
    SDL_Event sdl_event;
    while (SDL_PollEvent(&sdl_event)) {
        if (std::optional<event> translated = translate(sdl_event)) {
            return translated;
        }
    }
    return std::nullopt;
}

std::optional<event> context::wait_event(std::int32_t timeout_ms) {
    SDL_Event sdl_event;
    if (!SDL_WaitEventTimeout(&sdl_event, timeout_ms)) {
        return std::nullopt;
    }
    return translate(sdl_event);
}

// SDL cuenta desde que se inició, con CLOCK_MONOTONIC_RAW en Linux: el reloj crudo del
// hardware, que ni el cambio de hora ni el ajuste gradual de NTP tocan. Los timestamps de
// los eventos salen del mismo reloj.
std::uint64_t context::now_ns() const { return SDL_GetTicksNS(); }

std::string_view last_error() { return SDL_GetError(); }

namespace detail {

void window_opened() { ++open_windows(); }
void window_closed() { --open_windows(); }

void vulkan_loaded() { ++loaded_vulkan_libraries(); }
void vulkan_unloaded() { --loaded_vulkan_libraries(); }

} // namespace detail

} // namespace mz::platform
