# Exporta el .blend que Blender tiene abierto a un .glb (glTF 2.0 binario). Lo
# corre la regla de mz_export_blend (cmake/mz_blender.cmake):
#
#   blender -b --factory-startup -noaudio --python-exit-code 1 <entrada.blend> \
#       --python tools/blender/export_glb.py -- <salida.glb> <versión esperada>
#
# Falla, sin escribir nada, si este Blender no es la versión esperada: con otra versión
# del exportador los bytes pueden cambiar, y la verificación de la configuración no ve un
# binario que cambió después. Cada error termina en un build que falla: los que este script
# detecta salen con SystemExit (código 1, sin traceback), y una excepción que nadie captura
# (por ejemplo, dentro del exportador) sale con 1 por el --python-exit-code 1 de la regla.
# Sin ese flag, Blender saldría con 0.
#
# Las opciones del exportador son las de por defecto de la versión fijada, salvo el
# formato. Cada opción que se agregue lleva el porqué de su ticket.
import os
import sys

import bpy


def main():
    arguments = sys.argv[sys.argv.index("--") + 1 :]
    if len(arguments) != 2 or not arguments[0].endswith(".glb"):
        raise SystemExit("Uso: ... --python export_glb.py -- <salida.glb> <versión esperada>")
    output, expected = arguments

    actual = "%d.%d.%d" % bpy.app.version
    if actual != expected:
        raise SystemExit(
            f"Se esperaba Blender {expected} y este es Blender {actual}. "
            "La versión fijada está en tools/ci/blender-version."
        )

    # Primero a un temporal y después a su nombre: una exportación cortada a la mitad no
    # deja un .glb que parezca bueno. El temporal termina en .glb porque el exportador le
    # agrega la extensión a cualquier nombre que no la tenga.
    os.makedirs(os.path.dirname(os.path.abspath(output)), exist_ok=True)
    temporary = output.removesuffix(".glb") + ".tmp.glb"
    # Un operador puede terminar en CANCELLED sin lanzar una excepción.
    result = bpy.ops.export_scene.gltf(filepath=temporary, export_format="GLB")
    if result != {"FINISHED"}:
        raise SystemExit(f"El exportador de glTF terminó con {result}: no se escribió {output}")
    os.replace(temporary, output)


main()
