#include "mz/gpu/clear.h"
#include "mz/gpu/device.h"
#include "mz/gpu/frame_loop.h"
#include "mz/gpu/init_error.h"
#include "mz/gpu/instance.h"
#include "mz/gpu/surface.h"
#include "mz/gpu/swapchain.h"
#include "mz/platform/context.h"
#include "mz/platform/event.h"
#include "mz/platform/message_box.h"
#include "mz/platform/vulkan.h"
#include "mz/platform/window.h"

#include <vulkan/vulkan_core.h>

#include <array>
#include <cstddef>
#include <cstdio>
#include <format>
#include <optional>
#include <string_view>
#include <utility>

namespace {

using mz::platform::event;
using mz::platform::event_type;
using mz::platform::message_kind;

constexpr std::string_view title = "MedievalZombies";

// Mientras no se dibuja (la ventana minimizada), el juego igual da una vuelta por frame:
// espera eventos hasta este tope. Lo que corre una vez por frame (el audio, cuando exista)
// sigue vivo, y la CPU casi no trabaja.
constexpr int idle_frame_ms = 16;

// El fondo: un gris azulado oscuro, el ambiente frío de la noche. En lineal: el swapchain
// sRGB lo convierte al escribirlo.
constexpr VkClearColorValue background{{0.02F, 0.03F, 0.05F, 1.0F}};

// Lo que dicen los eventos del frame.
struct window_state {
    bool m_quit = false;
    bool m_minimized = false;
    bool m_resized = false;
};

void apply(const event &received, window_state &state) {
    switch (received.m_type) {
    case event_type::quit_requested:
        state.m_quit = true;
        break;
    case event_type::resized:
        state.m_resized = true;
        break;
    // Cuando exista la pausa, minimizar la abre, como Esc. Mientras está minimizada, el
    // juego no dibuja: nadie lo vería.
    case event_type::minimized:
        state.m_minimized = true;
        break;
    case event_type::restored:
        state.m_minimized = false;
        break;
    default:
        break;
    }
}

VkExtent2D pixels_of(const mz::platform::window &target) {
    const mz::platform::pixel_size size = target.size_in_pixels();
    return {.width = size.m_width, .height = size.m_height};
}

// El texto de un mensaje, armado en la pila.
using message_buffer = std::array<char, 1024>;

void print(std::FILE *stream, std::string_view text) {
    std::fprintf(stream, "%.*s\n", static_cast<int>(text.size()), text.data());
}

// Un error que no deja arrancar: a stderr y en un cuadro, para quien abrió el juego sin una
// terminal. Si el cuadro no se puede mostrar (sin pantalla), queda stderr. Devuelve el
// código de salida.
// TODO(#122): también al log, cuando exista.
int fail(std::string_view message) {
    print(stderr, message);
    static_cast<void>(mz::platform::show_message(message_kind::error, title, message));
    return 1;
}

// Un error de platform: lo que se intentaba, y el motivo que dejó SDL.
int fail_platform(std::string_view what) {
    message_buffer buffer{};
    const std::string_view reason = mz::platform::last_error();
    const auto written = std::format_to_n(buffer.data(), static_cast<std::ptrdiff_t>(buffer.size()),
                                          "{}: {}", what, reason);
    return fail({buffer.data(), written.out});
}

} // namespace

int main() {
    // Cada objeto se declara después de aquel del que depende: se destruyen en el orden
    // inverso, el dispositivo primero y el context al final.
    std::optional<mz::platform::context> platform = mz::platform::context::create();
    if (!platform) {
        return fail_platform("No se pudo iniciar la plataforma");
    }
    const std::optional<mz::platform::vulkan_library> library =
        mz::platform::vulkan_library::load(*platform);
    if (!library) {
        return fail_platform("No se encontró Vulkan. Revisa que el driver de la GPU esté "
                             "instalado");
    }
    const std::optional<mz::platform::window> main_window =
        mz::platform::window::create(*platform, "MedievalZombies", 1280, 720);
    if (!main_window) {
        return fail_platform("No se pudo crear la ventana");
    }

    message_buffer buffer{};
    const auto vulkan = mz::gpu::instance::create(library->get_instance_proc_addr(),
                                                  library->instance_extensions());
    if (!vulkan) {
        return fail(mz::gpu::describe(vulkan.error(), buffer));
    }
    const VkSurfaceKHR window_surface = main_window->create_vulkan_surface(vulkan.value().handle());
    if (window_surface == VK_NULL_HANDLE) {
        return fail_platform("No se pudo crear la superficie de Vulkan de la ventana");
    }
    const mz::gpu::surface target{vulkan.value(), window_surface};
    const auto gpu = mz::gpu::device::create(vulkan.value(), target);
    if (!gpu) {
        return fail(mz::gpu::describe(gpu.error(), buffer));
    }

    // TODO(#122): al log, cuando exista.
    std::printf("GPU: %.*s\n", static_cast<int>(gpu.value().name().size()),
                gpu.value().name().data());
    // Una GPU mejor que no cumplía, y el juego va a correr en otra: se avisa y se sigue.
    if (const std::string_view warning = mz::gpu::describe_skipped(gpu.value(), buffer);
        !warning.empty()) {
        print(stderr, warning);
        static_cast<void>(mz::platform::show_message(message_kind::warning, title, warning));
    }

    mz::gpu::swapchain output =
        mz::gpu::swapchain::create(vulkan.value(), gpu.value(), target, pixels_of(*main_window));
    // Se destruye primero: al destruirse espera a que la GPU termine (vkDeviceWaitIdle), y
    // después se destruye todo lo demás, en orden inverso.
    mz::gpu::frame_loop frames = mz::gpu::frame_loop::create(gpu.value());

    // Sale solo con el aviso de cierre: el botón o el atajo del compositor, o Ctrl+C. Esc no
    // cierra el juego: va a abrir la pausa.
    window_state state;
    bool drawing = true;
    while (true) {
        // Sin dibujar, nada frena la vuelta (con FIFO la frena el vsync): se espera el
        // próximo evento, hasta un frame.
        if (!drawing) {
            if (const std::optional<event> received = platform->wait_event(idle_frame_ms)) {
                apply(*received, state);
            }
        }
        while (const std::optional<event> received = platform->poll_event()) {
            apply(*received, state);
        }
        if (state.m_quit) {
            return 0;
        }

        // Tamaño 0: en Windows, la ventana minimizada. Un swapchain no puede medir 0.
        const VkExtent2D size = pixels_of(*main_window);
        drawing = !state.m_minimized && size.width != 0 && size.height != 0;
        if (!drawing) {
            continue;
        }
        // En Wayland, cambiar el tamaño no hace que la presentación avise: el swapchain se
        // recrea con el evento.
        if (std::exchange(state.m_resized, false)) {
            output.recreate(size);
        }

        const std::optional<mz::gpu::frame> current = frames.begin(output);
        if (!current) {
            output.recreate(size);
            continue;
        }
        mz::gpu::record_clear(gpu.value(), *current, background);
        if (frames.end(*current, output, mz::gpu::clear_waits_image_at) ==
            mz::gpu::present_result::recreate_swapchain) {
            output.recreate(size);
        }
    }
}
