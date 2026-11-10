// Un death test atrapa un abort: es lo que usan los tests de los assert del engine.

#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>

namespace {

[[noreturn]] void report_and_abort() {
    std::fputs("fallo detectado, abortando\n", stderr);
    std::abort();
}

// GoogleTest corre la sentencia en un proceso hijo y pasa si el hijo muere y su stderr
// calza con la regex. El padre sigue vivo y corre el resto de la suite.
TEST(AbortDeathTest, AbortIsCaught) { EXPECT_DEATH(report_and_abort(), "fallo detectado"); }

} // namespace
