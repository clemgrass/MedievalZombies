#include "fake_loader.h"
#include "gpu_test.h"

#include "mz/gpu/init_error.h"
#include "mz/gpu/instance.h"
#include "mz/gpu/requirements.h"
#include "mz/platform/context.h"
#include "mz/platform/vulkan.h"

#include <gtest/gtest.h>
#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace {

using mz::gpu::init_failure;
using mz::gpu::instance;
using mz::gpu::validation_warnings;

class InstanceTest : public FakeLoaderTest {};
class InstanceDeathTest : public FakeLoaderTest {};
class ValidationTest : public FakeLoaderTest {};
class ValidationDeathTest : public FakeLoaderTest {};

// Crea la instancia con el loader falso: con los reemplazos que el test ya pidió.
auto create_through_fake(const mz::platform::vulkan_library &library) {
    fake_loader::wrap(library.get_instance_proc_addr());
    return instance::create(fake_loader::entry(), library.instance_extensions());
}

// Manda un mensaje propio a la validación, con la severidad pedida, como si lo hubiera
// encontrado la layer.
void submit(const instance &target, VkDebugUtilsMessageSeverityFlagBitsEXT severity,
            const char *text) {
    VkDebugUtilsMessengerCallbackDataEXT data{};
    data.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT;
    data.pMessage = text;
    target.functions().vkSubmitDebugUtilsMessageEXT(
        target.handle(), severity, VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT, &data);
}

// Los reemplazos de las funciones del loader, para los caminos de error.
VKAPI_ATTR VkResult VKAPI_CALL vulkan_1_2(std::uint32_t *version) {
    *version = VK_API_VERSION_1_2;
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL no_layers(std::uint32_t *count, VkLayerProperties * /*layers*/) {
    *count = 0;
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL out_of_memory_extensions(const char * /*layer*/,
                                                        std::uint32_t * /*count*/,
                                                        VkExtensionProperties * /*extensions*/) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
}

VKAPI_ATTR VkResult VKAPI_CALL failing_create_instance(const VkInstanceCreateInfo * /*info*/,
                                                       const VkAllocationCallbacks * /*allocator*/,
                                                       VkInstance * /*instance*/) {
    return VK_ERROR_INITIALIZATION_FAILED;
}

VKAPI_ATTR VkResult VKAPI_CALL failing_create_messenger(
    VkInstance /*instance*/, const VkDebugUtilsMessengerCreateInfoEXT * /*info*/,
    const VkAllocationCallbacks * /*allocator*/, VkDebugUtilsMessengerEXT * /*messenger*/) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
}

TEST_F(InstanceTest, IsCreatedWithAllItsFunctions) {
    const vulkan_setup setup;
    const instance &created = setup.m_instance;

    EXPECT_NE(created.handle(), VK_NULL_HANDLE);
    EXPECT_NE(created.functions().vkCreateDevice, nullptr);
    EXPECT_NE(created.functions().vkDestroySurfaceKHR, nullptr);
    // Las de la validación: los tests corren en Debug.
    EXPECT_NE(created.functions().vkCreateDebugUtilsMessengerEXT, nullptr);
}

TEST_F(InstanceTest, MovingLeavesTheSourceEmpty) {
    mz::platform::context platform = started();
    const mz::platform::vulkan_library library = loaded(platform);
    instance first = created(library);
    const VkInstance handle = first.handle();

    const instance moved = std::move(first);
    EXPECT_EQ(moved.handle(), handle);
    // NOLINTNEXTLINE(bugprone-use-after-move): lo que se prueba es el origen vacío.
    EXPECT_EQ(first.handle(), VK_NULL_HANDLE);
}

// Un loader de Vulkan 1.0 no tiene vkEnumerateInstanceVersion: es demasiado viejo.
TEST_F(InstanceTest, ALoaderWithoutTheVersionQueryIsTooOld) {
    mz::platform::context platform = started();
    const mz::platform::vulkan_library library = loaded(platform);
    fake_loader::replace_raw("vkEnumerateInstanceVersion", nullptr);

    const auto made = create_through_fake(library);
    ASSERT_FALSE(made);
    EXPECT_EQ(made.error().m_failure, init_failure::old_vulkan);
    EXPECT_EQ(made.error().m_name, "Vulkan 1.3");
}

TEST_F(InstanceTest, AVulkan12LoaderIsTooOld) {
    mz::platform::context platform = started();
    const mz::platform::vulkan_library library = loaded(platform);
    fake_loader::replace("vkEnumerateInstanceVersion", &vulkan_1_2);

    const auto made = create_through_fake(library);
    ASSERT_FALSE(made);
    EXPECT_EQ(made.error().m_failure, init_failure::old_vulkan);
}

TEST_F(InstanceTest, NamesAMissingExtension) {
    mz::platform::context platform = started();
    const mz::platform::vulkan_library library = loaded(platform);
    const std::array<const char *, 2> extensions = {VK_KHR_SURFACE_EXTENSION_NAME,
                                                    "VK_KHR_que_no_existe"};

    const auto made = instance::create(library.get_instance_proc_addr(), extensions);
    ASSERT_FALSE(made);
    EXPECT_EQ(made.error().m_failure, init_failure::missing_extension);
    EXPECT_EQ(made.error().m_name, "VK_KHR_que_no_existe");
}

TEST_F(InstanceTest, ReportsTheCreationThatFailed) {
    mz::platform::context platform = started();
    const mz::platform::vulkan_library library = loaded(platform);
    fake_loader::replace("vkCreateInstance", &failing_create_instance);

    const auto made = create_through_fake(library);
    ASSERT_FALSE(made);
    EXPECT_EQ(made.error().m_failure, init_failure::call_failed);
    EXPECT_EQ(made.error().m_name, "vkCreateInstance");
    EXPECT_EQ(made.error().m_result, VK_ERROR_INITIALIZATION_FAILED);
}

// Que el loader no tenga una función del núcleo de Vulkan no es la máquina de quien juega:
// es un bug (una función de una extensión que no se pidió). Aborta con su nombre.
TEST_F(InstanceDeathTest, AMissingFunctionAbortsWithItsName) {
    EXPECT_DEATH(
        {
            mz::platform::context platform = started();
            const mz::platform::vulkan_library library = loaded(platform);
            fake_loader::replace_raw("vkCreateDevice", nullptr);
            static_cast<void>(create_through_fake(library));
        },
        "vkCreateDevice");
}

// Una enumeración que falla es que se acabó la memoria del sistema: aborta.
TEST_F(InstanceDeathTest, AFailedEnumerationAborts) {
    EXPECT_DEATH(
        {
            mz::platform::context platform = started();
            const mz::platform::vulkan_library library = loaded(platform);
            fake_loader::replace("vkEnumerateInstanceExtensionProperties",
                                 &out_of_memory_extensions);
            static_cast<void>(create_through_fake(library));
        },
        "vkEnumerateInstanceExtensionProperties");
}

TEST_F(ValidationTest, WithoutTheLayerTheInstanceIsNotCreated) {
    mz::platform::context platform = started();
    const mz::platform::vulkan_library library = loaded(platform);
    fake_loader::replace("vkEnumerateInstanceLayerProperties", &no_layers);

    const auto made = create_through_fake(library);
    ASSERT_FALSE(made);
    EXPECT_EQ(made.error().m_failure, init_failure::missing_layer);
    EXPECT_EQ(made.error().m_name, "VK_LAYER_KHRONOS_validation");
}

TEST_F(ValidationTest, ReportsAFailedMessenger) {
    mz::platform::context platform = started();
    const mz::platform::vulkan_library library = loaded(platform);
    fake_loader::replace("vkCreateDebugUtilsMessengerEXT", &failing_create_messenger);

    const auto made = create_through_fake(library);
    ASSERT_FALSE(made);
    EXPECT_EQ(made.error().m_failure, init_failure::call_failed);
    EXPECT_EQ(made.error().m_name, "vkCreateDebugUtilsMessengerEXT");
}

TEST_F(ValidationTest, AWarningIsCounted) {
    const vulkan_setup setup;
    const std::uint32_t before = validation_warnings();

    submit(setup.m_instance, VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT, "aviso de prueba");
    EXPECT_EQ(validation_warnings(), before + 1);
}

// Un error de la validación es un bug: aborta con el texto del mensaje.
TEST_F(ValidationDeathTest, AnErrorAbortsWithItsMessage) {
    EXPECT_DEATH(
        {
            const vulkan_setup setup;
            submit(setup.m_instance, VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
                   "error de prueba");
        },
        "error de prueba");
}

// El mensaje que ve quien juega: qué falta y para qué lo usa el juego.
std::string message_of(const mz::gpu::init_error &error) {
    std::array<char, 512> buffer{};
    return std::string{mz::gpu::describe(error, buffer)};
}

mz::gpu::gpu_name named(std::string_view text) {
    mz::gpu::gpu_name name{};
    std::ranges::copy(text, name.begin());
    return name;
}

// Si falla, muestra el mensaje entero.
::testing::AssertionResult contains(std::string_view text, std::string_view part) {
    if (text.find(part) != std::string_view::npos) {
        return ::testing::AssertionSuccess();
    }
    return ::testing::AssertionFailure() << "\"" << part << "\" no está en \"" << text << "\"";
}

TEST(InitErrorTest, AnOldDriverSaysWhatVersionAndWhy) {
    const std::string text =
        message_of({.m_failure = init_failure::old_vulkan, .m_name = "Vulkan 1.3"});
    EXPECT_TRUE(contains(text, "Vulkan 1.3"));
    EXPECT_TRUE(contains(text, mz::gpu::purpose_of("Vulkan 1.3")));
}

TEST(InitErrorTest, AMissingLayerSaysHowToInstallIt) {
    const std::string text = message_of(
        {.m_failure = init_failure::missing_layer, .m_name = "VK_LAYER_KHRONOS_validation"});
    EXPECT_TRUE(contains(text, "VK_LAYER_KHRONOS_validation"));
    EXPECT_TRUE(contains(text, "vulkan-validation-layers"));
}

TEST(InitErrorTest, AMissingExtensionIsNamed) {
    const std::string text = message_of(
        {.m_failure = init_failure::missing_extension, .m_name = "VK_KHR_wayland_surface"});
    EXPECT_TRUE(contains(text, "VK_KHR_wayland_surface"));
}

// El criterio del ticket: sin una feature, un mensaje claro que la nombra.
TEST(InitErrorTest, AGpuThatLacksAFeatureNamesTheGpuTheFeatureAndWhy) {
    const std::string text = message_of({.m_failure = init_failure::no_suitable_gpu,
                                         .m_name = "synchronization2",
                                         .m_gpu = named("Intel(R) UHD Graphics 620")});
    EXPECT_TRUE(contains(text, "Intel(R) UHD Graphics 620"));
    EXPECT_TRUE(contains(text, "synchronization2"));
    EXPECT_TRUE(contains(text, mz::gpu::purpose_of("synchronization2")));
}

TEST(InitErrorTest, WithoutAnyGpuSaysSo) {
    const std::string text =
        message_of({.m_failure = init_failure::no_suitable_gpu, .m_name = "una GPU con Vulkan"});
    EXPECT_TRUE(contains(text, "una GPU con Vulkan"));
}

TEST(InitErrorTest, AFailedCallNamesItAndItsResult) {
    const std::string text = message_of({.m_failure = init_failure::call_failed,
                                         .m_name = "vkCreateDevice",
                                         .m_result = VK_ERROR_INITIALIZATION_FAILED});
    EXPECT_TRUE(contains(text, "vkCreateDevice"));
    EXPECT_TRUE(contains(text, "-3"));
}

// Un valor fuera del enum (un uint8_t cualquiera es válido para el tipo) no se queda sin
// mensaje.
TEST(InitErrorTest, AnUnknownFailureStillHasAMessage) {
    const std::string text =
        message_of({.m_failure = static_cast<init_failure>(200), .m_name = {}});
    EXPECT_FALSE(text.empty());
}

// Un buffer chico corta el mensaje, sin escribir fuera de él.
TEST(InitErrorTest, ATinyBufferCutsTheMessage) {
    std::array<char, 8> buffer{};
    const std::string_view text = mz::gpu::describe(
        {.m_failure = init_failure::call_failed, .m_name = "vkCreateDevice"}, buffer);
    EXPECT_EQ(text.size(), buffer.size());
    EXPECT_EQ(text.data(), buffer.data());
}

} // namespace
