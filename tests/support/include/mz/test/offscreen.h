#pragma once

// La base de los tests que abren ventanas o usan Vulkan: los de platform y los de gpu. No
// incluye SDL, así que sirve también fuera de tests/platform.

#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <string_view>

// Corre SDL con el driver de video `offscreen`: crea ventanas de verdad, también con
// Vulkan, sin pantalla. Así los tests corren igual en la CI y no abren ventanas en el
// escritorio de quien los corre.
//
// Y le pide al loader de Vulkan un solo driver, lavapipe (el de software de Mesa), el mismo
// en la CI y en cada máquina. Sin eso, el loader carga todos los drivers instalados, y el
// test depende de la GPU de quien lo corre: cuánto tarda en cargarlos (el de NVIDIA, más
// de un segundo con ASan) y qué hacen al cargarse.
//
// Las dos van como variables de entorno y no como pistas de SDL (SDL_SetHint): SDL_Quit,
// al destruirse cada context, borra las pistas, y un test que crea un segundo context
// quedaría sin driver. Un test que quiere otro valor usa SDL_SetHintWithPriority con
// SDL_HINT_OVERRIDE, la única prioridad que le gana al entorno.
class OffscreenTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_EQ(setenv("VK_LOADER_DRIVERS_SELECT", "*lvp*", 1), 0);
        ASSERT_EQ(setenv("SDL_VIDEO_DRIVER", "offscreen", 1), 0);
    }
};

// Para los helpers de los tests que no prueban la creación: si lo que piden no existe, el
// test no puede seguir. give_up() deja el motivo y aborta (los helpers devuelven un valor,
// y FAIL() solo sirve en una función void). Va a stderr, sin buffer: lo que GoogleTest
// tuviera en el buffer de stdout se perdería con el abort.
[[noreturn]] inline void give_up(std::string_view why) {
    std::fprintf(stderr, "%.*s\n", static_cast<int>(why.size()), why.data());
    std::abort();
}
