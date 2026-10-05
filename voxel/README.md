# Voxel

A Minecraft-style voxel world with randomly generated terrain. [docs/Games.md](../docs/Games.md#voxel)
covers how to run it, its controls, and which api features it is the reference example for.

Voxel is the only app that links [libnoise](https://github.com/eXpl0it3r/libnoise). libnoise is
a git submodule that is built separately, and voxel and its test suite do not link without it.
[docs/contributing/Dependencies.md](../docs/contributing/Dependencies.md#libnoise) has the steps.
