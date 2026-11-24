mz_add_module(engine_platform LAYER platform
    SOURCES
        src/context_sdl.cpp
        src/memory_linux.cpp
        src/window_sdl.cpp
    # PRIVATE: SDL no sale de platform, y quien enlaza con platform no hereda sus headers.
    # Con el paquete del sistema igual se ven (están en /usr/include): la regla la hace
    # cumplir tools/ci/cpp-subset.sh.
    PRIVATE_DEPS SDL3::SDL3
)
