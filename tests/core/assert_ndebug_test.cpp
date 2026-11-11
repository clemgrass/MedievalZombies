// MZ_ASSERT y MZ_CHECK como quedan en relwithdebinfo y release: este archivo se compila con
// NDEBUG (lo define tests/core/CMakeLists.txt solo para él). Los tests corren en Debug, y
// esta es la única forma de probar el lado apagado sin correr la suite en release.
//
// Solo incluye mz/core/assert.h. Un header con funciones inline que usan MZ_ASSERT (como
// result.h) tendría aquí un cuerpo distinto que en el resto de la suite, y eso rompe la
// regla de una sola definición (ODR): el enlazador se quedaría con una de las dos.

#include "mz/core/assert.h"

#include <gtest/gtest.h>

#ifndef NDEBUG
#error "assert_ndebug_test.cpp tiene que compilarse con NDEBUG"
#endif

namespace {

TEST(AssertNdebugTest, DisabledAssertDoesNotEvaluateItsCondition) {
    int evaluations = 0;
    MZ_ASSERT(++evaluations == 0, "nunca se evalúa");
    EXPECT_EQ(evaluations, 0);
}

TEST(AssertNdebugTest, DisabledAssertDoesNotEvaluateItsArguments) {
    int evaluations = 0;
    MZ_ASSERT(false, "{}", ++evaluations);
    EXPECT_EQ(evaluations, 0);
}

TEST(AssertNdebugDeathTest, CheckStaysOnWithNdebug) {
    EXPECT_DEATH(MZ_CHECK(false, "siempre activo"), "siempre activo");
}

} // namespace
