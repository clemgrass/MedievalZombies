#include "mz/core/result.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>

namespace {

using mz::core::result;
using mz::core::unexpected;

enum class file_error : std::uint8_t { not_found, corrupt };

// Cuenta cuántos objetos están vivos. result guarda su valor en una union, y es result el
// que decide a mano qué miembro construir y destruir: este tipo deja ver si construye uno
// de más o se olvida de destruir uno.
class tracked {
public:
    explicit tracked(int value) : m_value(value) { ++m_live; }
    tracked(const tracked &other) : m_value(other.m_value) { ++m_live; }
    tracked(tracked &&other) noexcept : m_value(other.m_value) { ++m_live; }
    tracked &operator=(const tracked &) = default;
    tracked &operator=(tracked &&) noexcept = default;
    ~tracked() { --m_live; }

    [[nodiscard]] int value() const { return m_value; }

    static inline int m_live = 0;

private:
    int m_value;
};

class ResultTest : public testing::Test {
protected:
    void SetUp() override { tracked::m_live = 0; }
    void TearDown() override { EXPECT_EQ(tracked::m_live, 0) << "un objeto quedó sin destruir"; }
};

// Copiar existe solo si el valor y el error se pueden copiar.
static_assert(std::is_copy_constructible_v<result<int, file_error>>);
static_assert(std::is_copy_assignable_v<result<int, file_error>>);
static_assert(!std::is_copy_constructible_v<result<std::unique_ptr<int>, file_error>>);
static_assert(!std::is_copy_assignable_v<result<std::unique_ptr<int>, file_error>>);
static_assert(!std::is_copy_constructible_v<result<int, std::unique_ptr<int>>>);
static_assert(std::is_move_constructible_v<result<std::unique_ptr<int>, file_error>>);
static_assert(std::is_move_assignable_v<result<std::unique_ptr<int>, file_error>>);

// Un error no se convierte en result sin decir que es un error.
static_assert(!std::is_convertible_v<file_error, result<int, file_error>>);

TEST_F(ResultTest, HoldsAValue) {
    const result<int, file_error> loaded = 42;
    EXPECT_TRUE(loaded.has_value());
    EXPECT_TRUE(static_cast<bool>(loaded));
    EXPECT_EQ(loaded.value(), 42);
}

TEST_F(ResultTest, HoldsAnError) {
    const result<int, file_error> loaded = unexpected{file_error::not_found};
    EXPECT_FALSE(loaded.has_value());
    EXPECT_FALSE(static_cast<bool>(loaded));
    EXPECT_EQ(loaded.error(), file_error::not_found);
}

// Con el mismo tipo para el valor y el error, unexpected es lo único que los distingue.
TEST_F(ResultTest, SameTypeForValueAndError) {
    const result<int, int> value = 1;
    const result<int, int> error = unexpected{2};
    EXPECT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), 1);
    EXPECT_FALSE(error.has_value());
    EXPECT_EQ(error.error(), 2);
}

TEST_F(ResultTest, ValueCanBeModifiedInPlace) {
    result<int, file_error> loaded = 1;
    loaded.value() = 5;
    EXPECT_EQ(loaded.value(), 5);
}

TEST_F(ResultTest, ErrorCanBeModifiedInPlace) {
    result<int, file_error> loaded = unexpected{file_error::not_found};
    loaded.error() = file_error::corrupt;
    EXPECT_EQ(loaded.error(), file_error::corrupt);
}

TEST_F(ResultTest, MoveOnlyValueCanBeTakenOut) {
    result<std::unique_ptr<int>, file_error> loaded = std::make_unique<int>(7);
    const std::unique_ptr<int> taken = std::move(loaded).value();
    ASSERT_NE(taken, nullptr);
    EXPECT_EQ(*taken, 7);
}

TEST_F(ResultTest, MoveOnlyErrorCanBeTakenOut) {
    result<int, std::unique_ptr<int>> loaded = unexpected{std::make_unique<int>(9)};
    const std::unique_ptr<int> taken = std::move(loaded).error();
    ASSERT_NE(taken, nullptr);
    EXPECT_EQ(*taken, 9);
}

TEST_F(ResultTest, MovingTransfersAMoveOnlyValue) {
    result<std::unique_ptr<int>, file_error> source = std::make_unique<int>(3);
    const result<std::unique_ptr<int>, file_error> moved = std::move(source);
    ASSERT_TRUE(moved.has_value());
    EXPECT_EQ(*moved.value(), 3);
}

TEST_F(ResultTest, MovingTransfersAnError) {
    result<std::unique_ptr<int>, file_error> source = unexpected{file_error::corrupt};
    const result<std::unique_ptr<int>, file_error> moved = std::move(source);
    ASSERT_FALSE(moved.has_value());
    EXPECT_EQ(moved.error(), file_error::corrupt);
}

TEST_F(ResultTest, DestroysTheValue) {
    {
        const result<tracked, file_error> loaded = tracked{1};
        EXPECT_EQ(tracked::m_live, 1);
    }
    EXPECT_EQ(tracked::m_live, 0);
}

TEST_F(ResultTest, DestroysTheError) {
    {
        const result<int, tracked> loaded = unexpected{tracked{1}};
        EXPECT_EQ(tracked::m_live, 1);
    }
    EXPECT_EQ(tracked::m_live, 0);
}

// La copia es lo que se prueba, aunque clang-tidy la vea innecesaria.
TEST_F(ResultTest, CopyingDuplicatesTheValue) {
    const result<tracked, tracked> original = tracked{4};
    const result<tracked, tracked> copy = original; // NOLINT(performance-unnecessary-copy-*)
    EXPECT_EQ(tracked::m_live, 2);
    EXPECT_EQ(copy.value().value(), 4);
}

TEST_F(ResultTest, CopyingDuplicatesTheError) {
    const result<tracked, tracked> original = unexpected{tracked{5}};
    const result<tracked, tracked> copy = original; // NOLINT(performance-unnecessary-copy-*)
    EXPECT_EQ(tracked::m_live, 2);
    EXPECT_EQ(copy.error().value(), 5);
}

// Asignar destruye lo que había y construye lo nuevo, aunque cambie de valor a error o al
// revés. Sin excepciones no hay un "a mitad de camino" que proteger.
TEST_F(ResultTest, CopyAssignmentFromErrorReplacesValue) {
    result<tracked, tracked> target = tracked{1};
    const result<tracked, tracked> source = unexpected{tracked{2}};
    target = source;
    ASSERT_FALSE(target.has_value());
    EXPECT_EQ(target.error().value(), 2);
    EXPECT_EQ(tracked::m_live, 2);
}

TEST_F(ResultTest, CopyAssignmentFromValueReplacesError) {
    result<tracked, tracked> target = unexpected{tracked{1}};
    const result<tracked, tracked> source = tracked{2};
    target = source;
    ASSERT_TRUE(target.has_value());
    EXPECT_EQ(target.value().value(), 2);
    EXPECT_EQ(tracked::m_live, 2);
}

TEST_F(ResultTest, MoveAssignmentFromErrorReplacesValue) {
    result<tracked, tracked> target = tracked{1};
    result<tracked, tracked> source = unexpected{tracked{2}};
    target = std::move(source);
    ASSERT_FALSE(target.has_value());
    EXPECT_EQ(target.error().value(), 2);
}

TEST_F(ResultTest, MoveAssignmentFromValueReplacesError) {
    result<tracked, tracked> target = unexpected{tracked{1}};
    result<tracked, tracked> source = tracked{2};
    target = std::move(source);
    ASSERT_TRUE(target.has_value());
    EXPECT_EQ(target.value().value(), 2);
}

// Asignarse a sí mismo no puede destruir lo que está por copiar.
TEST_F(ResultTest, SelfCopyAssignmentKeepsTheValue) {
    result<tracked, tracked> loaded = tracked{6};
    const auto &same = loaded;
    loaded = same;
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded.value().value(), 6);
    EXPECT_EQ(tracked::m_live, 1);
}

TEST_F(ResultTest, SelfMoveAssignmentKeepsTheValue) {
    result<tracked, tracked> loaded = tracked{6};
    auto &same = loaded;
    loaded = std::move(same);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded.value().value(), 6);
    EXPECT_EQ(tracked::m_live, 1);
}

// Leer el valor de un error (o el error de un valor) es un error del programador, y aborta
// en todos los builds: en release, la alternativa sería leer el miembro inactivo de la
// union, que es comportamiento indefinido.
TEST(ResultDeathTest, ValueOfAnErrorAborts) {
    const result<int, file_error> loaded = unexpected{file_error::not_found};
    EXPECT_DEATH(static_cast<void>(loaded.value()), "value\\(\\) de un result que tiene un error");
}

TEST(ResultDeathTest, ErrorOfAValueAborts) {
    const result<int, file_error> loaded = 42;
    EXPECT_DEATH(static_cast<void>(loaded.error()), "error\\(\\) de un result que tiene un valor");
}

} // namespace
