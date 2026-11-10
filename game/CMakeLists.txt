mz_add_module(game LAYER game EXECUTABLE
    SOURCES src/main.cpp
    PRIVATE_DEPS engine_platform engine_core engine_gpu
)
