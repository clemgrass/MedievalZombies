#include "mz/core/handle.h"

#include <gtest/gtest.h>

#include <type_traits>

namespace {

// Las etiquetas no necesitan estar definidas: handle<T> nunca mira dentro de T.
struct mesh_piece;
struct body;

using piece_handle = mz::core::handle<mesh_piece>;

// Ocho bytes (índice y generación de 32 bits) y se copia como bytes: cabe en un registro y
// se guarda en componentes, colas y archivos sin constructor de copia.
static_assert(sizeof(piece_handle) == 8);
static_assert(std::is_trivially_copyable_v<piece_handle>);
static_assert(std::is_standard_layout_v<piece_handle>);

// El tipo de lo que apunta es parte del tipo del handle: uno de un pedazo no se puede usar
// donde se espera uno de un cuerpo.
static_assert(!std::is_convertible_v<piece_handle, mz::core::handle<body>>);
static_assert(!std::is_constructible_v<piece_handle, mz::core::handle<body>>);

// Un entero suelto no se convierte en handle sin querer.
static_assert(!std::is_convertible_v<int, piece_handle>);

TEST(HandleTest, DefaultHandleIsNull) {
    const piece_handle handle;
    EXPECT_TRUE(handle.is_null());
    EXPECT_EQ(handle.index(), 0U);
    EXPECT_EQ(handle.generation(), 0U);
}

TEST(HandleTest, KeepsIndexAndGeneration) {
    const piece_handle handle{7, 3};
    EXPECT_EQ(handle.index(), 7U);
    EXPECT_EQ(handle.generation(), 3U);
    EXPECT_FALSE(handle.is_null());
}

// La generación 0 está reservada para el nulo, aunque el índice no sea 0: los pools
// empiezan cada slot en la generación 1.
TEST(HandleTest, GenerationZeroIsNullForAnyIndex) {
    const piece_handle handle{7, 0};
    EXPECT_TRUE(handle.is_null());
}

TEST(HandleTest, EqualWhenIndexAndGenerationMatch) {
    EXPECT_EQ((piece_handle{7, 3}), (piece_handle{7, 3}));
}

// El caso del handle colgado: el slot 7 se devolvió y se volvió a entregar con la
// generación 4. El handle viejo (7, 3) tiene el mismo índice pero ya no es el mismo.
TEST(HandleTest, SameIndexWithOtherGenerationIsDifferent) {
    EXPECT_NE((piece_handle{7, 3}), (piece_handle{7, 4}));
}

TEST(HandleTest, SameGenerationWithOtherIndexIsDifferent) {
    EXPECT_NE((piece_handle{7, 3}), (piece_handle{8, 3}));
}

TEST(HandleTest, UsableInConstantExpressions) {
    constexpr piece_handle handle{2, 1};
    static_assert(handle.index() == 2);
    static_assert(!handle.is_null());
    static_assert(piece_handle{}.is_null());
}

} // namespace
