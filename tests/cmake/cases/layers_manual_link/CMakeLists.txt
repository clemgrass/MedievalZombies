cmake_minimum_required(VERSION 3.29)
project(layers_manual_link LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${MZ_CMAKE_DIR})
include(mz_module)

mz_add_module(engine_platform LAYER platform
    SOURCES ../common/placeholder.cpp
)
mz_add_module(engine_core LAYER core
    SOURCES ../common/placeholder.cpp
)

# Un enlace hacia arriba que no pasa por mz_add_module.
target_link_libraries(engine_platform PRIVATE engine_core)
