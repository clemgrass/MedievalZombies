# La meta de cobertura del 100 % en líneas y ramas, en las carpetas que la tienen.
#
#   mz_coverage_goal_check(<lcov> <source_dir> <goal> <out_var>)
#
# <lcov> es el reporte que exporta `llvm-cov export -format=lcov`. <goal> es la lista de
# carpetas con meta, relativas a <source_dir>. Deja en <out_var> una lista con una falta
# por elemento, "archivo:línea: qué falta", con la ruta relativa a <source_dir>, en el
# orden del reporte. Lista vacía: la meta se cumple.
#
# De lcov solo se leen tres registros:
#
#   SF:<ruta absoluta>             empieza un archivo
#   DA:<línea>,<veces>             cuántas veces se ejecutó una línea
#   BRDA:<línea>,<cond>,<id>,<veces>
#                                  un lado de una condición: llvm-cov da dos por
#                                  condición, el id par para el lado verdadero y el impar
#                                  para el falso; <cond> numera las condiciones de la
#                                  línea desde 0 (en `a && b`, a es la 0 y b la 1), y
#                                  <veces> es - si la condición nunca se evaluó
#
# Un .cpp de una carpeta con meta que no aparece en el reporte también es una falta: es
# un archivo que ningún ejecutable de tests enlaza, y llvm-cov no lo ve. Los headers no
# se buscan en disco: llvm-cov los ve cuando algún archivo que se mide los incluye.

function(mz_coverage_goal_check lcov source_dir goal out_var)
    set(failures "")
    set(reported "")
    set(in_goal FALSE)

    file(STRINGS ${lcov} records REGEX "^(SF|DA|BRDA):")
    foreach(record IN LISTS records)
        if(record MATCHES "^SF:(.+)$")
            set(path "${CMAKE_MATCH_1}")
            # Un prefijo literal, no una regex: la ruta del repo puede tener . o +.
            string(FIND "${path}" "${source_dir}/" position)
            if(NOT position EQUAL 0)
                set(in_goal FALSE)
                continue()
            endif()
            string(LENGTH "${source_dir}/" prefix_length)
            string(SUBSTRING "${path}" ${prefix_length} -1 file)
            list(APPEND reported ${file})
            _mz_coverage_in_goal("${file}" "${goal}" in_goal)
        elseif(NOT in_goal)
            continue()
        elseif(record MATCHES "^DA:([0-9]+),([0-9]+)")
            if(CMAKE_MATCH_2 EQUAL 0)
                list(APPEND failures "${file}:${CMAKE_MATCH_1}: línea sin cubrir")
            endif()
        elseif(record MATCHES "^BRDA:([0-9]+),([0-9]+),([0-9]+),(-|[0-9]+)$")
            if(CMAKE_MATCH_4 STREQUAL "-" OR CMAKE_MATCH_4 EQUAL 0)
                math(EXPR condition "${CMAKE_MATCH_2} + 1")
                math(EXPR side "${CMAKE_MATCH_3} % 2")
                if(side EQUAL 0)
                    set(side verdadera)
                else()
                    set(side falsa)
                endif()
                list(APPEND failures
                    "${file}:${CMAKE_MATCH_1}: la condición ${condition} nunca fue ${side}")
            endif()
        endif()
    endforeach()

    foreach(dir IN LISTS goal)
        file(GLOB_RECURSE sources RELATIVE ${source_dir} ${source_dir}/${dir}/*.cpp)
        list(SORT sources)
        foreach(source IN LISTS sources)
            if(NOT source IN_LIST reported)
                list(APPEND failures
                    "${source}: no está en el reporte, ningún ejecutable de tests lo enlaza")
            endif()
        endforeach()
    endforeach()

    set(${out_var} "${failures}" PARENT_SCOPE)
endfunction()

# Si <file> está dentro de alguna carpeta de <goal>. La / final evita que engine/core
# abarque engine/core_extra/.
function(_mz_coverage_in_goal file goal out_var)
    foreach(dir IN LISTS goal)
        string(FIND "${file}" "${dir}/" position)
        if(position EQUAL 0)
            set(${out_var} TRUE PARENT_SCOPE)
            return()
        endif()
    endforeach()
    set(${out_var} FALSE PARENT_SCOPE)
endfunction()
