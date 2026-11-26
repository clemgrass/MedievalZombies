# Dependencias de las herramientas de tools/, copiadas en el repo con su versión a la vista:
# el build no necesita red. Se compilan sin los warnings ni los flags del código propio (no
# son código nuestro), así que no se declaran con mz_add_module. Llevan la capa tool a mano:
# así la regla de capas rechaza que un módulo del engine o del juego las enlace.

# cgltf 1.15 (https://github.com/jkuhlmann/cgltf): lee glTF 2.0, solo en el cocinador.
add_library(tools_cgltf STATIC cgltf/cgltf.c)
set_target_properties(tools_cgltf PROPERTIES MZ_LAYER tool)
# SYSTEM: los warnings del header no aparecen en los archivos propios que lo incluyen.
target_include_directories(tools_cgltf SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/cgltf)
