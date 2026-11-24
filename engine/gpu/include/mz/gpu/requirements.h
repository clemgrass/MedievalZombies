#pragma once

// Lo que el engine exige de una GPU: la versión de Vulkan, las extensiones del dispositivo y
// las features. Se verifica al iniciar, antes de crear el dispositivo: si falta algo, el
// juego lo dice con su nombre al arrancar, y no falla después, en medio de una partida, con
// una llamada que el driver no sabe hacer.
//
// Cada feature está una sola vez en estas tablas, con su nombre: la misma tabla sirve para
// verificarla en la GPU y para pedirla al crear el dispositivo.

#include <vulkan/vulkan_core.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace mz::gpu {

// Vulkan 1.3: dynamic rendering y synchronization2 en el núcleo.
inline constexpr std::uint32_t required_api_version = VK_API_VERSION_1_3;
inline constexpr std::string_view required_api_name = "Vulkan 1.3";

// El swapchain: presentar en la ventana.
inline constexpr std::array<const char *, 1> required_device_extensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

// El nombre del requisito de la cola, para los mensajes.
inline constexpr std::string_view graphics_queue_requirement =
    "una cola de gráficos que presente en la ventana";

// Lo que la superficie de la ventana tiene que aceptar para el swapchain (swapchain.h): un
// formato sRGB, y que el compositor ignore el alfa de la imagen.
inline constexpr std::string_view srgb_format_requirement =
    "un formato sRGB para mostrar en la ventana";
inline constexpr std::string_view opaque_requirement = "VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR";

// Una feature, por su nombre, el miembro que la guarda en su estructura de Vulkan y para
// qué la usa el juego (va en el mensaje si falta).
template <typename Features> struct required_feature {
    std::string_view m_name;
    VkBool32 Features::*m_member;
    std::string_view m_why;
};

// Las de 1.2: el descriptor set bindless, un arreglo de texturas sin tamaño fijo, con
// huecos, indexado con un índice distinto por píxel.
inline constexpr std::array<required_feature<VkPhysicalDeviceVulkan12Features>, 4>
    required_vulkan12_features = {{
        {.m_name = "descriptorIndexing",
         .m_member = &VkPhysicalDeviceVulkan12Features::descriptorIndexing,
         .m_why = "un solo descriptor set con todas las texturas (bindless)"},
        {.m_name = "runtimeDescriptorArray",
         .m_member = &VkPhysicalDeviceVulkan12Features::runtimeDescriptorArray,
         .m_why = "un arreglo de texturas sin tamaño fijo en el shader"},
        {.m_name = "descriptorBindingPartiallyBound",
         .m_member = &VkPhysicalDeviceVulkan12Features::descriptorBindingPartiallyBound,
         .m_why = "un arreglo de texturas con huecos, de las zonas que no están cargadas"},
        {.m_name = "shaderSampledImageArrayNonUniformIndexing",
         .m_member = &VkPhysicalDeviceVulkan12Features::shaderSampledImageArrayNonUniformIndexing,
         .m_why = "elegir la textura con un índice distinto en cada píxel"},
    }};

// Las de 1.3: dibujar sin VkRenderPass, las barreras nuevas, y el discard del screen-door,
// que glslc compila como "demote to helper invocation".
inline constexpr std::array<required_feature<VkPhysicalDeviceVulkan13Features>, 3>
    required_vulkan13_features = {{
        {.m_name = "dynamicRendering",
         .m_member = &VkPhysicalDeviceVulkan13Features::dynamicRendering,
         .m_why = "dibujar sin VkRenderPass ni VkFramebuffer"},
        {.m_name = "synchronization2",
         .m_member = &VkPhysicalDeviceVulkan13Features::synchronization2,
         .m_why = "las barreras de sincronización entre la CPU y la GPU"},
        {.m_name = "shaderDemoteToHelperInvocation",
         .m_member = &VkPhysicalDeviceVulkan13Features::shaderDemoteToHelperInvocation,
         .m_why = "las sombras y las transparencias tramadas (screen-door)"},
    }};

// Para qué usa el juego un requisito, por su nombre ("synchronization2", "Vulkan 1.3",
// "VK_KHR_swapchain"...), o vacío si no es uno de los requisitos.
[[nodiscard]] std::string_view purpose_of(std::string_view requirement);

// Las features de una GPU, o las que se piden al crear el dispositivo. Las dos estructuras
// van encadenadas (pNext) detrás de un VkPhysicalDeviceFeatures2, que arma chain().
class device_features {
public:
    device_features();
    // La cadena apunta a los miembros: copiarla dejaría punteros al original.
    device_features(const device_features &) = delete;
    device_features &operator=(const device_features &) = delete;
    device_features(device_features &&) = delete;
    device_features &operator=(device_features &&) = delete;
    ~device_features() = default;

    // La cabeza de la cadena, para vkGetPhysicalDeviceFeatures2 o para el pNext de
    // VkDeviceCreateInfo.
    [[nodiscard]] VkPhysicalDeviceFeatures2 &chain() { return m_features; }
    [[nodiscard]] VkPhysicalDeviceVulkan12Features &vulkan12() { return m_vulkan12; }
    [[nodiscard]] VkPhysicalDeviceVulkan13Features &vulkan13() { return m_vulkan13; }

    // El nombre de la primera feature exigida que no está, o vacío si están todas.
    [[nodiscard]] std::string_view first_missing() const;

    // Prende las features exigidas, y solo esas.
    void enable_required();

private:
    VkPhysicalDeviceFeatures2 m_features{};
    VkPhysicalDeviceVulkan12Features m_vulkan12{};
    VkPhysicalDeviceVulkan13Features m_vulkan13{};
};

// Una GPU de la lista del loader, como la ve la elección.
struct gpu_candidate {
    VkPhysicalDeviceType m_type;
    // Lo primero que no cumple (su versión, una extensión, la cola o una feature), con su
    // nombre en la especificación; vacío si cumple todo.
    std::string_view m_missing;
};

// La posición de la GPU elegida en `candidates`, o vacío si ninguna cumple todo. Entre las
// que cumplen gana el mejor tipo: discreta, integrada, virtual y, al final, la CPU (lavapipe)
// o un tipo desconocido. A igual tipo, la primera: el orden del loader, que en Linux arregla
// la capa device-select de Mesa para poner primero la GPU por defecto del sistema.
[[nodiscard]] std::optional<std::size_t> choose_gpu(std::span<const gpu_candidate> candidates);

// La GPU que el juego dejó de lado al elegir `chosen`: una de mejor tipo que no cumplía
// algo (la mejor de ellas; a igual tipo, la primera). Es la que quien juega esperaba usar,
// y el juego le avisa qué le faltó. Vacío si se eligió el mejor tipo que había.
[[nodiscard]] std::optional<std::size_t> passed_over_gpu(std::span<const gpu_candidate> candidates,
                                                         std::size_t chosen);

} // namespace mz::gpu
