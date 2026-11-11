// MZ_ASSERT y MZ_CHECK con los asserts encendidos (sin NDEBUG). Los tests corren en Debug,
// así que aquí MZ_ASSERT está activo. Cómo se apaga MZ_ASSERT está en
// assert_ndebug_test.cpp.

#include "mz/core/assert.h"

#include <gtest/gtest.h>

#include <string>

namespace {

// La línea del MZ_ASSERT de la función de abajo. Si alguien reformatea estas dos líneas, el
// test de la línea falla y hay que ajustar el + 1.
constexpr int known_line = __LINE__ + 1;
void fail_on_known_line() { MZ_ASSERT(false, "en una línea conocida"); }

TEST(AssertDeathTest, FailingAssertPrintsTheCondition) {
    const int lives = 2;
    EXPECT_DEATH(MZ_ASSERT(lives == 3, "vidas"), "lives == 3");
}

TEST(AssertDeathTest, FailingAssertPrintsTheFormattedMessage) {
    const int lives = 2;
    EXPECT_DEATH(MZ_ASSERT(lives == 3, "esperaba {} vidas y hay {}", 3, lives),
                 "esperaba 3 vidas y hay 2");
}

TEST(AssertDeathTest, FailingAssertPrintsFileAndLine) {
    EXPECT_DEATH(fail_on_known_line(), "assert_test\\.cpp:" + std::to_string(known_line));
}

TEST(AssertDeathTest, FailingAssertPrintsTheFunction) {
    EXPECT_DEATH(fail_on_known_line(), "fail_on_known_line");
}

// El mensaje se formatea en un buffer fijo de la pila: si no cabe, se corta y lo dice, en
// vez de pedir memoria o escribir de más.
TEST(AssertDeathTest, LongMessageIsTruncatedAndMarked) {
    const std::string long_text(4096, 'x');
    EXPECT_DEATH(MZ_ASSERT(false, "{}", long_text), "x\\[mensaje cortado\\]");
}

TEST(AssertTest, PassingAssertDoesNothing) {
    int evaluations = 0;
    MZ_ASSERT(++evaluations == 1, "la condición se evalúa una sola vez");
    EXPECT_EQ(evaluations, 1);
}

TEST(CheckDeathTest, FailingCheckAbortsWithItsMessage) {
    const int used = 5;
    EXPECT_DEATH(MZ_CHECK(used <= 4, "región llena: {} de {}", used, 4), "región llena: 5 de 4");
}

TEST(CheckTest, PassingCheckDoesNothing) {
    int evaluations = 0;
    MZ_CHECK(++evaluations == 1, "la condición se evalúa una sola vez");
    EXPECT_EQ(evaluations, 1);
}

} // namespace
