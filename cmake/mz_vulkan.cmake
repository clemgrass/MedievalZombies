# Vulkan: solo los headers. El loader (libvulkan.so.1, vulkan-1.dll) no se enlaza: SDL3 lo
# carga en runtime, y engine/gpu pide cada función a partir del puntero que entrega
# platform. El loader lo pone el driver de quien juega.
#
# Dos fuentes para los headers, y una opción para elegir, como con SDL3:
#
#   - el paquete del sistema (por defecto): find_package, sin descargar nada;
#   - una copia con versión fijada (MZ_VENDOR_VULKAN_HEADERS=ON): FetchContent baja el
#     tarball de la versión de abajo y comprueba su hash. Es la que usan los builds de la CD.
#
# Las dos dejan el target mz_vulkan_headers, que agrega VK_NO_PROTOTYPES: los headers
# declaran los tipos y los punteros a función (PFN_vkCreateInstance), pero no las funciones.
# Una llamada directa a vkCreateInstance no compila, porque no hay nada con qué enlazarla.

include_guard(GLOBAL)

option(MZ_VENDOR_VULKAN_HEADERS
    "Usar los headers de Vulkan de una copia con versión fijada, no los del sistema" OFF)

if(MZ_VENDOR_VULKAN_HEADERS)
    include(FetchContent)

    # La misma versión que el snapshot del Arch Linux Archive de la CI
    # (tools/ci/archive-date): los dos caminos ven los mismos headers.
    set(MZ_VULKAN_HEADERS_VERSION 1.4.363)

    block()
        set(VULKAN_HEADERS_ENABLE_TESTS OFF)
        set(VULKAN_HEADERS_ENABLE_INSTALL OFF)
        set(VULKAN_HEADERS_ENABLE_MODULE OFF)

        FetchContent_Declare(VulkanHeaders
            URL https://github.com/KhronosGroup/Vulkan-Headers/archive/refs/tags/v${MZ_VULKAN_HEADERS_VERSION}.tar.gz
            URL_HASH SHA256=cbaf687d3c59b9666fe080e7f8396b6b4f4d344a768ee6b337c59852f4526b68
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            # Sus headers entran como headers del sistema: nuestros warnings no los revisan.
            SYSTEM
        )
        FetchContent_MakeAvailable(VulkanHeaders)
    endblock()
else()
    # 1.3 trae dynamic rendering y synchronization2 en el núcleo: lo mínimo que usa gpu.
    find_package(VulkanHeaders 1.3 REQUIRED CONFIG)
endif()

# La validación de Vulkan es una herramienta de depuración más: cada preset la prende o la
# apaga en CMakePresets.json (encendida en debug, debug-gcc y coverage). engine/gpu la
# recibe como MZ_VULKAN_VALIDATION, que vale 1 o 0.
option(MZ_VULKAN_VALIDATION
    "Activar la validation layer de Vulkan y su messenger en engine/gpu" OFF)

add_library(mz_vulkan_headers INTERFACE)
target_link_libraries(mz_vulkan_headers INTERFACE Vulkan::Headers)
target_compile_definitions(mz_vulkan_headers INTERFACE VK_NO_PROTOTYPES)
