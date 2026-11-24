// Las elecciones del swapchain, sin Vulkan: qué formato, cuántas imágenes y de qué tamaño,
// a partir de lo que la superficie dice que soporta.

#include "mz/gpu/swapchain.h"

#include <gtest/gtest.h>
#include <vulkan/vulkan_core.h>

#include <array>
#include <cstdint>
#include <limits>
#include <optional>

namespace {

using mz::gpu::choose_extent;
using mz::gpu::choose_image_count;
using mz::gpu::choose_surface_format;

constexpr VkSurfaceFormatKHR bgra_srgb{.format = VK_FORMAT_B8G8R8A8_SRGB,
                                       .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
constexpr VkSurfaceFormatKHR rgba_srgb{.format = VK_FORMAT_R8G8B8A8_SRGB,
                                       .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
constexpr VkSurfaceFormatKHR bgra_unorm{.format = VK_FORMAT_B8G8R8A8_UNORM,
                                        .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
// El mismo formato, pero en otro espacio de color: la pantalla lo interpretaría distinto.
constexpr VkSurfaceFormatKHR bgra_srgb_hdr{.format = VK_FORMAT_B8G8R8A8_SRGB,
                                           .colorSpace = VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT};

// currentExtent sin tamaño: el de Wayland, donde lo decide quien crea el swapchain.
constexpr std::uint32_t no_size = std::numeric_limits<std::uint32_t>::max();

bool same(const std::optional<VkSurfaceFormatKHR> &chosen, const VkSurfaceFormatKHR &expected) {
    return chosen && chosen->format == expected.format && chosen->colorSpace == expected.colorSpace;
}

bool same(VkExtent2D a, VkExtent2D b) { return a.width == b.width && a.height == b.height; }

constexpr VkExtent2D extent(std::uint32_t width, std::uint32_t height) {
    return {.width = width, .height = height};
}

struct surface_limits {
    std::uint32_t m_min_images = 2;
    std::uint32_t m_max_images = 8;
    VkExtent2D m_current = extent(no_size, no_size);
    VkExtent2D m_min_extent = extent(1, 1);
    VkExtent2D m_max_extent = extent(4096, 4096);
};

VkSurfaceCapabilitiesKHR capabilities(const surface_limits &limits) {
    return {.minImageCount = limits.m_min_images,
            .maxImageCount = limits.m_max_images,
            .currentExtent = limits.m_current,
            .minImageExtent = limits.m_min_extent,
            .maxImageExtent = limits.m_max_extent,
            .maxImageArrayLayers = 1,
            .supportedTransforms = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
            .currentTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
            .supportedCompositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .supportedUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT};
}

TEST(SurfaceFormatTest, PrefersBgraSrgb) {
    const std::array formats{bgra_unorm, rgba_srgb, bgra_srgb};
    EXPECT_TRUE(same(choose_surface_format(formats), bgra_srgb));
}

TEST(SurfaceFormatTest, TakesRgbaSrgbWithoutBgra) {
    const std::array formats{bgra_unorm, rgba_srgb};
    EXPECT_TRUE(same(choose_surface_format(formats), rgba_srgb));
}

TEST(SurfaceFormatTest, IgnoresAnSrgbFormatInAnotherColorSpace) {
    const std::array formats{bgra_srgb_hdr, rgba_srgb};
    EXPECT_TRUE(same(choose_surface_format(formats), rgba_srgb));
}

TEST(SurfaceFormatTest, WithoutSrgbThereIsNone) {
    const std::array formats{bgra_unorm, bgra_srgb_hdr};
    EXPECT_FALSE(choose_surface_format(formats).has_value());
}

TEST(ImageCountTest, AtLeastThree) {
    EXPECT_EQ(choose_image_count(capabilities({.m_min_images = 2})), 3U);
}

TEST(ImageCountTest, TheMinimumIfItIsMore) {
    EXPECT_EQ(choose_image_count(capabilities({.m_min_images = 4})), 4U);
}

TEST(ImageCountTest, NeverMoreThanTheMaximum) {
    EXPECT_EQ(choose_image_count(capabilities({.m_min_images = 1, .m_max_images = 2})), 2U);
}

// maxImageCount 0 quiere decir "sin máximo".
TEST(ImageCountTest, ZeroMaximumMeansNoLimit) {
    EXPECT_EQ(choose_image_count(capabilities({.m_min_images = 2, .m_max_images = 0})), 3U);
}

TEST(ExtentTest, TheSurfaceSizeWinsWhenItHasOne) {
    const VkSurfaceCapabilitiesKHR caps = capabilities({.m_current = extent(1920, 1080)});
    EXPECT_TRUE(same(choose_extent(caps, extent(1280, 720)), extent(1920, 1080)));
}

TEST(ExtentTest, WithoutASurfaceSizeTheWindowDecides) {
    EXPECT_TRUE(same(choose_extent(capabilities({}), extent(1280, 720)), extent(1280, 720)));
}

TEST(ExtentTest, TheWindowSizeIsClampedToWhatTheSurfaceAllows) {
    const VkSurfaceCapabilitiesKHR caps =
        capabilities({.m_min_extent = extent(64, 64), .m_max_extent = extent(2048, 1024)});
    EXPECT_TRUE(same(choose_extent(caps, extent(4000, 10)), extent(2048, 64)));
}

} // namespace
