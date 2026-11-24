# SDL3: la ventana, los eventos y el reloj de engine/platform.
#
# Dos fuentes, y una opción para elegir:
#
#   - el paquete del sistema (por defecto): find_package, sin descargar nada;
#   - una copia con versión fijada (MZ_VENDOR_SDL3=ON): FetchContent baja el tarball de
#     la versión de abajo, comprueba su hash y la compila estática junto al proyecto.
#     Es la que usan los builds de la CD.
#
# Las dos dejan el mismo target, SDL3::SDL3. Solo engine/platform y sus tests lo enlazan.

include_guard(GLOBAL)

option(MZ_VENDOR_SDL3 "Compilar SDL3 desde una copia con versión fijada, no la del sistema" OFF)

if(MZ_VENDOR_SDL3)
    include(FetchContent)

    # La misma versión que el snapshot del Arch Linux Archive de la CI
    # (tools/ci/archive-date): los dos caminos compilan el mismo SDL.
    set(MZ_SDL3_VERSION 3.4.18)

    # block() deja las opciones de SDL dentro de este bloque: no llegan al resto del build.
    block()
        set(SDL_SHARED OFF)
        set(SDL_STATIC ON)
        set(SDL_TEST_LIBRARY OFF)
        set(SDL_TESTS OFF)
        set(SDL_EXAMPLES OFF)
        set(SDL_INSTALL OFF)
        # El renderer 2D y la API gráfica de SDL no se usan: el renderer es propio. Apagados,
        # SDL no compila ninguno de sus drivers. Las funciones siguen existiendo (SDL
        # mantiene toda su API para no romper el ABI), pero fallan al llamarlas. La regla
        # que las prohíbe en el código la revisa tools/ci/cpp-subset.sh.
        set(SDL_GPU OFF)
        set(SDL_RENDER OFF)

        FetchContent_Declare(SDL3
            URL https://github.com/libsdl-org/SDL/releases/download/release-${MZ_SDL3_VERSION}/SDL3-${MZ_SDL3_VERSION}.tar.gz
            URL_HASH SHA256=9c75cf16330322c217dedd2e0609f1124f1b54b8633e763467b4684d0f4334a3
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            # Sus headers entran como headers del sistema: nuestros warnings no los revisan.
            SYSTEM
        )
        FetchContent_MakeAvailable(SDL3)
    endblock()
else()
    # 3.2 es la primera versión estable de SDL3. La API que usa platform ya está ahí.
    find_package(SDL3 3.2 REQUIRED CONFIG COMPONENTS SDL3)
endif()
