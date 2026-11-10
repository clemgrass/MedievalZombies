cmake_minimum_required(VERSION 3.29)
project(layers_same LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${MZ_CMAKE_DIR})
include(mz_module)

mz_add_module(engine_physics LAYER system
    SOURCES ../common/placeholder.cpp
)
mz_add_module(engine_slicing LAYER system
    SOURCES ../common/placeholder.cpp
    PRIVATE_DEPS engine_physics
)
