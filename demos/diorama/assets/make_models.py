"""Generate the diorama's models with Blender's own glTF exporter: ground.glb, house.glb,
tree.glb and well.glb, written to the demo's data/models directory.

Each model is one mesh, joined from simple solids, with a material for each colour and its
transforms applied. The exporter then writes one node holding a primitive per material. Every
model's origin is the centre of its base, so a model stands on the ground where it is placed.

  ground  a slab 16 units square and a fifth of a unit deep, its top at zero
  house   walls two units square and 1.6 tall, a gable roof along x, and a door facing -y
  tree    a trunk a unit tall and a cone of leaves 1.8 tall over it
  well    a stone ring with water in it, two posts and a bar across them

Blender is z up and glTF y up; the exporter converts, so -y here is +z in the files.

Run with Blender 5.2 from the root of the tree:
  blender --background --factory-startup --python demos/diorama/assets/make_models.py
"""

import pathlib

import bmesh
import bpy

OUT = pathlib.Path(__file__).resolve().parent.parent / "data" / "models"

COLOURS = {
    "grass": (0.30, 0.55, 0.25),
    "wall": (0.90, 0.85, 0.70),
    "roof": (0.60, 0.20, 0.15),
    "door": (0.35, 0.22, 0.12),
    "bark": (0.40, 0.26, 0.15),
    "leaves": (0.18, 0.45, 0.20),
    "stone": (0.55, 0.55, 0.58),
    "water": (0.20, 0.40, 0.75),
    "wood": (0.45, 0.30, 0.18),
}


def clear():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def material(name):
    found = bpy.data.materials.get(name)
    if found is not None:
        return found
    made = bpy.data.materials.new(name)
    made.use_nodes = True
    shader = made.node_tree.nodes["Principled BSDF"]
    red, green, blue = COLOURS[name]
    shader.inputs["Base Color"].default_value = (red, green, blue, 1.0)
    shader.inputs["Roughness"].default_value = 1.0
    return made


def solid(name, colour, build):
    """An object whose mesh build() fills through bmesh, in the given material."""
    mesh = bpy.data.meshes.new(name)
    shape = bmesh.new()
    build(shape)
    shape.to_mesh(mesh)
    shape.free()
    mesh.materials.append(material(colour))
    thing = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(thing)
    return thing


def box(name, colour, size, centre):
    def build(shape):
        bmesh.ops.create_cube(shape, size=1.0)
        bmesh.ops.scale(shape, vec=size, verts=shape.verts)
        bmesh.ops.translate(shape, vec=centre, verts=shape.verts)
    return solid(name, colour, build)


def cylinder(name, colour, radius, depth, base, segments=16, tip=None):
    """A cylinder, or a cone when tip is the radius at its top, standing on base."""
    def build(shape):
        bmesh.ops.create_cone(shape, cap_ends=True, segments=segments, radius1=radius,
                              radius2=radius if tip is None else tip, depth=depth)
        bmesh.ops.translate(shape, vec=(base[0], base[1], base[2] + depth / 2.0), verts=shape.verts)
    return solid(name, colour, build)


def roof(name, colour, width, depth, eaves, ridge):
    """A gable roof along x: a prism whose ridge runs over the middle of the walls."""
    def build(shape):
        half_x = width / 2.0
        half_y = depth / 2.0
        corners = [
            (-half_x, -half_y, eaves), (half_x, -half_y, eaves), (half_x, 0.0, ridge), (-half_x, 0.0, ridge),
            (-half_x, half_y, eaves), (half_x, half_y, eaves),
        ]
        verts = [shape.verts.new(corner) for corner in corners]
        # wound counterclockwise seen from outside, so every normal points out
        for face in [(0, 1, 2, 3), (3, 2, 5, 4), (0, 3, 4), (1, 5, 2), (0, 4, 5, 1)]:
            shape.faces.new([verts[index] for index in face])
    return solid(name, colour, build)


def join(name, parts):
    bpy.ops.object.select_all(action="DESELECT")
    for part in parts:
        part.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    joined = bpy.context.view_layer.objects.active
    joined.name = name
    joined.data.name = name
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    return joined


def export(model):
    bpy.ops.object.select_all(action="DESELECT")
    model.select_set(True)
    path = OUT / (model.name + ".glb")
    bpy.ops.export_scene.gltf(
        filepath=str(path),
        export_format="GLB",
        use_selection=True,
        export_yup=True,
        export_apply=True,
        export_skins=False,
        export_animations=False,
        export_morph=False,
        export_materials="EXPORT",
    )
    print("wrote " + str(path))


def ground():
    return join("ground", [box("slab", "grass", (16.0, 16.0, 0.2), (0.0, 0.0, -0.1))])


def house():
    return join("house", [
        box("walls", "wall", (2.0, 2.0, 1.6), (0.0, 0.0, 0.8)),
        roof("roof", "roof", 2.3, 2.4, 1.6, 2.5),
        box("door", "door", (0.5, 0.06, 0.9), (0.0, -1.02, 0.45)),
    ])


def tree():
    return join("tree", [
        cylinder("trunk", "bark", 0.15, 1.0, (0.0, 0.0, 0.0), segments=8),
        cylinder("leaves", "leaves", 0.8, 1.8, (0.0, 0.0, 0.8), segments=12, tip=0.0),
    ])


def well():
    return join("well", [
        cylinder("ring", "stone", 0.7, 0.6, (0.0, 0.0, 0.0)),
        cylinder("water", "water", 0.55, 0.02, (0.0, 0.0, 0.6)),
        box("left post", "wood", (0.1, 0.1, 1.2), (-0.6, 0.0, 1.2)),
        box("right post", "wood", (0.1, 0.1, 1.2), (0.6, 0.0, 1.2)),
        box("bar", "wood", (1.4, 0.12, 0.12), (0.0, 0.0, 1.8)),
    ])


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    for build in (ground, house, tree, well):
        clear()
        export(build())


main()
