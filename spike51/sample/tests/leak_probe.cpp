#include <cstdlib>

void* volatile g_probe = nullptr;

int main() {
    g_probe = std::malloc(64);
    g_probe = nullptr;
    return 0;
}
