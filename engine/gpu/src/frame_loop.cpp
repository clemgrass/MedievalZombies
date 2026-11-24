#include "mz/gpu/frame_loop.h"

#include "load.h"

#include "mz/gpu/device.h"
#include "mz/gpu/swapchain.h"

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <limits>
#include <optional>
#include <utility>

namespace mz::gpu {
namespace {

// Esperar sin tope: con FIFO, la espera más larga es la de un refresco de la pantalla. Si
// la GPU se colgara, el driver devuelve VK_ERROR_DEVICE_LOST, y eso aborta.
constexpr std::uint64_t forever = std::numeric_limits<std::uint64_t>::max();

} // namespace

frame_loop frame_loop::create(const device &gpu) {
    frame_loop made;
    made.m_device = gpu.handle();
    made.m_queue = gpu.queue();
    made.m_functions = gpu.functions();
    const device_functions &vk = made.m_functions;

    for (slot &each : made.m_slots) {
        VkCommandPoolCreateInfo pool{};
        pool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        // TRANSIENT: sus command buffers viven un frame. El pool se vacía entero cada vez
        // (vkResetCommandPool), así que no hace falta resetearlos uno por uno.
        pool.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        pool.queueFamilyIndex = gpu.queue_family();
        detail::check_call(vk.vkCreateCommandPool(made.m_device, &pool, nullptr, &each.m_pool),
                           "vkCreateCommandPool");

        VkCommandBufferAllocateInfo commands{};
        commands.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commands.commandPool = each.m_pool;
        commands.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commands.commandBufferCount = 1;
        detail::check_call(vk.vkAllocateCommandBuffers(made.m_device, &commands, &each.m_commands),
                           "vkAllocateCommandBuffers");

        // Nace señalada: el primer frame de cada slot no tiene un frame anterior que esperar.
        VkFenceCreateInfo fence{};
        fence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        detail::check_call(vk.vkCreateFence(made.m_device, &fence, nullptr, &each.m_done),
                           "vkCreateFence");

        VkSemaphoreCreateInfo semaphore{};
        semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        detail::check_call(
            vk.vkCreateSemaphore(made.m_device, &semaphore, nullptr, &each.m_acquired),
            "vkCreateSemaphore");
    }
    return made;
}

frame_loop::frame_loop(frame_loop &&other) noexcept
    : m_device(std::exchange(other.m_device, VK_NULL_HANDLE)), m_queue(other.m_queue),
      m_functions(other.m_functions), m_slots(other.m_slots), m_current(other.m_current),
      m_suboptimal(other.m_suboptimal) {}

frame_loop::~frame_loop() {
    if (m_device == VK_NULL_HANDLE) {
        return;
    }
    // Nada se destruye mientras la GPU lo usa: los frames en vuelo terminan primero.
    detail::check_call(m_functions.vkDeviceWaitIdle(m_device), "vkDeviceWaitIdle");
    for (const slot &each : m_slots) {
        m_functions.vkDestroySemaphore(m_device, each.m_acquired, nullptr);
        m_functions.vkDestroyFence(m_device, each.m_done, nullptr);
        // Destruir el pool libera sus command buffers.
        m_functions.vkDestroyCommandPool(m_device, each.m_pool, nullptr);
    }
}

std::optional<frame> frame_loop::begin(const swapchain &output) {
    const slot &current = m_slots[m_current];

    // Los cuatro pasos, cada uno por separado (el orden, abajo):
    //
    //   wait_until_free   la CPU espera la fence: la GPU terminó el frame anterior de este
    //                     slot, y su pool y su semáforo ya se pueden reusar
    //   mark_busy         la fence vuelve a "no señalada", para que end() la entregue al
    //                     envío y la GPU la señale de nuevo
    //   acquire           pide una imagen del swapchain; vacío si OUT_OF_DATE
    //   open_commands     vacía el pool y abre el command buffer
    const auto wait_until_free = [&] {
        detail::check_call(
            m_functions.vkWaitForFences(m_device, 1, &current.m_done, VK_TRUE, forever),
            "vkWaitForFences");
    };
    const auto mark_busy = [&] {
        detail::check_call(m_functions.vkResetFences(m_device, 1, &current.m_done),
                           "vkResetFences");
    };
    const auto acquire = [&]() -> std::optional<std::uint32_t> {
        std::uint32_t index = 0;
        const VkResult result = m_functions.vkAcquireNextImageKHR(
            m_device, output.handle(), forever, current.m_acquired, VK_NULL_HANDLE, &index);
        // SUBOPTIMAL entrega la imagen y señala el semáforo: el frame se dibuja igual, y
        // end() pide recrear el swapchain después de presentarla.
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            return std::nullopt;
        }
        detail::check_call(result, "vkAcquireNextImageKHR");
        m_suboptimal = result == VK_SUBOPTIMAL_KHR;
        return index;
    };
    const auto open_commands = [&] {
        detail::check_call(m_functions.vkResetCommandPool(m_device, current.m_pool, 0),
                           "vkResetCommandPool");
        VkCommandBufferBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        detail::check_call(m_functions.vkBeginCommandBuffer(current.m_commands, &info),
                           "vkBeginCommandBuffer");
    };

    // La fence se resetea recién cuando hay imagen: desde ahí, end() siempre envía y la GPU
    // la vuelve a señalar. Si se reseteara antes y acquire diera OUT_OF_DATE, no habría
    // envío que la señalara, y el próximo begin de este slot la esperaría para siempre.
    //
    // Y se adquiere después de esperarla: acquire señala el semáforo de este slot, y el
    // envío anterior del slot tiene que haber terminado de esperarlo antes.
    wait_until_free();
    const std::optional<std::uint32_t> index = acquire();
    if (!index) {
        // La fence sigue señalada: el próximo begin de este slot no espera nada.
        return std::nullopt;
    }
    mark_busy();
    open_commands();

    const swapchain::image &target = output.at(*index);
    return frame{.m_slot = m_current,
                 .m_commands = current.m_commands,
                 .m_image_index = *index,
                 .m_image = target.m_image,
                 .m_view = target.m_view,
                 .m_extent = output.extent()};
}

present_result frame_loop::end(const frame &current, const swapchain &output,
                               VkPipelineStageFlags2 image_wait_stage) {
    const slot &used = m_slots[m_current];
    const VkSemaphore rendered = output.at(current.m_image_index).m_rendered;
    detail::check_call(m_functions.vkEndCommandBuffer(current.m_commands), "vkEndCommandBuffer");

    // El envío espera la imagen adquirida en `image_wait_stage`: lo que corre antes de esa
    // etapa (nada, en el clear) no espera. Al terminar, señala el semáforo de render
    // terminado de la imagen (lo espera la presentación) y la fence del slot (la espera la
    // CPU en el próximo begin de este slot).
    VkSemaphoreSubmitInfo wait{};
    wait.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    wait.semaphore = used.m_acquired;
    wait.stageMask = image_wait_stage;
    VkSemaphoreSubmitInfo signal{};
    signal.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signal.semaphore = rendered;
    // Todo el frame tiene que haber terminado antes de presentar.
    signal.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    VkCommandBufferSubmitInfo commands{};
    commands.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    commands.commandBuffer = current.m_commands;
    VkSubmitInfo2 submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submit.waitSemaphoreInfoCount = 1;
    submit.pWaitSemaphoreInfos = &wait;
    submit.commandBufferInfoCount = 1;
    submit.pCommandBufferInfos = &commands;
    submit.signalSemaphoreInfoCount = 1;
    submit.pSignalSemaphoreInfos = &signal;
    detail::check_call(m_functions.vkQueueSubmit2(m_queue, 1, &submit, used.m_done),
                       "vkQueueSubmit2");

    const VkSwapchainKHR swapchain_handle = output.handle();
    VkPresentInfoKHR present{};
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &rendered;
    present.swapchainCount = 1;
    present.pSwapchains = &swapchain_handle;
    present.pImageIndices = &current.m_image_index;
    const VkResult result = m_functions.vkQueuePresentKHR(m_queue, &present);

    m_current = (m_current + 1) % frames_in_flight;
    const bool acquired_suboptimal = std::exchange(m_suboptimal, false);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        return present_result::recreate_swapchain;
    }
    detail::check_call(result, "vkQueuePresentKHR");
    return acquired_suboptimal ? present_result::recreate_swapchain : present_result::presented;
}

} // namespace mz::gpu
