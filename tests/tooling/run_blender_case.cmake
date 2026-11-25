# Prueba tools/ci/install-blender.sh y tools/ci/blender.sh sin red: copia los scripts a un
# repo falso, con su propio tools/ci/blender-version, y sirve un tarball falso de pocos
# bytes desde un mirror local (file://). El "blender" del tarball es un script que imprime
# su versión.
#
# Se llama con `cmake -DTOOLS_DIR=<tools/ci> -DWORK_DIR=<carpeta> -DMODE=<caso>
# -P run_blender_case.cmake`. Los casos:
#
#   cache          instala, borra el tarball del mirror, y la segunda corrida igual pasa:
#                  usó la caché.
#   bad_sha256     el pin tiene otro sha256: la instalación falla y no deja nada.
#   stale_marker   la caché tiene una instalación de otro sha256: se reinstala.
#   wrong_version  el binario dice otra versión que la del pin: el job blender.sh falla.
#
# Los casos que deben fallar muestran la salida del script, y su test la reconoce con
# PASS_REGULAR_EXPRESSION. Lo que el caso revisa por su cuenta falla con FATAL_ERROR sin
# repetir esa salida, para que el regex no lo tape.

set(version 5.2.2)
set(name blender-${version}-linux-x64)
set(cache ${WORK_DIR}/cache)
set(install_dir ${cache}/blender-${version})
set(tarball ${WORK_DIR}/mirror/Blender5.2/${name}.tar.xz)

file(REMOVE_RECURSE ${WORK_DIR})

# El script calcula la carpeta del pin desde su propia ubicación: tools/ci/.
file(COPY ${TOOLS_DIR}/install-blender.sh ${TOOLS_DIR}/blender.sh
    DESTINATION ${WORK_DIR}/tools/ci)

# El tarball falso, con la misma carpeta de primer nivel que el oficial.
set(printed_version ${version})
if(MODE STREQUAL "wrong_version")
    set(printed_version 5.2.3)
endif()
file(MAKE_DIRECTORY ${WORK_DIR}/payload/${name})
file(WRITE ${WORK_DIR}/payload/${name}/blender
    "#!/usr/bin/env bash\necho 'Blender ${printed_version} LTS'\necho 'build hash: falso'\n")
file(CHMOD ${WORK_DIR}/payload/${name}/blender
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
file(MAKE_DIRECTORY ${WORK_DIR}/mirror/Blender5.2)
file(ARCHIVE_CREATE OUTPUT ${tarball} PATHS ${name} FORMAT gnutar COMPRESSION XZ
    WORKING_DIRECTORY ${WORK_DIR}/payload)
file(SHA256 ${tarball} sha256)

set(pinned_sha256 ${sha256})
if(MODE STREQUAL "bad_sha256")
    string(REPEAT "0" 64 pinned_sha256)
endif()
file(WRITE ${WORK_DIR}/tools/ci/blender-version
    "# Pin falso del test.\nversion=${version}\nsha256=${pinned_sha256}\n")

if(MODE STREQUAL "stale_marker")
    file(MAKE_DIRECTORY ${install_dir})
    file(WRITE ${install_dir}/blender "viejo\n")
    string(REPEAT "1" 64 other_sha256)
    file(WRITE ${install_dir}/.mz-sha256 "${other_sha256}\n")
endif()

set(env ${CMAKE_COMMAND} -E env
    MZ_TOOLS_DIR=${cache}
    MZ_BLENDER_MIRROR=file://${WORK_DIR}/mirror)

function(run_script script)
    execute_process(
        COMMAND ${env} bash ${WORK_DIR}/tools/ci/${script}
        OUTPUT_VARIABLE stdout
        ERROR_VARIABLE stderr
        RESULT_VARIABLE result
    )
    set(stdout "${stdout}" PARENT_SCOPE)
    set(stderr "${stderr}" PARENT_SCOPE)
    set(result "${result}" PARENT_SCOPE)
endfunction()

if(MODE STREQUAL "cache" OR MODE STREQUAL "stale_marker")
    run_script(install-blender.sh)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "La instalación falló:\n${stderr}")
    endif()
    # stdout es solo la ruta del binario: el cocinador la usa tal cual.
    if(NOT stdout STREQUAL "${install_dir}/blender\n")
        message(FATAL_ERROR "Se esperaba la ruta '${install_dir}/blender', y salió '${stdout}'")
    endif()
    execute_process(COMMAND ${install_dir}/blender OUTPUT_VARIABLE printed)
    if(NOT printed MATCHES "^Blender ${version} ")
        message(FATAL_ERROR "La carpeta instalada no tiene el blender del tarball: '${printed}'")
    endif()
endif()

if(MODE STREQUAL "cache")
    # Sin el tarball en el mirror, solo la caché puede dar el binario.
    file(REMOVE ${tarball})
    run_script(install-blender.sh)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "La segunda corrida no usó la caché:\n${stderr}")
    endif()
    if(NOT stdout STREQUAL "${install_dir}/blender\n")
        message(FATAL_ERROR "La segunda corrida dio otra ruta: '${stdout}'")
    endif()
endif()

if(MODE STREQUAL "bad_sha256")
    run_script(install-blender.sh)
    if(result EQUAL 0)
        message(FATAL_ERROR "La instalación pasó con un sha256 distinto.")
    endif()
    if(EXISTS ${install_dir})
        message(FATAL_ERROR "La instalación fallida dejó ${install_dir}.")
    endif()
    file(GLOB leftovers LIST_DIRECTORIES true ${cache}/.blender-*)
    if(leftovers)
        message(FATAL_ERROR "La instalación fallida dejó la carpeta temporal: ${leftovers}")
    endif()
    message("${stderr}")
endif()

if(MODE STREQUAL "wrong_version")
    run_script(blender.sh)
    if(result EQUAL 0)
        message(FATAL_ERROR "El job pasó con un Blender de otra versión.")
    endif()
    message("${stderr}")
endif()
