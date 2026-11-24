#include "offscreen_test.h"

#include "mz/platform/context.h"
#include "mz/platform/event.h"
#include "mz/platform/window.h"

#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <optional>
#include <span>
#include <utility>

namespace {

using mz::platform::context;
using mz::platform::event;
using mz::platform::event_type;
using mz::platform::pixel_size;
using mz::platform::window;

class WindowTest : public OffscreenTest {};

// Las ventanas que SDL tiene abiertas, para mirar por detrás de la interfaz propia.
class open_windows {
public:
    open_windows() : m_windows(SDL_GetWindows(&m_count)) {}
    open_windows(const open_windows &) = delete;
    open_windows &operator=(const open_windows &) = delete;
    open_windows(open_windows &&) = delete;
    open_windows &operator=(open_windows &&) = delete;
    // SDL_GetWindows devuelve un arreglo nuevo, que es de quien lo pidió.
    ~open_windows() { SDL_free(static_cast<void *>(m_windows)); }

    [[nodiscard]] std::span<SDL_Window *> all() const {
        return {m_windows, static_cast<std::size_t>(m_count)};
    }

private:
    int m_count = 0;
    SDL_Window **m_windows;
};

TEST_F(WindowTest, IsCreatedForVulkanInRealPixels) {
    context platform = started();
    const window created = opened(platform);

    const open_windows sdl;
    ASSERT_EQ(sdl.all().size(), 1U);
    const SDL_WindowFlags flags = SDL_GetWindowFlags(sdl.all()[0]);
    EXPECT_NE(flags & SDL_WINDOW_VULKAN, 0U);
    EXPECT_NE(flags & SDL_WINDOW_HIGH_PIXEL_DENSITY, 0U);
    EXPECT_NE(flags & SDL_WINDOW_RESIZABLE, 0U);
}

// El driver offscreen tiene densidad 1: un píxel por unidad lógica.
TEST_F(WindowTest, ReportsItsSizeInPixels) {
    context platform = started();
    const window created = opened(platform, 640, 360);
    const pixel_size size = created.size_in_pixels();
    EXPECT_EQ(size.m_width, 640U);
    EXPECT_EQ(size.m_height, 360U);
}

TEST_F(WindowTest, AResizeArrivesAsAnEventWithTheNewSize) {
    context platform = started();
    const window created = opened(platform, 640, 360);
    while (platform.poll_event()) {
    }

    const open_windows sdl;
    ASSERT_EQ(sdl.all().size(), 1U);
    ASSERT_TRUE(SDL_SetWindowSize(sdl.all()[0], 800, 600));
    std::optional<pixel_size> resized;
    while (const std::optional<event> received = platform.poll_event()) {
        if (received->m_type == event_type::resized) {
            resized = received->m_size;
        }
    }
    if (!resized) {
        FAIL() << "No llegó el evento de cambio de tamaño.";
    }
    EXPECT_EQ(resized->m_width, 800U);
    EXPECT_EQ(resized->m_height, 600U);
    EXPECT_EQ(created.size_in_pixels().m_width, 800U);
    EXPECT_EQ(created.size_in_pixels().m_height, 600U);
}

TEST_F(WindowTest, IsEmptyWithoutVulkan) {
    context platform = started();
    SDL_SetHintWithPriority(SDL_HINT_VULKAN_LIBRARY, "/no-existe/libvulkan.so.1",
                            SDL_HINT_OVERRIDE);
    EXPECT_FALSE(window::create(platform, "test", 640, 360).has_value());
    EXPECT_FALSE(mz::platform::last_error().empty());
}

TEST_F(WindowTest, ClosesWhenDestroyed) {
    context platform = started();
    {
        const window created = opened(platform);
    }
    EXPECT_TRUE(open_windows{}.all().empty());
}

TEST_F(WindowTest, AMovedWindowStaysOpen) {
    context platform = started();
    std::optional<window> original{opened(platform, 640, 360)};
    const window moved{std::move(*original)};
    // El de origen se destruye primero: no tiene que cerrar la ventana.
    original.reset();
    EXPECT_EQ(open_windows{}.all().size(), 1U);
    EXPECT_EQ(moved.size_in_pixels().m_width, 640U);
}

} // namespace
