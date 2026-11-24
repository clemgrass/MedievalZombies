#include "mz/gpu/requirements.h"

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <optional>
#include <span>
#include <string_view>

namespace mz::gpu {

device_features::device_features() {
    m_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    m_features.pNext = &m_vulkan12;
    m_vulkan12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    m_vulkan12.pNext = &m_vulkan13;
    m_vulkan13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
}

std::string_view device_features::first_missing() const {
    for (const auto &required : required_vulkan12_features) {
        if (m_vulkan12.*required.m_member != VK_TRUE) {
            return required.m_name;
        }
    }
    for (const auto &required : required_vulkan13_features) {
        if (m_vulkan13.*required.m_member != VK_TRUE) {
            return required.m_name;
        }
    }
    return {};
}

void device_features::enable_required() {
    for (const auto &required : required_vulkan12_features) {
        m_vulkan12.*required.m_member = VK_TRUE;
    }
    for (const auto &required : required_vulkan13_features) {
        m_vulkan13.*required.m_member = VK_TRUE;
    }
}

namespace {

// La escala de la elección: más es mejor. Una discreta tiene su propia memoria y más
// unidades de cómputo; una integrada comparte la RAM con la CPU; una virtual es la GPU de
// una máquina virtual; la CPU (lavapipe) dibuja por software, y sirve solo si no hay otra.
int rank(VkPhysicalDeviceType type) {
    switch (type) {
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        return 3;
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        return 2;
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
        return 1;
    default:
        return 0;
    }
}

} // namespace

std::string_view purpose_of(std::string_view requirement) {
    for (const auto &required : required_vulkan12_features) {
        if (required.m_name == requirement) {
            return required.m_why;
        }
    }
    for (const auto &required : required_vulkan13_features) {
        if (required.m_name == requirement) {
            return required.m_why;
        }
    }
    if (requirement == required_api_name) {
        return "dibujar con dynamic rendering y las barreras de synchronization2";
    }
    if (requirement == VK_KHR_SWAPCHAIN_EXTENSION_NAME) {
        return "mostrar las imágenes en la ventana";
    }
    if (requirement == graphics_queue_requirement) {
        return "mandarle a la GPU el trabajo de dibujar y mostrar cada imagen";
    }
    if (requirement == srgb_format_requirement) {
        return "mostrar los colores como se ven en la pantalla, sin aclararlos ni oscurecerlos";
    }
    if (requirement == opaque_requirement) {
        return "que la ventana no se vea transparente: el juego guarda otra cosa en el alfa";
    }
    return {};
}

std::optional<std::size_t> choose_gpu(std::span<const gpu_candidate> candidates) {
    std::optional<std::size_t> best;
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        if (!candidates[i].m_missing.empty()) {
            continue;
        }
        // Estrictamente mejor: a igual tipo se queda la que llegó antes.
        if (!best || rank(candidates[i].m_type) > rank(candidates[*best].m_type)) {
            best = i;
        }
    }
    return best;
}

std::optional<std::size_t> passed_over_gpu(std::span<const gpu_candidate> candidates,
                                           std::size_t chosen) {
    std::optional<std::size_t> best;
    int best_rank = rank(candidates[chosen].m_type);
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        // La elegida cumple todo, así que nunca entra: solo las que les faltó algo.
        if (!candidates[i].m_missing.empty() && rank(candidates[i].m_type) > best_rank) {
            best = i;
            best_rank = rank(candidates[i].m_type);
        }
    }
    return best;
}

} // namespace mz::gpu
