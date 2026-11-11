cmake_minimum_required(VERSION 3.29)
project(result_nodiscard LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${MZ_CMAKE_DIR})
include(mz_module)

mz_add_module(engine_core LAYER core
    SOURCES ignored.cpp
)
# Los headers reales de engine_core, para probar el result.h de verdad.
target_include_directories(engine_core PRIVATE ${MZ_CMAKE_DIR}/../engine/core/include)
