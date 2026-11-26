#pragma once

// El formato binario de los assets cocinados: lo escribe el cocinador y lo lee el juego, así
// que la definición vive aquí, en engine/core, que enlazan los dos.
//
//   ┌──────────────┬──────────────────────────────┬─────┬───────────┬─────┬───────────┐
//   │ asset_header │ section_entry × section_count │ pad │ sección 0 │ pad │ sección 1 │…
//   └──────────────┴──────────────────────────────┴─────┴───────────┴─────┴───────────┘
//   0              24                                   ↑ alineada a 16   ↑ alineada a 16
//
// Las secciones son planas y se ubican por offsets desde el inicio del asset, nunca por
// punteros: un puntero vale solo en el proceso que lo escribió, un offset vale en cualquier
// dirección donde caiga el búfer (un slot, el paquete). Por eso el juego usa el archivo tal
// como llega (load in place), sin convertir nada: valida la cabecera y que cada sección caiga
// dentro del búfer (read_asset), y lee los datos ahí mismo.
//
// Lo cocinado no se versiona, así que no hay compatibilidad hacia atrás: si el formato cambia,
// sube asset_format_version y se recocina todo.
//
//   const auto asset = mz::core::read_asset(bytes);
//   if (!asset) {
//       // Un archivo roto es un error esperable: el juego no aborta.
//   }
//   const auto vertices = asset.value().find(section_kind{...});

#include "mz/core/result.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <type_traits>

namespace mz::core {

// Los structs van al disco tal como están en memoria, así que el orden de los bytes del
// disco es el de la máquina. Todas las plataformas del juego son little-endian; una
// big-endian tendría que convertir cada campo, que es justo lo que el formato evita.
static_assert(std::endian::native == std::endian::little,
              "el formato de assets asume una máquina little-endian");

// "MZAS" (MedievalZombies ASset). Bytes sueltos y no un uint32_t: así se leen igual en
// cualquier orden de bytes, y un volcado hexadecimal los muestra tal cual.
inline constexpr std::array<std::byte, 4> asset_signature{std::byte{'M'}, std::byte{'Z'},
                                                          std::byte{'A'}, std::byte{'S'}};
inline constexpr std::uint32_t asset_format_version = 1;

// Cada sección empieza en un offset múltiplo de 16: alcanza para los campos de 64 bits, para
// leer un vec4 con una instrucción SIMD y para el bufferOffset de vkCmdCopyBufferToImage
// (múltiplo de 4 y del tamaño del texel). El búfer donde llega el asset también tiene que
// estar alineado a 16, o las secciones no lo estarían en memoria.
inline constexpr std::size_t asset_section_alignment = 16;

// Un límite del formato, como el de huesos por esqueleto, no un presupuesto: la tabla nunca
// pasa de 1,5 MiB, y el escritor puede revisar que la cuenta entre en m_section_count con un
// caso que un test alcanza (con 2^32 secciones harían falta ~96 GiB).
inline constexpr std::uint32_t asset_max_sections = 65'535;

// Qué es el asset y qué hay en cada sección. El lector no los interpreta: los revisa el
// cargador de cada tipo de asset. Los valores se suman cuando existe el asset que los usa.
enum class asset_kind : std::uint32_t {};
enum class section_kind : std::uint32_t {};

// Cómo están guardados los bytes de una sección. Hoy nada se comprime; el campo
// existe para sumar compresión si el tamaño en disco lo pide, sin rehacer la tabla. Ocupa 4
// bytes porque es un campo de section_entry, no por sus valores.
// NOLINTNEXTLINE(performance-enum-size)
enum class section_codec : std::uint32_t {
    raw = 0,
};

// Los campos tienen tamaño fijo y cada uno cae en un offset múltiplo de su tamaño, así que el
// compilador no mete relleno: el diseño es el mismo con Clang y con GCC, y los static_assert
// de abajo lo fijan.
struct asset_header {
    std::array<std::byte, 4> m_signature;
    std::uint32_t m_format_version;
    asset_kind m_kind;
    std::uint32_t m_section_count;
    // El tamaño del asset entero. Si el búfer mide otra cosa, el archivo está truncado o
    // tiene bytes de más.
    std::uint64_t m_total_size;
};

// Una entrada de la tabla de secciones, que va justo después de la cabecera. Las secciones
// están en orden de offset y no se pisan: cada una empieza después de que termina la
// anterior (la primera, después de la tabla).
struct section_entry {
    section_kind m_kind;
    section_codec m_codec;
    std::uint64_t m_offset; // desde el inicio del asset
    std::uint64_t m_size;
};

static_assert(sizeof(asset_header) == 24);
static_assert(offsetof(asset_header, m_signature) == 0);
static_assert(offsetof(asset_header, m_format_version) == 4);
static_assert(offsetof(asset_header, m_kind) == 8);
static_assert(offsetof(asset_header, m_section_count) == 12);
static_assert(offsetof(asset_header, m_total_size) == 16);
static_assert(std::is_trivially_copyable_v<asset_header>);

static_assert(sizeof(section_entry) == 24);
static_assert(offsetof(section_entry, m_kind) == 0);
static_assert(offsetof(section_entry, m_codec) == 4);
static_assert(offsetof(section_entry, m_offset) == 8);
static_assert(offsetof(section_entry, m_size) == 16);
static_assert(std::is_trivially_copyable_v<section_entry>);

// Por qué read_asset rechazó un búfer. Un búfer mal alineado no está aquí: lo entrega el
// código del engine (un slot, el paquete), así que es un bug y aborta (MZ_CHECK).
enum class asset_error : std::uint8_t {
    too_small,             // no cabe ni la cabecera
    bad_signature,         // no es un asset
    bad_version,           // cocinado con otra versión del formato: hay que recocinar
    size_mismatch,         // m_total_size no es el tamaño del búfer (truncado o con bytes de más)
    table_out_of_bounds,   // la tabla no cabe en el búfer, o pasa de asset_max_sections
    unknown_codec,         // una sección con un codec que este lector no conoce
    section_misaligned,    // una sección cuyo offset no es múltiplo de asset_section_alignment
    section_out_of_bounds, // una sección que se sale del búfer
    sections_out_of_order, // una sección que pisa a la anterior o a la tabla
};

namespace detail {
struct asset_reader;
} // namespace detail

// Una sección ya validada: sus bytes están dentro del búfer del asset.
struct section_view {
    section_kind m_kind;
    section_codec m_codec;
    std::span<const std::byte> m_bytes;
};

// Un asset ya validado. No es dueño de los bytes: apunta al búfer que recibió read_asset, que
// tiene que vivir más que la vista. Solo lo crea read_asset, así que tener una es saber que
// todas sus secciones caen dentro del búfer.
class asset_view {
public:
    [[nodiscard]] asset_kind kind() const { return m_kind; }
    [[nodiscard]] std::uint32_t section_count() const { return m_section_count; }

    // Pedir una sección que no existe es un bug, y aborta en todos los builds.
    [[nodiscard]] section_view section(std::uint32_t index) const;

    // La primera sección de ese tipo, si hay alguna.
    [[nodiscard]] std::optional<section_view> find(section_kind kind) const;

private:
    // El amigo es un struct de asset_format.cpp y no read_asset: declarar aquí read_asset
    // nombraría result<asset_view, …> con asset_view todavía incompleto, y result exige un
    // tipo que se pueda mover (ver region.h).
    friend struct detail::asset_reader;

    asset_view(std::span<const std::byte> bytes, asset_kind kind, std::uint32_t section_count)
        : m_bytes(bytes), m_kind(kind), m_section_count(section_count) {}

    std::span<const std::byte> m_bytes;
    asset_kind m_kind;
    std::uint32_t m_section_count;
};

// Valida el búfer y devuelve una vista de sus secciones. Revisa la firma, la versión, el
// tamaño total y, por cada sección, el codec, la alineación, que caiga dentro del búfer y que
// no pise a la anterior. Cuesta lo que leer la tabla: no recorre los datos.
//
// `bytes` tiene que empezar en una dirección alineada a asset_section_alignment (si no,
// aborta).
[[nodiscard]] result<asset_view, asset_error> read_asset(std::span<const std::byte> bytes);

// Una sección para escribir: el cocinador arma una por cada bloque de datos.
struct section_input {
    section_kind m_kind{};
    std::span<const std::byte> m_bytes;
};

// El escritor, en dos pasos para no asignar memoria: asset_size dice cuánto mide el asset, y
// write_asset lo escribe en un búfer de ese tamaño exacto (el cocinador usa un std::vector).
// Las secciones quedan en el orden recibido, con codec crudo, y el relleno queda en cero, así
// que las mismas secciones dan siempre los mismos bytes.
[[nodiscard]] std::size_t asset_size(std::span<const section_input> sections);

// `out` tiene que medir asset_size(sections), y las secciones no pueden pasar de
// asset_max_sections (si no, aborta).
void write_asset(std::span<std::byte> out, asset_kind kind,
                 std::span<const section_input> sections);

} // namespace mz::core
