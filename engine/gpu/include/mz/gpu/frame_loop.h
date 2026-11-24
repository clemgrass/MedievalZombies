#pragma once

// frame_loop: los frames en vuelo. Mientras la GPU dibuja un frame, la CPU ya graba el
// siguiente; con 2, la CPU nunca se adelanta más de un frame.
//
//   frame_loop frames = frame_loop::create(*gpu);
//   while (jugando) {
//       std::optional<frame> current = frames.begin(output);
//       if (!current) {
//           output.recreate(tamaño);   // el swapchain ya no sirve
//           continue;
//       }
//       record_clear(*gpu, *current, color);
//       if (frames.end(*current, output, clear_waits_image_at) ==
//           present_result::recreate_swapchain) {
//           output.recreate(tamaño);
//       }
//   }
//
// Cada frame en vuelo tiene lo suyo, para no tocar lo que la GPU todavía usa:
//
//   command pool y command buffer  donde se graban sus comandos; el pool se vacía entero
//                                  al empezar el frame, cuando la GPU ya terminó con él
//   fence                          la GPU la señala al terminar el frame: la CPU la espera
//                                  antes de reusar el pool
//   semáforo de imagen adquirida   la presentación lo señala cuando la imagen se puede
//                                  dibujar: la GPU lo espera, sin que la CPU se entere
//
// El semáforo de render terminado, en cambio, es de cada imagen del swapchain
// (swapchain::image): lo espera la presentación, y la presentación no tiene fence que diga
// cuándo terminó de esperarlo. Solo se sabe que lo hizo cuando su imagen vuelve a salir de
// vkAcquireNextImageKHR.
//
// Solo se mueve, nunca se copia. Al destruirse espera a que la GPU termine
// (vkDeviceWaitIdle). Se destruye antes que el swapchain y el dispositivo.

#include "mz/gpu/device.h"
#include "mz/gpu/functions.h"
#include "mz/gpu/swapchain.h"

#include <vulkan/vulkan_core.h>

#include <array>
#include <cstdint>
#include <optional>

namespace mz::gpu {

// Cuántos frames puede haber en curso a la vez: uno en la GPU y otro grabándose en la CPU.
inline constexpr std::uint32_t frames_in_flight = 2;

// Un frame en curso: dónde se graban sus comandos y en qué imagen del swapchain se dibuja.
// El frame en vuelo (m_slot) y la imagen (m_image_index) no van juntos: los slots se turnan
// (0, 1, 0, 1...), pero la imagen la elige el driver, entre las que tiene libres y en el
// orden que quiere. lavapipe, con 4 imágenes, entrega 0, 1, 0, 2, 0, 1.
struct frame {
    // Cuál de los frames en vuelo es (de 0 a frames_in_flight - 1): lo que la CPU escribe
    // para este frame va en lo de este slot.
    std::uint32_t m_slot = 0;
    VkCommandBuffer m_commands = VK_NULL_HANDLE;
    std::uint32_t m_image_index = 0;
    VkImage m_image = VK_NULL_HANDLE;
    VkImageView m_view = VK_NULL_HANDLE;
    VkExtent2D m_extent{};
};

enum class present_result : std::uint8_t {
    presented,
    // La imagen se presentó, o no, pero el swapchain ya no coincide con la ventana
    // (OUT_OF_DATE o SUBOPTIMAL): hay que recrearlo antes del próximo frame.
    recreate_swapchain,
};

class frame_loop {
public:
    // Crea los recursos de cada frame en vuelo. Si una llamada falla, aborta con su nombre.
    [[nodiscard]] static frame_loop create(const device &gpu);

    frame_loop(const frame_loop &) = delete;
    frame_loop &operator=(const frame_loop &) = delete;
    // Mover deja el origen vacío: su destructor no destruye nada.
    frame_loop(frame_loop &&other) noexcept;
    frame_loop &operator=(frame_loop &&) = delete;
    ~frame_loop();

    // Empieza el frame: espera a que la GPU suelte los recursos de este frame en vuelo,
    // adquiere una imagen del swapchain y abre el command buffer. Vacío si el swapchain ya
    // no sirve (OUT_OF_DATE): hay que recrearlo y volver a llamar.
    [[nodiscard]] std::optional<frame> begin(const swapchain &output);

    // Cierra el command buffer, lo envía y presenta la imagen. Pasa al siguiente frame en
    // vuelo. `image_wait_stage` es la etapa del primer comando que toca la imagen: ahí la
    // GPU espera a que la presentación la suelte (el clear la da en clear_waits_image_at).
    [[nodiscard]] present_result end(const frame &current, const swapchain &output,
                                     VkPipelineStageFlags2 image_wait_stage);

private:
    // Lo de un frame en vuelo.
    struct slot {
        VkCommandPool m_pool = VK_NULL_HANDLE;
        VkCommandBuffer m_commands = VK_NULL_HANDLE;
        VkFence m_done = VK_NULL_HANDLE;
        VkSemaphore m_acquired = VK_NULL_HANDLE;
    };

    frame_loop() = default;

    VkDevice m_device = VK_NULL_HANDLE;
    VkQueue m_queue = VK_NULL_HANDLE;
    device_functions m_functions;
    std::array<slot, frames_in_flight> m_slots{};
    // El frame en vuelo que toca: 0, 1, 0, 1...
    std::uint32_t m_current = 0;
    // La imagen del frame en curso llegó con SUBOPTIMAL: se dibuja y se presenta, y end()
    // pide recrear el swapchain.
    bool m_suboptimal = false;
};

} // namespace mz::gpu
