// Rompe el subconjunto de C++: las corrutinas están prohibidas. Solo lo lee el chequeo de
// texto, así que no necesita compilar: le falta el promise_type de task.
#include <coroutine>

struct task;

task wait_for(int frames) {
    co_await std::suspend_always{};
    co_return;
}
