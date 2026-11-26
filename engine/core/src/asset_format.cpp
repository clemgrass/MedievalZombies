#include "mz/core/asset_format.h"

#include "mz/core/assert.h"

#include <algorithm>
#include <cstring>
#include <type_traits>

namespace mz::core {

namespace {

// La cabecera y la tabla se copian a variables locales con memcpy, en vez de leerlas con un
// reinterpret_cast sobre el búfer. En C++20, tratar unos bytes como un struct que nunca se
// construyó ahí es terreno gris del modelo de objetos (C++23 lo resuelve con
// std::start_lifetime_as), y copiar 24 bytes no cuesta nada: el compilador lo vuelve un par
// de cargas. memcpy solo copia bien un tipo trivialmente copiable: el concept lo exige.
template <typename T>
    requires std::is_trivially_copyable_v<T>
T load(std::span<const std::byte> bytes, std::size_t offset) {
    T value{};
    std::memcpy(&value, bytes.data() + offset, sizeof(T));
    return value;
}

template <typename T>
    requires std::is_trivially_copyable_v<T>
void store(std::span<std::byte> out, std::size_t offset, const T &value) {
    std::memcpy(out.data() + offset, &value, sizeof(T));
}

constexpr std::size_t entry_offset(std::size_t index) {
    return sizeof(asset_header) + (index * sizeof(section_entry));
}

// Redondea hacia arriba al siguiente múltiplo de asset_section_alignment (una potencia de 2).
constexpr std::size_t align_up(std::size_t offset) {
    return (offset + asset_section_alignment - 1) & ~(asset_section_alignment - 1);
}

} // namespace

namespace detail {

// El único que puede crear un asset_view (ver el header).
struct asset_reader {
    static asset_view make(std::span<const std::byte> bytes, const asset_header &header) {
        return {bytes, header.m_kind, header.m_section_count};
    }
};

} // namespace detail

section_view asset_view::section(std::uint32_t index) const {
    MZ_CHECK(index < m_section_count, "sección {} de un asset con {}", index, m_section_count);
    const auto entry = load<section_entry>(m_bytes, entry_offset(index));
    // read_asset ya comprobó que offset y size caen dentro de m_bytes, así que caben en un
    // size_t.
    return {
        .m_kind = entry.m_kind,
        .m_codec = entry.m_codec,
        .m_bytes = m_bytes.subspan(static_cast<std::size_t>(entry.m_offset),
                                   static_cast<std::size_t>(entry.m_size)),
    };
}

std::optional<section_view> asset_view::find(section_kind kind) const {
    for (std::uint32_t i = 0; i < m_section_count; ++i) {
        const section_view s = section(i);
        if (s.m_kind == kind) {
            return s;
        }
    }
    return std::nullopt;
}

result<asset_view, asset_error> read_asset(std::span<const std::byte> bytes) {
    MZ_CHECK(reinterpret_cast<std::uintptr_t>(bytes.data()) % asset_section_alignment == 0,
             "el búfer del asset no está alineado a {} bytes", asset_section_alignment);

    if (bytes.size() < sizeof(asset_header)) {
        return unexpected{asset_error::too_small};
    }
    const auto header = load<asset_header>(bytes, 0);
    if (header.m_signature != asset_signature) {
        return unexpected{asset_error::bad_signature};
    }
    if (header.m_format_version != asset_format_version) {
        return unexpected{asset_error::bad_version};
    }
    const std::uint64_t size = bytes.size();
    if (header.m_total_size != size) {
        return unexpected{asset_error::size_mismatch};
    }

    // Aun con 2^32 - 1 secciones, la tabla mide menos de 2^37 bytes: en 64 bits no desborda.
    const std::uint64_t table_end =
        sizeof(asset_header) + (std::uint64_t{header.m_section_count} * sizeof(section_entry));
    if (header.m_section_count > asset_max_sections || table_end > size) {
        return unexpected{asset_error::table_out_of_bounds};
    }

    // Las secciones van en orden y sin pisarse: alcanza con comparar cada una con el final de
    // la anterior (O(n)), sin revisar todos los pares.
    std::uint64_t previous_end = table_end;
    for (std::uint32_t i = 0; i < header.m_section_count; ++i) {
        const auto entry = load<section_entry>(bytes, entry_offset(i));
        if (entry.m_codec != section_codec::raw) {
            return unexpected{asset_error::unknown_codec};
        }
        if (entry.m_offset % asset_section_alignment != 0) {
            return unexpected{asset_error::section_misaligned};
        }
        // `offset + size <= size` podría dar la vuelta en 64 bits y pasar. Restar no
        // desborda, porque primero se sabe que offset <= size.
        if (entry.m_offset > size || entry.m_size > size - entry.m_offset) {
            return unexpected{asset_error::section_out_of_bounds};
        }
        if (entry.m_offset < previous_end) {
            return unexpected{asset_error::sections_out_of_order};
        }
        previous_end = entry.m_offset + entry.m_size;
    }
    return detail::asset_reader::make(bytes, header);
}

std::size_t asset_size(std::span<const section_input> sections) {
    std::size_t end = entry_offset(sections.size());
    for (const section_input &s : sections) {
        end = align_up(end) + s.m_bytes.size();
    }
    return end;
}

void write_asset(std::span<std::byte> out, asset_kind kind,
                 std::span<const section_input> sections) {
    MZ_CHECK(sections.size() <= asset_max_sections, "un asset con {} secciones (el máximo es {})",
             sections.size(), asset_max_sections);
    const std::size_t size = asset_size(sections);
    MZ_CHECK(out.size() == size, "el búfer de salida mide {} bytes y el asset {}", out.size(),
             size);

    // Todo en cero primero: el relleno entre secciones queda definido, y las mismas secciones
    // dan siempre los mismos bytes.
    std::ranges::fill(out, std::byte{0});
    store(out, 0,
          asset_header{
              .m_signature = asset_signature,
              .m_format_version = asset_format_version,
              .m_kind = kind,
              .m_section_count = static_cast<std::uint32_t>(sections.size()),
              .m_total_size = size,
          });

    std::size_t end = entry_offset(sections.size());
    for (std::size_t i = 0; i < sections.size(); ++i) {
        const std::span<const std::byte> payload = sections[i].m_bytes;
        const std::size_t offset = align_up(end);
        store(out, entry_offset(i),
              section_entry{
                  .m_kind = sections[i].m_kind,
                  .m_codec = section_codec::raw,
                  .m_offset = offset,
                  .m_size = payload.size(),
              });
        // Una sección vacía puede venir con data() nulo, y memcpy con un puntero nulo es
        // comportamiento indefinido aunque copie 0 bytes.
        if (!payload.empty()) {
            std::memcpy(out.data() + offset, payload.data(), payload.size());
        }
        end = offset + payload.size();
    }
}

} // namespace mz::core
