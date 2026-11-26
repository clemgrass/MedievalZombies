cmake_minimum_required(VERSION 3.29)
project(layers_into_tool_dep LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${MZ_CMAKE_DIR})
include(mz_module)

# Una dependencia de terceros de las herramientas, como tools_cgltf: no es un módulo, pero
# lleva la capa tool, así que el juego tampoco puede enlazarla directo.
add_library(tools_cgltf STATIC ../common/placeholder.cpp)
set_target_properties(tools_cgltf PROPERTIES MZ_LAYER tool)

mz_add_module(game LAYER game
    SOURCES ../common/placeholder.cpp
    PRIVATE_DEPS tools_cgltf
)
