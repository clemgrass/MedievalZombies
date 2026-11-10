# Prueba tools/ci/changes.sh sobre un repo git temporal con dos commits: el primero es la
# base, y el segundo agrega CHANGED_FILE.
#
# Se llama con `cmake -DSCRIPT=<changes.sh> -DWORK_DIR=<carpeta>
# -DCHANGED_FILE=<ruta relativa> -P run_changes_case.cmake`.
#
# Muestra la salida de changes.sh. El test lo decide su PASS_REGULAR_EXPRESSION.

file(REMOVE_RECURSE ${WORK_DIR})
file(MAKE_DIRECTORY ${WORK_DIR})

# La identidad y la firma van en la línea de comandos: el test no depende de la
# configuración de git de quien lo corre.
set(git git -c user.name=mz-test -c user.email=mz-test@example.invalid
    -c commit.gpgsign=false)

function(run)
    execute_process(
        COMMAND ${ARGN}
        WORKING_DIRECTORY ${WORK_DIR}
        OUTPUT_VARIABLE output
        ERROR_VARIABLE output
        RESULT_VARIABLE result
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Falló `${ARGN}`:\n${output}")
    endif()
    set(output "${output}" PARENT_SCOPE)
endfunction()

run(${git} init -q)
file(WRITE ${WORK_DIR}/README.md "base\n")
run(${git} add README.md)
run(${git} commit -q -m base)
run(${git} rev-parse HEAD)
string(STRIP "${output}" base)

file(WRITE ${WORK_DIR}/${CHANGED_FILE} "cambio\n")
run(${git} add ${CHANGED_FILE})
run(${git} commit -q -m cambio)

run(bash ${SCRIPT} ${base} HEAD)
message("${output}")
