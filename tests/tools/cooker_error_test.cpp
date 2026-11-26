#include "mz/cooker/error.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <set>
#include <string_view>

namespace {

using mz::cooker::cook_error;
using mz::cooker::cook_error_kind;
using mz::cooker::describe;
using mz::cooker::format_error;

TEST(CookErrorTest, NamesTheFileAndTheObject) {
    const cook_error error{.m_kind = cook_error_kind::missing_normal, .m_object = "SinNormales"};
    EXPECT_EQ(format_error("modelos/cubo.glb", error),
              "modelos/cubo.glb: SinNormales: a una primitiva le faltan las normales (NORMAL)");
}

TEST(CookErrorTest, WithoutAnObjectNamesOnlyTheFile) {
    const cook_error error{.m_kind = cook_error_kind::no_mesh, .m_object = {}};
    EXPECT_EQ(format_error("vacio.glb", error), "vacio.glb: no hay ningún objeto con malla");
}

// Cada motivo tiene su mensaje, y ninguno se repite: dos errores distintos con el mismo texto
// mandarían a buscar el problema en el lugar equivocado.
TEST(CookErrorTest, EveryKindHasItsOwnMessage) {
    std::set<std::string_view> messages;
    const auto last = static_cast<std::uint8_t>(cook_error_kind::cannot_write);
    for (std::uint8_t i = 0; i <= last; ++i) {
        const std::string_view message = describe(static_cast<cook_error_kind>(i));
        EXPECT_FALSE(message.empty()) << "motivo " << int{i};
        EXPECT_TRUE(messages.insert(message).second) << "motivo " << int{i} << ": " << message;
    }
}

TEST(CookErrorTest, AValueOutsideTheEnumStillHasAMessage) {
    EXPECT_EQ(describe(static_cast<cook_error_kind>(200)), "error desconocido del cocinador");
}

} // namespace
