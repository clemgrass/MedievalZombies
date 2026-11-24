#include "fake_loader.h"
#include "gpu_test.h"

#include "mz/gpu/device.h"
#include "mz/gpu/init_error.h"
#include "mz/gpu/instance.h"
#include "mz/gpu/requirements.h"
#include "mz/gpu/surface.h"

#include <gtest/gtest.h>
#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace {

using mz::gpu::device;
using mz::gpu::init_failure;
using mz::gpu::surface;
using mz::gpu::validation_warnings;

class DeviceTest : public FakeLoaderTest {};
class DeviceDeathTest : public FakeLoaderTest {};
class SurfaceTest : public FakeLoaderTest {};
class SurfaceDeathTest : public FakeLoaderTest {};

// Los reemplazos de las funciones del loader: cada uno le quita a lavapipe un requisito.
// Los que cambian un resultado llaman primero a la función real.

VKAPI_ATTR VkResult VKAPI_CALL no_gpus(VkInstance /*instance*/, std::uint32_t *count,
                                       VkPhysicalDevice * /*gpus*/) {
    *count = 0;
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL vulkan_1_2_gpu(VkPhysicalDevice gpu,
                                          VkPhysicalDeviceProperties *properties) {
    fake_loader::real<PFN_vkGetPhysicalDeviceProperties>("vkGetPhysicalDeviceProperties")(
        gpu, properties);
    properties->apiVersion = VK_API_VERSION_1_2;
}

VKAPI_ATTR VkResult VKAPI_CALL no_device_extensions(VkPhysicalDevice /*gpu*/,
                                                    const char * /*layer*/, std::uint32_t *count,
                                                    VkExtensionProperties * /*extensions*/) {
    *count = 0;
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL no_graphics_queue(VkPhysicalDevice gpu, std::uint32_t *count,
                                             VkQueueFamilyProperties *families) {
    fake_loader::real<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(
        "vkGetPhysicalDeviceQueueFamilyProperties")(gpu, count, families);
    if (families != nullptr) {
        for (VkQueueFamilyProperties &family : std::span{families, *count}) {
            family.queueFlags &= ~static_cast<VkQueueFlags>(VK_QUEUE_GRAPHICS_BIT);
        }
    }
}

VKAPI_ATTR VkResult VKAPI_CALL no_present(VkPhysicalDevice /*gpu*/, std::uint32_t /*family*/,
                                          VkSurfaceKHR /*surface*/, VkBool32 *supported) {
    *supported = VK_FALSE;
    return VK_SUCCESS;
}

// La cadena que arma device_features: Features2 -> Vulkan12Features -> Vulkan13Features.
VKAPI_ATTR void VKAPI_CALL no_synchronization2(VkPhysicalDevice gpu,
                                               VkPhysicalDeviceFeatures2 *features) {
    fake_loader::real<PFN_vkGetPhysicalDeviceFeatures2>("vkGetPhysicalDeviceFeatures2")(gpu,
                                                                                        features);
    auto *vulkan12 = static_cast<VkPhysicalDeviceVulkan12Features *>(features->pNext);
    auto *vulkan13 = static_cast<VkPhysicalDeviceVulkan13Features *>(vulkan12->pNext);
    vulkan13->synchronization2 = VK_FALSE;
}

VKAPI_ATTR VkResult VKAPI_CALL failing_create_device(VkPhysicalDevice /*gpu*/,
                                                     const VkDeviceCreateInfo * /*info*/,
                                                     const VkAllocationCallbacks * /*allocator*/,
                                                     VkDevice * /*device*/) {
    return VK_ERROR_INITIALIZATION_FAILED;
}

// Dos GPUs con lavapipe: la lista real repite su única GPU. Con fake_gpu_first, la primera
// parece una integrada sin Vulkan 1.3.
VKAPI_ATTR VkResult VKAPI_CALL lavapipe_twice(VkInstance instance, std::uint32_t *count,
                                              VkPhysicalDevice *gpus) {
    if (gpus == nullptr) {
        *count = 2;
        return VK_SUCCESS;
    }
    std::uint32_t one = 1;
    VkPhysicalDevice lavapipe = VK_NULL_HANDLE;
    const VkResult result = fake_loader::real<PFN_vkEnumeratePhysicalDevices>(
        "vkEnumeratePhysicalDevices")(instance, &one, &lavapipe);
    gpus[0] = lavapipe;
    gpus[1] = lavapipe;
    *count = 2;
    return result;
}

// Cuántas veces se preguntaron las propiedades en este test (lo pone en 0 cada test).
std::uint32_t &properties_asked() {
    static std::uint32_t count = 0;
    return count;
}

VKAPI_ATTR void VKAPI_CALL fake_gpu_first(VkPhysicalDevice gpu,
                                          VkPhysicalDeviceProperties *properties) {
    fake_loader::real<PFN_vkGetPhysicalDeviceProperties>("vkGetPhysicalDeviceProperties")(
        gpu, properties);
    if (properties_asked()++ == 0) {
        properties->deviceType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
        properties->apiVersion = VK_API_VERSION_1_2;
        std::ranges::copy(std::string_view{"GPU de prueba"}, &properties->deviceName[0]);
        properties->deviceName[std::string_view{"GPU de prueba"}.size()] = '\0';
    }
}

// Si falla, muestra el texto entero.
::testing::AssertionResult contains(std::string_view text, std::string_view part) {
    if (text.find(part) != std::string_view::npos) {
        return ::testing::AssertionSuccess();
    }
    return ::testing::AssertionFailure() << "\"" << part << "\" no está en \"" << text << "\"";
}

// Crea el dispositivo con el loader falso y devuelve su error.
mz::gpu::init_error failure_of_device() {
    const vulkan_setup setup;
    const auto made = device::create(setup.m_instance, setup.m_surface);
    if (made) {
        give_up("Se esperaba que el dispositivo fallara.");
    }
    return made.error();
}

TEST_F(DeviceTest, OpensLavapipeWithItsQueue) {
    const vulkan_setup setup;
    const device gpu = created(setup);

    EXPECT_NE(gpu.handle(), VK_NULL_HANDLE);
    EXPECT_NE(gpu.physical(), VK_NULL_HANDLE);
    EXPECT_NE(gpu.queue(), VK_NULL_HANDLE);
    EXPECT_NE(gpu.functions().vkGetDeviceQueue, nullptr);
    EXPECT_TRUE(gpu.name().starts_with("llvmpipe")) << gpu.name();
}

// El criterio del ticket: toda la cadena, creada y destruida, sin un solo mensaje. Los
// errores abortarían; los warnings se cuentan.
TEST_F(DeviceTest, TheWholeChainLeavesNoValidationMessages) {
    const std::uint32_t before = validation_warnings();
    {
        const vulkan_setup setup;
        const device gpu = created(setup);
    }
    EXPECT_EQ(validation_warnings(), before);
}

TEST_F(DeviceTest, MovingLeavesTheSourceEmpty) {
    const vulkan_setup setup;
    device first = created(setup);
    const VkDevice handle = first.handle();
    const std::string name{first.name()};

    const device moved = std::move(first);
    EXPECT_EQ(moved.handle(), handle);
    EXPECT_EQ(moved.name(), name);
    // NOLINTNEXTLINE(bugprone-use-after-move): lo que se prueba es el origen vacío.
    EXPECT_EQ(first.handle(), VK_NULL_HANDLE);
}

TEST_F(DeviceTest, WithoutGpusNamesWhatIsMissing) {
    fake_loader::replace("vkEnumeratePhysicalDevices", &no_gpus);

    const mz::gpu::init_error error = failure_of_device();
    EXPECT_EQ(error.m_failure, init_failure::no_suitable_gpu);
    EXPECT_EQ(error.m_name, "una GPU con Vulkan");
}

TEST_F(DeviceTest, AVulkan12GpuIsNotEnough) {
    fake_loader::replace("vkGetPhysicalDeviceProperties", &vulkan_1_2_gpu);

    const mz::gpu::init_error error = failure_of_device();
    EXPECT_EQ(error.m_failure, init_failure::no_suitable_gpu);
    EXPECT_EQ(error.m_name, "Vulkan 1.3");
}

TEST_F(DeviceTest, NamesTheMissingSwapchainExtension) {
    fake_loader::replace("vkEnumerateDeviceExtensionProperties", &no_device_extensions);

    const mz::gpu::init_error error = failure_of_device();
    EXPECT_EQ(error.m_failure, init_failure::no_suitable_gpu);
    EXPECT_EQ(error.m_name, "VK_KHR_swapchain");
}

TEST_F(DeviceTest, NeedsAGraphicsQueue) {
    fake_loader::replace("vkGetPhysicalDeviceQueueFamilyProperties", &no_graphics_queue);

    const mz::gpu::init_error error = failure_of_device();
    EXPECT_EQ(error.m_failure, init_failure::no_suitable_gpu);
    EXPECT_EQ(error.m_name, "una cola de gráficos que presente en la ventana");
}

TEST_F(DeviceTest, NeedsAQueueThatPresentsOnTheSurface) {
    fake_loader::replace("vkGetPhysicalDeviceSurfaceSupportKHR", &no_present);

    const mz::gpu::init_error error = failure_of_device();
    EXPECT_EQ(error.m_failure, init_failure::no_suitable_gpu);
    EXPECT_EQ(error.m_name, "una cola de gráficos que presente en la ventana");
}

// El criterio del ticket: sin una feature, un error que la nombra, no un crash.
TEST_F(DeviceTest, NamesTheMissingFeature) {
    fake_loader::replace("vkGetPhysicalDeviceFeatures2", &no_synchronization2);

    const mz::gpu::init_error error = failure_of_device();
    EXPECT_EQ(error.m_failure, init_failure::no_suitable_gpu);
    EXPECT_EQ(error.m_name, "synchronization2");
    // Y la GPU a la que le falta.
    EXPECT_TRUE(std::string_view{error.m_gpu.data()}.starts_with("llvmpipe"));
}

TEST_F(DeviceTest, WithoutGpusTheErrorHasNoGpuName) {
    fake_loader::replace("vkEnumeratePhysicalDevices", &no_gpus);

    EXPECT_EQ(failure_of_device().m_gpu[0], '\0');
}

// El caso de Ubuntu: la GPU real no cumple, lavapipe sí. El juego corre en lavapipe y avisa.
TEST_F(DeviceTest, FallsBackToTheCpuAndSaysWhichGpuWasSkippedAndWhy) {
    properties_asked() = 0;
    fake_loader::replace("vkEnumeratePhysicalDevices", &lavapipe_twice);
    fake_loader::replace("vkGetPhysicalDeviceProperties", &fake_gpu_first);
    const vulkan_setup setup;
    const device gpu = created(setup);

    EXPECT_TRUE(gpu.name().starts_with("llvmpipe"));
    // Un if y no ASSERT_TRUE: clang-tidy no sabe que ASSERT_TRUE corta el test.
    const std::optional<device::skipped_gpu> &skipped = gpu.skipped();
    if (!skipped) {
        FAIL() << "No se dejó de lado ninguna GPU.";
    }
    EXPECT_EQ(std::string_view{skipped->m_name.data()}, "GPU de prueba");
    EXPECT_EQ(skipped->m_missing, "Vulkan 1.3");

    std::array<char, 512> buffer{};
    const std::string_view text = mz::gpu::describe_skipped(gpu, buffer);
    EXPECT_TRUE(contains(text, "GPU de prueba"));
    EXPECT_TRUE(contains(text, "Vulkan 1.3"));
    EXPECT_TRUE(contains(text, mz::gpu::purpose_of("Vulkan 1.3")));
    EXPECT_TRUE(contains(text, gpu.name()));
}

TEST_F(DeviceTest, WithTheBestGpuThereIsNothingToSay) {
    const vulkan_setup setup;
    const device gpu = created(setup);

    EXPECT_FALSE(gpu.skipped().has_value());
    std::array<char, 512> buffer{};
    EXPECT_EQ(mz::gpu::describe_skipped(gpu, buffer), "");
}

TEST_F(DeviceTest, ReportsTheCreationThatFailed) {
    fake_loader::replace("vkCreateDevice", &failing_create_device);

    const mz::gpu::init_error error = failure_of_device();
    EXPECT_EQ(error.m_failure, init_failure::call_failed);
    EXPECT_EQ(error.m_name, "vkCreateDevice");
    EXPECT_EQ(error.m_result, VK_ERROR_INITIALIZATION_FAILED);
}

// El criterio del ticket: un error de validación provocado a propósito aborta con su
// texto. La cola 99 no existe: la familia tiene una sola. La validación lo detecta antes de
// que la llamada llegue al driver.
TEST_F(DeviceDeathTest, AnInvalidCallAbortsWithTheValidationMessage) {
    EXPECT_DEATH(
        {
            const vulkan_setup setup;
            const device gpu = created(setup);
            VkQueue queue = VK_NULL_HANDLE;
            gpu.functions().vkGetDeviceQueue(gpu.handle(), gpu.queue_family(), 99, &queue);
        },
        "VUID-vkGetDeviceQueue-queueIndex-00385");
}

TEST_F(SurfaceTest, MovingLeavesTheSourceEmpty) {
    vulkan_setup setup;
    const VkSurfaceKHR handle = setup.m_surface.handle();

    const surface moved = std::move(setup.m_surface);
    EXPECT_EQ(moved.handle(), handle);
    // NOLINTNEXTLINE(bugprone-use-after-move): lo que se prueba es el origen vacío.
    EXPECT_EQ(setup.m_surface.handle(), VK_NULL_HANDLE);
}

TEST_F(SurfaceDeathTest, ANullSurfaceAborts) {
    EXPECT_DEATH(
        {
            mz::platform::context platform = started();
            const mz::platform::vulkan_library library = loaded(platform);
            const mz::gpu::instance owner = created(library);
            // Con paréntesis: la coma entre llaves partiría los argumentos de EXPECT_DEATH.
            const surface adopted(owner, VK_NULL_HANDLE);
        },
        "superficie");
}

} // namespace
