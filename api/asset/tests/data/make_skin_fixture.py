#!/usr/bin/env python3
"""Generate bending_strip.glb and bending_strip_shuffled.glb, the fixtures a skin is asserted
against. No rigged asset exists in the tree, and every number here is chosen to be exact in
float, so that what a loader or a sampler makes of them can be compared with no tolerance.

bending_strip.glb

  A strip two units wide and three tall along +y, a row of two vertices every half unit, bound
  to a chain of three joints standing at y = 0, 1 and 2:

    node 0  "strip"     the skinned mesh, translated (0, 0, 7) - which a loader has to ignore,
                        because a skinned mesh is placed by its joints alone
    node 1  "armature"  above the skeleton, at the identity
    node 2  "root"      a joint at (0, 0, 0)
    node 3  "middle"    a joint, (0, 1, 0) from root
    node 4  "top"       a joint, (0, 1, 0) from middle
    node 5  "tip"       an unskinned triangle, (0, 0.5, 0) from top, which follows top rigidly

  Every joint's rotation is the identity and its translation a whole number, so each inverse
  bind matrix is an exact negated translation. A vertex below y = 1 is wholly root's, one
  between 1 and 2 wholly middle's, one above 2 wholly top's, and the rows at 1 and 2 are half
  and half - every weight exact. Joints are unsigned shorts and weights floats.

  The strip is material 0 and the tip material 1, so the model is two parts.

  Three clips, each a second long:

    bend   middle turns a quarter about +z, linearly, so top swings to (-1, 1, 0). It also
           moves the tip, which is not a joint, and which a loader drops
    step   the same turn, held at the first key until the second
    cubic  top rises from y = 1 to 2 as a cubic spline, with an out tangent of +x and an in
           tangent of -x, which at a half puts it at (0.25, 1.5, 0)

bending_strip_shuffled.glb

  The same skeleton listed child first in the skin - top, root, middle - so a loader has to
  reorder it and remap every vertex's joints. The armature stands at (0, 0, 4), which the
  inverse bind matrices and the strip's positions include, and which a loader keeps as the
  skeleton's root. Joints are unsigned bytes and weights normalised unsigned bytes. No tip.

Run from the root of the tree:
  python api/asset/tests/data/make_skin_fixture.py
"""

import json
import pathlib
import struct

ARRAY_BUFFER = 34962
ELEMENT_ARRAY_BUFFER = 34963
FLOAT = 5126
UNSIGNED_BYTE = 5121
UNSIGNED_SHORT = 5123

ROWS = [0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0]
COLUMNS = [-1.0, 1.0]

# the joints by name, in the skeleton's own order, and where each stands at bind
JOINTS = ["root", "middle", "top"]
BIND_Y = [0.0, 1.0, 2.0]


def influence(y):
    """The joints a row follows, by skeleton index, and how far."""
    if y < 1.0:
        return [(0, 1.0)]
    if y == 1.0:
        return [(0, 0.5), (1, 0.5)]
    if y < 2.0:
        return [(1, 1.0)]
    if y == 2.0:
        return [(1, 0.5), (2, 0.5)]
    return [(2, 1.0)]


def translation(x, y, z):
    """A column major 4x4 translation, as glTF stores a matrix."""
    return [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, y, z, 1]


class Writer:
    def __init__(self):
        self.blob = bytearray()
        self.accessors = []
        self.views = []

    def view(self, data, target=None):
        while len(self.blob) % 4:
            self.blob.append(0)
        offset = len(self.blob)
        self.blob.extend(data)
        view = {"buffer": 0, "byteOffset": offset, "byteLength": len(data)}
        if target is not None:
            view["target"] = target
        self.views.append(view)
        return len(self.views) - 1

    def accessor(self, data, component, count, kind, target=None, **extra):
        accessor = {"bufferView": self.view(data, target), "componentType": component,
                    "count": count, "type": kind}
        accessor.update(extra)
        self.accessors.append(accessor)
        return len(self.accessors) - 1

    def glb(self, document, name):
        document["accessors"] = self.accessors
        document["bufferViews"] = self.views
        document["buffers"] = [{"byteLength": len(self.blob)}]
        json_chunk = json.dumps(document, separators=(",", ":")).encode("utf-8")
        json_chunk += b" " * (-len(json_chunk) % 4)
        bin_chunk = bytes(self.blob) + bytes(-len(self.blob) % 4)
        total = 12 + 8 + len(json_chunk) + 8 + len(bin_chunk)
        out = struct.pack("<III", 0x46546C67, 2, total)
        out += struct.pack("<II", len(json_chunk), 0x4E4F534A) + json_chunk
        out += struct.pack("<II", len(bin_chunk), 0x004E4942) + bin_chunk
        path = pathlib.Path(__file__).parent / name
        path.write_bytes(out)
        print("wrote " + str(path) + " (" + str(len(out)) + " bytes)")


def strip(writer, z, skin_index, joint_format, weight_format):
    """The strip's mesh: its vertices at the given z, and its joints written by skin index."""
    positions = b""
    normals = b""
    joints = b""
    weights = b""
    for y in ROWS:
        for x in COLUMNS:
            positions += struct.pack("<3f", x, y, z)
            normals += struct.pack("<3f", 0.0, 0.0, 1.0)
            pairs = influence(y) + [(0, 0.0)] * (4 - len(influence(y)))
            joints += struct.pack(joint_format, *[skin_index[joint] for joint, _ in pairs])
            weights += weight_format([weight for _, weight in pairs])

    indices = b""
    for row in range(len(ROWS) - 1):
        a, b = row * 2, row * 2 + 1
        c, d = a + 2, b + 2
        indices += struct.pack("<6H", a, b, d, a, d, c)

    count = len(ROWS) * len(COLUMNS)
    joint_component = UNSIGNED_SHORT if joint_format.endswith("H") else UNSIGNED_BYTE
    weight_component, normalized = weight_format.component
    attributes = {
        "POSITION": writer.accessor(positions, FLOAT, count, "VEC3", ARRAY_BUFFER,
                                    min=[-1.0, 0.0, z], max=[1.0, 3.0, z]),
        "NORMAL": writer.accessor(normals, FLOAT, count, "VEC3", ARRAY_BUFFER),
        "JOINTS_0": writer.accessor(joints, joint_component, count, "VEC4", ARRAY_BUFFER),
        "WEIGHTS_0": writer.accessor(weights, weight_component, count, "VEC4", ARRAY_BUFFER,
                                     **({"normalized": True} if normalized else {})),
    }
    index_accessor = writer.accessor(indices, UNSIGNED_SHORT, len(indices) // 2, "SCALAR",
                                     ELEMENT_ARRAY_BUFFER)
    return {"primitives": [{"attributes": attributes, "indices": index_accessor, "material": 0}]}


def float_weights(values):
    return struct.pack("<4f", *values)


float_weights.component = (FLOAT, False)


def byte_weights(values):
    # a half and a half are 128 and 127, which still sum to 255; a loader renormalises
    scaled = [round(value * 255) for value in values]
    if scaled.count(128) == 2:
        scaled[scaled.index(128, scaled.index(128) + 1)] = 127
    return struct.pack("<4B", *scaled)


byte_weights.component = (UNSIGNED_BYTE, True)


def inverse_binds(writer, skin_order, armature_z):
    data = b""
    for joint in skin_order:
        data += struct.pack("<16f", *translation(0.0, -BIND_Y[joint], -armature_z))
    return writer.accessor(data, FLOAT, len(skin_order), "MAT4")


MATERIALS = [
    {"pbrMetallicRoughness": {"baseColorFactor": [1.0, 1.0, 1.0, 1.0]}},
    {"pbrMetallicRoughness": {"baseColorFactor": [1.0, 0.0, 0.0, 1.0]}},
]


QUARTER = 0.5 ** 0.5


def animations(writer):
    """The three clips, over the plain file's nodes."""
    times = writer.accessor(struct.pack("<2f", 0.0, 1.0), FLOAT, 2, "SCALAR", min=[0.0], max=[1.0])
    turn = writer.accessor(struct.pack("<8f", 0, 0, 0, 1, 0, 0, QUARTER, QUARTER), FLOAT, 2, "VEC4")
    lift = writer.accessor(struct.pack("<6f", 0, 0, 0, 0, 1, 0), FLOAT, 2, "VEC3")
    # in tangent, value, out tangent, for each of the two keys
    rise = writer.accessor(struct.pack("<18f", 0, 0, 0, 0, 1, 0, 1, 0, 0, -1, 0, 0, 0, 2, 0, 0, 0, 0),
                           FLOAT, 6, "VEC3")
    return [
        {
            "name": "bend",
            "samplers": [{"input": times, "output": turn, "interpolation": "LINEAR"},
                         {"input": times, "output": lift, "interpolation": "LINEAR"}],
            "channels": [{"sampler": 0, "target": {"node": 3, "path": "rotation"}},
                         {"sampler": 1, "target": {"node": 5, "path": "translation"}}],
        },
        {
            "name": "step",
            "samplers": [{"input": times, "output": turn, "interpolation": "STEP"}],
            "channels": [{"sampler": 0, "target": {"node": 3, "path": "rotation"}}],
        },
        {
            "name": "cubic",
            "samplers": [{"input": times, "output": rise, "interpolation": "CUBICSPLINE"}],
            "channels": [{"sampler": 0, "target": {"node": 4, "path": "translation"}}],
        },
    ]


def plain():
    writer = Writer()
    skin_order = [0, 1, 2]
    skin_index = {joint: joint for joint in skin_order}
    tip = writer.accessor(struct.pack("<9f", 0, 0, 0, 0.25, 0, 0, 0, 0.25, 0), FLOAT, 3, "VEC3",
                          ARRAY_BUFFER, min=[0.0, 0.0, 0.0], max=[0.25, 0.25, 0.0])
    document = {
        "asset": {"version": "2.0", "generator": "vertical3d test fixture generator"},
        "scene": 0,
        "scenes": [{"nodes": [0, 1]}],
        "nodes": [
            {"name": "strip", "mesh": 0, "skin": 0, "translation": [0.0, 0.0, 7.0]},
            {"name": "armature", "children": [2]},
            {"name": "root", "children": [3]},
            {"name": "middle", "translation": [0.0, 1.0, 0.0], "children": [4]},
            {"name": "top", "translation": [0.0, 1.0, 0.0], "children": [5]},
            {"name": "tip", "mesh": 1, "translation": [0.0, 0.5, 0.0]},
        ],
        "meshes": [
            strip(writer, 0.0, skin_index, "<4H", float_weights),
            {"primitives": [{"attributes": {"POSITION": tip}, "material": 1}]},
        ],
        "skins": [{"joints": [2, 3, 4], "inverseBindMatrices": inverse_binds(writer, skin_order, 0.0)}],
        "animations": animations(writer),
        "materials": MATERIALS,
    }
    writer.glb(document, "bending_strip.glb")


def shuffled():
    writer = Writer()
    skin_order = [2, 0, 1]
    skin_index = {joint: skin_order.index(joint) for joint in skin_order}
    document = {
        "asset": {"version": "2.0", "generator": "vertical3d test fixture generator"},
        "scene": 0,
        "scenes": [{"nodes": [0, 1]}],
        "nodes": [
            {"name": "strip", "mesh": 0, "skin": 0},
            {"name": "armature", "translation": [0.0, 0.0, 4.0], "children": [2]},
            {"name": "root", "children": [3]},
            {"name": "middle", "translation": [0.0, 1.0, 0.0], "children": [4]},
            {"name": "top", "translation": [0.0, 1.0, 0.0]},
        ],
        "meshes": [strip(writer, 4.0, skin_index, "<4B", byte_weights)],
        "skins": [{"joints": [4, 2, 3], "inverseBindMatrices": inverse_binds(writer, skin_order, 4.0)}],
        "materials": MATERIALS[:1],
    }
    writer.glb(document, "bending_strip_shuffled.glb")


plain()
shuffled()
