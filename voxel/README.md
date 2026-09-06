# Voxel

A Minecraft inspired random voxel world generation app.


Ported onto the current api and Vulkan on 2026-09-01: terrain draws through a depth tested,
sorted pass of voxel's own pipeline, one draw item per meshed chunk, with a second pass of
batched quads for the debug overlay and the game menu. [../docs/audits/completed/VoxelSurvey.md](../docs/audits/completed/VoxelSurvey.md)
is the record of what it was before that.

## Dependencies

Everything but libnoise comes from the repository's own libraries and its vcpkg manifest — see
[../docs/Dependencies.md](../docs/Dependencies.md). Voxel is the only app that links
[libnoise](https://github.com/eXpl0it3r/libnoise), which is a git submodule built separately, and
it will not link without it. Its test suite links libnoise too: `Chunk` is built against a
`TerrainMap`, and the vtable of the flat one a test supplies refers to the perlin implementation
whether or not any case generates noise.
