cmake_minimum_required(VERSION 3.29)
project(layers_into_tool LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${MZ_CMAKE_DIR})
include(mz_module)

# game está en la capa más alta de la pila, pero no puede enlazar una herramienta: nada de
# engine/ ni de game/ enlaza el cocinador.
mz_add_module(tools_cooker LAYER tool
    SOURCES ../common/placeholder.cpp
)
mz_add_module(game LAYER game
    SOURCES ../common/placeholder.cpp
    PRIVATE_DEPS tools_cooker
)
