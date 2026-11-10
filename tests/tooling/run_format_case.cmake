# Prueba tools/ci/format.sh sobre una copia nueva de un fixture, para no tocar el original.
#
# Se llama con `cmake -DSCRIPT=<format.sh> -DFIXTURE=<archivo> -DWORK_DIR=<carpeta>
# -DMODE=<detect|fix> -P run_format_case.cmake`.
#
#   detect: corre el chequeo sobre la copia y muestra la salida. El test lo decide su
#           PASS_REGULAR_EXPRESSION.
#   fix:    comprueba que el chequeo falla, corre --fix y comprueba que después pasa.

file(REMOVE_RECURSE ${WORK_DIR})
file(MAKE_DIRECTORY ${WORK_DIR})
get_filename_component(name ${FIXTURE} NAME)
set(copy ${WORK_DIR}/${name})
file(COPY_FILE ${FIXTURE} ${copy})

function(run_format)
    execute_process(
        COMMAND bash ${SCRIPT} ${ARGN} ${copy}
        OUTPUT_VARIABLE output
        ERROR_VARIABLE output
        RESULT_VARIABLE result
    )
    message("${output}")
    set(result ${result} PARENT_SCOPE)
endfunction()

if(MODE STREQUAL "detect")
    run_format()
elseif(MODE STREQUAL "fix")
    run_format()
    if(result EQUAL 0)
        message(FATAL_ERROR "El fixture debería estar mal formateado y el chequeo pasó.")
    endif()
    run_format(--fix)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "format.sh --fix falló.")
    endif()
    run_format()
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Después de --fix, el chequeo sigue fallando.")
    endif()
else()
    message(FATAL_ERROR "MODE desconocido: ${MODE}")
endif()
