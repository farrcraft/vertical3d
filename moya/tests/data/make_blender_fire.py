"""Generate blender_fire.vdb: a small fire simulated by Blender's own Mantaflow and written by its
own OpenVDB cache, so that moya's reference of a fire glows from grids another tool wrote.

  Domain  a gas domain two units on a side, from z zero to two, at a resolution of 32
  Flow    a sphere of a quarter unit's radius, low in the domain, that lets fire and smoke in

The simulation runs for sixteen frames, and the cache's last frame is the fixture. Its grids are
Mantaflow's: density is the smoke, flame is the fire's reaction from zero to one, and temperature
is its heat. Each is placed in Blender's world, which is z up.

Run with Blender 5.2 from the root of the tree:
  blender --background --factory-startup --python moya/tests/data/make_blender_fire.py
"""

import pathlib
import shutil
import tempfile

import bpy

OUT = pathlib.Path(__file__).resolve().parent / "blender_fire.vdb"
FRAMES = 16

bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.frame_start = 1
scene.frame_end = FRAMES

with tempfile.TemporaryDirectory() as cache:
    bpy.ops.mesh.primitive_cube_add(size=2.0, location=(0.0, 0.0, 1.0))
    domain = bpy.context.active_object
    domain.name = "Domain"
    fluid = domain.modifiers.new("Fluid", "FLUID")
    fluid.fluid_type = "DOMAIN"
    settings = fluid.domain_settings
    settings.domain_type = "GAS"
    settings.resolution_max = 32
    settings.cache_type = "ALL"
    settings.cache_data_format = "OPENVDB"
    settings.cache_directory = cache
    settings.cache_frame_start = 1
    settings.cache_frame_end = FRAMES

    bpy.ops.mesh.primitive_uv_sphere_add(radius=0.25, location=(0.0, 0.0, 0.4))
    flow = bpy.context.active_object
    flow.name = "Flow"
    source = flow.modifiers.new("Fluid", "FLUID")
    source.fluid_type = "FLOW"
    source.flow_settings.flow_type = "BOTH"
    source.flow_settings.flow_behavior = "INFLOW"

    bpy.context.view_layer.objects.active = domain
    domain.select_set(True)
    with bpy.context.temp_override(object=domain, active_object=domain, scene=scene):
        bpy.ops.fluid.bake_all()
    shutil.copyfile(pathlib.Path(cache) / "data" / f"fluid_data_{FRAMES:04d}.vdb", OUT)

print(f"wrote {OUT}")
