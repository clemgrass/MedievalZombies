#include "mz/test/memory.h"

#include <gtest/gtest.h>

#include <sys/mman.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <vector>

namespace mz::test {

std::size_t page_size() { return static_cast<std::size_t>(sysconf(_SC_PAGESIZE)); }

std::size_t page_count(std::size_t size) { return (size + page_size() - 1) / page_size(); }

std::size_t resident_pages(std::byte *data, std::size_t size) {
    // Un byte por página; el bit 0 dice si la página está en la RAM.
    std::vector<unsigned char> pages(page_count(size));
    if (mincore(data, size, pages.data()) != 0) {
        ADD_FAILURE() << "mincore falló con errno " << errno;
        return 0;
    }
    return static_cast<std::size_t>(
        std::ranges::count_if(pages, [](unsigned char page) { return (page & 1U) != 0; }));
}

bool is_mapped(std::byte *data, std::size_t size) {
    std::vector<unsigned char> pages(page_count(size));
    if (mincore(data, size, pages.data()) == 0) {
        return true;
    }
    EXPECT_EQ(errno, ENOMEM) << "mincore falló por otra razón que un rango sin mapear";
    return false;
}

} // namespace mz::test
