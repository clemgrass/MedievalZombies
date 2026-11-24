#include "mz/gpu/instance.h"

#include "load.h"

#include "mz/core/assert.h"
#include "mz/gpu/init_error.h"
#include "mz/gpu/requirements.h"

#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace mz::gpu {
namespace {

std::atomic<std::uint32_t> &warning_count() {
    static std::atomic<std::uint32_t> count{0};
    return count;
}

// Lo que hace el engine con cada mensaje de la validación. Corre dentro de la llamada de
// Vulkan que lo provocó, así que un error aborta con esa llamada en el stack.
VKAPI_ATTR VkBool32 VKAPI_CALL on_validation_message(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT /*types*/,
    const VkDebugUtilsMessengerCallbackDataEXT *data, void * /*user_data*/) {
    const bool is_error = severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    MZ_CHECK(!is_error, "La validación de Vulkan encontró un error:\n{}", data->pMessage);
    // TODO(#122): al log, con la categoría gpu, cuando exista.
    std::fprintf(stderr, "gpu: aviso de la validación de Vulkan:\n%s\n", data->pMessage);
    ++warning_count();
    // VK_FALSE: la llamada que provocó el mensaje sigue. VK_TRUE es solo para probar las
    // layers.
    return VK_FALSE;
}

// Solo errores y warnings (info y verbose cuentan lo que pasa, no problemas), y solo de dos
// tipos: VALIDATION, un uso de la API que la especificación no permite, y PERFORMANCE, un
// uso válido pero lento. Fuera queda GENERAL, que es lo que el loader cuenta de la
// instalación: por ejemplo, que ignoró los drivers que VK_LOADER_DRIVERS_SELECT dejó
// afuera. No son bugs del engine.
VkDebugUtilsMessengerCreateInfoEXT messenger_info() {
    VkDebugUtilsMessengerCreateInfoEXT info{};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info.pfnUserCallback = &on_validation_message;
    return info;
}

bool has_layer(const global_functions &global, std::string_view name) {
    const std::vector<VkLayerProperties> layers = detail::enumerate<VkLayerProperties>(
        "vkEnumerateInstanceLayerProperties", [&](std::uint32_t *count, VkLayerProperties *items) {
            return global.vkEnumerateInstanceLayerProperties(count, items);
        });
    return std::ranges::any_of(
        layers, [&](const VkLayerProperties &each) { return name == each.layerName; });
}

// Pide la layer de validación y la extensión de sus mensajes. El error, si la layer no está
// instalada. [[maybe_unused]]: sin la validación (MZ_VULKAN_VALIDATION=0) nadie la llama, y Clang
// avisaría que no hace falta.
[[maybe_unused]] std::optional<init_error> enable_validation(const global_functions &global,
                                                             std::vector<const char *> &layers,
                                                             std::vector<const char *> &wanted) {
    if (!has_layer(global, validation_layer_name)) {
        return init_error{.m_failure = init_failure::missing_layer,
                          .m_name = validation_layer_name};
    }
    layers.push_back(validation_layer_name);
    wanted.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    wanted.push_back(VK_EXT_LAYER_SETTINGS_EXTENSION_NAME);
    return std::nullopt;
}

// La validación de sincronización: además de revisar cada llamada, la layer sigue qué lee y
// qué escribe cada comando en cada recurso, y avisa de un *hazard*: una lectura o una
// escritura sin una barrera que la ordene con la anterior (por ejemplo, el clear de una
// imagen que la presentación todavía está leyendo). Viene apagada porque cuesta: la layer
// guarda la historia de acceso de cada recurso. Los hazards llegan como errores: abortan.
constexpr VkBool32 sync_validation = VK_TRUE;
constexpr std::array<VkLayerSettingEXT, 1> validation_settings = {{
    {.pLayerName = validation_layer_name,
     .pSettingName = "validate_sync",
     .type = VK_LAYER_SETTING_TYPE_BOOL32_EXT,
     .valueCount = 1,
     .pValues = &sync_validation},
}};

// La primera de `wanted` que el loader no tiene, o vacío.
std::string_view first_missing_extension(const global_functions &global,
                                         std::span<const char *const> wanted) {
    const std::vector<VkExtensionProperties> available = detail::enumerate<VkExtensionProperties>(
        "vkEnumerateInstanceExtensionProperties",
        [&](std::uint32_t *count, VkExtensionProperties *items) {
            return global.vkEnumerateInstanceExtensionProperties(nullptr, count, items);
        });
    for (const char *name : wanted) {
        const auto matches = [&](const VkExtensionProperties &each) {
            return std::string_view{name} == each.extensionName;
        };
        if (!std::ranges::any_of(available, matches)) {
            return name;
        }
    }
    return {};
}

} // namespace

std::optional<init_error> instance::build(PFN_vkGetInstanceProcAddr get,
                                          std::span<const char *const> extensions) {
    const global_functions global = detail::load_global(get);

    // La versión del loader. La de cada GPU se mira después, al elegir una.
    if (global.vkEnumerateInstanceVersion == nullptr) {
        return init_error{.m_failure = init_failure::old_vulkan, .m_name = required_api_name};
    }
    std::uint32_t version = 0;
    detail::check_call(global.vkEnumerateInstanceVersion(&version), "vkEnumerateInstanceVersion");
    if (version < required_api_version) {
        return init_error{.m_failure = init_failure::old_vulkan, .m_name = required_api_name};
    }

    std::vector<const char *> wanted(extensions.begin(), extensions.end());
    std::vector<const char *> layers;
    // Sin un return dentro del if constexpr: con uno, la cobertura de Clang cuenta en 0 la
    // línea que sigue al bloque, aunque se ejecute.
    std::optional<init_error> validation_failed;
    if constexpr (validation_enabled) {
        validation_failed = enable_validation(global, layers, wanted);
    }
    if (validation_failed) {
        return validation_failed;
    }
    if (const std::string_view missing = first_missing_extension(global, wanted);
        !missing.empty()) {
        return init_error{.m_failure = init_failure::missing_extension, .m_name = missing};
    }

    VkApplicationInfo application{};
    application.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application.pApplicationName = "MedievalZombies";
    application.pEngineName = "MedievalZombies";
    // La versión más alta que usa el engine; la GPU puede tener una más nueva.
    application.apiVersion = required_api_version;

    // El messenger también va en la cadena de la instancia: así recibe los mensajes de
    // vkCreateInstance y vkDestroyInstance, cuando el messenger propio todavía no existe o
    // ya no existe.
    // Delante, los ajustes de la layer (la validación de sincronización). El messenger va
    // último: se reusa tal cual para crear el propio, y ahí su pNext tiene que ser nulo.
    const VkDebugUtilsMessengerCreateInfoEXT messages = messenger_info();
    const VkLayerSettingsCreateInfoEXT settings{
        .sType = VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT,
        .pNext = &messages,
        .settingCount = static_cast<std::uint32_t>(validation_settings.size()),
        .pSettings = validation_settings.data(),
    };
    VkInstanceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    if constexpr (validation_enabled) {
        info.pNext = &settings;
    }
    info.pApplicationInfo = &application;
    info.enabledLayerCount = static_cast<std::uint32_t>(layers.size());
    info.ppEnabledLayerNames = layers.data();
    info.enabledExtensionCount = static_cast<std::uint32_t>(wanted.size());
    info.ppEnabledExtensionNames = wanted.data();
    if (const VkResult result = global.vkCreateInstance(&info, nullptr, &m_instance);
        result != VK_SUCCESS) {
        m_instance = VK_NULL_HANDLE;
        return init_error{.m_failure = init_failure::call_failed,
                          .m_name = "vkCreateInstance",
                          .m_result = result};
    }

    m_functions = detail::load_instance(get, m_instance);

    if constexpr (validation_enabled) {
        if (const VkResult result = m_functions.vkCreateDebugUtilsMessengerEXT(
                m_instance, &messages, nullptr, &m_messenger);
            result != VK_SUCCESS) {
            m_messenger = VK_NULL_HANDLE;
            return init_error{.m_failure = init_failure::call_failed,
                              .m_name = "vkCreateDebugUtilsMessengerEXT",
                              .m_result = result};
        }
    }
    return std::nullopt;
}

instance::instance(instance &&other) noexcept
    : m_instance(std::exchange(other.m_instance, VK_NULL_HANDLE)),
      m_messenger(std::exchange(other.m_messenger, VK_NULL_HANDLE)),
      m_functions(other.m_functions) {}

instance::~instance() {
    if (m_messenger != VK_NULL_HANDLE) {
        m_functions.vkDestroyDebugUtilsMessengerEXT(m_instance, m_messenger, nullptr);
    }
    if (m_instance != VK_NULL_HANDLE) {
        m_functions.vkDestroyInstance(m_instance, nullptr);
    }
}

std::uint32_t validation_warnings() { return warning_count().load(); }

} // namespace mz::gpu
