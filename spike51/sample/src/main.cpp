#include <SDL3/SDL.h>

#include <cinttypes>
#include <cstdio>

#include "core.hpp"

int main() {
    // Sin subsistemas: no necesita pantalla, sirve para probar el enlace con SDL3.
    if (!SDL_Init(0)) {
        std::printf("SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    std::printf("sdl %d.%d.%d hash %016" PRIx64 "\n", SDL_MAJOR_VERSION, SDL_MINOR_VERSION,
                SDL_MICRO_VERSION, mz::state_hash(1'000'000));
    SDL_Quit();
    return 0;
}
