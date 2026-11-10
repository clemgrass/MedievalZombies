mz_add_module(engine_gpu LAYER gpu
    SOURCES src/gpu.cpp
    PRIVATE_DEPS engine_platform engine_core
)
