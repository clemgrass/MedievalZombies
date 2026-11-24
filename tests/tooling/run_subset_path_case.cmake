# Prueba las reglas de tools/ci/cpp-subset.sh que dependen de la carpeta del archivo: copia
# el script y un fixture a un repo falso, con el fixture en RELATIVE_PATH, y corre el
# chequeo ahí. Así un caso puede vivir en engine/platform/include/ sin que el repo de
# verdad tenga ese archivo.
#
# Se llama con `cmake -DSCRIPT=<cpp-subset.sh> -DFIXTURE=<archivo> -DWORK_DIR=<carpeta>
# -DRELATIVE_PATH=<ruta dentro del repo falso> -P run_subset_path_case.cmake`.
#
# Muestra la salida del chequeo. Si el chequeo falla, el caso también: el test que espera
# la falla la reconoce por su PASS_REGULAR_EXPRESSION, y el que espera que pase, por el
# código de salida.

file(REMOVE_RECURSE ${WORK_DIR})
# El script calcula la raíz del repo desde su propia ubicación: tools/ci/.
file(COPY ${SCRIPT} DESTINATION ${WORK_DIR}/tools/ci)
cmake_path(GET SCRIPT FILENAME script_name)
cmake_path(GET RELATIVE_PATH PARENT_PATH relative_dir)
file(MAKE_DIRECTORY ${WORK_DIR}/${relative_dir})
file(COPY_FILE ${FIXTURE} ${WORK_DIR}/${RELATIVE_PATH})

execute_process(
    COMMAND bash ${WORK_DIR}/tools/ci/${script_name} ${WORK_DIR}/${RELATIVE_PATH}
    OUTPUT_VARIABLE output
    ERROR_VARIABLE output
    RESULT_VARIABLE result
)
message("${output}")
if(NOT result EQUAL 0)
    message(FATAL_ERROR "El chequeo falló (código ${result}).")
endif()
