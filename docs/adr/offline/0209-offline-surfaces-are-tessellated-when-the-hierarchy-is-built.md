# ADR-0209: Offline: surfaces are tessellated when the hierarchy is built

**Status**: accepted
**Date**: 2026-11-08
**Tags**: offline
**Documented in**: [offline/Geometry.md](../../offline/Geometry.md)

## Context

A subdivision surface or a displaced surface becomes triangles before a ray meets it, cut finely
enough that no edge is longer than a length in pixels. A GPU's hardware intersects triangles, and
moya's other primitives by its own code. A tessellation that depends on the render camera
changes whenever the camera moves. Two patches that cut a shared edge differently leave a crack
between them.

## Decision

Every surface is tessellated on the CPU when the hierarchy is built, to an edge length in pixels
measured from a dicing camera that is the render camera unless the scene names another. A shared
edge is cut alike by both patches. OpenSubdiv evaluates the limit surface at the tessellator's
points.

## Alternatives

### On demand, into a cache of limited size
- **For**: Memory scales with what rays reach, and detail can follow each ray's footprint, as
  RenderMan's geometry cache does.
- **Against**: A GPU needs a tessellator of its own, or a round trip to the CPU, and the cache is
  shared between threads.
- **Rejected because**: It breaks the rule that a GPU sees only triangles and moya's own
  primitives.

### An edge length measured in object space
- **For**: Stable under camera moves and instancing.
- **Against**: Not sized to the pixel, so near surfaces are coarse and far ones wasted.
- **Rejected because**: A dicing camera gives the same stability and keeps the pixel measure.

### OpenSubdiv's uniform refinement, cut further by moya
- **For**: Simpler use of OpenSubdiv.
- **Against**: The refined mesh is not the limit surface, and every patch is refined to the
  finest level any part needs.
- **Rejected because**: Cycles evaluates the limit surface at its dicing points for both reasons.

## Consequences

- **Gains**:
  - Every device sees the same triangles.
  - A dicing camera keeps tessellation still while the render camera moves.
  - Surfaces meet without cracks.
- **Costs**:
  - Every tessellated triangle is held in memory, including those no ray reaches.
  - The first render waits for tessellation, and moving the dicing camera tessellates again.
  - An instance is cut once, for its nearest placement or another the scene names.
- **Revisit when**: scenes exceed memory once tessellated, or GPUs tessellate on demand at a speed
  that matches moya's CPU.
