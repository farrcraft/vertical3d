"""Generate scene.glb: a small scene built and exported by Blender's own glTF exporter, so the
interchange reader is asserted against what a real exporter writes.

  Rig    an empty at (2, 0, 0), which groups the box
  Box    a unit cube one unit above the rig, its child. Its location is keyed from x 0 at frame
         1 to x 4 at frame 25, a second at 24 frames a second, with linear interpolation
  Shot   a perspective camera at (0, -10, 2), turned 80 degrees about x so it looks along +y
         and a little down. It sees 40 degrees from bottom to top, and clips at 0.5 and 200
  Lamp   a spot light at (0, 0, 5) shining straight down, 60 degrees across, with a soft
         edge a quarter of its half angle wide, coloured (1, 0.5, 0.25)

Blender is z up and glTF y up; the exporter converts, so the box sits at (0, 1, 0) in the rig's
space and the lamp shines along -y.

Run with Blender 5.2 from the root of the tree:
  blender --background --factory-startup --python api/interchange/tests/data/make_scene_fixture.py
"""

import math
import pathlib

import bpy

OUT = pathlib.Path(__file__).resolve().parent / "scene.glb"


def clear():
    for thing in list(bpy.data.objects):
        bpy.data.objects.remove(thing, do_unlink=True)
    for mesh in list(bpy.data.meshes):
        bpy.data.meshes.remove(mesh)
    for camera in list(bpy.data.cameras):
        bpy.data.cameras.remove(camera)
    for light in list(bpy.data.lights):
        bpy.data.lights.remove(light)


def link(thing):
    bpy.context.scene.collection.objects.link(thing)
    return thing


def build():
    scene = bpy.context.scene
    scene.render.fps = 24
    scene.render.resolution_x = 1920
    scene.render.resolution_y = 1080
    scene.frame_start = 1
    scene.frame_end = 25

    rig = link(bpy.data.objects.new("Rig", None))
    rig.location = (2.0, 0.0, 0.0)

    bpy.ops.mesh.primitive_cube_add(size=1.0)
    box = bpy.context.active_object
    box.name = "Box"
    box.data.name = "BoxMesh"
    box.parent = rig
    box.location = (0.0, 0.0, 1.0)
    box.keyframe_insert(data_path="location", frame=1)
    box.location = (4.0, 0.0, 1.0)
    box.keyframe_insert(data_path="location", frame=25)
    for curve in box.animation_data.action.layers[0].strips[0].channelbag(box.animation_data.action_slot).fcurves:
        for key in curve.keyframe_points:
            key.interpolation = "LINEAR"

    lens = bpy.data.cameras.new("ShotLens")
    lens.sensor_fit = "VERTICAL"
    lens.lens_unit = "FOV"
    lens.angle_y = math.radians(40.0)
    lens.clip_start = 0.5
    lens.clip_end = 200.0
    shot = link(bpy.data.objects.new("Shot", lens))
    shot.location = (0.0, -10.0, 2.0)
    shot.rotation_euler = (math.radians(80.0), 0.0, 0.0)

    beam = bpy.data.lights.new("LampBeam", type="SPOT")
    beam.spot_size = math.radians(60.0)
    beam.spot_blend = 0.25
    beam.energy = 1000.0
    beam.color = (1.0, 0.5, 0.25)
    lamp = link(bpy.data.objects.new("Lamp", beam))
    lamp.location = (0.0, 0.0, 5.0)


def export():
    bpy.ops.export_scene.gltf(
        filepath=str(OUT),
        export_format="GLB",
        export_yup=True,
        export_cameras=True,
        export_lights=True,
        export_animations=True,
        export_apply=False,
    )


clear()
build()
export()
print("wrote", OUT)
