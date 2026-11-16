#include "mz/core/arena.h"
#include "mz/core/region.h"
#include "mz/test/property.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using mz::core::arena;
using mz::core::region;

constexpr std::size_t kib = std::size_t{1} << 10U;

// Una arena sobre una región nueva. Los tests no prueban la región (tiene los suyos): si el
// sistema operativo no la da, el test no tiene sentido y se corta ahí.
arena make_arena(std::string_view name, std::size_t size) {
    auto created = region::create(name, size);
    MZ_CHECK(created.has_value(), "el test no pudo crear la región \"{}\"", name);
    return arena{std::move(created).value()};
}

std::uintptr_t address(const std::byte *p) { return reinterpret_cast<std::uintptr_t>(p); }

TEST(ArenaTest, TakesTheNameAndCapacityOfItsRegion) {
    const arena a = make_arena("arena del paso", 4 * kib);
    EXPECT_EQ(a.name(), "arena del paso");
    EXPECT_EQ(a.capacity(), 4 * kib);
    EXPECT_EQ(a.used(), 0U);
}

// Cada alineación de 1 a 64, después de un pedido de 1 byte que deja el offset impar.
TEST(ArenaTest, EveryAllocationHasTheRequestedAlignment) {
    arena a = make_arena("alineación", 4 * kib);
    for (std::size_t alignment = 1; alignment <= 64; alignment *= 2) {
        (void)a.allocate(1, 1);
        const std::byte *p = a.allocate(3, alignment);
        EXPECT_EQ(address(p) % alignment, 0U) << "alineación " << alignment;
    }
}

TEST(ArenaTest, UsedCountsTheAlignmentPadding) {
    arena a = make_arena("relleno", 4 * kib);
    (void)a.allocate(1, 1);
    (void)a.allocate(8, 16);
    // 1 byte, 15 de relleno hasta el 16 y 8 más: la región tiene ocupados 24.
    EXPECT_EQ(a.used(), 24U);
}

// Un pedido de 0 bytes no ocupa nada: un contenedor de capacidad 0 lo va a pedir.
TEST(ArenaTest, ZeroSizedAllocationDoesNotAdvance) {
    arena a = make_arena("cero", 4 * kib);
    (void)a.allocate(4, 4);
    const std::byte *p = a.allocate(0, 4);
    EXPECT_EQ(address(p) % 4, 0U);
    EXPECT_EQ(a.used(), 4U);
}

// Llenarla justo hasta el último byte no aborta.
TEST(ArenaTest, CanBeFilledToTheLastByte) {
    arena a = make_arena("justa", 4 * kib);
    (void)a.allocate(4 * kib - 1, 1);
    (void)a.allocate(1, 1);
    EXPECT_EQ(a.used(), 4 * kib);
}

TEST(ArenaTest, PeakIsTheMostItEverUsed) {
    arena a = make_arena("pico", 4 * kib);
    EXPECT_EQ(a.peak(), 0U);
    (void)a.allocate(100, 1);
    EXPECT_EQ(a.peak(), 100U);
    (void)a.allocate(28, 4);
    EXPECT_EQ(a.peak(), 128U);
}

// Reiniciar empieza desde el principio de la región: el primer pedido vuelve a la misma
// dirección.
TEST(ArenaTest, ResetStartsOverFromTheBeginning) {
    arena a = make_arena("reinicio", 4 * kib);
    const std::byte *first = a.allocate(64, 16);
    (void)a.allocate(512, 16);
    a.reset();
    EXPECT_EQ(a.used(), 0U);
    EXPECT_EQ(a.allocate(64, 16), first);
}

// El pico es lo que dimensiona la región en presupuestos.md: un paso grande seguido de
// muchos chicos no lo pierde al reiniciar.
TEST(ArenaTest, PeakSurvivesTheReset) {
    arena a = make_arena("pico y reinicio", 4 * kib);
    (void)a.allocate(3000, 1);
    a.reset();
    (void)a.allocate(10, 1);
    EXPECT_EQ(a.peak(), 3000U);
}

TEST(ArenaDeathTest, OverflowAbortsWithTheNameAndThePeak) {
    arena a = make_arena("arena llena", 4 * kib);
    (void)a.allocate(4000, 1);
    a.reset();
    (void)a.allocate(100, 1);
    // Pide 4000 bytes con 100 ocupados: no cabe, y el pico (4000) es de antes del reinicio.
    EXPECT_DEATH((void)a.allocate(4000, 1), "arena \"arena llena\" llena.*pico 4000");
}

// Lo que falta por un solo byte también aborta: el límite es exacto.
TEST(ArenaDeathTest, OneByteTooManyAborts) {
    arena a = make_arena("un byte de más", 4 * kib);
    (void)a.allocate(4 * kib, 1);
    EXPECT_DEATH((void)a.allocate(1, 1), "un byte de más");
}

// El relleno de alineación también cuenta: 1 byte alineado a 64 con 4033 ocupados necesita
// llegar al 4096 + 1.
TEST(ArenaDeathTest, AlignmentPaddingThatDoesNotFitAborts) {
    arena a = make_arena("relleno de más", 4 * kib);
    (void)a.allocate(4 * kib - 63, 1);
    EXPECT_DEATH((void)a.allocate(1, 64), "relleno de más");
}

// Si el relleno solo ya pasa el final, ni un pedido de 0 bytes cabe. Con una capacidad que
// no es múltiplo de la alineación: 1 byte ocupado, alinear a 4096 cae en el 4096, pasado
// el final de los 3000.
TEST(ArenaDeathTest, PaddingThatGoesPastTheEndAborts) {
    auto created = region::create("relleno afuera", 3000);
    ASSERT_TRUE(created.has_value());
    arena a{std::move(created).value()};
    (void)a.allocate(1, 1);
    EXPECT_DEATH((void)a.allocate(0, 4096), "relleno afuera");
}

// Un tamaño cercano al máximo de size_t no da la vuelta y "cabe".
TEST(ArenaDeathTest, HugeSizeAbortsInsteadOfWrappingAround) {
    arena a = make_arena("enorme", 4 * kib);
    (void)a.allocate(16, 1);
    EXPECT_DEATH((void)a.allocate(~std::size_t{0} - 8, 1), "enorme");
}

TEST(ArenaDeathTest, AlignmentThatIsNotAPowerOfTwoAborts) {
    arena a = make_arena("alineación rara", 4 * kib);
    EXPECT_DEATH((void)a.allocate(8, 3), "potencia de 2");
    EXPECT_DEATH((void)a.allocate(8, 0), "potencia de 2");
    EXPECT_DEATH((void)a.allocate(8, 8192), "potencia de 2");
}

// Lo de antes del marcador sigue ocupado; lo de después vuelve a estar libre.
TEST(ArenaTest, RewindFreesOnlyWhatCameAfterTheMarker) {
    arena a = make_arena("marcador", 4 * kib);
    const std::byte *kept = a.allocate(100, 8);
    const arena::marker mark = a.mark();
    const std::byte *temporary = a.allocate(200, 8);
    a.rewind(mark);
    EXPECT_EQ(a.used(), 100U);
    // El siguiente pedido reusa el lugar del temporario, no el de lo que quedó.
    const std::byte *next = a.allocate(200, 8);
    EXPECT_EQ(next, temporary);
    EXPECT_NE(next, kept);
}

// Como una pila: cada función guarda su marcador al entrar y vuelve a él al salir.
TEST(ArenaTest, NestedMarkersUnwindInOrder) {
    arena a = make_arena("anidados", 4 * kib);
    (void)a.allocate(10, 1);
    const arena::marker outer = a.mark();
    (void)a.allocate(20, 1);
    const arena::marker inner = a.mark();
    (void)a.allocate(40, 1);

    a.rewind(inner);
    EXPECT_EQ(a.used(), 30U);
    a.rewind(outer);
    EXPECT_EQ(a.used(), 10U);
}

TEST(ArenaTest, RewindKeepsThePeak) {
    arena a = make_arena("pico y marcador", 4 * kib);
    const arena::marker mark = a.mark();
    (void)a.allocate(1000, 1);
    a.rewind(mark);
    EXPECT_EQ(a.peak(), 1000U);
}

// Un marcador que quedó adelante (de antes de un reset, o de un marcador interno usado
// después del externo) "liberaría" memoria que nunca se entregó.
TEST(ArenaDeathTest, RewindingForwardAborts) {
    arena a = make_arena("marcador adelante", 4 * kib);
    (void)a.allocate(100, 1);
    const arena::marker ahead = a.mark();
    a.reset();
    EXPECT_DEATH(a.rewind(ahead), "marcador adelante.*100.*0");
}

// Pedidos al azar (tamaño y alineación): ninguno se sale de la región ni pisa a otro.
TEST(ArenaProperty, AllocationsNeverOverlap) {
    mz::test::property_rng rng;
    // 500 pedidos de hasta 99 bytes más 63 de relleno caben siempre.
    auto created = region::create("propiedad", 128 * kib);
    ASSERT_TRUE(created.has_value());
    const region &block = created.value();
    // La arena se lleva la región: guardamos sus límites antes de moverla.
    const std::byte *const begin = block.data();
    const std::size_t size = block.size();
    arena a{std::move(created).value()};

    struct interval {
        std::uintptr_t m_first;
        std::uintptr_t m_last; // uno después del final
    };
    std::vector<interval> taken;
    for (int i = 0; i < 500; ++i) {
        const std::size_t bytes = rng.next_u32() % 100U;
        const std::size_t alignment = std::size_t{1} << (rng.next_u32() % 7U);
        const std::byte *p = a.allocate(bytes, alignment);
        ASSERT_EQ(address(p) % alignment, 0U);
        ASSERT_GE(address(p), address(begin));
        ASSERT_LE(address(p) + bytes, address(begin) + size);
        const interval s{.m_first = address(p), .m_last = address(p) + bytes};
        for (const interval &other : taken) {
            ASSERT_TRUE(s.m_last <= other.m_first || other.m_last <= s.m_first)
                << "el pedido " << i << " pisa a uno anterior";
        }
        taken.push_back(s);
    }
}

} // namespace
