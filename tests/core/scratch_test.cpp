#include "mz/core/arena.h"
#include "mz/core/assert.h"
#include "mz/core/region.h"
#include "mz/core/scratch.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <string_view>
#include <utility>

namespace {

using mz::core::arena;
using mz::core::region;
using mz::core::scratch_arenas;
using mz::core::scratch_scope;

constexpr std::size_t kib = std::size_t{1} << 10U;

region make_region(std::string_view name) {
    auto created = region::create(name, 4 * kib);
    MZ_CHECK(created.has_value(), "el test no pudo crear la región \"{}\"", name);
    return std::move(created).value();
}

TEST(ScratchScopeTest, GivesBackEverythingItTookWhenItEnds) {
    arena a{make_region("scope")};
    (void)a.allocate(100, 1);
    {
        scratch_scope temp{a};
        (void)temp.memory().allocate(500, 16);
        EXPECT_GT(a.used(), 100U);
    }
    EXPECT_EQ(a.used(), 100U);
}

TEST(ScratchScopeTest, IsOverTheArenaItWasGiven) {
    arena a{make_region("scope sobre a")};
    const scratch_scope temp{a};
    EXPECT_EQ(&temp.memory(), &a);
}

// Una función con su scope llama a otra que abre el suyo sobre la misma arena: cada una
// vuelve a su marcador.
TEST(ScratchScopeTest, NestedScopesUnwindInOrder) {
    arena a{make_region("scopes anidados")};
    {
        scratch_scope outer{a};
        (void)outer.memory().allocate(10, 1);
        {
            scratch_scope inner{a};
            (void)inner.memory().allocate(20, 1);
            EXPECT_EQ(a.used(), 30U);
        }
        EXPECT_EQ(a.used(), 10U);
    }
    EXPECT_EQ(a.used(), 0U);
}

TEST(ScratchArenasTest, WithNothingInUseGivesTheFirst) {
    scratch_arenas scratch{make_region("scratch 1"), make_region("scratch 2")};
    const scratch_scope temp = scratch.begin();
    EXPECT_EQ(&temp.memory(), &scratch.first());
}

// El caso de las dos scratch: una función recibe una scratch como arena de salida y además
// necesita una temporal. Si le tocara la misma, volver al marcador al salir borraría
// también lo que escribió en la salida después de ese marcador.
TEST(ScratchArenasTest, GivesTheOtherOneWhenOneIsInUse) {
    scratch_arenas scratch{make_region("scratch 1"), make_region("scratch 2")};
    {
        const scratch_scope temp = scratch.begin(scratch.first());
        EXPECT_EQ(&temp.memory(), &scratch.second());
    }
    {
        const scratch_scope temp = scratch.begin(scratch.second());
        EXPECT_EQ(&temp.memory(), &scratch.first());
    }
}

// Una arena que no es scratch (la del paso, por ejemplo) no choca con ninguna.
TEST(ScratchArenasTest, AnUnrelatedArenaInUseGivesTheFirst) {
    scratch_arenas scratch{make_region("scratch 1"), make_region("scratch 2")};
    arena step{make_region("paso")};
    const scratch_scope temp = scratch.begin(step);
    EXPECT_EQ(&temp.memory(), &scratch.first());
}

// Lo que la función escribe en su salida (una scratch) sobrevive a su temporal (la otra).
TEST(ScratchArenasTest, OutputInOneScratchSurvivesTheTemporaryInTheOther) {
    scratch_arenas scratch{make_region("scratch 1"), make_region("scratch 2")};
    const scratch_scope caller = scratch.begin();
    arena &out = caller.memory();

    const auto produce = [&scratch](arena &output) {
        const scratch_scope temp = scratch.begin(output);
        (void)temp.memory().allocate(300, 8); // temporal
        (void)output.allocate(40, 8);         // resultado
    };
    produce(out);

    EXPECT_EQ(out.used(), 40U);
    EXPECT_EQ(scratch.second().used(), 0U);
    EXPECT_EQ(scratch.second().peak(), 300U);
}

} // namespace
