#include "mz/core/arena.h"
#include "mz/core/handle.h"
#include "mz/core/pool.h"
#include "mz/core/region.h"
#include "mz/test/property.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using mz::core::arena;
using mz::core::handle;
using mz::core::pool;
using mz::core::pool_error;
using mz::core::region;

constexpr std::size_t kib = std::size_t{1} << 10U;

arena make_arena(std::string_view name, std::size_t size) {
    auto created = region::create(name, size);
    MZ_CHECK(created.has_value(), "el test no pudo crear la región \"{}\"", name);
    return arena{std::move(created).value()};
}

// Un objeto chico, como los que viven en los pools del juego.
struct piece {
    int m_id = 0;
    float m_mass = 0.0F;
};

// Crea en un pool que el test sabe que tiene lugar.
handle<piece> create_piece(pool<piece> &p, int id) {
    auto created = p.create(piece{.m_id = id, .m_mass = 1.0F});
    MZ_CHECK(created.has_value(), "el test esperaba lugar en el pool \"{}\"", p.name());
    return created.value();
}

// Cuenta sus destrucciones en un contador de afuera.
class tracked {
public:
    explicit tracked(int &destroyed) : m_destroyed(&destroyed) {}
    tracked(const tracked &) = delete;
    tracked &operator=(const tracked &) = delete;
    tracked(tracked &&) = delete;
    tracked &operator=(tracked &&) = delete;
    ~tracked() { ++*m_destroyed; }

private:
    int *m_destroyed;
};

TEST(PoolTest, StartsEmptyWithItsCapacityAndName) {
    arena memory = make_arena("pool", 4 * kib);
    const pool<piece> p{memory, 20, "pedidos de corte"};
    EXPECT_EQ(p.size(), 0U);
    EXPECT_EQ(p.capacity(), 20U);
    EXPECT_EQ(p.peak(), 0U);
    EXPECT_TRUE(p.has_room());
    EXPECT_EQ(p.name(), "pedidos de corte");
}

TEST(PoolTest, CreateGivesAHandleToTheNewObject) {
    arena memory = make_arena("pool", 4 * kib);
    pool<piece> p{memory, 4, "crear"};
    const handle<piece> h = create_piece(p, 42);
    EXPECT_FALSE(h.is_null());
    ASSERT_NE(p.get(h), nullptr);
    EXPECT_EQ(p.get(h)->m_id, 42);
    EXPECT_EQ(p.size(), 1U);
}

// create recibe los argumentos del constructor de T y lo construye en el slot.
TEST(PoolTest, CreateConstructsInPlace) {
    arena memory = make_arena("pool", 4 * kib);
    int destroyed = 0;
    pool<tracked> p{memory, 2, "en su lugar"};
    const auto created = p.create(destroyed);
    ASSERT_TRUE(created.has_value());
    EXPECT_EQ(destroyed, 0);
}

// Los slots se entregan en orden la primera vez: el orden depende solo de la secuencia de
// crear y destruir (determinismo).
TEST(PoolTest, FirstHandlesTakeTheSlotsInOrder) {
    arena memory = make_arena("pool", 4 * kib);
    pool<piece> p{memory, 3, "orden"};
    EXPECT_EQ(create_piece(p, 0).index(), 0U);
    EXPECT_EQ(create_piece(p, 1).index(), 1U);
    EXPECT_EQ(create_piece(p, 2).index(), 2U);
}

// Desde const se puede leer, no escribir.
TEST(PoolTest, ConstPoolReadsThroughAHandle) {
    arena memory = make_arena("pool", 4 * kib);
    pool<piece> p{memory, 2, "const"};
    const handle<piece> h = create_piece(p, 5);
    const pool<piece> &read_only = p;
    ASSERT_NE(read_only.get(h), nullptr);
    EXPECT_EQ(read_only.get(h)->m_id, 5);
    p.destroy(h);
    EXPECT_EQ(read_only.get(h), nullptr);
}

TEST(PoolTest, DestroyRunsTheDestructorAndFreesTheSlot) {
    arena memory = make_arena("pool", 4 * kib);
    int destroyed = 0;
    pool<tracked> p{memory, 1, "destruir"};
    const handle<tracked> h = p.create(destroyed).value();
    EXPECT_FALSE(p.has_room());
    p.destroy(h);
    EXPECT_EQ(destroyed, 1);
    EXPECT_EQ(p.size(), 0U);
    EXPECT_TRUE(p.has_room());
}

// El criterio del ticket: el slot liberado se reusa, la generación sube, y el handle viejo
// (mismo índice) ya no llega al objeto nuevo.
TEST(PoolTest, AReusedSlotInvalidatesTheOldHandle) {
    arena memory = make_arena("pool", 4 * kib);
    pool<piece> p{memory, 4, "reuso"};
    const handle<piece> old = create_piece(p, 1);
    p.destroy(old);
    const handle<piece> reused = create_piece(p, 2);

    EXPECT_EQ(reused.index(), old.index());
    EXPECT_GT(reused.generation(), old.generation());
    EXPECT_EQ(p.get(old), nullptr);
    ASSERT_NE(p.get(reused), nullptr);
    EXPECT_EQ(p.get(reused)->m_id, 2);
}

// El slot que se reusa es el último que se devolvió: probablemente sigue en la caché.
TEST(PoolTest, ReusesTheLastFreedSlotFirst) {
    arena memory = make_arena("pool", 4 * kib);
    pool<piece> p{memory, 4, "LIFO"};
    const handle<piece> a = create_piece(p, 0);
    const handle<piece> b = create_piece(p, 1);
    (void)create_piece(p, 2);
    p.destroy(a);
    p.destroy(b);
    EXPECT_EQ(create_piece(p, 3).index(), b.index());
    EXPECT_EQ(create_piece(p, 4).index(), a.index());
}

// Un handle nulo, uno fuera de rango, o uno inventado con la generación del slot libre no
// llegan nunca a un slot: un slot libre guarda la lista libre, no un T.
TEST(PoolTest, HandlesThatThePoolDidNotGiveFindNothing) {
    arena memory = make_arena("pool", 4 * kib);
    pool<piece> p{memory, 2, "inventados"};
    const handle<piece> live = create_piece(p, 1);
    EXPECT_EQ(p.get(handle<piece>{}), nullptr);
    EXPECT_EQ(p.get(handle<piece>{2, live.generation()}), nullptr);
    // El slot 1 nunca se usó: está libre con la generación 0.
    EXPECT_EQ(p.get(handle<piece>{1, 0}), nullptr);
    // El slot 0, ya devuelto, está libre con la generación del handle más uno.
    p.destroy(live);
    EXPECT_EQ(p.get(handle<piece>{0, live.generation() + 1}), nullptr);
}

// El criterio del ticket: lleno, has_room() da falso y create() devuelve un error en vez de
// abortar. El juego degrada con su regla.
TEST(PoolTest, CreateWhenFullReturnsAnError) {
    arena memory = make_arena("pool", 4 * kib);
    pool<piece> p{memory, 2, "lleno"};
    (void)create_piece(p, 1);
    (void)create_piece(p, 2);
    EXPECT_FALSE(p.has_room());
    const auto rejected = p.create(piece{});
    ASSERT_FALSE(rejected.has_value());
    EXPECT_EQ(rejected.error(), pool_error::full);
    EXPECT_EQ(p.size(), 2U);
}

// Cada create() rechazado cuenta: es cuántas veces degradó el pool, para medirlo.
TEST(PoolTest, FailuresCountsEveryRejectedCreate) {
    arena memory = make_arena("pool", 4 * kib);
    pool<piece> p{memory, 1, "fallos"};
    EXPECT_EQ(p.failures(), 0U);
    const handle<piece> h = create_piece(p, 1);
    (void)p.create(piece{});
    (void)p.create(piece{});
    EXPECT_EQ(p.failures(), 2U);
    // Un create que entra no cuenta, y lo contado no se borra al liberar.
    p.destroy(h);
    (void)create_piece(p, 2);
    EXPECT_EQ(p.failures(), 2U);
}

TEST(PoolTest, PeakKeepsTheMostThatWasAliveAtOnce) {
    arena memory = make_arena("pool", 4 * kib);
    pool<piece> p{memory, 4, "pico"};
    const handle<piece> a = create_piece(p, 1);
    const handle<piece> b = create_piece(p, 2);
    const handle<piece> c = create_piece(p, 3);
    p.destroy(a);
    p.destroy(b);
    p.destroy(c);
    (void)create_piece(p, 4);
    EXPECT_EQ(p.size(), 1U);
    EXPECT_EQ(p.peak(), 3U);
}

TEST(PoolTest, DestructorDestroysTheLiveObjects) {
    arena memory = make_arena("pool", 4 * kib);
    int destroyed = 0;
    {
        pool<tracked> p{memory, 3, "al cerrar"};
        (void)p.create(destroyed).value();
        const handle<tracked> freed = p.create(destroyed).value();
        (void)p.create(destroyed).value();
        p.destroy(freed);
        EXPECT_EQ(destroyed, 1);
    }
    // Los dos vivos, no el que ya se había devuelto.
    EXPECT_EQ(destroyed, 3);
}

// Después de crearse, el pool no le pide nada más a su arena (el allocator que cuenta).
TEST(PoolTest, NeverAllocatesAfterCreation) {
    arena memory = make_arena("pool", 4 * kib);
    pool<piece> p{memory, 16, "sin crecer"};
    const std::size_t after_creation = memory.used();
    std::vector<handle<piece>> live;
    for (int round = 0; round < 3; ++round) {
        while (p.has_room()) {
            live.push_back(create_piece(p, round));
        }
        for (const handle<piece> h : live) {
            p.destroy(h);
        }
        live.clear();
    }
    EXPECT_EQ(memory.used(), after_creation);
}

// Mover se lleva los slots: los handles siguen valiendo en el pool nuevo.
TEST(PoolTest, MoveKeepsTheHandlesValid) {
    arena memory = make_arena("pool", 4 * kib);
    pool<piece> source{memory, 2, "mover"};
    const handle<piece> h = create_piece(source, 8);

    pool<piece> target{std::move(source)};
    ASSERT_NE(target.get(h), nullptr);
    EXPECT_EQ(target.get(h)->m_id, 8);
    EXPECT_EQ(target.name(), "mover");
    EXPECT_EQ(target.capacity(), 2U);
    // NOLINTBEGIN(bugprone-use-after-move): el estado del origen movido es parte del contrato.
    EXPECT_EQ(source.capacity(), 0U);
    EXPECT_EQ(source.size(), 0U);
    EXPECT_FALSE(source.has_room());
    EXPECT_EQ(source.get(h), nullptr);
    EXPECT_EQ(source.failures(), 0U);
    // NOLINTEND(bugprone-use-after-move)
}

// Destruir con un handle viejo es destruir dos veces: un bug, en todos los builds.
TEST(PoolDeathTest, DestroyWithAStaleHandleAbortsWithTheName) {
    arena memory = make_arena("pool", 4 * kib);
    pool<piece> p{memory, 2, "pedazos"};
    const handle<piece> h = create_piece(p, 1);
    p.destroy(h);
    EXPECT_DEATH(p.destroy(h), "pool \"pedazos\": destroy de un handle que no está vivo");
}

TEST(PoolDeathTest, DestroyWithANullHandleAborts) {
    arena memory = make_arena("pool", 4 * kib);
    pool<piece> p{memory, 2, "nulo"};
    EXPECT_DEATH(p.destroy(handle<piece>{}), "pool \"nulo\": destroy de un handle");
}

// Secuencias al azar de crear y destruir contra un modelo simple: una lista de lo vivo (con
// el valor que se guardó) y otra de los handles ya devueltos.
TEST(PoolProperty, BehavesLikeAListOfLiveObjects) {
    mz::test::property_rng rng;
    arena memory = make_arena("propiedad", 4 * kib);
    // Chico, para que se llene y reuse slots muchas veces.
    constexpr std::uint32_t capacity = 8;
    pool<piece> p{memory, capacity, "modelo"};

    struct live_object {
        handle<piece> m_handle;
        int m_id;
    };
    std::vector<live_object> live;
    std::vector<handle<piece>> dead;
    std::size_t peak = 0;
    std::uint32_t failures = 0;

    for (int step = 0; step < 2000; ++step) {
        const bool create = rng.next_u32() % 2U == 0U;
        if (create) {
            const int id = static_cast<int>(rng.next_u32() % 1000U);
            auto created = p.create(piece{.m_id = id});
            ASSERT_EQ(created.has_value(), live.size() < capacity) << "paso " << step;
            if (created.has_value()) {
                live.push_back({.m_handle = created.value(), .m_id = id});
            } else {
                ++failures;
            }
        } else if (!live.empty()) {
            const std::size_t victim = rng.next_u32() % live.size();
            p.destroy(live[victim].m_handle);
            dead.push_back(live[victim].m_handle);
            live.erase(live.begin() + static_cast<std::ptrdiff_t>(victim));
        }
        peak = std::max(peak, live.size());

        ASSERT_EQ(p.size(), live.size()) << "paso " << step;
        ASSERT_EQ(p.peak(), peak) << "paso " << step;
        ASSERT_EQ(p.failures(), failures) << "paso " << step;
        ASSERT_EQ(p.has_room(), live.size() < capacity) << "paso " << step;
        for (const live_object &object : live) {
            const piece *found = p.get(object.m_handle);
            ASSERT_NE(found, nullptr) << "paso " << step;
            ASSERT_EQ(found->m_id, object.m_id) << "paso " << step;
        }
        for (const handle<piece> old : dead) {
            ASSERT_EQ(p.get(old), nullptr) << "paso " << step;
        }
    }
}

} // namespace
