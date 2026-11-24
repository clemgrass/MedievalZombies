mz_add_module(engine_gpu LAYER gpu
    SOURCES
        src/clear.cpp
        src/device.cpp
        src/frame_loop.cpp
        src/init_error.cpp
        src/instance.cpp
        src/load.cpp
        src/requirements.cpp
        src/surface.cpp
        src/swapchain.cpp
    # PUBLIC: los headers de gpu nombran tipos de Vulkan y devuelven core::result. No usa
    # platform: recibe el puntero inicial del loader y la superficie ya creados, y así sus
    # tests pueden darle un loader falso.
    PUBLIC_DEPS mz_vulkan_headers engine_core
)

# PUBLIC: validation_enabled está en un header público, y los tests lo leen.
target_compile_definitions(engine_gpu PUBLIC MZ_VULKAN_VALIDATION=$<BOOL:${MZ_VULKAN_VALIDATION}>)
