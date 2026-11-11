# Reporte de cobertura. Lo corre el target `coverage` (mz_add_coverage_target) con
# cmake -P; las variables llegan con -D.
#
# 1. Corre los tests de C++ con LLVM_PROFILE_FILE: cada proceso escribe un .profraw con
#    los contadores de su ejecución. %c activa el modo continuo (cmake/mz_flags.cmake),
#    que guarda también los de un proceso que aborta; %p (el pid) separa los procesos,
#    incluidos los hijos de los death tests, y %m (una firma del binario) separa los
#    ejecutables.
# 2. llvm-profdata junta todos los .profraw en un .profdata.
# 3. llvm-cov cruza ese perfil con el mapa de cobertura que el compilador dejó en cada
#    ejecutable, y escribe el HTML y un resumen por módulo, con líneas y ramas.
# 4. La meta: en las carpetas de GOAL, cada línea sin cubrir y cada lado de una condición
#    que nunca se tomó va al resumen con su archivo y su línea, y el script falla
#    (cmake/mz_coverage_goal.cmake). Falla al final, cuando el HTML y el resumen ya están
#    escritos, para que se puedan revisar.
#
# Solo mide el código propio de engine/: ni los tests ni las dependencias.

foreach(var IN ITEMS CTEST LLVM_PROFDATA LLVM_COV BUILD_DIR SOURCE_DIR OUTPUT_DIR OBJECTS
        GOAL)
    if(NOT DEFINED ${var})
        message(FATAL_ERROR "mz_coverage_report: falta -D${var}.")
    endif()
endforeach()
string(REPLACE "|" ";" objects "${OBJECTS}")
string(REPLACE "|" ";" goal "${GOAL}")
include(${CMAKE_CURRENT_LIST_DIR}/mz_coverage_goal.cmake)

set(profraw_dir ${OUTPUT_DIR}/profraw)
set(profdata ${OUTPUT_DIR}/coverage.profdata)
file(REMOVE_RECURSE ${OUTPUT_DIR})
file(MAKE_DIRECTORY ${profraw_dir})

# Los tests de cmake y de tooling no ejecutan código del engine: no aportan cobertura.
set(ENV{LLVM_PROFILE_FILE} "${profraw_dir}/%c%p-%m.profraw")
execute_process(
    COMMAND ${CTEST} --test-dir ${BUILD_DIR} --output-on-failure -LE "^(cmake|tooling)$"
    RESULT_VARIABLE tests_result
)
if(NOT tests_result EQUAL 0)
    message(FATAL_ERROR "Hay tests en rojo: la cobertura de una suite que falla no sirve.")
endif()

file(GLOB profraws ${profraw_dir}/*.profraw)
execute_process(
    COMMAND ${LLVM_PROFDATA} merge -sparse ${profraws} -o ${profdata}
    COMMAND_ERROR_IS_FATAL ANY
)

# llvm-cov toma el primer binario sin bandera y el resto con -object.
#
# Puede avisar "N functions have mismatched data", y es benigno: una función inline de un
# header que un ejecutable usa y otro no queda en el segundo como un registro "sin usar"
# con otro hash. llvm-cov descarta ese registro y cuenta el del ejecutable que la usa.
list(POP_FRONT objects first_object)
set(object_args ${first_object})
foreach(object IN LISTS objects)
    list(APPEND object_args -object ${object})
endforeach()
set(common_args
    ${object_args}
    -instr-profile=${profdata}
    "-ignore-filename-regex=(^/usr/|/tests/)"
)

execute_process(
    COMMAND ${LLVM_COV} show ${common_args}
        -format=html
        -output-dir=${OUTPUT_DIR}/html
        -show-branches=count
        -show-line-counts-or-regions
    COMMAND_ERROR_IS_FATAL ANY
)

# El resumen por módulo sale del JSON de llvm-cov export, sumando los archivos de cada
# engine/<módulo>/. (llvm-cov report con una carpeta como filtro no sirve: si ningún
# archivo calza, ignora el filtro y lista todo.) Un módulo sin ningún archivo en el
# reporte es un módulo que ningún ejecutable de tests enlaza.
execute_process(
    COMMAND ${LLVM_COV} export ${common_args} -summary-only
    OUTPUT_VARIABLE export_json
    COMMAND_ERROR_IS_FATAL ANY
)

set(metrics lines branches functions)
string(JSON file_count LENGTH "${export_json}" data 0 files)
file(GLOB module_dirs LIST_DIRECTORIES true ${SOURCE_DIR}/engine/*)
set(modules "")
foreach(module_dir IN LISTS module_dirs)
    if(IS_DIRECTORY ${module_dir})
        get_filename_component(module ${module_dir} NAME)
        list(APPEND modules ${module})
        foreach(metric IN LISTS metrics)
            set(${module}_${metric}_count 0)
            set(${module}_${metric}_covered 0)
        endforeach()
        set(${module}_files 0)
    endif()
endforeach()

if(file_count GREATER 0)
    math(EXPR last_file "${file_count} - 1")
    foreach(i RANGE ${last_file})
        string(JSON filename GET "${export_json}" data 0 files ${i} filename)
        foreach(module IN LISTS modules)
            # Un prefijo literal, no una regex: la ruta del repo puede tener . o +.
            string(FIND "${filename}" "${SOURCE_DIR}/engine/${module}/" position)
            if(NOT position EQUAL 0)
                continue()
            endif()
            math(EXPR ${module}_files "${${module}_files} + 1")
            foreach(metric IN LISTS metrics)
                string(JSON count GET "${export_json}" data 0 files ${i} summary ${metric} count)
                string(JSON covered GET "${export_json}" data 0 files ${i} summary ${metric}
                    covered)
                math(EXPR ${module}_${metric}_count "${${module}_${metric}_count} + ${count}")
                math(EXPR ${module}_${metric}_covered
                    "${${module}_${metric}_covered} + ${covered}")
            endforeach()
        endforeach()
    endforeach()
endif()

# "cubiertas/total (porcentaje)", con un decimal. math() solo hace enteros: el porcentaje
# se calcula en milésimas.
function(_mz_format_metric covered count out)
    if(count EQUAL 0)
        set(${out} "0/0 (-)" PARENT_SCOPE)
        return()
    endif()
    math(EXPR permille "${covered} * 1000 / ${count}")
    math(EXPR whole "${permille} / 10")
    math(EXPR decimal "${permille} % 10")
    set(${out} "${covered}/${count} (${whole}.${decimal} %)" PARENT_SCOPE)
endfunction()

set(summary "Cobertura por módulo de engine/\n\n")
string(APPEND summary "módulo      líneas                ramas                 funciones\n")
foreach(module IN LISTS modules)
    string(SUBSTRING "${module}            " 0 12 row)
    if(${module}_files EQUAL 0)
        string(APPEND summary "${row}sin tests: ningún ejecutable de tests lo enlaza\n")
        continue()
    endif()
    foreach(metric IN LISTS metrics)
        _mz_format_metric(${${module}_${metric}_covered} ${${module}_${metric}_count} cell)
        string(SUBSTRING "${cell}                      " 0 22 cell)
        string(APPEND row "${cell}")
    endforeach()
    string(STRIP "${row}" row)
    string(APPEND summary "${row}\n")
endforeach()

# La meta, con el detalle por línea que el JSON resumido no tiene: sale del formato lcov.
execute_process(
    COMMAND ${LLVM_COV} export ${common_args} -format=lcov
    OUTPUT_FILE ${OUTPUT_DIR}/coverage.lcov
    COMMAND_ERROR_IS_FATAL ANY
)
mz_coverage_goal_check(${OUTPUT_DIR}/coverage.lcov ${SOURCE_DIR} "${goal}" failures)

list(JOIN goal ", " goal_text)
string(APPEND summary "\nMeta del 100 % en líneas y ramas: ${goal_text}\n")
list(LENGTH failures failure_count)
if(failure_count EQUAL 0)
    string(APPEND summary "Cumplida.\n")
else()
    string(APPEND summary "No se cumple. Sin cubrir (${failure_count}):\n")
    foreach(failure IN LISTS failures)
        string(APPEND summary "  ${failure}\n")
    endforeach()
endif()

file(WRITE ${OUTPUT_DIR}/summary.txt "${summary}")
message("${summary}")
message("Reporte HTML: ${OUTPUT_DIR}/html/index.html")
message("Resumen por módulo: ${OUTPUT_DIR}/summary.txt")

if(failure_count GREATER 0)
    message(FATAL_ERROR "No se cumple la meta del 100 % de cobertura: ${failure_count} "
        "faltas, listadas en el resumen.")
endif()
