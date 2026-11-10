#pragma once

// Soporte de los tests de propiedades: un generador con semilla fija que se imprime al
// fallar.
//
// Un test de propiedades genera muchas entradas aleatorias y verifica que una propiedad
// se cumpla en todas. Para que una falla se pueda reproducir, la secuencia sale de una
// semilla fija y conocida:
//
//   TEST(Vec3Property, NormalizeHasUnitLength) {
//       mz::test::property_rng rng;
//       for (int i = 0; i < 1000; ++i) {
//           const float x = rng.next_float(-100.0F, 100.0F);
//           ...
//       }
//   }
//
// Mientras `rng` existe, cualquier aserción que falle imprime la semilla y el comando que
// reproduce la corrida (`--seed=N`). El nombre de la suite termina en `Property`: así
// CTest le pone la etiqueta `property` (cmake/mz_testing.cmake).

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <source_location>
#include <string_view>

namespace mz::test {

/// PCG32 (PCG-XSH-RR 64/32), de Melissa O'Neill (2014): un LCG de 64 bits cuya salida se
/// mezcla con un xorshift y una rotación que depende del propio estado. Es chico, rápido
/// y pasa las baterías estadísticas TestU01. Es propio, en vez de <random>, porque el
/// estándar fija la salida de los motores pero no la de las distribuciones: con la misma
/// semilla, libstdc++ y libc++ darían floats distintos.
class pcg32 {
public:
    /// `stream` elige una de 2^63 secuencias independientes para la misma semilla.
    explicit pcg32(std::uint64_t seed, std::uint64_t stream = 54U);

    std::uint32_t next_u32();

private:
    std::uint64_t m_state = 0;
    std::uint64_t m_increment = 0;
};

/// Semilla por defecto de un test: el hash FNV-1a de 64 bits de su nombre completo
/// ("Suite.Test"). Cada test tiene su propia secuencia, y agregar un test no cambia las
/// de los demás.
std::uint64_t default_seed(std::string_view test_name);

/// La semilla de `--seed=N`, que reemplaza a la por defecto en todos los tests de la
/// corrida. La fija el main de los tests.
void set_seed_override(std::uint64_t seed);
void clear_seed_override();
[[nodiscard]] std::optional<std::uint64_t> seed_override();

/// Nombre del ejecutable, para armar el comando que reproduce una falla. Lo fija el main.
void set_executable_name(std::string_view name);

/// El generador de un test de propiedades. Toma la semilla del test que está corriendo
/// y, mientras existe, la agrega a cada aserción que falla.
class property_rng {
public:
    /// `where` es la línea del test que crea el generador; el reporte la muestra.
    explicit property_rng(std::source_location where = std::source_location::current());

    [[nodiscard]] std::uint64_t seed() const { return m_seed; }

    std::uint32_t next_u32() { return m_engine.next_u32(); }

    /// Un float uniforme en [lo, hi). Necesita lo < hi.
    float next_float(float lo, float hi);

private:
    std::uint64_t m_seed;
    // ScopedTrace agrega su mensaje a toda aserción que falle mientras vive. Como miembro,
    // vive lo mismo que el generador: el cuerpo entero del test.
    ::testing::ScopedTrace m_trace;
    pcg32 m_engine;
};

} // namespace mz::test
