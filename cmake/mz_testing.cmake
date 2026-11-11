# Tests con GoogleTest: un ejecutable por módulo y las regresiones de los bugs.
#
#   mz_add_test_suite(<módulo> SOURCES <archivos>... [DEPS <targets>...])
#
# Crea el ejecutable tests_<módulo> y registra cada TEST en CTest como
# <módulo>.<Suite>.<Test>, con la etiqueta del módulo y la de su tipo. El tipo sale del
# nombre de la suite:
#
#   Vec3Test               -> unit
#   Vec3Property           -> property            (termina en Property)
#   ArenaSanitizerDeathTest -> unit;sanitizer     (termina en SanitizerDeathTest)
#
# `sanitizer` marca los tests que solo pasan con un sanitizer encendido; la suite sin
# sanitizers (Windows bajo Wine) los excluye con `ctest -LE sanitizer`. Una suite que no
# sigue ninguna convención queda como `unit`: ningún test queda sin registrar.
#
#   mz_add_regression_test(BUG <N> [XFAIL] [SANITIZER])
#
# Registra el test de regresión del bug #N, que vive en el ejecutable tests_regression,
# como regression.bug<N>, más regression.bug<N>.selected, que comprueba que el filtro
# encuentra el test. XFAIL marca el primero como falla esperada (WILL_FAIL) mientras el
# bug está abierto. El flujo completo está en docs/process/tickets.md.

include_guard(GLOBAL)

include(GoogleTest)
include(${CMAKE_CURRENT_LIST_DIR}/mz_flags.cmake)

# 1.18: la primera versión con --gtest_fail_if_no_test_selected (mz_add_regression_test).
find_package(GTest 1.18 REQUIRED)

# Los ejecutables de tests, para el reporte de cobertura.
set_property(GLOBAL PROPERTY MZ_TEST_EXECUTABLES "")

#   mz_add_test_executable(<nombre> SOURCES <archivos>... [DEPS <targets>...])
#
# Un ejecutable de GoogleTest, sin registrar en CTest. Lo usan mz_add_test_suite, el de
# las regresiones y los que registran sus tests a mano. Los tests se compilan como el
# engine (warnings como errores, sin excepciones ni RTTI, con sus sanitizers), para que
# un template del engine se instancie igual en el test que en el juego.
function(mz_add_test_executable name)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "" "SOURCES;DEPS")
    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "mz_add_test_executable(${name}): argumentos desconocidos: "
            "${arg_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT arg_SOURCES)
        message(FATAL_ERROR "mz_add_test_executable(${name}): falta SOURCES.")
    endif()

    add_executable(${name} ${arg_SOURCES})
    target_compile_features(${name} PRIVATE cxx_std_20)
    set_target_properties(${name} PROPERTIES
        CXX_EXTENSIONS OFF
        CXX_SCAN_FOR_MODULES OFF
        COMPILE_WARNING_AS_ERROR ON
    )
    target_link_libraries(${name} PRIVATE
        mz_warnings mz_engine_flags mz_sanitizers mz_coverage
        mz_test_support ${arg_DEPS})
    set_property(GLOBAL APPEND PROPERTY MZ_TEST_EXECUTABLES ${name})
endfunction()

function(mz_add_test_suite module)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "" "SOURCES;DEPS")
    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "mz_add_test_suite(${module}): argumentos desconocidos: "
            "${arg_UNPARSED_ARGUMENTS}")
    endif()

    set(target tests_${module})
    mz_add_test_executable(${target} SOURCES ${arg_SOURCES} DEPS ${arg_DEPS})

    # Una llamada por tipo, cada una con su filtro de GoogleTest (positivo-negativo). Los
    # tres filtros no se pisan y entre los tres cubren todo. PRE_TEST lista los tests al
    # correr CTest, no al compilar: un test que se cae al listarse no rompe el build.
    set(property_filter "*Property.*")
    set(sanitizer_filter "*SanitizerDeathTest.*")
    foreach(kind IN ITEMS property sanitizer unit)
        if(kind STREQUAL "property")
            set(filter "${property_filter}-${sanitizer_filter}")
            set(labels ${module} property)
        elseif(kind STREQUAL "sanitizer")
            set(filter "${sanitizer_filter}")
            set(labels ${module} unit sanitizer)
        else()
            set(filter "-${property_filter}:${sanitizer_filter}")
            set(labels ${module} unit)
        endif()
        gtest_discover_tests(${target}
            TEST_PREFIX "${module}."
            TEST_FILTER "${filter}"
            DISCOVERY_MODE PRE_TEST
            PROPERTIES LABELS "${labels}"
        )
    endforeach()
endfunction()

#   mz_add_coverage_target(GOAL <carpeta>...)
#
# Con MZ_COVERAGE, crea el target `coverage`: corre los tests de C++ y deja en
# <build>/coverage/ el reporte HTML (líneas y ramas) y summary.txt, con un resumen por
# módulo de engine/. Se llama después de declarar todos los ejecutables de tests.
#
# GOAL son las carpetas con la meta del 100 % en líneas y ramas, relativas a la raíz del
# repo: el target falla si en ellas queda algo sin cubrir
# (cmake/mz_coverage_goal.cmake). Las carpetas se validan siempre, también sin
# MZ_COVERAGE: una carpeta mal escrita dejaría la meta vacía sin avisar.
function(mz_add_coverage_target)
    cmake_parse_arguments(PARSE_ARGV 0 arg "" "" "GOAL")
    if(NOT arg_GOAL)
        message(FATAL_ERROR "mz_add_coverage_target: falta GOAL, las carpetas con meta.")
    endif()
    foreach(dir IN LISTS arg_GOAL)
        if(NOT IS_DIRECTORY ${PROJECT_SOURCE_DIR}/${dir})
            message(FATAL_ERROR "mz_add_coverage_target: la carpeta con meta ${dir} no existe.")
        endif()
    endforeach()
    if(NOT MZ_COVERAGE)
        return()
    endif()
    find_program(MZ_LLVM_PROFDATA llvm-profdata REQUIRED)
    find_program(MZ_LLVM_COV llvm-cov REQUIRED)

    get_property(executables GLOBAL PROPERTY MZ_TEST_EXECUTABLES)
    set(objects "")
    foreach(executable IN LISTS executables)
        list(APPEND objects "$<TARGET_FILE:${executable}>")
    endforeach()

    # Las listas viajan separadas por | : un ; partiría el argumento en varios.
    string(JOIN "|" goal ${arg_GOAL})
    add_custom_target(coverage
        COMMAND ${CMAKE_COMMAND}
            -DCTEST=${CMAKE_CTEST_COMMAND}
            -DLLVM_PROFDATA=${MZ_LLVM_PROFDATA}
            -DLLVM_COV=${MZ_LLVM_COV}
            -DBUILD_DIR=${PROJECT_BINARY_DIR}
            -DSOURCE_DIR=${PROJECT_SOURCE_DIR}
            -DOUTPUT_DIR=${PROJECT_BINARY_DIR}/coverage
            "-DOBJECTS=$<JOIN:${objects},|>"
            "-DGOAL=${goal}"
            -P ${PROJECT_SOURCE_DIR}/cmake/mz_coverage_report.cmake
        USES_TERMINAL
        VERBATIM
    )
    add_dependencies(coverage ${executables})
endfunction()

function(mz_add_regression_test)
    cmake_parse_arguments(PARSE_ARGV 0 arg "XFAIL;SANITIZER" "BUG" "")
    if(NOT arg_BUG MATCHES "^[0-9]+$")
        message(FATAL_ERROR "mz_add_regression_test: BUG tiene que ser el número del issue.")
    endif()
    if(NOT TARGET tests_regression)
        message(FATAL_ERROR "mz_add_regression_test: primero se crea tests_regression.")
    endif()

    # Las suites del bug se llaman Bug<N>_<Descripción>; el filtro las toma todas.
    # Sin --gtest_fail_if_no_test_selected, un filtro que no encuentra nada (un número mal
    # escrito) correría cero tests y pasaría en verde.
    set(test_name regression.bug${arg_BUG})
    add_test(NAME ${test_name}
        COMMAND tests_regression --gtest_filter=Bug${arg_BUG}_*
            --gtest_fail_if_no_test_selected)
    set(labels regression)
    if(arg_SANITIZER)
        list(APPEND labels sanitizer)
    endif()
    set_tests_properties(${test_name} PROPERTIES LABELS "${labels}")
    if(arg_XFAIL)
        set_tests_properties(${test_name} PROPERTIES WILL_FAIL TRUE)
    endif()

    # Con XFAIL, la protección de arriba se invierte: el filtro vacío falla, WILL_FAIL lo
    # da por bueno, y el test queda en verde sin haber corrido nada. Este segundo test,
    # siempre sin WILL_FAIL, comprueba que el filtro encuentra al menos un test del bug.
    add_test(NAME ${test_name}.selected
        COMMAND tests_regression --gtest_list_tests --gtest_filter=Bug${arg_BUG}_*)
    set_tests_properties(${test_name}.selected PROPERTIES
        LABELS "${labels}"
        PASS_REGULAR_EXPRESSION "Bug${arg_BUG}_"
    )
endfunction()
