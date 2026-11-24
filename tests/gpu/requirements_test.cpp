#include "mz/gpu/requirements.h"

#include <gtest/gtest.h>
#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <array>
#include <optional>
#include <string_view>

namespace {

using mz::gpu::choose_gpu;
using mz::gpu::device_features;
using mz::gpu::gpu_candidate;
using mz::gpu::passed_over_gpu;
using mz::gpu::purpose_of;
using mz::gpu::required_vulkan12_features;
using mz::gpu::required_vulkan13_features;

TEST(DeviceFeaturesTest, ChainsTheVulkan12AndVulkan13Structures) {
    device_features features;

    EXPECT_EQ(features.chain().sType, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2);
    EXPECT_EQ(features.vulkan12().sType, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES);
    EXPECT_EQ(features.vulkan13().sType, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES);
    EXPECT_EQ(features.chain().pNext, &features.vulkan12());
    EXPECT_EQ(features.vulkan12().pNext, &features.vulkan13());
    EXPECT_EQ(features.vulkan13().pNext, nullptr);
}

TEST(DeviceFeaturesTest, StartsWithEverythingOff) {
    const device_features features;

    EXPECT_EQ(features.first_missing(), "descriptorIndexing");
}

TEST(DeviceFeaturesTest, EnablingTheRequiredLeavesNothingMissing) {
    device_features features;
    features.enable_required();

    EXPECT_EQ(features.first_missing(), "");
}

// Solo las exigidas: una feature de más puede costar rendimiento en algunos drivers, y la
// validación avisa si se usa una que no se pidió.
TEST(DeviceFeaturesTest, EnablesOnlyTheRequired) {
    device_features features;
    features.enable_required();

    EXPECT_EQ(features.vulkan12().bufferDeviceAddress, VK_FALSE);
    EXPECT_EQ(features.vulkan13().maintenance4, VK_FALSE);
    EXPECT_EQ(features.chain().features.geometryShader, VK_FALSE);
}

gpu_candidate fits(VkPhysicalDeviceType type) { return {.m_type = type, .m_missing = {}}; }

gpu_candidate lacks(VkPhysicalDeviceType type, std::string_view missing) {
    return {.m_type = type, .m_missing = missing};
}

TEST(ChooseGpuTest, WithoutGpusThereIsNoChoice) { EXPECT_EQ(choose_gpu({}), std::nullopt); }

TEST(ChooseGpuTest, OnlyAGpuThatMeetsEverythingCanBeChosen) {
    const std::array candidates = {lacks(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, "synchronization2"),
                                   lacks(VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, "Vulkan 1.3")};

    EXPECT_EQ(choose_gpu(candidates), std::nullopt);
}

// Discreta, integrada, virtual, CPU: en cualquier orden de la lista gana la mejor.
TEST(ChooseGpuTest, PrefersTheBestType) {
    std::array candidates = {
        fits(VK_PHYSICAL_DEVICE_TYPE_CPU), fits(VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU),
        fits(VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU), fits(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)};
    EXPECT_EQ(choose_gpu(candidates), 3U);

    std::ranges::reverse(candidates);
    EXPECT_EQ(choose_gpu(candidates), 0U);

    // Sin la discreta, la integrada; sin esa, la virtual.
    const std::array without_discrete = {fits(VK_PHYSICAL_DEVICE_TYPE_CPU),
                                         fits(VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU),
                                         fits(VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)};
    EXPECT_EQ(choose_gpu(without_discrete), 1U);
    const std::array without_integrated = {fits(VK_PHYSICAL_DEVICE_TYPE_CPU),
                                           fits(VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)};
    EXPECT_EQ(choose_gpu(without_integrated), 1U);
}

// La máquina de desarrollo: la UHD 620 y lavapipe cumplen las dos.
TEST(ChooseGpuTest, TheCpuOnlyWinsWhenItIsTheOnlyOne) {
    const std::array both = {fits(VK_PHYSICAL_DEVICE_TYPE_CPU),
                             fits(VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)};
    EXPECT_EQ(choose_gpu(both), 1U);

    const std::array alone = {fits(VK_PHYSICAL_DEVICE_TYPE_CPU)};
    EXPECT_EQ(choose_gpu(alone), 0U);
}

// Una discreta a la que le falta algo pierde contra una integrada que cumple.
TEST(ChooseGpuTest, ABetterTypeThatMissesSomethingLoses) {
    const std::array candidates = {
        lacks(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, "shaderDemoteToHelperInvocation"),
        fits(VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)};

    EXPECT_EQ(choose_gpu(candidates), 1U);
}

// A igual tipo decide el orden del loader: en Linux, la capa device-select de Mesa pone
// primero la GPU por defecto del sistema.
TEST(ChooseGpuTest, OnATieTheLoaderOrderDecides) {
    const std::array candidates = {fits(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU),
                                   fits(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)};

    EXPECT_EQ(choose_gpu(candidates), 0U);
}

// Un tipo que no está en la escala (OTHER) cuenta como la CPU: el último.
TEST(ChooseGpuTest, AnUnknownTypeRanksLikeTheCpu) {
    const std::array candidates = {fits(VK_PHYSICAL_DEVICE_TYPE_OTHER),
                                   fits(VK_PHYSICAL_DEVICE_TYPE_CPU)};
    EXPECT_EQ(choose_gpu(candidates), 0U);

    const std::array with_virtual = {fits(VK_PHYSICAL_DEVICE_TYPE_OTHER),
                                     fits(VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)};
    EXPECT_EQ(choose_gpu(with_virtual), 1U);
}

// La GPU que se dejó de lado: una de mejor tipo que la elegida, a la que le faltó algo.
TEST(PassedOverGpuTest, IsABetterTypeThatMissedSomething) {
    const std::array candidates = {
        lacks(VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, "synchronization2"),
        fits(VK_PHYSICAL_DEVICE_TYPE_CPU)};

    EXPECT_EQ(passed_over_gpu(candidates, 1), 0U);
}

TEST(PassedOverGpuTest, IsNothingWhenTheBestTypeWasChosen) {
    const std::array candidates = {fits(VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU),
                                   lacks(VK_PHYSICAL_DEVICE_TYPE_CPU, "Vulkan 1.3")};

    EXPECT_EQ(passed_over_gpu(candidates, 0), std::nullopt);
}

// Una del mismo tipo que no cumplía no es "mejor": no hay nada que avisar.
TEST(PassedOverGpuTest, IgnoresWorseOrEqualTypes) {
    const std::array candidates = {lacks(VK_PHYSICAL_DEVICE_TYPE_CPU, "VK_KHR_swapchain"),
                                   lacks(VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, "Vulkan 1.3"),
                                   fits(VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)};

    EXPECT_EQ(passed_over_gpu(candidates, 2), std::nullopt);
}

// Con varias, la de mejor tipo: es la que el jugador esperaba usar.
TEST(PassedOverGpuTest, PrefersTheBestOfThePassedOver) {
    const std::array candidates = {
        lacks(VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, "synchronization2"),
        lacks(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, "dynamicRendering"),
        lacks(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, "Vulkan 1.3"),
        fits(VK_PHYSICAL_DEVICE_TYPE_CPU)};

    EXPECT_EQ(passed_over_gpu(candidates, 3), 1U);
}

// Cada requisito explica para qué lo usa el juego: el mensaje de error dice qué falta y
// por qué hace falta.
TEST(PurposeOfTest, ExplainsEveryRequirement) {
    for (const auto &required : required_vulkan12_features) {
        EXPECT_FALSE(purpose_of(required.m_name).empty()) << required.m_name;
    }
    for (const auto &required : required_vulkan13_features) {
        EXPECT_FALSE(purpose_of(required.m_name).empty()) << required.m_name;
    }
    for (const char *extension : mz::gpu::required_device_extensions) {
        EXPECT_FALSE(purpose_of(extension).empty()) << extension;
    }
    EXPECT_FALSE(purpose_of(mz::gpu::required_api_name).empty());
    EXPECT_FALSE(purpose_of(mz::gpu::graphics_queue_requirement).empty());
}

TEST(PurposeOfTest, IsEmptyForSomethingThatIsNotARequirement) {
    EXPECT_EQ(purpose_of("geometryShader"), "");
}

// Cada feature exigida, apagada sola, es la que nombra el mensaje.
TEST(DeviceFeaturesTest, NamesEachMissingFeature) {
    for (const auto &required : required_vulkan12_features) {
        device_features features;
        features.enable_required();
        features.vulkan12().*required.m_member = VK_FALSE;
        EXPECT_EQ(features.first_missing(), required.m_name);
    }
    for (const auto &required : required_vulkan13_features) {
        device_features features;
        features.enable_required();
        features.vulkan13().*required.m_member = VK_FALSE;
        EXPECT_EQ(features.first_missing(), required.m_name);
    }
}

} // namespace
