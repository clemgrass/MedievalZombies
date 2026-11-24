#pragma once

// El frame mínimo: pintar la imagen del swapchain de un color. Es lo único que dibuja el
// juego hasta que exista el renderer, que lo reemplaza por sus pasadas (la primera, el blit
// de la imagen de 640×360).
//
// Graba, en el command buffer del frame:
//
//   barrera     la imagen pasa a poder dibujarse (COLOR_ATTACHMENT_OPTIMAL)
//   rendering   vkCmdBeginRendering con loadOp CLEAR: la GPU la pinta del color al empezar
//   barrera     la imagen pasa a poder presentarse (PRESENT_SRC_KHR)

#include "mz/gpu/device.h"
#include "mz/gpu/frame_loop.h"

#include <vulkan/vulkan_core.h>

namespace mz::gpu {

// La etapa en la que el frame espera el semáforo de imagen adquirida (frame_loop::end): la
// imagen todavía puede estar en pantalla hasta que se señala. Es la etapa donde el clear
// escribe, la primera que la toca: lo que corre antes no espera a la presentación. La
// primera barrera del clear parte de esta misma etapa, y así queda encadenada a la espera.
inline constexpr VkPipelineStageFlags2 clear_waits_image_at =
    VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

// `color` en lineal: el formato sRGB del swapchain lo convierte al escribir.
void record_clear(const device &gpu, const frame &current, VkClearColorValue color);

} // namespace mz::gpu
