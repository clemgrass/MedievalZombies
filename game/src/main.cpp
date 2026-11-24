#include "mz/gpu/device.h"
#include "mz/gpu/init_error.h"
#include "mz/gpu/instance.h"
#include "mz/gpu/surface.h"
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

namespace {

using mz::platform::event;
using mz::platform::event_type;
using mz::platform::message_kind;

constexpr std::string_view title = "MedievalZombies";

// Lo que tarda en volver wait_event si no llega nada. Todavía no hay frames: el juego
// duerme hasta el próximo evento.
constexpr int event_timeout_ms = 100;

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

    // Sale solo con el aviso de cierre: el botón o el atajo del compositor, o Ctrl+C. Esc no
    // cierra el juego: va a abrir la pausa.
    while (true) {
        const std::optional<event> received = platform->wait_event(event_timeout_ms);
        if (received && received->m_type == event_type::quit_requested) {
            return 0;
        }
    }
}
