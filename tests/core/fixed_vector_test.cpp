#include "mz/core/arena.h"
#include "mz/core/fixed_vector.h"
#include "mz/core/region.h"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <utility>

namespace {

using mz::core::arena;
using mz::core::fixed_vector;
using mz::core::region;

constexpr std::size_t kib = std::size_t{1} << 10U;

arena make_arena(std::string_view name, std::size_t size) {
    auto created = region::create(name, size);
    MZ_CHECK(created.has_value(), "el test no pudo crear la región \"{}\"", name);
    return arena{std::move(created).value()};
}

// Cuenta sus destrucciones en un contador de afuera, para ver cuándo el vector destruye.
class tracked {
public:
    tracked(int value, int &destroyed) : m_value(value), m_destroyed(&destroyed) {}
    tracked(const tracked &) = default;
    tracked &operator=(const tracked &) = default;
    tracked(tracked &&) = default;
    tracked &operator=(tracked &&) = default;
    ~tracked() { ++*m_destroyed; }

    [[nodiscard]] int value() const { return m_value; }

private:
    int m_value;
    int *m_destroyed;
};

TEST(FixedVectorTest, StartsEmptyWithItsCapacityAndName) {
    arena memory = make_arena("vector", 4 * kib);
    const fixed_vector<int> v{memory, 10, "temporales"};
    EXPECT_EQ(v.size(), 0U);
    EXPECT_EQ(v.capacity(), 10U);
    EXPECT_TRUE(v.empty());
    EXPECT_FALSE(v.full());
    EXPECT_EQ(v.name(), "temporales");
}

// Al crearse pide a la arena lugar para toda su capacidad, alineado para T.
TEST(FixedVectorTest, TakesItsWholeCapacityFromTheArena) {
    arena memory = make_arena("vector", 4 * kib);
    (void)memory.allocate(1, 1);
    struct alignas(64) wide {
        std::array<std::byte, 64> m_bytes;
    };
    const fixed_vector<wide> v{memory, 3, "anchos"};
    // 1 byte, 63 de relleno hasta el 64 y 3 × 64.
    EXPECT_EQ(memory.used(), 64U + (3U * 64U));
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(v.data()) % 64, 0U);
}

TEST(FixedVectorTest, PushBackKeepsTheOrder) {
    arena memory = make_arena("vector", 4 * kib);
    fixed_vector<int> v{memory, 4, "orden"};
    v.push_back(10);
    v.push_back(20);
    const int thirty = 30;
    v.push_back(thirty);
    ASSERT_EQ(v.size(), 3U);
    EXPECT_EQ(v[0], 10);
    EXPECT_EQ(v[1], 20);
    EXPECT_EQ(v[2], 30);
}

TEST(FixedVectorTest, EmplaceBackConstructsInPlaceAndReturnsTheElement) {
    arena memory = make_arena("vector", 4 * kib);
    int destroyed = 0;
    fixed_vector<tracked> v{memory, 2, "en su lugar"};
    tracked &added = v.emplace_back(7, destroyed);
    EXPECT_EQ(&added, &v[0]);
    EXPECT_EQ(added.value(), 7);
    // Se construyó directo en el vector: no hubo un temporal que destruir.
    EXPECT_EQ(destroyed, 0);
}

TEST(FixedVectorTest, FullWhenSizeReachesCapacity) {
    arena memory = make_arena("vector", 4 * kib);
    fixed_vector<int> v{memory, 2, "lleno"};
    v.push_back(1);
    EXPECT_FALSE(v.full());
    v.push_back(2);
    EXPECT_TRUE(v.full());
}

// Un vector de capacidad 0 existe (un sistema sin nada que guardar en esta configuración) y
// está lleno desde el principio.
TEST(FixedVectorTest, ZeroCapacityIsFullFromTheStart) {
    arena memory = make_arena("vector", 4 * kib);
    const fixed_vector<int> v{memory, 0, "vacío"};
    EXPECT_TRUE(v.empty());
    EXPECT_TRUE(v.full());
}

TEST(FixedVectorTest, PopBackDestroysTheLastElement) {
    arena memory = make_arena("vector", 4 * kib);
    int destroyed = 0;
    fixed_vector<tracked> v{memory, 2, "pop"};
    v.emplace_back(1, destroyed);
    v.emplace_back(2, destroyed);
    v.pop_back();
    EXPECT_EQ(destroyed, 1);
    ASSERT_EQ(v.size(), 1U);
    EXPECT_EQ(v[0].value(), 1);
}

TEST(FixedVectorTest, ClearDestroysEveryElementAndKeepsTheCapacity) {
    arena memory = make_arena("vector", 4 * kib);
    int destroyed = 0;
    fixed_vector<tracked> v{memory, 3, "clear"};
    v.emplace_back(1, destroyed);
    v.emplace_back(2, destroyed);
    v.emplace_back(3, destroyed);
    v.clear();
    EXPECT_EQ(destroyed, 3);
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(v.capacity(), 3U);
}

TEST(FixedVectorTest, DestructorDestroysTheElements) {
    arena memory = make_arena("vector", 4 * kib);
    int destroyed = 0;
    {
        fixed_vector<tracked> v{memory, 2, "destructor"};
        v.emplace_back(1, destroyed);
        v.emplace_back(2, destroyed);
    }
    EXPECT_EQ(destroyed, 2);
}

// El criterio del ticket: después de crearse, el vector no le pide nada más a su
// allocator. La arena cuenta lo que entrega (used), así que es el allocator que cuenta.
TEST(FixedVectorTest, NeverAllocatesAfterCreation) {
    arena memory = make_arena("vector", 4 * kib);
    fixed_vector<int> v{memory, 100, "sin crecer"};
    const std::size_t after_creation = memory.used();
    for (int round = 0; round < 3; ++round) {
        while (!v.full()) {
            v.push_back(round);
        }
        v.pop_back();
        v.clear();
    }
    EXPECT_EQ(memory.used(), after_creation);
    EXPECT_EQ(memory.peak(), after_creation);
}

TEST(FixedVectorTest, IteratesInOrderWithRangeFor) {
    arena memory = make_arena("vector", 4 * kib);
    fixed_vector<int> v{memory, 4, "for"};
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    int sum = 0;
    int expected = 1;
    for (int &value : v) {
        EXPECT_EQ(value, expected++);
        sum += value;
    }
    EXPECT_EQ(sum, 6);

    const fixed_vector<int> &read_only = v;
    int count = 0;
    for (const int value : read_only) {
        count += value;
    }
    EXPECT_EQ(count, 6);
}

// La vista con std::span ve solo lo ocupado, no la capacidad.
TEST(FixedVectorTest, SpanSeesOnlyTheElements) {
    arena memory = make_arena("vector", 4 * kib);
    fixed_vector<int> v{memory, 8, "span"};
    v.push_back(4);
    v.push_back(5);
    const std::span<int> view = v.span();
    EXPECT_EQ(view.size(), 2U);
    EXPECT_EQ(view.data(), v.data());

    const fixed_vector<int> &read_only = v;
    const std::span<const int> const_view = read_only.span();
    EXPECT_EQ(const_view.size(), 2U);
    EXPECT_EQ(const_view[1], 5);
}

// Mover pasa los elementos sin copiarlos y deja el origen vacío y sin capacidad.
TEST(FixedVectorTest, MoveTakesTheElementsAndLeavesTheSourceEmpty) {
    arena memory = make_arena("vector", 4 * kib);
    int destroyed = 0;
    fixed_vector<tracked> source{memory, 2, "mover"};
    source.emplace_back(9, destroyed);
    const tracked *element = &source[0];

    fixed_vector<tracked> target{std::move(source)};
    EXPECT_EQ(&target[0], element);
    EXPECT_EQ(target.size(), 1U);
    EXPECT_EQ(target.capacity(), 2U);
    EXPECT_EQ(target.name(), "mover");
    EXPECT_EQ(destroyed, 0);
    // NOLINTBEGIN(bugprone-use-after-move): el estado del origen movido es parte del contrato.
    EXPECT_TRUE(source.empty());
    EXPECT_EQ(source.capacity(), 0U);
    EXPECT_TRUE(source.full());
    // NOLINTEND(bugprone-use-after-move)
}

TEST(FixedVectorDeathTest, PushBackWhenFullAbortsWithTheName) {
    arena memory = make_arena("vector", 4 * kib);
    fixed_vector<int> v{memory, 2, "pedidos de corte"};
    v.push_back(1);
    v.push_back(2);
    EXPECT_DEATH(v.push_back(3), "fixed_vector \"pedidos de corte\" lleno.*capacidad 2");
}

TEST(FixedVectorDeathTest, IndexPastTheSizeAborts) {
    arena memory = make_arena("vector", 4 * kib);
    fixed_vector<int> v{memory, 4, "índice"};
    v.push_back(1);
    // El 1 está dentro de la capacidad, pero no hay un elemento ahí.
    EXPECT_DEATH((void)v[1], "fixed_vector \"índice\": índice 1 con 1 elementos");
    const fixed_vector<int> &read_only = v;
    EXPECT_DEATH((void)read_only[1], "índice 1 con 1 elementos");
}

TEST(FixedVectorDeathTest, PopBackWhenEmptyAborts) {
    arena memory = make_arena("vector", 4 * kib);
    fixed_vector<int> v{memory, 4, "pop vacío"};
    EXPECT_DEATH(v.pop_back(), "fixed_vector \"pop vacío\": pop_back sin elementos");
}

} // namespace
