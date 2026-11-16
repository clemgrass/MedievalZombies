#include "mz/core/region.h"
#include "mz/test/memory.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <utility>

namespace {

using mz::core::region;
using mz::core::region_error;

// Un MiB y un byte: la última página queda casi vacía.
constexpr std::size_t odd_size = (std::size_t{1} << 20U) + 1;

TEST(RegionTest, KeepsItsNameAndTheRequestedSize) {
    const auto created = region::create("arena del paso", odd_size);
    ASSERT_TRUE(created.has_value());
    EXPECT_EQ(created.value().name(), "arena del paso");
    EXPECT_EQ(created.value().size(), odd_size);
}

TEST(RegionTest, DataIsPageAligned) {
    const auto created = region::create("alineada", odd_size);
    ASSERT_TRUE(created.has_value());
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(created.value().data()) % mz::test::page_size(), 0U);
}

// Al crear la región ninguna página espera su primer toque: ese fallo de página caería en el
// frame.
TEST(RegionTest, EveryPageIsResidentAfterCreate) {
    auto created = region::create("poblada", odd_size);
    ASSERT_TRUE(created.has_value());
    region &block = created.value();
    EXPECT_EQ(mz::test::resident_pages(block.data(), block.size()), mz::test::page_count(odd_size));
}

// Un tamaño que ninguna máquina puede dar: es un error esperable, no aborta.
TEST(RegionTest, ImpossibleSizeReturnsOutOfMemory) {
    const auto created = region::create("imposible", std::size_t{1} << 60U);
    ASSERT_FALSE(created.has_value());
    EXPECT_EQ(created.error(), region_error::out_of_memory);
}

TEST(RegionTest, DestroyingItReturnsThePagesToTheOperatingSystem) {
    std::byte *data = nullptr;
    {
        auto created = region::create("liberada", odd_size);
        ASSERT_TRUE(created.has_value());
        data = created.value().data();
        ASSERT_TRUE(mz::test::is_mapped(data, odd_size));
    }
    EXPECT_FALSE(mz::test::is_mapped(data, odd_size));
}

TEST(RegionTest, MovingTransfersTheBlockAndEmptiesTheSource) {
    auto created = region::create("movida", odd_size);
    ASSERT_TRUE(created.has_value());
    std::byte *const data = created.value().data();

    const region moved{std::move(created).value()};
    EXPECT_EQ(moved.name(), "movida");
    EXPECT_EQ(moved.data(), data);
    EXPECT_EQ(moved.size(), odd_size);

    // NOLINTBEGIN(bugprone-use-after-move): el estado después de mover es el contrato.
    const region &source = created.value();
    EXPECT_EQ(source.name(), "");
    EXPECT_EQ(source.data(), nullptr);
    EXPECT_EQ(source.size(), 0U);
    // NOLINTEND(bugprone-use-after-move)
}

// La región vacía que deja un movimiento no devuelve nada al destruirse: el bloque sigue
// siendo de la región nueva.
TEST(RegionTest, DestroyingAMovedFromRegionKeepsTheBlockMapped) {
    // `created`, con la región vacía adentro, muere al salir de la lambda.
    const auto take = [] {
        auto created = region::create("origen", odd_size);
        return std::move(created).value();
    };
    region moved = take();
    EXPECT_TRUE(mz::test::is_mapped(moved.data(), odd_size));
}

TEST(RegionDeathTest, ZeroSizeAborts) {
    EXPECT_DEATH((void)region::create("vacía", 0), "tamaño 0");
}

TEST(RegionDeathTest, EmptyNameAborts) {
    EXPECT_DEATH((void)region::create("", odd_size), "sin nombre");
}

} // namespace
