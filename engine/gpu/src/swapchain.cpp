#include "mz/gpu/swapchain.h"

#include "load.h"

#include "mz/core/assert.h"
#include "mz/gpu/device.h"
#include "mz/gpu/functions.h"
#include "mz/gpu/instance.h"
#include "mz/gpu/surface.h"

#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace mz::gpu {

std::optional<VkSurfaceFormatKHR>
choose_surface_format(std::span<const VkSurfaceFormatKHR> formats) {
    // En orden de preferencia. BGRA primero: es el orden en que guardan los píxeles las
    // pantallas y los compositores de escritorio, y el que ofrecen todos los drivers.
    constexpr std::array preferred = {VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB};
    for (const VkFormat wanted : preferred) {
        const auto matches = [&](const VkSurfaceFormatKHR &each) {
            return each.format == wanted && each.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        };
        if (const auto found = std::ranges::find_if(formats, matches); found != formats.end()) {
            return *found;
        }
    }
    return std::nullopt;
}

std::uint32_t choose_image_count(const VkSurfaceCapabilitiesKHR &capabilities) {
    constexpr std::uint32_t wanted = 3;
    const std::uint32_t count = std::max(wanted, capabilities.minImageCount);
    if (capabilities.maxImageCount == 0) {
        return count;
    }
    return std::min(count, capabilities.maxImageCount);
}

VkExtent2D choose_extent(const VkSurfaceCapabilitiesKHR &capabilities, VkExtent2D window) {
    if (capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    return {.width = std::clamp(window.width, capabilities.minImageExtent.width,
                                capabilities.maxImageExtent.width),
            .height = std::clamp(window.height, capabilities.minImageExtent.height,
                                 capabilities.maxImageExtent.height)};
}

swapchain swapchain::create(const instance &owner, const device &gpu, const surface &target,
                            VkExtent2D window) {
    swapchain made;
    made.m_physical = gpu.physical();
    made.m_surface = target.handle();
    made.m_get_capabilities = owner.functions().vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
    made.m_get_formats = owner.functions().vkGetPhysicalDeviceSurfaceFormatsKHR;
    made.m_device = gpu.handle();
    made.m_functions = gpu.functions();

    // El formato se elige una vez: la superficie no cambia sus formatos al cambiar de
    // tamaño, y así recrear no necesita pedir memoria para la lista.
    const std::vector<VkSurfaceFormatKHR> formats = detail::enumerate<VkSurfaceFormatKHR>(
        "vkGetPhysicalDeviceSurfaceFormatsKHR",
        [&](std::uint32_t *count, VkSurfaceFormatKHR *items) {
            return made.m_get_formats(made.m_physical, made.m_surface, count, items);
        });
    const std::optional<VkSurfaceFormatKHR> format = choose_surface_format(formats);
    MZ_CHECK(format.has_value(), "La superficie no tiene un formato sRGB, y la GPU se eligió "
                                 "verificando que lo tuviera");
    made.m_format = *format;

    made.build(window);
    return made;
}

swapchain::swapchain(swapchain &&other) noexcept
    : m_physical(other.m_physical), m_surface(other.m_surface),
      m_get_capabilities(other.m_get_capabilities), m_get_formats(other.m_get_formats),
      m_device(other.m_device), m_functions(other.m_functions),
      m_swapchain(std::exchange(other.m_swapchain, VK_NULL_HANDLE)), m_format(other.m_format),
      m_extent(other.m_extent), m_image_count(std::exchange(other.m_image_count, 0)),
      m_images(other.m_images) {}

swapchain::~swapchain() {
    destroy_images();
    if (m_swapchain != VK_NULL_HANDLE) {
        m_functions.vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    }
}

void swapchain::recreate(VkExtent2D window) {
    detail::check_call(m_functions.vkDeviceWaitIdle(m_device), "vkDeviceWaitIdle");
    build(window);
}

const swapchain::image &swapchain::at(std::uint32_t index) const {
    MZ_CHECK(index < m_image_count, "La imagen {} no existe: el swapchain tiene {}", index,
             m_image_count);
    return m_images[index];
}

void swapchain::build(VkExtent2D window) {
    // NOLINTNEXTLINE(bugprone-invalid-enum-default-initialization): la llamada la llena entera.
    VkSurfaceCapabilitiesKHR capabilities{};
    detail::check_call(m_get_capabilities(m_physical, m_surface, &capabilities),
                       "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    const VkExtent2D extent = choose_extent(capabilities, window);

    const VkSwapchainCreateInfoKHR info{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .surface = m_surface,
        .minImageCount = choose_image_count(capabilities),
        .imageFormat = m_format.format,
        .imageColorSpace = m_format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        // Solo se dibuja en ellas (el clear). Copiar a ellas (el blit) se suma con el blit.
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        // Una sola cola dibuja y presenta: las imágenes no se comparten entre familias.
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        // Sin rotar: la que tiene la pantalla (en el escritorio, ninguna).
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        // Los píxeles tapados por otra ventana no hace falta que existan.
        .clipped = VK_TRUE,
        // El viejo, si hay: el driver puede reusar lo suyo, y sus imágenes ya presentadas
        // terminan de mostrarse.
        .oldSwapchain = m_swapchain,
    };

    VkSwapchainKHR created = VK_NULL_HANDLE;
    detail::check_call(m_functions.vkCreateSwapchainKHR(m_device, &info, nullptr, &created),
                       "vkCreateSwapchainKHR");

    // El viejo ya no hace falta: la GPU no lo está usando (recreate esperó).
    destroy_images();
    if (m_swapchain != VK_NULL_HANDLE) {
        m_functions.vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    }
    m_swapchain = created;
    m_extent = extent;

    // El driver puede crear más imágenes que las pedidas: se pregunta cuántas hay.
    std::uint32_t count = 0;
    detail::check_call(m_functions.vkGetSwapchainImagesKHR(m_device, m_swapchain, &count, nullptr),
                       "vkGetSwapchainImagesKHR");
    MZ_CHECK(count <= max_swapchain_images, "El swapchain tiene {} imágenes, más que el tope de {}",
             count, max_swapchain_images);
    std::array<VkImage, max_swapchain_images> images{};
    detail::check_call(
        m_functions.vkGetSwapchainImagesKHR(m_device, m_swapchain, &count, images.data()),
        "vkGetSwapchainImagesKHR");

    for (std::uint32_t i = 0; i < count; ++i) {
        image &each = m_images[i];
        each.m_image = images[i];

        VkImageViewCreateInfo view{};
        view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view.image = each.m_image;
        view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view.format = m_format.format;
        view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view.subresourceRange.levelCount = 1;
        view.subresourceRange.layerCount = 1;
        detail::check_call(m_functions.vkCreateImageView(m_device, &view, nullptr, &each.m_view),
                           "vkCreateImageView");

        VkSemaphoreCreateInfo semaphore{};
        semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        detail::check_call(
            m_functions.vkCreateSemaphore(m_device, &semaphore, nullptr, &each.m_rendered),
            "vkCreateSemaphore");
    }
    m_image_count = count;
}

void swapchain::destroy_images() {
    for (std::uint32_t i = 0; i < m_image_count; ++i) {
        m_functions.vkDestroySemaphore(m_device, m_images[i].m_rendered, nullptr);
        m_functions.vkDestroyImageView(m_device, m_images[i].m_view, nullptr);
        m_images[i] = image{};
    }
    m_image_count = 0;
}

} // namespace mz::gpu
