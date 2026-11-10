# Tests de regresión: un test por bug, todos en el ejecutable tests_regression.
#
# Para un bug #N que se puede reproducir con código (docs/process/tickets.md):
#
#   1. bug_<N>.cpp, con la URL del issue en un comentario y una suite que se llama
#      Bug<N>_<Descripción> (por ejemplo, Bug123_ThirdCutLeavesHole). El test verifica el
#      comportamiento correcto, así que falla mientras el bug existe.
#   2. El archivo va en `sources`, y los módulos que prueba en `deps`.
#   3. mz_add_regression_test(BUG <N> XFAIL): falla esperada, la suite sigue en verde.
#      SANITIZER si solo falla con un sanitizer encendido (un uso después de liberar).
#   4. En el PR que lo arregla, el test pasa de forma inesperada: se quita XFAIL.
#
# Un bug que hace caer el proceso (un assert, un segfault) no puede ir directo en el
# test: CTest marca un proceso muerto por una señal como rojo aunque tenga WILL_FAIL. Se
# envuelve en EXPECT_EXIT, que lo corre en un proceso hijo:
#
#   EXPECT_EXIT({ sentencia_del_bug(); std::exit(0); }, ::testing::ExitedWithCode(0), "");
#
# Mientras el bug existe, el hijo muere y el test falla con un código normal, que
# WILL_FAIL sí invierte. Cuando se arregla, el hijo llega a exit(0) y el test pasa.
set(sources)
set(deps)

if(sources)
    mz_add_test_executable(tests_regression SOURCES ${sources} DEPS ${deps})
endif()
