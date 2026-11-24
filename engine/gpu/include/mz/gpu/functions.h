#pragma once

// Las funciones de Vulkan que usa el engine, en tablas propias de punteros. Nada enlaza con
// libvulkan: con VK_NO_PROTOTYPES, los headers solo declaran los tipos de los punteros
// (PFN_vkCreateDevice), y cada puntero se le pide al loader en runtime, a partir de uno
// solo, vkGetInstanceProcAddr, que entrega platform::vulkan_library.
//
// Hay tres niveles, según a quién se le pide cada función:
//
//   global      vkGetInstanceProcAddr(VK_NULL_HANDLE, ...)  lo que se usa antes de la instancia
//   instancia   vkGetInstanceProcAddr(instancia, ...)       la instancia, las GPUs físicas
//   dispositivo vkGetDeviceProcAddr(dispositivo, ...)       todo lo que trabaja con la GPU
//
// Las de dispositivo se piden al dispositivo y no a la instancia: la de la instancia es un
// trampolín del loader que mira a qué driver va la llamada y salta a él. La que entrega el
// dispositivo es directamente la del driver, sin ese salto en cada llamada del frame.
//
// Cada lista de abajo nombra una vez cada función. Con ellas se declaran los miembros de
// las tablas y se escribe su carga (src/load.cpp): una función nueva se agrega en un solo
// lugar. Los miembros se llaman como en la especificación: functions.vkCreateDevice(...).

#include <vulkan/vulkan_core.h>

// Las que se piden sin instancia. vkEnumerateInstanceVersion no está en la lista: un loader
// de Vulkan 1.0 no la tiene, y que falte no es un bug sino un loader viejo (global_functions
// la declara aparte).
#define MZ_VK_GLOBAL_FUNCTIONS(X)                                                                  \
    X(vkEnumerateInstanceLayerProperties)                                                          \
    X(vkEnumerateInstanceExtensionProperties)                                                      \
    X(vkCreateInstance)

#define MZ_VK_INSTANCE_FUNCTIONS(X)                                                                \
    X(vkDestroyInstance)                                                                           \
    X(vkEnumeratePhysicalDevices)                                                                  \
    X(vkGetPhysicalDeviceProperties)                                                               \
    X(vkGetPhysicalDeviceFeatures2)                                                                \
    X(vkGetPhysicalDeviceQueueFamilyProperties)                                                    \
    X(vkEnumerateDeviceExtensionProperties)                                                        \
    X(vkGetPhysicalDeviceSurfaceSupportKHR)                                                        \
    X(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)                                                   \
    X(vkGetPhysicalDeviceSurfaceFormatsKHR)                                                        \
    X(vkDestroySurfaceKHR)                                                                         \
    X(vkCreateDevice)                                                                              \
    X(vkGetDeviceProcAddr)

// Las de VK_EXT_debug_utils: solo existen con la validación activa. Sin ella quedan en
// nullptr. vkSubmitDebugUtilsMessageEXT manda un mensaje propio a la validación: los tests
// lo usan para probar qué hace el engine con cada severidad.
#define MZ_VK_DEBUG_FUNCTIONS(X)                                                                   \
    X(vkCreateDebugUtilsMessengerEXT)                                                              \
    X(vkDestroyDebugUtilsMessengerEXT)                                                             \
    X(vkSubmitDebugUtilsMessageEXT)

#define MZ_VK_DEVICE_FUNCTIONS(X)                                                                  \
    X(vkDestroyDevice)                                                                             \
    X(vkGetDeviceQueue)                                                                            \
    X(vkDeviceWaitIdle)                                                                            \
    X(vkCreateSwapchainKHR)                                                                        \
    X(vkDestroySwapchainKHR)                                                                       \
    X(vkGetSwapchainImagesKHR)                                                                     \
    X(vkAcquireNextImageKHR)                                                                       \
    X(vkQueuePresentKHR)                                                                           \
    X(vkCreateImageView)                                                                           \
    X(vkDestroyImageView)                                                                          \
    X(vkCreateSemaphore)                                                                           \
    X(vkDestroySemaphore)                                                                          \
    X(vkCreateFence)                                                                               \
    X(vkDestroyFence)                                                                              \
    X(vkWaitForFences)                                                                             \
    X(vkResetFences)                                                                               \
    X(vkCreateCommandPool)                                                                         \
    X(vkDestroyCommandPool)                                                                        \
    X(vkResetCommandPool)                                                                          \
    X(vkAllocateCommandBuffers)                                                                    \
    X(vkBeginCommandBuffer)                                                                        \
    X(vkEndCommandBuffer)                                                                          \
    X(vkQueueSubmit2)                                                                              \
    X(vkCmdPipelineBarrier2)                                                                       \
    X(vkCmdBeginRendering)                                                                         \
    X(vkCmdEndRendering)

namespace mz::gpu {

#define MZ_VK_DECLARE(name) PFN_##name name = nullptr;

struct global_functions {
    // nullptr con un loader de Vulkan 1.0.
    PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion = nullptr;
    MZ_VK_GLOBAL_FUNCTIONS(MZ_VK_DECLARE)
};

struct instance_functions {
    MZ_VK_INSTANCE_FUNCTIONS(MZ_VK_DECLARE)
    MZ_VK_DEBUG_FUNCTIONS(MZ_VK_DECLARE)
};

struct device_functions {
    MZ_VK_DEVICE_FUNCTIONS(MZ_VK_DECLARE)
};

#undef MZ_VK_DECLARE

} // namespace mz::gpu
