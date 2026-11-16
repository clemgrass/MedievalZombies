mz_add_module(engine_core LAYER core
    SOURCES
        src/assert.cpp
        src/core.cpp
        src/mat.cpp
        src/projection.cpp
        src/quat.cpp
        src/region.cpp
    PRIVATE_DEPS engine_platform
)
