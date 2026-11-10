cmake_minimum_required(VERSION 3.29)
project(dependency_warnings LANGUAGES CXX)

option(MZ_GLOBAL_WARNINGS "Aplica los warnings en forma global, con add_compile_options" OFF)

list(APPEND CMAKE_MODULE_PATH ${MZ_CMAKE_DIR})
include(mz_module)

if(MZ_GLOBAL_WARNINGS)
    # Lo que no se hace en el proyecto: las banderas globales llegan a la dependencia.
    add_compile_options(-Wconversion -Werror)
endif()

# SYSTEM hace que los headers de la dependencia se incluyan como headers de sistema, así
# que sus warnings no aparecen al compilar el código propio. FetchContent_Declare tiene la
# misma opción.
add_subdirectory(noisy SYSTEM)

mz_add_module(engine_core LAYER core
    SOURCES core.cpp
    PRIVATE_DEPS noisy
)
