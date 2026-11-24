#include "mz/gpu/clear.h"

#include "mz/gpu/device.h"
#include "mz/gpu/frame_loop.h"

#include <vulkan/vulkan_core.h>

namespace mz::gpu {
namespace {

// Una barrera de synchronization2 sobre la imagen entera del swapchain: ordena los accesos
// de antes (src) con los de después (dst), y cambia el layout de la imagen.
VkImageMemoryBarrier2 image_barrier(VkImage image) {
    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    // La misma cola antes y después: la imagen no cambia de dueño.
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    return barrier;
}

void record_barrier(const device &gpu, VkCommandBuffer commands,
                    const VkImageMemoryBarrier2 &barrier) {
    VkDependencyInfo dependency{};
    dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependency.imageMemoryBarrierCount = 1;
    dependency.pImageMemoryBarriers = &barrier;
    gpu.functions().vkCmdPipelineBarrier2(commands, &dependency);
}

} // namespace

void record_clear(const device &gpu, const frame &current, VkClearColorValue color) {
    // Antes: de UNDEFINED, porque lo que tenía la imagen no importa (el clear la pinta
    // entera), y así también sirve la primera vez que se usa, cuando todavía no tiene
    // layout. El origen es la etapa en la que el envío espera la imagen adquirida
    // (clear_waits_image_at): la barrera queda encadenada a esa espera, y la transición de
    // layout no puede ocurrir mientras la pantalla todavía lee la imagen. Sin acceso de
    // origen: la presentación no escribió nada que haya que hacer visible. Con TOP_OF_PIPE
    // de origen, en cambio, la cadena se cortaría.
    VkImageMemoryBarrier2 to_drawable = image_barrier(current.m_image);
    to_drawable.srcStageMask = clear_waits_image_at;
    to_drawable.srcAccessMask = VK_ACCESS_2_NONE;
    to_drawable.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    to_drawable.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    to_drawable.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    to_drawable.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    record_barrier(gpu, current.m_commands, to_drawable);

    VkRenderingAttachmentInfo target{};
    target.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    target.imageView = current.m_view;
    target.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // CLEAR: lo que había no importa, la GPU la pinta al empezar. STORE: lo pintado queda
    // en la imagen, para presentarla.
    target.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    target.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    target.clearValue.color = color;
    VkRenderingInfo rendering{};
    rendering.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    rendering.renderArea.extent = current.m_extent;
    rendering.layerCount = 1;
    rendering.colorAttachmentCount = 1;
    rendering.pColorAttachments = &target;
    gpu.functions().vkCmdBeginRendering(current.m_commands, &rendering);
    gpu.functions().vkCmdEndRendering(current.m_commands);

    // Después: la escritura del clear termina y queda visible antes de la transición a
    // PRESENT_SRC_KHR. Sin etapa ni acceso de destino: la presentación no es una etapa de la
    // GPU. La ordena el semáforo de render terminado, que el envío señala en ALL_COMMANDS,
    // después de esta barrera.
    VkImageMemoryBarrier2 to_presentable = image_barrier(current.m_image);
    to_presentable.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    to_presentable.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    to_presentable.dstStageMask = VK_PIPELINE_STAGE_2_NONE;
    to_presentable.dstAccessMask = VK_ACCESS_2_NONE;
    to_presentable.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    to_presentable.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    record_barrier(gpu, current.m_commands, to_presentable);
}

} // namespace mz::gpu
