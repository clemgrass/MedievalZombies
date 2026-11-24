#include "offscreen_test.h"

#include "mz/platform/context.h"
#include "mz/platform/vulkan.h"
#include "mz/platform/window.h"

#include <SDL3/SDL_hints.h>
#include <gtest/gtest.h>
#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

namespace {

using mz::platform::context;
using mz::platform::vulkan_library;
using mz::platform::window;

class VulkanLibraryTest : public OffscreenTest {};
class VulkanLibraryDeathTest : public OffscreenTest {};
class VulkanSurfaceTest : public OffscreenTest {};

vulkan_library loaded(context &platform) {
    std::optional<vulkan_library> library = vulkan_library::load(platform);
    if (!library) {
        give_up(mz::platform::last_error());
    }
    return std::move(*library);
}

// Una instancia mínima, armada a mano con el puntero inicial: las pruebas de platform no
// usan engine/gpu, que está una capa más arriba.
class bare_instance {
public:
    bare_instance(const vulkan_library &library, std::span<const char *const> extensions)
        : m_get(library.get_instance_proc_addr()) {
        const auto create =
            reinterpret_cast<PFN_vkCreateInstance>(m_get(VK_NULL_HANDLE, "vkCreateInstance"));
        VkApplicationInfo application{};
        application.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        application.apiVersion = VK_API_VERSION_1_3;
        VkInstanceCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        info.pApplicationInfo = &application;
        info.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
        info.ppEnabledExtensionNames = extensions.data();
        if (create(&info, nullptr, &m_instance) != VK_SUCCESS) {
            give_up("vkCreateInstance falló.");
        }
    }
    bare_instance(const bare_instance &) = delete;
    bare_instance &operator=(const bare_instance &) = delete;
    bare_instance(bare_instance &&) = delete;
    bare_instance &operator=(bare_instance &&) = delete;
    ~bare_instance() {
        const auto destroy =
            reinterpret_cast<PFN_vkDestroyInstance>(m_get(m_instance, "vkDestroyInstance"));
        destroy(m_instance, nullptr);
    }

    [[nodiscard]] VkInstance handle() const { return m_instance; }

    void destroy_surface(VkSurfaceKHR surface) const {
        const auto destroy =
            reinterpret_cast<PFN_vkDestroySurfaceKHR>(m_get(m_instance, "vkDestroySurfaceKHR"));
        destroy(m_instance, surface, nullptr);
    }

private:
    PFN_vkGetInstanceProcAddr m_get;
    VkInstance m_instance = VK_NULL_HANDLE;
};

// Sin ventana: el modo de prueba de humo pregunta por Vulkan sin abrir una.
TEST_F(VulkanLibraryTest, LoadsWithoutAWindowAndGivesTheFirstPointer) {
    context platform = started();
    const vulkan_library library = loaded(platform);

    const PFN_vkGetInstanceProcAddr get = library.get_instance_proc_addr();
    ASSERT_NE(get, nullptr);
    // Con ese puntero sale todo lo demás, empezando por las funciones globales.
    EXPECT_NE(get(VK_NULL_HANDLE, "vkCreateInstance"), nullptr);
    EXPECT_NE(get(VK_NULL_HANDLE, "vkEnumerateInstanceVersion"), nullptr);
}

// Las extensiones que hacen falta para crear una superficie en el sistema de ventanas
// actual. Con el driver offscreen, la superficie es VK_EXT_headless_surface.
TEST_F(VulkanLibraryTest, ListsTheSurfaceExtensionsOfTheWindowSystem) {
    context platform = started();
    const vulkan_library library = loaded(platform);

    const std::span<const char *const> extensions = library.instance_extensions();
    const auto has = [&](std::string_view name) {
        return std::ranges::any_of(
            extensions, [&](const char *each) { return std::string_view{each} == name; });
    };
    EXPECT_TRUE(has(VK_KHR_SURFACE_EXTENSION_NAME));
    EXPECT_TRUE(has(VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME));
}

TEST_F(VulkanLibraryTest, MovingLeavesTheSourceEmpty) {
    context platform = started();
    vulkan_library first = loaded(platform);
    const vulkan_library second = std::move(first);

    EXPECT_NE(second.get_instance_proc_addr(), nullptr);
    // first queda vacío: su destructor no descarga el loader una segunda vez.
}

// Un loader que no existe: el jugador sin driver de Vulkan.
TEST_F(VulkanLibraryTest, IsEmptyWithoutALoader) {
    context platform = started();
    ASSERT_TRUE(SDL_SetHintWithPriority(SDL_HINT_VULKAN_LIBRARY, "libvulkan-que-no-existe.so",
                                        SDL_HINT_OVERRIDE));

    EXPECT_FALSE(vulkan_library::load(platform).has_value());
    EXPECT_FALSE(mz::platform::last_error().empty());
}

TEST_F(VulkanLibraryDeathTest, DestroyingTheContextFirstAborts) {
    EXPECT_DEATH(
        {
            std::optional<context> platform = context::create();
            if (!platform) {
                give_up(mz::platform::last_error());
            }
            const vulkan_library library = loaded(*platform);
            platform.reset();
        },
        "loader de Vulkan");
}

TEST_F(VulkanSurfaceTest, TheWindowCreatesASurfaceOnTheInstance) {
    context platform = started();
    const vulkan_library library = loaded(platform);
    const window created = opened(platform);
    const bare_instance instance{library, library.instance_extensions()};

    const VkSurfaceKHR surface = created.create_vulkan_surface(instance.handle());
    ASSERT_NE(surface, VK_NULL_HANDLE);
    instance.destroy_surface(surface);
}

// Una instancia sin las extensiones de superficie no puede tener una.
TEST_F(VulkanSurfaceTest, IsNullWhenTheInstanceLacksTheExtensions) {
    context platform = started();
    const vulkan_library library = loaded(platform);
    const window created = opened(platform);
    const bare_instance instance{library, {}};

    EXPECT_EQ(created.create_vulkan_surface(instance.handle()), VK_NULL_HANDLE);
    EXPECT_FALSE(mz::platform::last_error().empty());
}

} // namespace
