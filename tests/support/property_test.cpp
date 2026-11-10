// Tests del soporte de los tests de propiedades: el generador y la semilla.

#include "mz/test/property.h"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <optional>

namespace {

using mz::test::pcg32;
using mz::test::property_rng;

// Fija (o quita) el override de la semilla mientras vive, y al salir deja el que había:
// así estos tests no pisan el --seed con que se corrió el ejecutable.
class seed_override_guard {
public:
    explicit seed_override_guard(std::optional<std::uint64_t> seed)
        : m_saved(mz::test::seed_override()) {
        apply(seed);
    }
    ~seed_override_guard() { apply(m_saved); }

    seed_override_guard(const seed_override_guard &) = delete;
    seed_override_guard &operator=(const seed_override_guard &) = delete;
    seed_override_guard(seed_override_guard &&) = delete;
    seed_override_guard &operator=(seed_override_guard &&) = delete;

private:
    static void apply(std::optional<std::uint64_t> seed) {
        if (seed.has_value()) {
            mz::test::set_seed_override(*seed);
        } else {
            mz::test::clear_seed_override();
        }
    }

    std::optional<std::uint64_t> m_saved;
};

// Valores de referencia del autor de PCG: la demo de pcg-c-basic con semilla 42 y
// secuencia 54 imprime estos seis primeros números. Si el generador los reproduce, es el
// PCG32 publicado y no una variante parecida.
TEST(Pcg32Test, MatchesReferenceSequence) {
    pcg32 rng(42U, 54U);
    constexpr std::array<std::uint32_t, 6> expected = {
        0xa15c02b7U, 0x7b47f409U, 0xba1d3330U, 0x83d2f293U, 0xbfa4784bU, 0xcbed606eU,
    };
    for (const std::uint32_t value : expected) {
        EXPECT_EQ(rng.next_u32(), value);
    }
}

TEST(Pcg32Test, DifferentSeedsGiveDifferentSequences) {
    pcg32 a(1U);
    pcg32 b(2U);
    EXPECT_NE(a.next_u32(), b.next_u32());
}

// La semilla por defecto es el FNV-1a de 64 bits del nombre del test. Valores de
// referencia de la especificación de FNV.
TEST(DefaultSeedTest, IsFnv1aOfTheName) {
    EXPECT_EQ(mz::test::default_seed(""), 0xcbf29ce484222325U);
    EXPECT_EQ(mz::test::default_seed("a"), 0xaf63dc4c8601ec8cU);
    EXPECT_EQ(mz::test::default_seed("foobar"), 0x85944171f73967e8U);
}

TEST(PropertyRngTest, DefaultSeedComesFromTheTestName) {
    const seed_override_guard guard(std::nullopt);
    const property_rng rng;
    EXPECT_EQ(rng.seed(),
              mz::test::default_seed("PropertyRngTest.DefaultSeedComesFromTheTestName"));
}

TEST(PropertyRngTest, SeedOverrideWins) {
    const seed_override_guard guard(1234U);
    const property_rng rng;
    EXPECT_EQ(rng.seed(), 1234U);
}

TEST(PropertyRngTest, SameSeedSameSequence) {
    const seed_override_guard guard(99U);
    property_rng a;
    property_rng b;
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(a.next_u32(), b.next_u32());
    }
}

// Un test de propiedades del propio generador: con cualquier semilla, cada float cae en
// [lo, hi).
TEST(PropertyRngProperty, NextFloatStaysInRange) {
    property_rng rng;
    for (int i = 0; i < 10000; ++i) {
        const float value = rng.next_float(-2.5F, 7.0F);
        ASSERT_GE(value, -2.5F);
        ASSERT_LT(value, 7.0F);
    }
}

} // namespace
