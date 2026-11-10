cmake_minimum_required(VERSION 3.29)
project(layers_upward LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${MZ_CMAKE_DIR})
include(mz_module)

# engine_core se declara antes que engine_gpu: el chequeo no depende del orden.
mz_add_module(engine_core LAYER core
    SOURCES ../common/placeholder.cpp
    PUBLIC_DEPS engine_gpu
)
mz_add_module(engine_gpu LAYER gpu
    SOURCES ../common/placeholder.cpp
)
