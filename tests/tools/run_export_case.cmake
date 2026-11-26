# Prueba la exportación .blend → .glb (cmake/mz_blender.cmake y tools/blender/export_glb.py)
# con el Blender fijado. Cada caso arma un proyecto mínimo en WORK_DIR que usa la misma
# función mz_export_blend que el proyecto real, con una copia de cubo.blend: el caso
# incremental la toca, y el fixture del repo no se modifica.
#
# Se llama con `cmake -DMZ_CMAKE_DIR=<cmake/> -DROOT=<raíz del repo> -DBLENDER=<binario>
# -DFIXTURE=<cubo.blend> -DWORK_DIR=<carpeta> -DGENERATOR=<generador>
# -DMAKE_PROGRAM=<ninja> -DMODE=<caso> -P run_export_case.cmake`. BLENDER es el binario que
# verificó la configuración del proyecto real, así que los casos prueban el mismo Blender
# que el build. Los casos:
#
#   incremental              compila y exporta; compilar otra vez no reexporta; tocar el
#                            .blend sí.
#   deterministic            dos builds del mismo .blend dan los mismos bytes.
#   wrong_version_configure  un Blender que dice otra versión hace fallar la configuración.
#   wrong_version_script     el script de exportación, con otra versión esperada, falla y
#                            no deja el .glb.
#
# Los casos que deben fallar muestran la salida, y su test la reconoce con
# PASS_REGULAR_EXPRESSION. Lo que el caso revisa por su cuenta falla con FATAL_ERROR.

file(REMOVE_RECURSE ${WORK_DIR})

# El proyecto mínimo. El .blend va en una subcarpeta para probar que el .glb queda en la
# misma ruta dentro del build.
set(source_dir ${WORK_DIR}/src)
file(WRITE ${source_dir}/CMakeLists.txt "\
cmake_minimum_required(VERSION 3.29)
project(export_case NONE)
list(APPEND CMAKE_MODULE_PATH \${MZ_CMAKE_DIR})
include(mz_blender)
mz_export_blend(assets modelos/cubo.blend)
")
file(COPY ${FIXTURE} DESTINATION ${source_dir}/modelos)
set(blend ${source_dir}/modelos/cubo.blend)

# Configura el proyecto mínimo en build_dir, con el binario de Blender que se le pase.
function(configure build_dir blender)
    execute_process(
        COMMAND ${CMAKE_COMMAND}
            -S ${source_dir}
            -B ${build_dir}
            -G ${GENERATOR}
            -DCMAKE_MAKE_PROGRAM=${MAKE_PROGRAM}
            -DMZ_CMAKE_DIR=${MZ_CMAKE_DIR}
            -DMZ_BLENDER=${blender}
        OUTPUT_VARIABLE output
        ERROR_VARIABLE output
        RESULT_VARIABLE result
    )
    set(output "${output}" PARENT_SCOPE)
    set(result "${result}" PARENT_SCOPE)
endfunction()

function(build build_dir)
    execute_process(
        COMMAND ${CMAKE_COMMAND} --build ${build_dir}
        OUTPUT_VARIABLE output
        ERROR_VARIABLE output
        RESULT_VARIABLE result
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "La compilación falló:\n${output}")
    endif()
    set(output "${output}" PARENT_SCOPE)
endfunction()

# Configura y compila; falla si algo de eso falla.
function(configure_and_build build_dir)
    configure(${build_dir} ${BLENDER})
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "La configuración falló:\n${output}")
    endif()
    build(${build_dir})
    set(output "${output}" PARENT_SCOPE)
endfunction()

if(MODE STREQUAL "incremental")
    set(build_dir ${WORK_DIR}/build)
    set(glb ${build_dir}/modelos/cubo.glb)

    configure_and_build(${build_dir})
    if(NOT EXISTS ${glb})
        message(FATAL_ERROR "El build no exportó ${glb}:\n${output}")
    endif()

    # Se reemplaza el .glb por una marca: si el segundo build lo reexportara, la marca se
    # perdería. Comparar contenidos evita depender de la resolución de las fechas.
    file(WRITE ${glb} "marca")
    build(${build_dir})
    if(NOT output MATCHES "no work to do")
        message(FATAL_ERROR "Sin tocar el .blend, el build volvió a trabajar:\n${output}")
    endif()
    file(READ ${glb} content)
    if(NOT content STREQUAL "marca")
        message(FATAL_ERROR "Sin tocar el .blend, el build reexportó el .glb.")
    endif()

    file(TOUCH ${blend})
    build(${build_dir})
    # Los 4 bytes mágicos de un .glb, "glTF", en hexadecimal: el archivo es binario.
    file(READ ${glb} magic LIMIT 4 HEX)
    if(NOT magic STREQUAL "676c5446")
        message(FATAL_ERROR "Después de tocar el .blend, el build no reexportó:\n${output}")
    endif()
endif()

if(MODE STREQUAL "deterministic")
    configure_and_build(${WORK_DIR}/build_a)
    configure_and_build(${WORK_DIR}/build_b)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E compare_files
            ${WORK_DIR}/build_a/modelos/cubo.glb
            ${WORK_DIR}/build_b/modelos/cubo.glb
        RESULT_VARIABLE result
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Dos exportaciones del mismo .blend dieron bytes distintos.")
    endif()
endif()

if(MODE STREQUAL "wrong_version_configure")
    # Un "blender" que solo dice una versión que nunca va a estar fijada.
    set(fake ${WORK_DIR}/falso/blender)
    file(WRITE ${fake} "#!/usr/bin/env bash\necho 'Blender 1.0.0 LTS'\n")
    file(CHMOD ${fake} PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
    configure(${WORK_DIR}/build ${fake})
    if(result EQUAL 0)
        message(FATAL_ERROR "La configuración pasó con un Blender de otra versión.")
    endif()
    message("${output}")
endif()

if(MODE STREQUAL "wrong_version_script")
    # El mismo comando que la regla de mz_export_blend, con una versión esperada que nunca
    # va a estar fijada: así falla el script, no la verificación de la configuración.
    set(glb ${WORK_DIR}/cubo.glb)
    execute_process(
        COMMAND ${BLENDER} -b --factory-startup -noaudio --python-exit-code 1
            ${blend} --python ${ROOT}/tools/blender/export_glb.py -- ${glb} 1.0.0
        OUTPUT_VARIABLE stdout
        ERROR_VARIABLE stderr
        RESULT_VARIABLE result
    )
    if(result EQUAL 0)
        message(FATAL_ERROR "La exportación pasó esperando otra versión de Blender.")
    endif()
    file(GLOB leftovers ${WORK_DIR}/*.glb)
    if(leftovers)
        message(FATAL_ERROR "La exportación fallida dejó: ${leftovers}")
    endif()
    message("${stderr}")
endif()
