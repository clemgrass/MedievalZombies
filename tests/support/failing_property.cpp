// Un test de propiedades que falla a propósito. No es parte de ninguna suite: lo corre
// solo el test support.property.prints_seed_and_reproduces, que revisa su salida.

#include "mz/test/property.h"

#include <gtest/gtest.h>

#include <cstdint>

namespace {

// Una "propiedad" falsa: la mitad de los números de 32 bits están por encima de 2^31.
TEST(FailingProperty, AlwaysBelowHalf) {
    mz::test::property_rng rng;
    for (int i = 0; i < 100; ++i) {
        const std::uint32_t value = rng.next_u32();
        ASSERT_LT(value, 0x80000000U) << "value = " << value;
    }
}

} // namespace
