#!/usr/bin/env python3
"""Generate primitive_modes.gltf and data_uri_texture.gltf, the fixtures for the parts of the
gltf loader that no exporter writes.

primitive_modes.gltf is one mesh with four primitives over the same four corners of a unit
square, each drawn a different way:

  primitive 0  a triangle strip of the four corners, which is two triangles
  primitive 1  a triangle fan of the four corners, which is two triangles
  primitive 2  lines, which a model of triangles leaves out
  primitive 3  points, which it leaves out too

So the merged model is twelve indices over the strip's and the fan's eight vertices. The
corners are ordered so that both the strip and the fan wind every triangle counter clockwise,
seen from +z, when they are read as glTF reads them.

Its material names a texture whose file name has a space in it, which a uri carries
percent-encoded as "my%20texture.png".

data_uri_texture.gltf is one triangle whose base colour texture is pixel.png inlined as a
data uri, with no mimeType property. The property is optional for a data uri, which states
the type itself.

Both are .gltf rather than .glb, and carry their buffer as a data uri, so each is one file.

Run from the root of the tree:
  python api/asset/tests/data/make_modes_fixture.py
"""

import base64
import json
import pathlib
import struct

HERE = pathlib.Path(__file__).parent

ARRAY_BUFFER = 34962
FLOAT = 5126

TRIANGLE_STRIP = 5
TRIANGLE_FAN = 6
LINES = 1
POINTS = 0

# strip order: bottom left, bottom right, top left, top right
STRIP = [(0.0, 0.0, 0.0), (1.0, 0.0, 0.0), (0.0, 1.0, 0.0), (1.0, 1.0, 0.0)]
# fan order: round the square from the bottom left, counter clockwise
FAN = [(0.0, 0.0, 0.0), (1.0, 0.0, 0.0), (1.0, 1.0, 0.0), (0.0, 1.0, 0.0)]


def write(name, gltf, blob):
    """Write a .gltf whose single buffer is the blob, inlined as a data uri."""
    gltf["buffers"] = [
        {
            "byteLength": len(blob),
            "uri": "data:application/octet-stream;base64," + base64.b64encode(blob).decode("ascii"),
        }
    ]
    out = HERE / name
    out.write_text(json.dumps(gltf, indent=1) + "\n", encoding="utf-8", newline="\n")
    print("wrote " + str(out))


def positions(blob, views, accessors, corners):
    """Append corners as a position accessor and return its index."""
    data = b"".join(struct.pack("<3f", *corner) for corner in corners)
    views.append({"buffer": 0, "byteOffset": len(blob), "byteLength": len(data), "target": ARRAY_BUFFER})
    blob.extend(data)
    xs = [corner[0] for corner in corners]
    ys = [corner[1] for corner in corners]
    zs = [corner[2] for corner in corners]
    accessors.append(
        {
            "bufferView": len(views) - 1,
            "componentType": FLOAT,
            "count": len(corners),
            "type": "VEC3",
            "min": [min(xs), min(ys), min(zs)],
            "max": [max(xs), max(ys), max(zs)],
        }
    )
    return len(accessors) - 1


def modes():
    blob = bytearray()
    views = []
    accessors = []
    primitives = []
    for corners, mode in ((STRIP, TRIANGLE_STRIP), (FAN, TRIANGLE_FAN), (STRIP, LINES), (STRIP, POINTS)):
        primitives.append(
            {"attributes": {"POSITION": positions(blob, views, accessors, corners)}, "mode": mode, "material": 0}
        )
    gltf = {
        "asset": {"version": "2.0", "generator": "vertical3d test fixture generator"},
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [{"mesh": 0}],
        "meshes": [{"primitives": primitives}],
        "materials": [{"pbrMetallicRoughness": {"baseColorTexture": {"index": 0}}}],
        "textures": [{"source": 0}],
        "images": [{"uri": "my%20texture.png"}],
        "accessors": accessors,
        "bufferViews": views,
    }
    write("primitive_modes.gltf", gltf, blob)


def data_uri():
    blob = bytearray()
    views = []
    accessors = []
    triangle = [(0.0, 0.0, 0.0), (1.0, 0.0, 0.0), (0.0, 1.0, 0.0)]
    position = positions(blob, views, accessors, triangle)
    pixel = (HERE / "pixel.png").read_bytes()
    gltf = {
        "asset": {"version": "2.0", "generator": "vertical3d test fixture generator"},
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [{"mesh": 0}],
        "meshes": [{"primitives": [{"attributes": {"POSITION": position}, "material": 0}]}],
        "materials": [{"pbrMetallicRoughness": {"baseColorTexture": {"index": 0}}}],
        "textures": [{"source": 0}],
        # no mimeType: the uri says what it is
        "images": [{"uri": "data:image/png;base64," + base64.b64encode(pixel).decode("ascii")}],
        "accessors": accessors,
        "bufferViews": views,
    }
    write("data_uri_texture.gltf", gltf, blob)


modes()
data_uri()
