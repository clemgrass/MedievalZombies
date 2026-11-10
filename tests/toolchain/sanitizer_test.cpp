// Los sanitizers del preset debug detectan y abortan. Solo se compila con MZ_SANITIZE:
// sin sanitizers, estas sentencias son comportamiento indefinido silencioso y los tests
// fallarían. La suite termina en SanitizerDeathTest, y por eso llevan la etiqueta
// `sanitizer`.

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <limits>

namespace {

// `volatile` impide que el compilador vea los valores y resuelva el error al compilar:
// tiene que pasar en ejecución, donde lo ve el sanitizer.
//
// La lectura va por un puntero crudo: sin optimizar, libstdc++ (desde GCC 15) revisa el
// rango de std::array::operator[] con _GLIBCXX_ASSERTIONS y aborta antes de que ASan vea
// el acceso. Por el puntero no hay revisión de la biblioteca, que es lo que ASan cubre.
int read_at(std::size_t index) {
    const std::array<int, 4> values = {1, 2, 3, 4};
    const int *data = values.data();
    return data[index];
}

int add(int a, int b) { return a + b; }

TEST(SanitizerDeathTest, AsanCatchesOutOfBoundsRead) {
    volatile std::size_t index = 4;
    EXPECT_DEATH(static_cast<void>(read_at(index)), "AddressSanitizer: stack-buffer-overflow");
}

TEST(SanitizerDeathTest, UbsanCatchesSignedOverflow) {
    volatile int big = std::numeric_limits<int>::max();
    EXPECT_DEATH(static_cast<void>(add(big, 1)), "runtime error: signed integer overflow");
}

} // namespace
