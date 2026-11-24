#include "mz/gpu/surface.h"

#include "mz/core/assert.h"
#include "mz/gpu/instance.h"

#include <vulkan/vulkan_core.h>

#include <utility>

namespace mz::gpu {

surface::surface(const instance &owner, VkSurfaceKHR handle)
    : m_instance(owner.handle()), m_destroy(owner.functions().vkDestroySurfaceKHR),
      m_surface(handle) {
    MZ_ASSERT(handle != VK_NULL_HANDLE, "La ventana no pudo crear la superficie");
}

surface::surface(surface &&other) noexcept
    : m_instance(other.m_instance), m_destroy(other.m_destroy),
      m_surface(std::exchange(other.m_surface, VK_NULL_HANDLE)) {}

surface::~surface() {
    if (m_surface != VK_NULL_HANDLE) {
        m_destroy(m_instance, m_surface, nullptr);
    }
}

} // namespace mz::gpu
