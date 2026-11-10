cmake_minimum_required(VERSION 3.29)

# C entra porque varias dependencias están escritas en C.
project(MedievalZombies
    VERSION 0.0.0
    DESCRIPTION "Videojuego 3D con engine propio y corte de mallas en tiempo real"
    LANGUAGES C CXX
)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(mz_module)

enable_testing()

add_subdirectory(engine/platform)
add_subdirectory(engine/core)
add_subdirectory(engine/gpu)
add_subdirectory(game)
add_subdirectory(tests)
