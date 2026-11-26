# Genera cubo.blend, el asset de prueba de la exportación .blend → .glb: la escena de
# fábrica de Blender (un cubo de 2 m con su material) sin la cámara ni la luz.
#
#   blender="$(tools/ci/install-blender.sh)"
#   "$blender" -b --factory-startup -noaudio --python-exit-code 1 \
#       --python tests/tools/fixtures/make_cubo.py -- tests/tools/fixtures/cubo.blend
#
# El .blend es binario, y guardarlo dos veces da bytes distintos (lleva la fecha): este
# script es la forma de revisar qué tiene y de volver a generarlo.
import sys

import bpy

output = sys.argv[sys.argv.index("--") + 1]

for name in ("Camera", "Light"):
    bpy.data.objects.remove(bpy.data.objects[name])

bpy.ops.wm.save_as_mainfile(filepath=output)
