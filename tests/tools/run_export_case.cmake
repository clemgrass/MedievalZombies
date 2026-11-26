# Prueba la cadena de assets del build, .blend → .glb → .mzasset (cmake/mz_blender.cmake,
# tools/blender/export_glb.py, cmake/mz_cook.cmake y mz_cook), con el Blender fijado. Cada
# caso de la cadena arma un proyecto mínimo en WORK_DIR que usa la misma función
# mz_cook_blend que el proyecto real, con una copia de cubo.blend: el caso incremental la
# toca, y el fixture del repo no se modifica.
#
# Se llama con `cmake -DMZ_CMAKE_DIR=<cmake/> -DROOT=<raíz del repo> -DBLENDER=<binario>
# -DCOOK=<mz_cook> -DFIXTURES=<tests/tools/fixtures> -DWORK_DIR=<carpeta>
# -DGENERATOR=<generador> -DMAKE_PROGRAM=<ninja> -DMODE=<caso> -P run_export_case.cmake`.
# BLENDER es el binario que verificó la configuración del proyecto real, y COOK, el mz_cook
# que compiló: los casos prueban lo mismo que usa el build. Los casos:
#
#   incremental              compila, exporta y cocina; compilar otra vez no hace nada;
#                            tocar el .blend reexporta y recocina.
#   deterministic            dos builds del mismo .blend dan los mismos bytes, en el .glb y
#                            en el .mzasset.
#   wrong_version_configure  un Blender que dice otra versión hace fallar la configuración.
#   wrong_version_script     el script de exportación, con otra versión esperada, falla y
#                            no deja el .glb.
#   invalid                  mz_cook con un glTF inválido falla, y no deja salida.
#   usage                    mz_cook sin sus dos argumentos falla, y dice cómo se usa.
#
# Los casos que deben fallar muestran la salida, y su test la reconoce con
# PASS_REGULAR_EXPRESSION. Lo que el caso revisa por su cuenta falla con FATAL_ERROR.

file(REMOVE_RECURSE ${WORK_DIR})

# El proyecto mínimo. El .blend va en una subcarpeta para probar que el .glb y el .mzasset
# quedan en la misma ruta dentro del build.
set(source_dir ${WORK_DIR}/src)
file(WRITE ${source_dir}/CMakeLists.txt "\
cmake_minimum_required(VERSION 3.29)
project(export_case NONE)
list(APPEND CMAKE_MODULE_PATH \${MZ_CMAKE_DIR})
include(mz_cook)
mz_cook_blend(assets modelos/cubo.blend)
")
file(COPY ${FIXTURES}/cubo.blend DESTINATION ${source_dir}/modelos)
set(blend ${source_dir}/modelos/cubo.blend)

# Los 4 bytes mágicos, en hexadecimal (los archivos son binarios): "glTF" y "MZAS".
set(glb_magic 676c5446)
set(asset_magic 4d5a4153)

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
            -DMZ_COOK_EXECUTABLE=${COOK}
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
    set(asset ${build_dir}/modelos/cubo.mzasset)

    configure_and_build(${build_dir})
    foreach(file IN ITEMS ${glb} ${asset})
        if(NOT EXISTS ${file})
            message(FATAL_ERROR "El build no generó ${file}:\n${output}")
        endif()
    endforeach()

    # El .mzasset se reemplaza por una marca: si el segundo build lo recocinara, la marca se
    # perdería. Comparar contenidos evita depender de la resolución de las fechas. El .glb no
    # se puede marcar: es la entrada del cocinado, y Ninja recocinaría al verlo más nuevo
    # que el .mzasset que anotó en .ninja_log.
    file(WRITE ${asset} "marca")
    build(${build_dir})
    if(NOT output MATCHES "no work to do")
        message(FATAL_ERROR "Sin tocar el .blend, el build volvió a trabajar:\n${output}")
    endif()
    file(READ ${asset} content)
    if(NOT content STREQUAL "marca")
        message(FATAL_ERROR "Sin tocar el .blend, el build recocinó ${asset}.")
    endif()

    file(TOUCH ${blend})
    build(${build_dir})
    if(NOT output MATCHES "Exportando modelos/cubo\\.blend" OR
       NOT output MATCHES "Cocinando modelos/cubo\\.mzasset")
        message(FATAL_ERROR "Después de tocar el .blend, el build no reexportó y recocinó:\n"
            "${output}")
    endif()
    file(READ ${glb} magic LIMIT 4 HEX)
    if(NOT magic STREQUAL "${glb_magic}")
        message(FATAL_ERROR "Después de tocar el .blend, el .glb no es un glTF binario.")
    endif()
    file(READ ${asset} magic LIMIT 4 HEX)
    if(NOT magic STREQUAL "${asset_magic}")
        message(FATAL_ERROR "Después de tocar el .blend, el .mzasset no es un asset cocinado.")
    endif()
endif()

if(MODE STREQUAL "deterministic")
    configure_and_build(${WORK_DIR}/build_a)
    configure_and_build(${WORK_DIR}/build_b)
    foreach(file IN ITEMS modelos/cubo.glb modelos/cubo.mzasset)
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E compare_files
                ${WORK_DIR}/build_a/${file}
                ${WORK_DIR}/build_b/${file}
            RESULT_VARIABLE result
        )
        if(NOT result EQUAL 0)
            message(FATAL_ERROR "Dos builds del mismo .blend dieron bytes distintos en ${file}.")
        endif()
    endforeach()
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
    # El mismo comando que la regla de _mz_export_blend, con una versión esperada que nunca
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

# Corre mz_cook con `args`, y falla el caso si mz_cook sale con 0 o deja algo en WORK_DIR.
# Muestra su stderr, que el test reconoce.
function(expect_cook_failure)
    file(MAKE_DIRECTORY ${WORK_DIR}/salida)
    execute_process(
        COMMAND ${COOK} ${ARGN}
        OUTPUT_VARIABLE stdout
        ERROR_VARIABLE stderr
        RESULT_VARIABLE result
    )
    if(result EQUAL 0)
        message(FATAL_ERROR "mz_cook salió con 0 con: ${ARGN}")
    endif()
    file(GLOB leftovers ${WORK_DIR}/salida/*)
    if(leftovers)
        message(FATAL_ERROR "mz_cook falló y dejó: ${leftovers}")
    endif()
    message("${stderr}")
endfunction()

if(MODE STREQUAL "invalid")
    expect_cook_failure(${FIXTURES}/sin_normales.gltf ${WORK_DIR}/salida/sin_normales.mzasset)
endif()

if(MODE STREQUAL "usage")
    expect_cook_failure(${FIXTURES}/triangulo.gltf)
endif()
