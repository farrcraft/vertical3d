#!/usr/bin/env python3
"""Generate three_primitives.glb, the fixture the gltf loader is asserted against.

One mesh with three primitives, each a separate triangle stepped along x so a merge that
loses one is visible in the positions rather than only in a count:

  primitive 0  indexed, at x = 0
  primitive 1  indexed, at x = 2      its indices run 0,1,2 like the first one's, so the
                                      merge is only correct if they are rebased
  primitive 2  not indexed, at x = 4  merging it needs an index run synthesised, or its
                                      geometry is dropped rather than drawn

So the merged model is nine vertices and the index run 0..8.

The material carries a base colour factor and names an external base colour texture. The
image is never written: a loader hands the name over for the asset manager to resolve, so
nothing here has to decode one, and a fixture that shipped an image would be asserting the
png reader instead.

Run from the root of the tree:
  python api/asset/tests/data/make_model_fixture.py
"""

import json
import pathlib
import struct

# Three triangles separated along x, so a rebasing or a synthesis failure moves geometry
# rather than only changing a count.
TRIANGLES = [
    [(0.0, 0.0, 0.0), (1.0, 0.0, 0.0), (0.0, 1.0, 0.0)],
    [(2.0, 0.0, 0.0), (3.0, 0.0, 0.0), (2.0, 1.0, 0.0)],
    [(4.0, 0.0, 0.0), (5.0, 0.0, 0.0), (4.0, 1.0, 0.0)],
]
INDEXED = [True, True, False]
NORMAL = (0.0, 0.0, 1.0)
UVS = [(0.0, 0.0), (1.0, 0.0), (0.0, 1.0)]

ARRAY_BUFFER = 34962
ELEMENT_ARRAY_BUFFER = 34963
FLOAT = 5126
UNSIGNED_SHORT = 5123

blob = bytearray()
accessors = []
views = []
primitives = []


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


for triangle, indexed in zip(TRIANGLES, INDEXED):
    positions = b"".join(struct.pack("<3f", *vertex) for vertex in triangle)
    normals = b"".join(struct.pack("<3f", *NORMAL) for _ in triangle)
    uvs = b"".join(struct.pack("<2f", *uv) for uv in UVS)

    position_view = add_view(positions, ARRAY_BUFFER)
    normal_view = add_view(normals, ARRAY_BUFFER)
    uv_view = add_view(uvs, ARRAY_BUFFER)

    xs = [vertex[0] for vertex in triangle]
    ys = [vertex[1] for vertex in triangle]
    zs = [vertex[2] for vertex in triangle]

    base = len(accessors)
    accessors.append(
        {
            "bufferView": position_view,
            "componentType": FLOAT,
            "count": 3,
            "type": "VEC3",
            "min": [min(xs), min(ys), min(zs)],
            "max": [max(xs), max(ys), max(zs)],
        }
    )
    accessors.append(
        {"bufferView": normal_view, "componentType": FLOAT, "count": 3, "type": "VEC3"}
    )
    accessors.append(
        {"bufferView": uv_view, "componentType": FLOAT, "count": 3, "type": "VEC2"}
    )

    primitive = {
        "attributes": {
            "POSITION": base,
            "NORMAL": base + 1,
            "TEXCOORD_0": base + 2,
        },
        "material": 0,
    }

    if indexed:
        index_view = add_view(struct.pack("<3H", 0, 1, 2), ELEMENT_ARRAY_BUFFER)
        accessors.append(
            {
                "bufferView": index_view,
                "componentType": UNSIGNED_SHORT,
                "count": 3,
                "type": "SCALAR",
            }
        )
        primitive["indices"] = base + 3

    primitives.append(primitive)

gltf = {
    "asset": {"version": "2.0", "generator": "vertical3d test fixture generator"},
    "scene": 0,
    "scenes": [{"nodes": [0]}],
    "nodes": [{"mesh": 0}],
    "meshes": [{"primitives": primitives}],
    "materials": [
        {
            "pbrMetallicRoughness": {
                "baseColorFactor": [0.25, 0.5, 0.75, 1.0],
                "baseColorTexture": {"index": 0},
                "metallicFactor": 0.0,
                "roughnessFactor": 1.0,
            }
        }
    ],
    "textures": [{"source": 0}],
    "images": [{"uri": "albedo.png"}],
    "accessors": accessors,
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

out = pathlib.Path(__file__).parent / "three_primitives.glb"
out.write_bytes(glb)
print("wrote " + str(out) + " (" + str(len(glb)) + " bytes)")
