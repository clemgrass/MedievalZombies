// Cumple el subconjunto de C++. Este comentario nombra std::shared_ptr y co_await, y no
// debe hacer fallar el chequeo: lo que va después de // no es código.
#include <memory>

std::unique_ptr<int> make_value() { return std::make_unique<int>(1); } // no std::shared_ptr

int shared_ptr_count = 0; // un identificador que solo empieza igual
