cmake_minimum_required(VERSION 3.29)

# C entra porque varias dependencias están escritas en C.
project(MedievalZombies
    VERSION 0.0.0
    DESCRIPTION "Videojuego 3D con engine propio y corte de mallas en tiempo real"
    LANGUAGES C CXX
)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(mz_module)
include(mz_sdl3)
include(mz_vulkan)
include(mz_cook)

enable_testing()

add_subdirectory(engine/platform)
add_subdirectory(engine/core)
add_subdirectory(engine/gpu)
add_subdirectory(game)
# El cocinador va antes que todo lo que cocina assets (mz_cook_blend necesita el target).
add_subdirectory(tools/third_party)
add_subdirectory(tools/cooker)
add_subdirectory(tests)
