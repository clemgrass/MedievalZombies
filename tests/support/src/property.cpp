#include "mz/test/property.h"

#include <bit>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string>

namespace mz::test {

namespace {

std::optional<std::uint64_t> override_seed;
std::string executable_name = "<ejecutable>";

// El nombre completo del test que está corriendo, "Suite.Test".
std::string current_test_name() {
    const ::testing::TestInfo *info = ::testing::UnitTest::GetInstance()->current_test_info();
    if (info == nullptr) {
        std::fputs("mz::test::property_rng se creó fuera de un test.\n", stderr);
        std::abort();
    }
    return std::string(info->test_suite_name()) + "." + info->name();
}

std::string trace_message(std::uint64_t seed) {
    const std::string seed_text = std::to_string(seed);
    return "semilla: " + seed_text + " (reproducir: " + executable_name +
           " --gtest_filter=" + current_test_name() + " --seed=" + seed_text + ")";
}

std::uint64_t current_seed() {
    return override_seed.has_value() ? *override_seed : default_seed(current_test_name());
}

} // namespace

// Inicialización de pcg32_srandom_r, de la implementación de referencia: el incremento
// del LCG tiene que ser impar para que el período sea el completo (2^64), y la semilla se
// suma entre dos pasos para que semillas parecidas no den salidas parecidas.
pcg32::pcg32(std::uint64_t seed, std::uint64_t stream) : m_increment((stream << 1U) | 1U) {
    next_u32();
    m_state += seed;
    next_u32();
}

std::uint32_t pcg32::next_u32() {
    constexpr std::uint64_t multiplier = 6364136223846793005U;
    const std::uint64_t old = m_state;
    m_state = old * multiplier + m_increment;
    // XSH-RR: un xorshift lleva los bits altos (los de mejor calidad en un LCG) a los 32
    // de abajo, y los 5 bits más altos eligen una rotación.
    const auto xorshifted = static_cast<std::uint32_t>(((old >> 18U) ^ old) >> 27U);
    const auto rotation = static_cast<int>(old >> 59U);
    return std::rotr(xorshifted, rotation);
}

// FNV-1a de 64 bits: por cada byte, xor y después multiplicación por el primo de FNV.
std::uint64_t default_seed(std::string_view test_name) {
    constexpr std::uint64_t offset_basis = 0xcbf29ce484222325U;
    constexpr std::uint64_t prime = 0x100000001b3U;
    std::uint64_t hash = offset_basis;
    for (const char c : test_name) {
        hash ^= static_cast<unsigned char>(c);
        hash *= prime;
    }
    return hash;
}

void set_seed_override(std::uint64_t seed) { override_seed = seed; }

void clear_seed_override() { override_seed.reset(); }

std::optional<std::uint64_t> seed_override() { return override_seed; }

void set_executable_name(std::string_view name) { executable_name = name; }

property_rng::property_rng(std::source_location where)
    : m_seed(current_seed()),
      m_trace(where.file_name(), static_cast<int>(where.line()), trace_message(m_seed)),
      m_engine(m_seed) {}

float property_rng::next_float(float lo, float hi) {
    assert(lo < hi);
    // Los 24 bits altos: los que caben exactos en la mantisa de un float. `unit` queda en
    // [0, 1), con paso 2^-24.
    const float unit = static_cast<float>(next_u32() >> 8U) * (1.0F / 16777216.0F);
    const float value = lo + (hi - lo) * unit;
    // El redondeo puede dar justo `hi`; en ese caso, el float anterior.
    return value < hi ? value : std::nextafter(hi, lo);
}

} // namespace mz::test
