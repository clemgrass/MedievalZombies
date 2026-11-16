#include "mz/platform/memory.h"
#include "mz/test/memory.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <limits>

namespace {

using mz::platform::map_populated;
using mz::platform::mapping;
using mz::platform::unmap;

// Un MiB y un byte: la última página queda casi vacía, y también tiene que estar poblada.
constexpr std::size_t odd_size = (std::size_t{1} << 20U) + 1;

TEST(PageSizeTest, MatchesTheOperatingSystem) {
    EXPECT_EQ(mz::platform::page_size(), mz::test::page_size());
}

TEST(MapPopulatedTest, HasTheRequestedSize) {
    const mapping block = map_populated(odd_size);
    ASSERT_NE(block.m_data, nullptr);
    EXPECT_EQ(block.m_size, odd_size);
    unmap(block);
}

TEST(MapPopulatedTest, IsPageAligned) {
    const mapping block = map_populated(odd_size);
    ASSERT_NE(block.m_data, nullptr);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(block.m_data) % mz::test::page_size(), 0U);
    unmap(block);
}

// Después de reservar ninguna página espera su primer toque: ese fallo de página caería en
// el frame.
TEST(MapPopulatedTest, EveryPageIsResident) {
    const mapping block = map_populated(odd_size);
    ASSERT_NE(block.m_data, nullptr);
    EXPECT_EQ(mz::test::resident_pages(block.m_data, block.m_size), mz::test::page_count(odd_size));
    unmap(block);
}

TEST(MapPopulatedTest, FirstAndLastBytesAreWritable) {
    const mapping block = map_populated(odd_size);
    ASSERT_NE(block.m_data, nullptr);
    block.m_data[0] = std::byte{0x5A};
    block.m_data[odd_size - 1] = std::byte{0xA5};
    EXPECT_EQ(block.m_data[0], std::byte{0x5A});
    EXPECT_EQ(block.m_data[odd_size - 1], std::byte{0xA5});
    unmap(block);
}

// Un tamaño que no cabe en ningún espacio de direcciones (x86-64 llega a 2^57 con 5
// niveles de tablas): falla igual con cualquier configuración de overcommit, sin intentar
// poblar nada.
TEST(MapPopulatedTest, SizeBeyondTheAddressSpaceReturnsAnEmptyMapping) {
    const mapping block = map_populated(std::size_t{1} << 60U);
    EXPECT_EQ(block.m_data, nullptr);
    EXPECT_EQ(block.m_size, 0U);
}

// Redondear el máximo a páginas enteras desborda: el sistema operativo lo rechaza.
TEST(MapPopulatedTest, MaximumSizeReturnsAnEmptyMapping) {
    const mapping block = map_populated(std::numeric_limits<std::size_t>::max());
    EXPECT_EQ(block.m_data, nullptr);
    EXPECT_EQ(block.m_size, 0U);
}

TEST(MapPopulatedTest, ZeroSizeReturnsAnEmptyMapping) {
    const mapping block = map_populated(0);
    EXPECT_EQ(block.m_data, nullptr);
    EXPECT_EQ(block.m_size, 0U);
}

TEST(UnmapTest, ReturnsThePagesToTheOperatingSystem) {
    const mapping block = map_populated(odd_size);
    ASSERT_NE(block.m_data, nullptr);
    ASSERT_TRUE(mz::test::is_mapped(block.m_data, block.m_size));
    unmap(block);
    EXPECT_FALSE(mz::test::is_mapped(block.m_data, block.m_size));
}

// Un bloque que no empieza en una página no salió de map_populated().
TEST(UnmapDeathTest, BlockThatIsNotAMappingAborts) {
    EXPECT_DEATH(
        {
            const mapping block = map_populated(odd_size);
            unmap({block.m_data + 1, block.m_size - 1});
        },
        "munmap");
}

} // namespace
