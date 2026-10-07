"""Generate blender_strip.glb: the bending strip again, built and exported by Blender's own glTF
exporter rather than written by hand, so that the loader is asserted against what a real
exporter writes - node order, inverse bind matrices, a sampled animation and a scaled armature.

Shaped the way a Mixamo rig reaches Blender: the armature object carries an unapplied scale of
a hundredth and its bones are a hundred units long, so the skeleton stands three units tall and
the exporter has to put the scale above the root joint.

  Armature  three bones in a chain up +z - root, middle and top - each 100 units
  Strip     a strip two units wide and three tall, a row every half unit, parented to the
            armature with an Armature modifier and weighted as make_skin_fixture.py's: wholly
            to one bone between the joins and half and half on them
  bend      an action turning middle a quarter about its own x over one second, at 24 frames a
            second, which the exporter samples into a key a frame
  sway      an action turning root a twentieth of a turn and back, so the file has two clips

Blender is z up and glTF y up; the exporter converts, so the strip comes out standing along +y.

Run with Blender 5.2 from the root of the tree:
  blender --background --factory-startup --python api/asset/tests/data/make_blender_fixture.py
"""

import math
import pathlib

import bpy

OUT = pathlib.Path(__file__).resolve().parent / "blender_strip.glb"

ROWS = [0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0]
COLUMNS = [-1.0, 1.0]
BONES = ["root", "middle", "top"]
SCALE = 0.01
LENGTH = 100.0


def influence(height):
    """The bones a row follows and how far, by the row's height in the finished skeleton."""
    if height < 1.0:
        return [("root", 1.0)]
    if height == 1.0:
        return [("root", 0.5), ("middle", 0.5)]
    if height < 2.0:
        return [("middle", 1.0)]
    if height == 2.0:
        return [("middle", 0.5), ("top", 0.5)]
    return [("top", 1.0)]


def clear():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.render.fps = 24
    scene.frame_start = 1
    scene.frame_end = 25


def armature():
    data = bpy.data.armatures.new("Armature")
    rig = bpy.data.objects.new("Armature", data)
    bpy.context.scene.collection.objects.link(rig)
    rig.scale = (SCALE, SCALE, SCALE)
    bpy.context.view_layer.objects.active = rig
    bpy.ops.object.mode_set(mode="EDIT")
    parent = None
    for index, name in enumerate(BONES):
        bone = data.edit_bones.new(name)
        bone.head = (0.0, 0.0, index * LENGTH)
        bone.tail = (0.0, 0.0, (index + 1) * LENGTH)
        if parent is not None:
            bone.parent = parent
            bone.use_connect = True
        parent = bone
    bpy.ops.object.mode_set(mode="OBJECT")
    return rig


def strip(rig):
    # the mesh is built in the armature's unscaled space, as an import of a Mixamo character is
    vertices = [(x / SCALE, 0.0, height / SCALE) for height in ROWS for x in COLUMNS]
    faces = []
    for row in range(len(ROWS) - 1):
        a, b = row * 2, row * 2 + 1
        faces.append((a, b, b + 2, a + 2))
    mesh = bpy.data.meshes.new("Strip")
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    body = bpy.data.objects.new("Strip", mesh)
    bpy.context.scene.collection.objects.link(body)
    body.parent = rig

    groups = {name: body.vertex_groups.new(name=name) for name in BONES}
    for index, height in enumerate(height for height in ROWS for _ in COLUMNS):
        for name, weight in influence(height):
            groups[name].add([index], weight, "REPLACE")
    modifier = body.modifiers.new("Armature", "ARMATURE")
    modifier.object = rig

    material = bpy.data.materials.new("Strip")
    material.diffuse_color = (1.0, 1.0, 1.0, 1.0)
    mesh.materials.append(material)
    return body


def action(rig, name, bone, keys):
    """An action turning one bone about its own x, keyed by frame, pushed onto its own NLA
    track so that the exporter writes every action as a clip."""
    rig.animation_data_create()
    made = bpy.data.actions.new(name)
    rig.animation_data.action = made
    pose = rig.pose.bones[bone]
    pose.rotation_mode = "QUATERNION"
    for frame, angle in keys:
        pose.rotation_quaternion = (math.cos(angle / 2.0), math.sin(angle / 2.0), 0.0, 0.0)
        pose.keyframe_insert("rotation_quaternion", frame=frame)
    pose.rotation_quaternion = (1.0, 0.0, 0.0, 0.0)
    track = rig.animation_data.nla_tracks.new()
    track.name = name
    track.strips.new(name, keys[0][0], made)
    rig.animation_data.action = None


def main():
    clear()
    rig = armature()
    strip(rig)
    action(rig, "bend", "middle", [(1, 0.0), (25, math.pi / 2.0)])
    action(rig, "sway", "root", [(1, 0.0), (13, math.pi / 10.0), (25, 0.0)])

    bpy.ops.export_scene.gltf(
        filepath=str(OUT),
        export_format="GLB",
        export_yup=True,
        export_skins=True,
        export_animations=True,
        export_animation_mode="ACTIONS",
        export_morph=False,
        export_materials="EXPORT",
    )
    print("wrote " + str(OUT))


main()
