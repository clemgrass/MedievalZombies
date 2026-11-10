// Rompe el subconjunto de C++: std::shared_ptr está prohibido.
#include <memory>

std::shared_ptr<int> make_value() { return std::make_shared<int>(1); }
