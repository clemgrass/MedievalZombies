cmake_minimum_required(VERSION 3.29)
project(layers_tool_lower LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${MZ_CMAKE_DIR})
include(mz_module)

# platform está debajo de core, pero una herramienta tampoco lo puede enlazar directo: solo
# enlaza core.
mz_add_module(engine_platform LAYER platform
    SOURCES ../common/placeholder.cpp
)
mz_add_module(tools_cooker LAYER tool
    SOURCES ../common/placeholder.cpp
    PRIVATE_DEPS engine_platform
)
