# Tests de la infraestructura de tests: que un death test atrape un abort y que los
# sanitizers del preset estén de verdad encendidos. Si alguien los apaga sin querer, estos
# tests se ponen en rojo.
set(sources abort_test.cpp)
if(MZ_SANITIZE)
    list(APPEND sources sanitizer_test.cpp)
endif()

mz_add_test_suite(toolchain SOURCES ${sources})
