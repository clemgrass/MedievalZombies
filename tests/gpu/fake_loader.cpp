#include "fake_loader.h"

#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <cstring>
#include <string_view>
#include <vector>

namespace fake_loader {
namespace {

struct replacement {
    std::string_view m_name;
    PFN_vkVoidFunction m_with;
};

struct state {
    PFN_vkGetInstanceProcAddr m_real = nullptr;
    // La última instancia por la que preguntó gpu: a ella se le piden las funciones reales.
    VkInstance m_instance = VK_NULL_HANDLE;
    std::vector<replacement> m_replacements;
};

state &current() {
    static state shared;
    return shared;
}

const replacement *find(std::string_view name) {
    const std::vector<replacement> &all = current().m_replacements;
    const auto found =
        std::ranges::find_if(all, [&](const replacement &each) { return each.m_name == name; });
    return found == all.end() ? nullptr : &*found;
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL fake_get_device_proc_addr(VkDevice device,
                                                                   const char *name) {
    if (const replacement *replaced = find(name)) {
        return replaced->m_with;
    }
    const auto real_get = real<PFN_vkGetDeviceProcAddr>("vkGetDeviceProcAddr");
    return real_get(device, name);
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL fake_get_instance_proc_addr(VkInstance instance,
                                                                     const char *name) {
    if (instance != VK_NULL_HANDLE) {
        current().m_instance = instance;
    }
    if (const replacement *replaced = find(name)) {
        return replaced->m_with;
    }
    if (std::strcmp(name, "vkGetDeviceProcAddr") == 0) {
        return reinterpret_cast<PFN_vkVoidFunction>(&fake_get_device_proc_addr);
    }
    return current().m_real(instance, name);
}

} // namespace

void reset() { current() = state{}; }

void wrap(PFN_vkGetInstanceProcAddr real) { current().m_real = real; }

PFN_vkGetInstanceProcAddr entry() { return &fake_get_instance_proc_addr; }

void replace_raw(std::string_view name, PFN_vkVoidFunction with) {
    current().m_replacements.push_back({.m_name = name, .m_with = with});
}

PFN_vkVoidFunction real_raw(const char *name) {
    return current().m_real(current().m_instance, name);
}

} // namespace fake_loader
