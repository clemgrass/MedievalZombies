#include "mz/core/asset_format.h"
#include "mz/test/property.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace {

using mz::core::asset_error;
using mz::core::asset_format_version;
using mz::core::asset_header;
using mz::core::asset_kind;
using mz::core::asset_max_sections;
using mz::core::asset_section_alignment;
using mz::core::asset_size;
using mz::core::asset_view;
using mz::core::read_asset;
using mz::core::section_codec;
using mz::core::section_entry;
using mz::core::section_input;
using mz::core::section_kind;
using mz::core::section_view;
using mz::core::write_asset;

// Los búferes de los tests son std::vector: su memoria viene de operator new, que alinea a
// __STDCPP_DEFAULT_NEW_ALIGNMENT__. Así cumplen lo que read_asset pide a un slot.
static_assert(__STDCPP_DEFAULT_NEW_ALIGNMENT__ >= asset_section_alignment);

using bytes = std::vector<std::byte>;

bytes text(std::string_view s) {
    bytes out(s.size());
    std::ranges::transform(s, out.begin(), [](char c) { return static_cast<std::byte>(c); });
    return out;
}

bytes write(asset_kind kind, std::span<const section_input> sections) {
    bytes out(asset_size(sections));
    write_asset(out, kind, sections);
    return out;
}

// Dónde empieza la entrada `index` de la tabla de secciones.
constexpr std::size_t entry_at(std::size_t index) {
    return sizeof(asset_header) + (index * sizeof(section_entry));
}

// Pisa un campo del archivo, para armar archivos rotos a partir de uno bueno.
template <typename T> void patch(bytes &file, std::size_t offset, T value) {
    ASSERT_LE(offset + sizeof(T), file.size());
    std::memcpy(file.data() + offset, &value, sizeof(T));
}

bool equal(std::span<const std::byte> a, std::span<const std::byte> b) {
    return std::ranges::equal(a, b);
}

std::uintptr_t address(const std::byte *p) { return reinterpret_cast<std::uintptr_t>(p); }

// Los bytes de la primera sección de ese tipo, o ninguno si no hay. La revisión del optional
// va aquí, explícita: bugprone-unchecked-optional-access no ve a través de ASSERT_TRUE.
std::span<const std::byte> found_bytes(const asset_view &asset, section_kind kind) {
    const std::optional<section_view> found = asset.find(kind);
    return found.has_value() ? found->m_bytes : std::span<const std::byte>{};
}

// Un asset con dos secciones, el que usan los tests de archivos rotos:
//
//   0   cabecera            24 B
//   24  entrada 0           24 B
//   48  entrada 1           24 B
//   72  relleno              8 B
//   80  sección 0 "hello"    5 B
//   85  relleno             11 B
//   96  sección 1           20 B
//   116 fin
constexpr std::size_t first_section = 80;
constexpr std::size_t second_section = 96;
constexpr std::size_t two_section_size = 116;

bytes two_section_asset() {
    const bytes hello = text("hello");
    const bytes twenty = text("veinte bytes exactos");
    const std::array sections{section_input{.m_kind = section_kind{1}, .m_bytes = hello},
                              section_input{.m_kind = section_kind{2}, .m_bytes = twenty}};
    return write(asset_kind{1}, sections);
}

void expect_error(const bytes &file, asset_error expected) {
    const auto asset = read_asset(file);
    ASSERT_FALSE(asset.has_value());
    EXPECT_EQ(asset.error(), expected);
}

// --- El escritor y el lector ---

// Los bytes exactos de un asset chico: el orden de los bytes y los offsets de cada campo son
// parte del formato, no un detalle del compilador.
TEST(AssetFormatTest, BytesAreLittleEndianAtFixedOffsets) {
    const bytes hello = text("hello");
    const std::array sections{section_input{.m_kind = section_kind{3}, .m_bytes = hello}};
    const bytes file = write(asset_kind{7}, sections);

    // La tabla termina en 48, que ya es múltiplo de 16: la sección va ahí, sin relleno.
    const std::array<std::uint8_t, 53> expected{
        'M',  'Z',  'A',  'S',                          // firma
        0x01, 0x00, 0x00, 0x00,                         // versión del formato
        0x07, 0x00, 0x00, 0x00,                         // tipo de asset
        0x01, 0x00, 0x00, 0x00,                         // cantidad de secciones
        0x35, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // tamaño total: 53
        0x03, 0x00, 0x00, 0x00,                         // tipo de la sección
        0x00, 0x00, 0x00, 0x00,                         // codec: crudo
        0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // offset: 48
        0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // tamaño: 5
        'h',  'e',  'l',  'l',  'o',
    };
    ASSERT_EQ(file.size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(file[i], static_cast<std::byte>(expected[i])) << "byte " << i;
    }
}

TEST(AssetFormatTest, AssetWithoutSectionsIsJustTheHeader) {
    const bytes file = write(asset_kind{4}, {});
    EXPECT_EQ(file.size(), sizeof(asset_header));

    const auto asset = read_asset(file);
    ASSERT_TRUE(asset.has_value());
    EXPECT_EQ(asset.value().kind(), asset_kind{4});
    EXPECT_EQ(asset.value().section_count(), 0U);
    EXPECT_FALSE(asset.value().find(section_kind{1}).has_value());
}

// Escribir y leer da las mismas secciones, en el mismo orden, con una vacía en el medio.
TEST(AssetFormatTest, ReadingWhatWasWrittenGivesTheSameSections) {
    const bytes hello = text("hello");
    const bytes empty;
    const bytes twenty = text("veinte bytes exactos");
    const std::array sections{section_input{.m_kind = section_kind{10}, .m_bytes = hello},
                              section_input{.m_kind = section_kind{11}, .m_bytes = empty},
                              section_input{.m_kind = section_kind{12}, .m_bytes = twenty}};
    const bytes file = write(asset_kind{9}, sections);

    const auto asset = read_asset(file);
    ASSERT_TRUE(asset.has_value());
    EXPECT_EQ(asset.value().kind(), asset_kind{9});
    ASSERT_EQ(asset.value().section_count(), sections.size());
    for (std::uint32_t i = 0; i < sections.size(); ++i) {
        const section_view s = asset.value().section(i);
        EXPECT_EQ(s.m_kind, sections[i].m_kind) << "sección " << i;
        EXPECT_EQ(s.m_codec, section_codec::raw) << "sección " << i;
        EXPECT_TRUE(equal(s.m_bytes, sections[i].m_bytes)) << "sección " << i;
        // La sección se lee ahí mismo, dentro del archivo, y alineada.
        EXPECT_GE(address(s.m_bytes.data()), address(file.data())) << "sección " << i;
        EXPECT_EQ(address(s.m_bytes.data()) % asset_section_alignment, 0U) << "sección " << i;
    }
}

TEST(AssetFormatTest, SizeCountsHeaderTablePaddingAndSections) {
    EXPECT_EQ(two_section_asset().size(), two_section_size);
}

// El relleno queda en cero aunque el búfer de salida traiga basura: las mismas secciones dan
// los mismos bytes, que es lo que el cocinador promete (cocinar dos veces, mismo archivo).
TEST(AssetFormatTest, PaddingIsZeroEvenOverGarbage) {
    const bytes hello = text("hello");
    const bytes twenty = text("veinte bytes exactos");
    const std::array sections{section_input{.m_kind = section_kind{1}, .m_bytes = hello},
                              section_input{.m_kind = section_kind{2}, .m_bytes = twenty}};
    bytes dirty(asset_size(sections), std::byte{0xAB});
    write_asset(dirty, asset_kind{1}, sections);

    EXPECT_EQ(dirty, two_section_asset());
    for (std::size_t i = entry_at(2); i < first_section; ++i) {
        EXPECT_EQ(dirty[i], std::byte{0}) << "byte " << i;
    }
    for (std::size_t i = first_section + hello.size(); i < second_section; ++i) {
        EXPECT_EQ(dirty[i], std::byte{0}) << "byte " << i;
    }
}

TEST(AssetFormatTest, FindReturnsTheFirstSectionOfThatKind) {
    const bytes first = text("primera");
    const bytes other = text("otra");
    const bytes second = text("segunda");
    const std::array sections{section_input{.m_kind = section_kind{1}, .m_bytes = first},
                              section_input{.m_kind = section_kind{2}, .m_bytes = other},
                              section_input{.m_kind = section_kind{1}, .m_bytes = second}};
    const bytes file = write(asset_kind{1}, sections);
    const auto asset = read_asset(file);
    ASSERT_TRUE(asset.has_value());

    EXPECT_TRUE(equal(found_bytes(asset.value(), section_kind{1}), first));
    EXPECT_TRUE(equal(found_bytes(asset.value(), section_kind{2}), other));
    EXPECT_FALSE(asset.value().find(section_kind{3}).has_value());
}

// --- Archivos rotos: todos dan un error, ninguno aborta ---

TEST(AssetFormatTest, BufferSmallerThanTheHeaderIsTooSmall) {
    expect_error(bytes{}, asset_error::too_small);
    bytes file = two_section_asset();
    file.resize(sizeof(asset_header) - 1);
    expect_error(file, asset_error::too_small);
}

TEST(AssetFormatTest, WrongSignatureIsRejected) {
    bytes file = two_section_asset();
    patch(file, offsetof(asset_header, m_signature) + 3, 'X');
    expect_error(file, asset_error::bad_signature);
}

TEST(AssetFormatTest, OtherFormatVersionIsRejected) {
    bytes file = two_section_asset();
    patch(file, offsetof(asset_header, m_format_version), asset_format_version + 1);
    expect_error(file, asset_error::bad_version);
}

TEST(AssetFormatTest, TruncatedFileIsRejected) {
    bytes file = two_section_asset();
    file.pop_back();
    expect_error(file, asset_error::size_mismatch);
}

TEST(AssetFormatTest, FileWithExtraBytesIsRejected) {
    bytes file = two_section_asset();
    file.push_back(std::byte{0});
    expect_error(file, asset_error::size_mismatch);
}

TEST(AssetFormatTest, TableLargerThanTheFileIsRejected) {
    bytes file = two_section_asset();
    patch(file, offsetof(asset_header, m_section_count), std::uint32_t{4});
    expect_error(file, asset_error::table_out_of_bounds);
}

// La cuenta más grande posible: la tabla mediría ~96 GiB. El tamaño de la tabla se calcula en
// 64 bits, donde no desborda.
TEST(AssetFormatTest, LargestSectionCountIsRejected) {
    bytes file = two_section_asset();
    patch(file, offsetof(asset_header, m_section_count), std::numeric_limits<std::uint32_t>::max());
    expect_error(file, asset_error::table_out_of_bounds);
}

// El borde del límite del formato: con asset_max_sections secciones el asset es válido.
TEST(AssetFormatTest, MaximumSectionCountIsValid) {
    const std::vector<section_input> sections(asset_max_sections);
    const bytes file = write(asset_kind{1}, sections);
    const auto asset = read_asset(file);
    ASSERT_TRUE(asset.has_value());
    EXPECT_EQ(asset.value().section_count(), asset_max_sections);
}

// Una sección más que el límite, en un búfer donde la tabla cabe: sin la revisión del
// límite, el lector seguiría hasta las entradas (en cero) y daría otro error.
TEST(AssetFormatTest, MoreSectionsThanTheFormatAllowsIsRejected) {
    bytes file(entry_at(asset_max_sections + 1));
    patch(file, offsetof(asset_header, m_signature), mz::core::asset_signature);
    patch(file, offsetof(asset_header, m_format_version), asset_format_version);
    patch(file, offsetof(asset_header, m_section_count), asset_max_sections + 1);
    patch(file, offsetof(asset_header, m_total_size), std::uint64_t{file.size()});
    expect_error(file, asset_error::table_out_of_bounds);
}

TEST(AssetFormatTest, UnknownCodecIsRejected) {
    bytes file = two_section_asset();
    patch(file, entry_at(1) + offsetof(section_entry, m_codec), std::uint32_t{1});
    expect_error(file, asset_error::unknown_codec);
}

TEST(AssetFormatTest, MisalignedSectionIsRejected) {
    bytes file = two_section_asset();
    patch(file, entry_at(0) + offsetof(section_entry, m_offset), std::uint64_t{first_section + 1});
    expect_error(file, asset_error::section_misaligned);
}

TEST(AssetFormatTest, SectionEndingPastTheFileIsRejected) {
    bytes file = two_section_asset();
    patch(file, entry_at(1) + offsetof(section_entry, m_size), std::uint64_t{21});
    expect_error(file, asset_error::section_out_of_bounds);
}

TEST(AssetFormatTest, SectionStartingPastTheFileIsRejected) {
    bytes file = two_section_asset();
    patch(file, entry_at(1) + offsetof(section_entry, m_offset), std::uint64_t{128});
    patch(file, entry_at(1) + offsetof(section_entry, m_size), std::uint64_t{0});
    expect_error(file, asset_error::section_out_of_bounds);
}

// offset + size da la vuelta en 64 bits y queda chico (96 + (2^64 - 96) = 0): una revisión
// ingenua, `offset + size <= total`, lo dejaría pasar.
TEST(AssetFormatTest, SizeThatOverflowsWhenAddedIsRejected) {
    bytes file = two_section_asset();
    const std::uint64_t wraps = std::numeric_limits<std::uint64_t>::max() - second_section + 1;
    patch(file, entry_at(1) + offsetof(section_entry, m_size), wraps);
    expect_error(file, asset_error::section_out_of_bounds);
}

TEST(AssetFormatTest, SectionOverlappingThePreviousOneIsRejected) {
    bytes file = two_section_asset();
    patch(file, entry_at(1) + offsetof(section_entry, m_offset), std::uint64_t{first_section});
    expect_error(file, asset_error::sections_out_of_order);
}

TEST(AssetFormatTest, SectionOverlappingTheTableIsRejected) {
    bytes file = two_section_asset();
    patch(file, entry_at(0) + offsetof(section_entry, m_offset), std::uint64_t{64});
    expect_error(file, asset_error::sections_out_of_order);
}

// --- Errores del programador: abortan en todos los builds ---

TEST(AssetFormatDeathTest, SectionIndexPastTheCountAborts) {
    const bytes file = two_section_asset();
    const auto asset = read_asset(file);
    ASSERT_TRUE(asset.has_value());
    EXPECT_DEATH(static_cast<void>(asset.value().section(2)), "sección 2 de un asset con 2");
}

TEST(AssetFormatDeathTest, MisalignedBufferAborts) {
    bytes file = two_section_asset();
    file.insert(file.begin(), std::byte{0});
    const std::span<const std::byte> shifted = std::span{file}.subspan(1);
    EXPECT_DEATH(static_cast<void>(read_asset(shifted)), "no está alineado a 16");
}

TEST(AssetFormatDeathTest, OutputOfTheWrongSizeAborts) {
    const bytes hello = text("hello");
    const std::array sections{section_input{.m_kind = section_kind{1}, .m_bytes = hello}};
    bytes out(asset_size(sections) + 1);
    EXPECT_DEATH(write_asset(out, asset_kind{1}, sections), "mide 54 bytes y el asset 53");
}

TEST(AssetFormatDeathTest, MoreSectionsThanTheFormatAllowsAborts) {
    const std::vector<section_input> sections(asset_max_sections + 1);
    bytes out(asset_size(sections));
    EXPECT_DEATH(write_asset(out, asset_kind{1}, sections),
                 "65536 secciones \\(el máximo es 65535\\)");
}

// --- Propiedades: ningún búfer hace leer fuera de él (con ASan en debug) ---

// Cada búfer tiene su propio std::vector del tamaño exacto: un byte leído de más cae fuera
// de la asignación y ASan lo detecta. Si read_asset acepta el búfer, se leen todos los bytes
// de cada sección.
void read_every_section(const bytes &file) {
    const auto asset = read_asset(file);
    if (!asset.has_value()) {
        return;
    }
    const auto begin = address(file.data());
    for (std::uint32_t i = 0; i < asset.value().section_count(); ++i) {
        const section_view s = asset.value().section(i);
        ASSERT_GE(address(s.m_bytes.data()), begin);
        ASSERT_LE(address(s.m_bytes.data()) + s.m_bytes.size(), begin + file.size());
        // Copiar pasa por memcpy, que ASan intercepta y revisa entero, aunque el compilador
        // optimice: un bucle cuyo resultado nadie usa podría desaparecer.
        const bytes copy(s.m_bytes.begin(), s.m_bytes.end());
        ASSERT_EQ(copy.size(), s.m_bytes.size());
    }
    static_cast<void>(asset.value().find(section_kind{1}));
}

// Un asset válido al azar: de 0 a 4 secciones de 0 a 40 bytes.
bytes random_asset(mz::test::property_rng &rng) {
    const std::uint32_t count = rng.next_u32() % 5U;
    std::array<bytes, 4> payloads;
    std::array<section_input, 4> sections{};
    for (std::uint32_t i = 0; i < count; ++i) {
        payloads.at(i).resize(rng.next_u32() % 41U);
        for (std::byte &b : payloads.at(i)) {
            b = static_cast<std::byte>(rng.next_u32());
        }
        sections.at(i) = {.m_kind = section_kind{rng.next_u32() % 3U}, .m_bytes = payloads.at(i)};
    }
    return write(asset_kind{1}, std::span{sections}.first(count));
}

constexpr int property_runs = 10'000;

TEST(AssetFormatProperty, RandomBytesNeverReadOutsideTheBuffer) {
    mz::test::property_rng rng;
    for (int run = 0; run < property_runs; ++run) {
        bytes file(rng.next_u32() % 200U);
        for (std::byte &b : file) {
            b = static_cast<std::byte>(rng.next_u32());
        }
        // Con bytes al azar, casi ninguno pasa la firma, y casi ninguna entrada pasa el codec:
        // la mitad de las veces la cabecera es válida y las entradas que caben tienen codec
        // crudo y un offset alineado chico, para llegar a revisar los límites de cada sección.
        if (file.size() >= sizeof(asset_header) && rng.next_u32() % 2U == 0) {
            const std::uint32_t count = rng.next_u32() % 8U;
            patch(file, offsetof(asset_header, m_signature), mz::core::asset_signature);
            patch(file, offsetof(asset_header, m_format_version), asset_format_version);
            patch(file, offsetof(asset_header, m_total_size), std::uint64_t{file.size()});
            patch(file, offsetof(asset_header, m_section_count), count);
            for (std::uint32_t i = 0; i < count && entry_at(i + 1) <= file.size(); ++i) {
                patch(file, entry_at(i) + offsetof(section_entry, m_codec), section_codec::raw);
                patch(file, entry_at(i) + offsetof(section_entry, m_offset),
                      std::uint64_t{rng.next_u32() % 256U} & ~std::uint64_t{15});
                patch(file, entry_at(i) + offsetof(section_entry, m_size),
                      std::uint64_t{rng.next_u32() % 256U});
            }
        }
        read_every_section(file);
    }
}

// Un asset válido con algunos bytes de la cabecera o de la tabla cambiados: las variantes
// caen cerca de lo válido (un offset corrido unos bytes, un tamaño un poco más grande).
TEST(AssetFormatProperty, MutatedAssetsNeverReadOutsideTheBuffer) {
    mz::test::property_rng rng;
    for (int run = 0; run < property_runs; ++run) {
        bytes file = random_asset(rng);
        const auto valid = read_asset(file);
        ASSERT_TRUE(valid.has_value());
        const std::size_t table_end = entry_at(valid.value().section_count());
        const std::uint32_t mutations = 1 + (rng.next_u32() % 4U);
        for (std::uint32_t m = 0; m < mutations; ++m) {
            file[rng.next_u32() % table_end] = static_cast<std::byte>(rng.next_u32());
        }
        read_every_section(file);
    }
}

// Un asset válido cortado en cualquier punto, con el tamaño total corregido para que la
// revisión de tamaño no lo frene y lleguen a revisarse las secciones.
TEST(AssetFormatProperty, TruncatedAssetsNeverReadOutsideTheBuffer) {
    mz::test::property_rng rng;
    for (int run = 0; run < property_runs; ++run) {
        bytes file = random_asset(rng);
        file.resize(rng.next_u32() % (file.size() + 1));
        if (file.size() >= sizeof(asset_header)) {
            patch(file, offsetof(asset_header, m_total_size), std::uint64_t{file.size()});
        }
        read_every_section(file);
    }
}

} // namespace
