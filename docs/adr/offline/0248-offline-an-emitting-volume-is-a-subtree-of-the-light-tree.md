# ADR-0248: Offline: an emitting volume is a subtree of the light tree

**Status**: accepted
**Date**: 2026-11-16
**Tags**: offline, volumes, lighting
**Documented in**: [offline/Volumes.md](../../offline/Volumes.md)

## Context

A fire or a glowing gas lights the scene from inside a volume, and a path that only gathers
emission along the stretches it tracks through rarely finds a small, hot one. The light tree picks
a light by what it may bring to a shading point, and gives each emitting geometry a subtree over
its triangles. A grid is held as NanoVDB, whose leaf nodes are bricks of eight voxels a side, each
with a box. A volume's emission is a MaterialX shader's output, which varies over the volume and is
known only by running its program. Null-scattering tracking weighs a light sample against a
tracked path by MIS over the whole path.

## Decision

Each emitting volume has a subtree of the light tree over its NanoVDB leaf nodes, whose boxes and
summed emission come from running the shader on the host. A light sample draws a brick by the
tree, a voxel by its emission and a point in the voxel, carries the transmittance on its way out,
and is weighed against tracked emission by MIS.
An emitting medium inside a closed surface is one leaf, whose points are drawn uniformly in the
surface's box, a point outside the surface emitting nothing.

## Alternatives

### Emission gathered by tracking alone
- **For**: Nothing to build, as pbrt-v4 gathers it.
- **Against**: A small, hot volume that lights a room is found only by the rare path that tracks
  into it, so the room is noisy.
- **Rejected because**: A fire lighting a scene is the case it renders worst.

### One distribution per volume
- **For**: Simpler than a subtree: a table over each volume's voxels with MIS against tracking,
  as Villemin and Hery give it.
- **Against**: It draws a whole volume by power, ignoring how far each part lies, and stands
  beside the tree, so a scene's lights compete by count rather than by power.
- **Rejected because**: The tree already weighs every light by distance and power.

## Consequences

- **Gains**:
  - A small emitting volume lights a scene with little noise, and a large one favours the bricks
    near the shading point.
  - Emitting volumes, mesh lights and every other light are weighed in one tree.
  - Light linking applies to a volume's light as to any light of the tree.
- **Costs**:
  - The host runs the volume shader over every voxel of an emitting volume to find the powers.
  - Each brick holds a distribution over its voxels.
  - A medium inside a closed surface wastes the samples that land outside the surface in its box.
- **Revisit when**: A volume's emission can be bounded without running its shader on every voxel,
  or path guiding learns where volumes' light comes from.
