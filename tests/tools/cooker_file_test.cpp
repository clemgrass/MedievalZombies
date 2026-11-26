#include "mz/cooker/error.h"
#include "mz/cooker/file.h"
#include "test_files.h"

#include <gtest/gtest.h>

#include <sys/resource.h>

#include <csignal>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace {

using mz::cooker::cook_error_kind;
using mz::cooker::write_file_atomic;
using mz::test::exists;
using mz::test::output_path;
using mz::test::read_file;

std::vector<std::byte> some_bytes(std::size_t size) {
    std::vector<std::byte> bytes(size);
    for (std::size_t i = 0; i < size; ++i) {
        bytes[i] = static_cast<std::byte>(i % 251);
    }
    return bytes;
}

TEST(WriteFileAtomicTest, WritesTheBytesAndLeavesNoTemporary) {
    const std::string path = output_path("escrito.bin");
    const std::vector<std::byte> bytes = some_bytes(100);

    ASSERT_TRUE(write_file_atomic(path, bytes).has_value());
    EXPECT_EQ(read_file(path), bytes);
    EXPECT_FALSE(exists(path + ".tmp"));
}

TEST(WriteFileAtomicTest, ReplacesAnOldFile) {
    const std::string path = output_path("reemplazado.bin");
    ASSERT_TRUE(write_file_atomic(path, some_bytes(100)).has_value());
    const std::vector<std::byte> bytes = some_bytes(10);

    ASSERT_TRUE(write_file_atomic(path, bytes).has_value());
    EXPECT_EQ(read_file(path), bytes);
}

TEST(WriteFileAtomicTest, AMissingFolderIsAnErrorThatNamesTheFile) {
    const std::string path = output_path("no_existe/asset.mzasset");

    const auto written = write_file_atomic(path, some_bytes(10));
    ASSERT_FALSE(written.has_value());
    EXPECT_EQ(written.error().m_kind, cook_error_kind::cannot_write);
    EXPECT_EQ(written.error().m_object, path);
}

// El temporal se escribe entero, pero no se puede renombrar: la salida es una carpeta.
TEST(WriteFileAtomicTest, AFailedRenameRemovesTheTemporary) {
    const std::string path = output_path("");

    const auto written = write_file_atomic(path, some_bytes(10));
    ASSERT_FALSE(written.has_value());
    EXPECT_EQ(written.error().m_kind, cook_error_kind::cannot_write);
    EXPECT_FALSE(exists(path + ".tmp"));
}

// Un disco que se llena a la mitad, simulado con un límite al tamaño de los archivos del
// proceso (RLIMIT_FSIZE). Con SIGXFSZ ignorada, la escritura que lo pasa falla con EFBIG en
// vez de matar el proceso. Corre en un proceso hijo, para que el límite no toque al resto de
// los tests.
void write_with_a_full_disk(const std::string &path, std::size_t size) {
    std::signal(SIGXFSZ, SIG_IGN);
    const rlimit limit{.rlim_cur = 16, .rlim_max = 16};
    if (setrlimit(RLIMIT_FSIZE, &limit) != 0) {
        std::_Exit(2);
    }
    const auto written = write_file_atomic(path, some_bytes(size));
    const bool failed =
        !written.has_value() && written.error().m_kind == cook_error_kind::cannot_write;
    std::_Exit(failed ? 0 : 1);
}

// Pocos bytes: fwrite los deja en el búfer de stdio y no falla. Falla fclose, al vaciarlo.
TEST(WriteFileAtomicDeathTest, AFailedFlushLeavesNoFile) {
    const std::string path = output_path("disco_lleno_al_cerrar.bin");
    EXPECT_EXIT(write_with_a_full_disk(path, 100), ::testing::ExitedWithCode(0), "");
    EXPECT_FALSE(exists(path));
    EXPECT_FALSE(exists(path + ".tmp"));
}

// Más bytes que el búfer de stdio: falla el mismo fwrite.
TEST(WriteFileAtomicDeathTest, AFailedWriteLeavesNoFile) {
    const std::string path = output_path("disco_lleno_al_escribir.bin");
    EXPECT_EXIT(write_with_a_full_disk(path, std::size_t{1} << 20U), ::testing::ExitedWithCode(0),
                "");
    EXPECT_FALSE(exists(path));
    EXPECT_FALSE(exists(path + ".tmp"));
}

} // namespace
