#include "offscreen_test.h"

#include "mz/platform/context.h"
#include "mz/platform/event.h"
#include "mz/platform/window.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_scancode.h>
#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <optional>
#include <thread>
#include <utility>

namespace {

using mz::platform::context;
using mz::platform::event;
using mz::platform::event_type;
using mz::platform::key;

class ContextTest : public OffscreenTest {};
class ContextDeathTest : public OffscreenTest {};
class ClockTest : public OffscreenTest {};

SDL_Event window_event(SDL_EventType type, std::int32_t data1 = 0, std::int32_t data2 = 0) {
    SDL_Event sdl_event{};
    sdl_event.window.type = type;
    sdl_event.window.data1 = data1;
    sdl_event.window.data2 = data2;
    return sdl_event;
}

SDL_Event key_event(SDL_EventType type, SDL_Scancode scancode, bool repeat = false) {
    SDL_Event sdl_event{};
    sdl_event.key.type = type;
    sdl_event.key.scancode = scancode;
    sdl_event.key.down = type == SDL_EVENT_KEY_DOWN;
    sdl_event.key.repeat = repeat;
    return sdl_event;
}

SDL_Event plain_event(SDL_EventType type) {
    SDL_Event sdl_event{};
    sdl_event.type = type;
    return sdl_event;
}

TEST_F(ContextTest, StartsWithTheOffscreenDriver) {
    EXPECT_TRUE(context::create().has_value()) << mz::platform::last_error();
}

TEST_F(ContextTest, IsEmptyWithoutAVideoDriver) {
    SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER, "no-existe", SDL_HINT_OVERRIDE);
    EXPECT_FALSE(context::create().has_value());
    EXPECT_FALSE(mz::platform::last_error().empty());
    // Lo que SDL alcanzó a iniciar antes de fallar, y la pista, que SDL_Quit borra.
    SDL_Quit();
}

TEST_F(ContextTest, CanStartAgainAfterBeingDestroyed) {
    EXPECT_TRUE(context::create().has_value());
    EXPECT_TRUE(context::create().has_value()) << mz::platform::last_error();
}

TEST_F(ContextTest, PollOnAnEmptyQueueIsEmpty) {
    context platform = started();
    EXPECT_FALSE(platform.poll_event().has_value());
}

TEST_F(ContextTest, QuitArrivesAsQuitRequested) {
    context platform = started();
    push(plain_event(SDL_EVENT_QUIT));
    EXPECT_EQ(next(platform).m_type, event_type::quit_requested);
}

TEST_F(ContextTest, WindowCloseArrivesAsQuitRequested) {
    context platform = started();
    push(window_event(SDL_EVENT_WINDOW_CLOSE_REQUESTED));
    EXPECT_EQ(next(platform).m_type, event_type::quit_requested);
}

TEST_F(ContextTest, PixelSizeChangeArrivesAsResized) {
    context platform = started();
    push(window_event(SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED, 2560, 1440));
    const event received = next(platform);
    EXPECT_EQ(received.m_type, event_type::resized);
    EXPECT_EQ(received.m_size.m_width, 2560U);
    EXPECT_EQ(received.m_size.m_height, 1440U);
}

TEST_F(ContextTest, FocusArrivesGainedAndLost) {
    context platform = started();
    push(window_event(SDL_EVENT_WINDOW_FOCUS_GAINED));
    push(window_event(SDL_EVENT_WINDOW_FOCUS_LOST));
    EXPECT_EQ(next(platform).m_type, event_type::focus_gained);
    EXPECT_EQ(next(platform).m_type, event_type::focus_lost);
}

TEST_F(ContextTest, MinimizingArrivesMinimizedAndRestored) {
    context platform = started();
    push(window_event(SDL_EVENT_WINDOW_MINIMIZED));
    push(window_event(SDL_EVENT_WINDOW_RESTORED));
    EXPECT_EQ(next(platform).m_type, event_type::minimized);
    EXPECT_EQ(next(platform).m_type, event_type::restored);
}

TEST_F(ContextTest, EscapeArrivesByItsPosition) {
    context platform = started();
    push(key_event(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_ESCAPE));
    push(key_event(SDL_EVENT_KEY_UP, SDL_SCANCODE_ESCAPE));
    const event down = next(platform);
    const event up = next(platform);
    EXPECT_EQ(down.m_type, event_type::key_down);
    EXPECT_EQ(down.m_key, key::escape);
    EXPECT_FALSE(down.m_repeat);
    EXPECT_EQ(up.m_type, event_type::key_up);
    EXPECT_EQ(up.m_key, key::escape);
}

TEST_F(ContextTest, KeyRepeatIsMarked) {
    context platform = started();
    push(key_event(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_ESCAPE, true));
    EXPECT_TRUE(next(platform).m_repeat);
}

TEST_F(ContextTest, AnUnnamedKeyArrivesAsUnknown) {
    context platform = started();
    push(key_event(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_A));
    const event received = next(platform);
    EXPECT_EQ(received.m_type, event_type::key_down);
    EXPECT_EQ(received.m_key, key::unknown);
}

TEST_F(ContextTest, PollSkipsEventsTheEngineDoesNotUse) {
    context platform = started();
    push(plain_event(SDL_EVENT_MOUSE_MOTION));
    push(plain_event(SDL_EVENT_QUIT));
    EXPECT_EQ(next(platform).m_type, event_type::quit_requested);
}

TEST_F(ContextTest, WaitReturnsAQueuedEvent) {
    context platform = started();
    push(plain_event(SDL_EVENT_QUIT));
    const std::optional<event> received = platform.wait_event(1000);
    if (!received) {
        FAIL() << "wait_event volvió vacío con un evento en la cola.";
    }
    EXPECT_EQ(received->m_type, event_type::quit_requested);
}

TEST_F(ContextTest, WaitOnAnEmptyQueueTimesOut) {
    context platform = started();
    EXPECT_FALSE(platform.wait_event(0).has_value());
}

TEST_F(ContextTest, WaitReturnsEmptyOnAnEventTheEngineDoesNotUse) {
    context platform = started();
    push(plain_event(SDL_EVENT_MOUSE_MOTION));
    EXPECT_FALSE(platform.wait_event(1000).has_value());
}

TEST_F(ContextTest, AMovedContextKeepsSdlRunning) {
    std::optional<context> original{started()};
    context moved{std::move(*original)};
    // El de origen se destruye primero: no tiene que cerrar SDL.
    original.reset();
    push(plain_event(SDL_EVENT_QUIT));
    EXPECT_EQ(next(moved).m_type, event_type::quit_requested);
}

TEST_F(ContextDeathTest, ASecondContextAborts) {
    EXPECT_DEATH(
        {
            const std::optional<context> first = context::create();
            const std::optional<context> second = context::create();
        },
        "ya hay un context");
}

TEST_F(ContextDeathTest, DestroyingItWithAWindowOpenAborts) {
    EXPECT_DEATH(
        {
            std::optional<context> platform{started()};
            const mz::platform::window main_window = opened(*platform);
            platform.reset();
        },
        "ventana viva");
}

TEST_F(ClockTest, NeverGoesBackwards) {
    const context platform = started();
    std::uint64_t previous = platform.now_ns();
    for (int i = 0; i < 100'000; ++i) {
        const std::uint64_t now = platform.now_ns();
        ASSERT_GE(now, previous);
        previous = now;
    }
}

// Si el reloj contara en otra unidad (micro o milisegundos), 2 ms darían 2000 o 2.
TEST_F(ClockTest, CountsInNanoseconds) {
    const context platform = started();
    const std::uint64_t before = platform.now_ns();
    std::this_thread::sleep_for(std::chrono::milliseconds{2});
    const std::uint64_t elapsed = platform.now_ns() - before;
    EXPECT_GE(elapsed, 2'000'000U);
    EXPECT_LT(elapsed, 1'000'000'000U);
}

} // namespace
