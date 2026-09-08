#!/usr/bin/env python3
"""Generate embedded_texture.glb, the fixture the gltf loader's embedded image path is
asserted against.

One indexed triangle with a material whose base colour texture is an image the file
*carries* rather than names: a bufferView holding the bytes of pixel.png, with the mime
type glTF requires for one. That is the case three_primitives.glb cannot cover, because a
named texture is resolved by the asset manager and never decoded by the loader at all.

The image is pixel.png rather than one generated here, so that what the loader decodes can
be compared against what the png reader makes of the same file.

Run from the root of the tree:
  python api/asset/tests/data/make_embedded_fixture.py
"""

import json
import pathlib
import struct

HERE = pathlib.Path(__file__).parent

TRIANGLE = [(0.0, 0.0, 0.0), (1.0, 0.0, 0.0), (0.0, 1.0, 0.0)]
NORMAL = (0.0, 0.0, 1.0)
UVS = [(0.0, 0.0), (1.0, 0.0), (0.0, 1.0)]

ARRAY_BUFFER = 34962
ELEMENT_ARRAY_BUFFER = 34963
FLOAT = 5126
UNSIGNED_SHORT = 5123

blob = bytearray()
views = []


def add_view(data, target=None):
    """Append data to the BIN chunk, 4 byte aligned, and return its bufferView index."""
    while len(blob) % 4:
        blob.append(0)
    offset = len(blob)
    blob.extend(data)
    view = {"buffer": 0, "byteOffset": offset, "byteLength": len(data)}
    # an image's view carries no target - it is not vertex or index data
    if target is not None:
        view["target"] = target
    views.append(view)
    return len(views) - 1


positions = b"".join(struct.pack("<3f", *vertex) for vertex in TRIANGLE)
normals = b"".join(struct.pack("<3f", *NORMAL) for _ in TRIANGLE)
uvs = b"".join(struct.pack("<2f", *uv) for uv in UVS)

position_view = add_view(positions, ARRAY_BUFFER)
normal_view = add_view(normals, ARRAY_BUFFER)
uv_view = add_view(uvs, ARRAY_BUFFER)
index_view = add_view(struct.pack("<3H", 0, 1, 2), ELEMENT_ARRAY_BUFFER)
image_view = add_view((HERE / "pixel.png").read_bytes())

xs = [vertex[0] for vertex in TRIANGLE]
ys = [vertex[1] for vertex in TRIANGLE]
zs = [vertex[2] for vertex in TRIANGLE]

gltf = {
    "asset": {"version": "2.0", "generator": "vertical3d test fixture generator"},
    "scene": 0,
    "scenes": [{"nodes": [0]}],
    "nodes": [{"mesh": 0}],
    "meshes": [
        {
            "primitives": [
                {
                    "attributes": {"POSITION": 0, "NORMAL": 1, "TEXCOORD_0": 2},
                    "indices": 3,
                    "material": 0,
                }
            ]
        }
    ],
    "materials": [
        {
            "pbrMetallicRoughness": {
                "baseColorFactor": [1.0, 1.0, 1.0, 1.0],
                "baseColorTexture": {"index": 0},
                "metallicFactor": 0.0,
                "roughnessFactor": 1.0,
            }
        }
    ],
    "textures": [{"source": 0}],
    "images": [{"bufferView": image_view, "mimeType": "image/png"}],
    "accessors": [
        {
            "bufferView": position_view,
            "componentType": FLOAT,
            "count": 3,
            "type": "VEC3",
            "min": [min(xs), min(ys), min(zs)],
            "max": [max(xs), max(ys), max(zs)],
        },
        {"bufferView": normal_view, "componentType": FLOAT, "count": 3, "type": "VEC3"},
        {"bufferView": uv_view, "componentType": FLOAT, "count": 3, "type": "VEC2"},
        {
            "bufferView": index_view,
            "componentType": UNSIGNED_SHORT,
            "count": 3,
            "type": "SCALAR",
        },
    ],
    "bufferViews": views,
    "buffers": [{"byteLength": len(blob)}],
}

GLB_MAGIC = 0x46546C67
JSON_CHUNK = 0x4E4F534A
BIN_CHUNK = 0x004E4942

json_chunk = json.dumps(gltf, separators=(",", ":")).encode("utf-8")
json_chunk += b" " * (-len(json_chunk) % 4)
bin_chunk = bytes(blob)
bin_chunk += bytes(-len(bin_chunk) % 4)

total = 12 + 8 + len(json_chunk) + 8 + len(bin_chunk)
glb = struct.pack("<III", GLB_MAGIC, 2, total)
glb += struct.pack("<II", len(json_chunk), JSON_CHUNK) + json_chunk
glb += struct.pack("<II", len(bin_chunk), BIN_CHUNK) + bin_chunk

out = HERE / "embedded_texture.glb"
out.write_bytes(glb)
print("wrote " + str(out) + " (" + str(len(glb)) + " bytes)")
