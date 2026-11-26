# Genera los .gltf chicos de los tests del cocinador: cada caso es una malla mínima, válida o
# con un solo defecto, escrita aquí con sus números a la vista.
#
#   python3 tests/tools/fixtures/make_gltf.py tests/tools/fixtures
#
# Cada .gltf lleva su buffer adentro, en base64 (un data URI), salvo buffer_perdido.gltf, que
# nombra un .bin que no existe, y buffer_sin_datos.gltf, cuyo buffer no tiene uri. Los
# archivos generados se versionan: los tests no necesitan Python, y el diff de un cambio en
# este script muestra qué cambió en cada caso.
import base64
import json
import os
import struct
import sys

# Los tipos de componente de glTF.
UNSIGNED_BYTE = 5121
UNSIGNED_SHORT = 5123
UNSIGNED_INT = 5125
FLOAT = 5126

LINES = 1
TRIANGLES = 4

# Un triángulo con las tres coordenadas distintas de cero, para que el cambio de ejes de cada
# una se vea en los tests. Mirado desde +Z está en orden antihorario, y su normal es +Z:
# cross(b − a, c − a) = (2, 0, 0) × (0, 2, 0) = (0, 0, 4).
TRIANGLE = {
    "POSITION": [(1.0, 2.0, 3.0), (3.0, 2.0, 3.0), (1.0, 4.0, 3.0)],
    "NORMAL": [(0.0, 0.0, 1.0)] * 3,
    "TEXCOORD_0": [(0.0, 0.0), (1.0, 0.0), (0.0, 1.0)],
}

# Otro triángulo, en el plano XZ con su normal hacia +Y, para la segunda primitiva.
SECOND_TRIANGLE = {
    "POSITION": [(0.0, 0.0, 0.0), (0.0, 0.0, 1.0), (1.0, 0.0, 0.0)],
    "NORMAL": [(0.0, 1.0, 0.0)] * 3,
    "TEXCOORD_0": [(0.0, 0.0), (0.0, 1.0), (1.0, 0.0)],
}

FORMATS = {
    UNSIGNED_BYTE: "B",
    UNSIGNED_SHORT: "H",
    UNSIGNED_INT: "I",
    FLOAT: "f",
}

# El buffer va dentro del .gltf, en base64.
EMBEDDED = "embedded"


def cube():
    """Un cubo de 2 m centrado en el origen, como el de fábrica de Blender: 4 vértices por
    cara, cada uno con la normal de su cara, y 2 triángulos por cara. Devuelve los atributos
    y los índices."""
    # Cada cara: su normal n y dos ejes (u, v) con u × v = n. Las esquinas n ± u ± v,
    # recorridas (−u, −v) → (+u, −v) → (+u, +v) → (−u, +v), van en orden antihorario vistas
    # desde afuera.
    faces = [
        ((1, 0, 0), (0, 0, -1), (0, 1, 0)),
        ((-1, 0, 0), (0, 0, 1), (0, 1, 0)),
        ((0, 1, 0), (1, 0, 0), (0, 0, -1)),
        ((0, -1, 0), (1, 0, 0), (0, 0, 1)),
        ((0, 0, 1), (1, 0, 0), (0, 1, 0)),
        ((0, 0, -1), (-1, 0, 0), (0, 1, 0)),
    ]
    attributes = {"POSITION": [], "NORMAL": [], "TEXCOORD_0": []}
    indices = []
    for n, u, v in faces:
        first = len(attributes["POSITION"])
        for a, b in ((-1, -1), (1, -1), (1, 1), (-1, 1)):
            attributes["POSITION"].append(
                tuple(float(n[i] + a * u[i] + b * v[i]) for i in range(3)))
            attributes["NORMAL"].append(tuple(float(c) for c in n))
            attributes["TEXCOORD_0"].append(((a + 1) / 2, (b + 1) / 2))
        indices += [first, first + 1, first + 2, first, first + 2, first + 3]
    return attributes, indices


class Gltf:
    """Arma un glTF: cada accessor tiene su bufferView, todos sobre un solo buffer."""

    def __init__(self):
        self.data = bytearray()
        self.doc = {
            "asset": {"version": "2.0", "generator": "make_gltf.py"},
            "scene": 0,
            "scenes": [{"nodes": []}],
            "nodes": [],
            "meshes": [],
            "accessors": [],
            "bufferViews": [],
        }

    def accessor(self, values, component, kind, target=None, normalized=False):
        """Agrega un accessor con sus valores y devuelve su índice."""
        # Cada bufferView empieza alineado a 4 bytes, como pide glTF.
        while len(self.data) % 4:
            self.data.append(0)
        rows = [v if isinstance(v, tuple) else (v,) for v in values]
        start = len(self.data)
        for row in rows:
            self.data += struct.pack("<" + FORMATS[component] * len(row), *row)
        view = {"buffer": 0, "byteOffset": start, "byteLength": len(self.data) - start}
        if target is not None:
            view["target"] = target
        self.doc["bufferViews"].append(view)
        accessor = {
            "bufferView": len(self.doc["bufferViews"]) - 1,
            "componentType": component,
            "count": len(rows),
            "type": kind,
        }
        if normalized:
            accessor["normalized"] = True
        if kind == "VEC3" and component == FLOAT:
            # glTF exige min y max en las posiciones; no molestan en las normales.
            accessor["min"] = [min(r[i] for r in rows) for i in range(3)]
            accessor["max"] = [max(r[i] for r in rows) for i in range(3)]
        self.doc["accessors"].append(accessor)
        return len(self.doc["accessors"]) - 1

    def primitive(self, attributes, indices=(0, 1, 2), index_component=UNSIGNED_SHORT,
                  mode=TRIANGLES, kinds=None):
        """Una primitiva. `attributes` va de nombre a valores; `kinds` cambia el tipo de un
        atributo (nombre → (tipo, componente, normalizado))."""
        kinds = kinds or {}
        default = {"POSITION": ("VEC3", FLOAT, False), "NORMAL": ("VEC3", FLOAT, False),
                   "TEXCOORD_0": ("VEC2", FLOAT, False)}
        result = {"attributes": {}, "mode": mode}
        for name, values in attributes.items():
            kind, component, normalized = kinds.get(name, default[name])
            result["attributes"][name] = self.accessor(values, component, kind, 34962,
                                                       normalized)
        if indices is not None:
            result["indices"] = self.accessor(list(indices), index_component, "SCALAR", 34963)
        return result

    def mesh_node(self, primitives, name=None, **node):
        """Un nodo de la escena con una malla hecha de `primitives`."""
        self.doc["meshes"].append({"primitives": primitives})
        node["mesh"] = len(self.doc["meshes"]) - 1
        return self.node(name, **node)

    def node(self, name=None, **node):
        if name is not None:
            node = {"name": name, **node}
        self.doc["nodes"].append(node)
        index = len(self.doc["nodes"]) - 1
        self.doc["scenes"][0]["nodes"].append(index)
        return index

    def write(self, path, uri=EMBEDDED):
        """Escribe el .gltf. `uri` es EMBEDDED (el buffer en base64), el nombre de un archivo,
        o None (un buffer sin uri: nadie carga sus datos)."""
        buffer = {"byteLength": len(self.data)}
        if uri == EMBEDDED:
            buffer["uri"] = "data:application/octet-stream;base64," + base64.b64encode(
                bytes(self.data)).decode("ascii")
        elif uri is not None:
            buffer["uri"] = uri
        self.doc["buffers"] = [buffer]
        with open(path, "w", encoding="utf-8", newline="\n") as f:
            json.dump(self.doc, f, indent=2)
            f.write("\n")


def without(attributes, name):
    return {k: v for k, v in attributes.items() if k != name}


def main(out_dir):
    cases = []

    def case(name, uri=EMBEDDED):
        """Un caso nuevo: se escribe al final como <name>.gltf."""
        gltf = Gltf()
        cases.append((name, gltf, uri))
        return gltf

    # Válidos.
    g = case("triangulo")
    g.mesh_node([g.primitive(TRIANGLE)], "Triangulo")

    g = case("cubo")
    cube_attributes, cube_indices = cube()
    g.mesh_node([g.primitive(cube_attributes, indices=cube_indices)], "Cubo")

    # Dos primitivas con índices de tipos distintos (16 y 32 bits): el cocinador las junta
    # en un solo buffer, y corre los índices de la segunda.
    g = case("dos_primitivas")
    g.mesh_node([g.primitive(TRIANGLE),
                 g.primitive(SECOND_TRIANGLE, index_component=UNSIGNED_INT)],
                "DosPrimitivas")

    # Inválidos: uno por motivo.
    g = case("sin_normales")
    g.mesh_node([g.primitive(without(TRIANGLE, "NORMAL"))], "SinNormales")

    g = case("posiciones_vec2")
    g.mesh_node([g.primitive(
        {**TRIANGLE, "POSITION": [(1.0, 2.0), (3.0, 2.0), (1.0, 4.0)]},
        kinds={"POSITION": ("VEC2", FLOAT, False)})], "PosicionesVec2")

    # UV en enteros normalizados (KHR_mesh_quantization): el cocinador solo acepta float.
    g = case("uv_enteros")
    g.mesh_node([g.primitive(
        {**TRIANGLE, "TEXCOORD_0": [(0, 0), (65535, 0), (0, 65535)]},
        kinds={"TEXCOORD_0": ("VEC2", UNSIGNED_SHORT, True)})], "UvEnteros")

    g = case("sin_indices")
    g.mesh_node([g.primitive(TRIANGLE, indices=None)], "SinIndices")

    # Líneas, en un nodo sin nombre: el mensaje lo nombra por su número.
    g = case("lineas")
    g.mesh_node([g.primitive(TRIANGLE, indices=(0, 1), mode=LINES)])

    g = case("indices_sueltos")
    g.mesh_node([g.primitive(TRIANGLE, indices=(0, 1, 2, 0))], "IndicesSueltos")

    g = case("indice_fuera")
    g.mesh_node([g.primitive(TRIANGLE, indices=(0, 1, 3))], "IndiceFuera")

    g = case("malla_vacia")
    g.mesh_node([], "MallaVacia")

    g = case("sin_malla")
    g.node("Vacio")

    g = case("dos_mallas")
    g.mesh_node([g.primitive(TRIANGLE)], "Primera")
    g.mesh_node([g.primitive(SECOND_TRIANGLE)], "Segunda")

    g = case("trasladado")
    g.mesh_node([g.primitive(TRIANGLE)], "Trasladado", translation=[0.0, 1.0, 0.0])

    g = case("con_esqueleto")
    bone = g.node("Hueso")
    g.doc["skins"] = [{"joints": [bone]}]
    g.mesh_node([g.primitive(TRIANGLE)], "ConEsqueleto", skin=0)

    g = case("buffer_perdido", uri="no_existe.bin")
    g.mesh_node([g.primitive(TRIANGLE)], "BufferPerdido")

    # Un buffer sin uri en un .gltf: es válido, pero nadie carga sus datos, y cgltf no puede
    # leer las posiciones.
    g = case("buffer_sin_datos", uri=None)
    g.mesh_node([g.primitive(TRIANGLE)], "BufferSinDatos")

    # Índices sin bufferView: glTF los toma como ceros, y cgltf no los entrega.
    g = case("indices_sin_vista")
    primitive = g.primitive(TRIANGLE)
    del g.doc["accessors"][primitive["indices"]]["bufferView"]
    g.mesh_node([primitive], "IndicesSinVista")

    # Una extensión obligatoria que el cocinador no decodifica (las mallas comprimidas).
    g = case("extension_requerida")
    g.doc["extensionsUsed"] = ["KHR_draco_mesh_compression"]
    g.doc["extensionsRequired"] = ["KHR_draco_mesh_compression"]
    g.mesh_node([g.primitive(TRIANGLE)], "ExtensionRequerida")

    for name, gltf, uri in cases:
        gltf.write(os.path.join(out_dir, name + ".gltf"), uri)


if __name__ == "__main__":
    main(sys.argv[1])
