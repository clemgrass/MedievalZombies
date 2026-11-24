mz_add_module(engine_platform LAYER platform
    SOURCES
        src/context_sdl.cpp
        src/memory_linux.cpp
        src/message_box_sdl.cpp
        src/vulkan_sdl.cpp
        src/window_sdl.cpp
    # PUBLIC: los headers de platform nombran tipos de Vulkan (VkInstance, VkSurfaceKHR).
    PUBLIC_DEPS mz_vulkan_headers
    # PRIVATE: SDL no sale de platform, y quien enlaza con platform no hereda sus headers.
    # Con el paquete del sistema igual se ven (están en /usr/include): la regla la hace
    # cumplir tools/ci/cpp-subset.sh.
    PRIVATE_DEPS SDL3::SDL3
)
