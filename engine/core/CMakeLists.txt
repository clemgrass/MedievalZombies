mz_add_module(engine_core LAYER core
    SOURCES
        src/assert.cpp
        src/core.cpp
    PRIVATE_DEPS engine_platform
)
