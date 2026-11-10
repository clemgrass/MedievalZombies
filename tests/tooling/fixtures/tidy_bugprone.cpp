// Compila sin warnings, pero lee un objeto después de moverlo: su estado no está
// especificado. Lo encuentra bugprone-use-after-move.
#include <string>
#include <utility>

std::size_t take(std::string name) {
    const std::string owner = std::move(name);
    return owner.size() + name.size();
}
