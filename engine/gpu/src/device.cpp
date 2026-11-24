#include "mz/gpu/device.h"

#include "load.h"

#include "mz/gpu/functions.h"
#include "mz/gpu/init_error.h"
#include "mz/gpu/instance.h"
#include "mz/gpu/requirements.h"
#include "mz/gpu/surface.h"
#include "mz/gpu/swapchain.h"

#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace mz::gpu {
namespace {

constexpr std::string_view no_gpu = "una GPU con Vulkan";

// La primera familia de colas que dibuja (gráficos) y presenta en la superficie. Son dos
// preguntas distintas: una GPU puede dibujar en una familia y presentar desde otra, y
// entonces la imagen tendría que pasar de una cola a la otra.
std::optional<std::uint32_t> graphics_and_present_family(const instance_functions &vk,
                                                         VkPhysicalDevice gpu,
                                                         VkSurfaceKHR target) {
    const std::vector<VkQueueFamilyProperties> families =
        detail::enumerate<VkQueueFamilyProperties>(
            "vkGetPhysicalDeviceQueueFamilyProperties",
            [&](std::uint32_t *count, VkQueueFamilyProperties *items) {
                vk.vkGetPhysicalDeviceQueueFamilyProperties(gpu, count, items);
                return VK_SUCCESS;
            });
    for (std::uint32_t family = 0; family < families.size(); ++family) {
        if ((families[family].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) {
            continue;
        }
        VkBool32 presents = VK_FALSE;
        detail::check_call(vk.vkGetPhysicalDeviceSurfaceSupportKHR(gpu, family, target, &presents),
                           "vkGetPhysicalDeviceSurfaceSupportKHR");
        if (presents == VK_TRUE) {
            return family;
        }
    }
    return std::nullopt;
}

bool has_extension(const std::vector<VkExtensionProperties> &available, std::string_view name) {
    return std::ranges::any_of(
        available, [&](const VkExtensionProperties &each) { return name == each.extensionName; });
}

// Lo que una GPU tiene que cumplir, en orden: primero la versión, porque sin 1.3 ni
// siquiera se pueden preguntar sus features de 1.3.
struct examined {
    gpu_candidate m_candidate;
    std::uint32_t m_queue_family = 0;
};

examined examine(const instance_functions &vk, VkPhysicalDevice gpu,
                 const VkPhysicalDeviceProperties &properties, VkSurfaceKHR target) {
    const auto lacking = [&](std::string_view missing) {
        return examined{.m_candidate = {.m_type = properties.deviceType, .m_missing = missing}};
    };
    if (properties.apiVersion < required_api_version) {
        return lacking(required_api_name);
    }

    const std::vector<VkExtensionProperties> extensions = detail::enumerate<VkExtensionProperties>(
        "vkEnumerateDeviceExtensionProperties",
        [&](std::uint32_t *count, VkExtensionProperties *items) {
            return vk.vkEnumerateDeviceExtensionProperties(gpu, nullptr, count, items);
        });
    for (const char *name : required_device_extensions) {
        if (!has_extension(extensions, name)) {
            return lacking(name);
        }
    }

    const std::optional<std::uint32_t> family = graphics_and_present_family(vk, gpu, target);
    if (!family) {
        return lacking(graphics_queue_requirement);
    }

    // Lo que necesita el swapchain de la superficie. Se mira aquí, y no al crearlo, para que
    // el juego lo diga al arrancar, como los demás requisitos.
    const std::vector<VkSurfaceFormatKHR> formats = detail::enumerate<VkSurfaceFormatKHR>(
        "vkGetPhysicalDeviceSurfaceFormatsKHR",
        [&](std::uint32_t *count, VkSurfaceFormatKHR *items) {
            return vk.vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, target, count, items);
        });
    if (!choose_surface_format(formats)) {
        return lacking(srgb_format_requirement);
    }
    // NOLINTNEXTLINE(bugprone-invalid-enum-default-initialization): la llamada la llena entera.
    VkSurfaceCapabilitiesKHR capabilities{};
    detail::check_call(vk.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, target, &capabilities),
                       "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    if ((capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) == 0) {
        return lacking(opaque_requirement);
    }

    device_features supported;
    vk.vkGetPhysicalDeviceFeatures2(gpu, &supported.chain());
    if (const std::string_view missing = supported.first_missing(); !missing.empty()) {
        return lacking(missing);
    }

    return examined{.m_candidate = {.m_type = properties.deviceType, .m_missing = {}},
                    .m_queue_family = *family};
}

} // namespace

std::optional<init_error> device::build(const instance &owner, const surface &target) {
    const instance_functions &vk = owner.functions();

    const std::vector<VkPhysicalDevice> gpus = detail::enumerate<VkPhysicalDevice>(
        "vkEnumeratePhysicalDevices", [&](std::uint32_t *count, VkPhysicalDevice *items) {
            return vk.vkEnumeratePhysicalDevices(owner.handle(), count, items);
        });

    std::vector<VkPhysicalDeviceProperties> properties(gpus.size());
    std::vector<examined> results;
    std::vector<gpu_candidate> candidates;
    for (std::size_t i = 0; i < gpus.size(); ++i) {
        vk.vkGetPhysicalDeviceProperties(gpus[i], &properties[i]);
        results.push_back(examine(vk, gpus[i], properties[i], target.handle()));
        candidates.push_back(results.back().m_candidate);
    }

    const std::optional<std::size_t> chosen = choose_gpu(candidates);
    if (!chosen) {
        if (candidates.empty()) {
            return init_error{.m_failure = init_failure::no_suitable_gpu, .m_name = no_gpu};
        }
        // Todas tienen algo que falta: se nombra lo de la primera, que en la mayoría de las
        // máquinas es la única.
        init_error error{.m_failure = init_failure::no_suitable_gpu,
                         .m_name = candidates.front().m_missing};
        std::ranges::copy(properties.front().deviceName, error.m_gpu.begin());
        return error;
    }
    m_physical = gpus[*chosen];
    m_queue_family = results[*chosen].m_queue_family;
    std::ranges::copy(properties[*chosen].deviceName, m_name.begin());
    if (const std::optional<std::size_t> skipped = passed_over_gpu(candidates, *chosen)) {
        m_skipped = skipped_gpu{.m_name = {}, .m_missing = candidates[*skipped].m_missing};
        std::ranges::copy(properties[*skipped].deviceName, m_skipped->m_name.begin());
    }

    // Una cola, con la prioridad máxima: con una sola, la prioridad no compite con nada.
    const float priority = 1.0F;
    VkDeviceQueueCreateInfo queue_info{};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = m_queue_family;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &priority;

    // Solo las features exigidas: la misma tabla con la que se verificaron.
    device_features enabled;
    enabled.enable_required();

    VkDeviceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    info.pNext = &enabled.chain();
    info.queueCreateInfoCount = 1;
    info.pQueueCreateInfos = &queue_info;
    info.enabledExtensionCount = static_cast<std::uint32_t>(required_device_extensions.size());
    info.ppEnabledExtensionNames = required_device_extensions.data();
    if (const VkResult result = vk.vkCreateDevice(m_physical, &info, nullptr, &m_device);
        result != VK_SUCCESS) {
        m_device = VK_NULL_HANDLE;
        return init_error{
            .m_failure = init_failure::call_failed, .m_name = "vkCreateDevice", .m_result = result};
    }

    m_functions = detail::load_device(vk.vkGetDeviceProcAddr, m_device);
    m_functions.vkGetDeviceQueue(m_device, m_queue_family, 0, &m_queue);
    return std::nullopt;
}

device::device(device &&other) noexcept
    : m_physical(std::exchange(other.m_physical, VK_NULL_HANDLE)),
      m_device(std::exchange(other.m_device, VK_NULL_HANDLE)),
      m_queue(std::exchange(other.m_queue, VK_NULL_HANDLE)), m_queue_family(other.m_queue_family),
      m_functions(other.m_functions), m_name(other.m_name), m_skipped(other.m_skipped) {}

device::~device() {
    if (m_device != VK_NULL_HANDLE) {
        m_functions.vkDestroyDevice(m_device, nullptr);
    }
}

std::string_view describe_skipped(const device &gpu, std::span<char> buffer) {
    if (!gpu.skipped()) {
        return {};
    }
    const device::skipped_gpu &skipped = *gpu.skipped();
    const auto written = std::format_to_n(
        buffer.data(), static_cast<std::ptrdiff_t>(buffer.size()),
        "El juego no puede usar la GPU \"{}\": le falta {}, que el juego usa para {}. Va a "
        "correr en \"{}\", que puede ser mucho más lenta.",
        skipped.m_name.data(), skipped.m_missing, purpose_of(skipped.m_missing), gpu.name());
    return {buffer.data(), written.out};
}

} // namespace mz::gpu
