#!/usr/bin/env python3
"""Generate two_surfaces.glb and two_surfaces_no_scene.glb, the fixtures a model's parts and
its node placement are asserted against.

Two meshes under one parent node:

  node 0  the parent, a quarter turn about +z given as a matrix, so (x, y, z) -> (-y, x, z)
          exactly - a quaternion would put sin(45 degrees) in every term
  node 1  translated (2, 0, 0), naming mesh 0
  node 2  translated (0, 3, 0), naming mesh 1

  mesh 0  primitive A, material 0, indexed, at z = 0
  mesh 1  primitive B, material 1, indexed, at z = 1
          primitive C, material 0, not indexed, at z = 2

Each primitive is the triangle (0,0), (1,0), (0,1) at its own z, and z survives the turn, so a
vertex's z says which primitive it came from. Every normal is +x, which is not the face's, so
that the turn is visible in the normals as well: it reads back as +y.

Material 0 is reached first, so it is part 0 and holds A and then C, in the order the walk
reaches them; material 1 is part 1 and holds B. Nine vertices, nine indices, two parts.

The second file is the first with no scene, which a loader has to read by walking every root.

Run from the root of the tree:
  python api/asset/tests/data/make_parts_fixture.py
"""

import json
import pathlib
import struct

TRIANGLE = [(0.0, 0.0), (1.0, 0.0), (0.0, 1.0)]
NORMAL = (1.0, 0.0, 0.0)
QUARTER_TURN_ABOUT_Z = [0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]

ARRAY_BUFFER = 34962
ELEMENT_ARRAY_BUFFER = 34963
FLOAT = 5126
UNSIGNED_SHORT = 5123

blob = bytearray()
accessors = []
views = []


def add_view(data, target):
    """Append data to the BIN chunk, 4 byte aligned, and return its bufferView index."""
    while len(blob) % 4:
        blob.append(0)
    offset = len(blob)
    blob.extend(data)
    views.append(
        {"buffer": 0, "byteOffset": offset, "byteLength": len(data), "target": target}
    )
    return len(views) - 1


def primitive(z, material, indexed):
    """One triangle at the given z, and the primitive drawing it."""
    triangle = [(x, y, z) for x, y in TRIANGLE]
    positions = b"".join(struct.pack("<3f", *vertex) for vertex in triangle)
    normals = b"".join(struct.pack("<3f", *NORMAL) for _ in triangle)

    base = len(accessors)
    accessors.append(
        {
            "bufferView": add_view(positions, ARRAY_BUFFER),
            "componentType": FLOAT,
            "count": 3,
            "type": "VEC3",
            "min": [0.0, 0.0, z],
            "max": [1.0, 1.0, z],
        }
    )
    accessors.append(
        {
            "bufferView": add_view(normals, ARRAY_BUFFER),
            "componentType": FLOAT,
            "count": 3,
            "type": "VEC3",
        }
    )
    result = {"attributes": {"POSITION": base, "NORMAL": base + 1}, "material": material}
    if indexed:
        accessors.append(
            {
                "bufferView": add_view(struct.pack("<3H", 0, 1, 2), ELEMENT_ARRAY_BUFFER),
                "componentType": UNSIGNED_SHORT,
                "count": 3,
                "type": "SCALAR",
            }
        )
        result["indices"] = base + 2
    return result


meshes = [
    {"primitives": [primitive(0.0, 0, True)]},
    {"primitives": [primitive(1.0, 1, True), primitive(2.0, 0, False)]},
]


def material(colour, texture=None):
    pbr = {"baseColorFactor": colour, "metallicFactor": 0.0, "roughnessFactor": 1.0}
    if texture is not None:
        pbr["baseColorTexture"] = {"index": texture}
    return {"pbrMetallicRoughness": pbr}


gltf = {
    "asset": {"version": "2.0", "generator": "vertical3d test fixture generator"},
    "scene": 0,
    "scenes": [{"nodes": [0]}],
    "nodes": [
        {"matrix": QUARTER_TURN_ABOUT_Z, "children": [1, 2]},
        {"translation": [2.0, 0.0, 0.0], "mesh": 0},
        {"translation": [0.0, 3.0, 0.0], "mesh": 1},
    ],
    "meshes": meshes,
    "materials": [material([1.0, 0.0, 0.0, 1.0], 0), material([0.0, 0.0, 1.0, 1.0])],
    "textures": [{"source": 0}],
    "images": [{"uri": "red.png"}],
    "accessors": accessors,
    "bufferViews": views,
    "buffers": [{"byteLength": len(blob)}],
}

GLB_MAGIC = 0x46546C67
JSON_CHUNK = 0x4E4F534A
BIN_CHUNK = 0x004E4942


def write(document, name):
    json_chunk = json.dumps(document, separators=(",", ":")).encode("utf-8")
    json_chunk += b" " * (-len(json_chunk) % 4)
    bin_chunk = bytes(blob)
    bin_chunk += bytes(-len(bin_chunk) % 4)

    total = 12 + 8 + len(json_chunk) + 8 + len(bin_chunk)
    glb = struct.pack("<III", GLB_MAGIC, 2, total)
    glb += struct.pack("<II", len(json_chunk), JSON_CHUNK) + json_chunk
    glb += struct.pack("<II", len(bin_chunk), BIN_CHUNK) + bin_chunk

    out = pathlib.Path(__file__).parent / name
    out.write_bytes(glb)
    print("wrote " + str(out) + " (" + str(len(glb)) + " bytes)")


write(gltf, "two_surfaces.glb")

sceneless = dict(gltf)
del sceneless["scene"]
del sceneless["scenes"]
write(sceneless, "two_surfaces_no_scene.glb")
