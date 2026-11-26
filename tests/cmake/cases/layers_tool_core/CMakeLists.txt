cmake_minimum_required(VERSION 3.29)
project(layers_tool_core LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${MZ_CMAKE_DIR})
include(mz_module)

# Lo que una herramienta sí puede enlazar: core y otras herramientas.
mz_add_module(engine_core LAYER core
    SOURCES ../common/placeholder.cpp
)
mz_add_module(tools_cooker LAYER tool
    SOURCES ../common/placeholder.cpp
    PUBLIC_DEPS engine_core
)
mz_add_module(tools_other LAYER tool
    SOURCES ../common/placeholder.cpp
    PRIVATE_DEPS tools_cooker
)
