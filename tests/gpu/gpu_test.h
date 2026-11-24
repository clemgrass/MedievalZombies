#pragma once

// Los helpers de los tests de gpu. Corren con lavapipe y la ventana offscreen
// (FakeLoaderTest), y piden el loader a platform, como el juego. Si lo que piden no existe,
// el test no puede seguir: give_up() deja el motivo y aborta.

#include "fake_loader.h"

#include "mz/gpu/device.h"
#include "mz/gpu/instance.h"
#include "mz/gpu/surface.h"
#include "mz/platform/context.h"
#include "mz/platform/vulkan.h"
#include "mz/platform/window.h"
#include "mz/test/offscreen.h"

#include <vulkan/vulkan_core.h>

#include <optional>
#include <utility>

inline mz::platform::context started() {
    std::optional<mz::platform::context> platform = mz::platform::context::create();
    if (!platform) {
        give_up(mz::platform::last_error());
    }
    return std::move(*platform);
}

inline mz::platform::vulkan_library loaded(mz::platform::context &platform) {
    std::optional<mz::platform::vulkan_library> library =
        mz::platform::vulkan_library::load(platform);
    if (!library) {
        give_up(mz::platform::last_error());
    }
    return std::move(*library);
}

inline mz::platform::window opened(mz::platform::context &platform) {
    std::optional<mz::platform::window> created =
        mz::platform::window::create(platform, "test", 640, 360);
    if (!created) {
        give_up(mz::platform::last_error());
    }
    return std::move(*created);
}

// La instancia, con las funciones pedidas a través del loader falso: los reemplazos que el
// test pidió antes valen para ella y para lo que se cree con ella.
inline mz::gpu::instance created(const mz::platform::vulkan_library &library) {
    fake_loader::wrap(library.get_instance_proc_addr());
    auto made = mz::gpu::instance::create(fake_loader::entry(), library.instance_extensions());
    if (!made) {
        give_up(made.error().m_name);
    }
    return std::move(made).value();
}

inline VkSurfaceKHR surface_of(const mz::platform::window &target, const mz::gpu::instance &owner) {
    const VkSurfaceKHR handle = target.create_vulkan_surface(owner.handle());
    if (handle == VK_NULL_HANDLE) {
        give_up(mz::platform::last_error());
    }
    return handle;
}

// Lo que un test necesita vivo para crear el dispositivo. Los miembros se destruyen en el
// orden inverso al que aparecen: primero la superficie, al final el context.
struct vulkan_setup {
    mz::platform::context m_platform = started();
    mz::platform::vulkan_library m_library = loaded(m_platform);
    mz::platform::window m_window = opened(m_platform);
    mz::gpu::instance m_instance = created(m_library);
    mz::gpu::surface m_surface{m_instance, surface_of(m_window, m_instance)};
};

inline mz::gpu::device created(const vulkan_setup &setup) {
    auto made = mz::gpu::device::create(setup.m_instance, setup.m_surface);
    if (!made) {
        give_up(made.error().m_name);
    }
    return std::move(made).value();
}
