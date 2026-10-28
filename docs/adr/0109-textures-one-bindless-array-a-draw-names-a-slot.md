# ADR-0109: Textures: one bindless array, a draw names a slot

**Status**: accepted
**Date**: 2026-10-25
**Amends**: ADR-0008
**Documented in**: [internals/realtime/Pipelines.md](../internals/realtime/Pipelines.md), [api/rendering/](../api/rendering/README.md)

## Context

A texture requested by path has to be named by a draw before its image has loaded, and has to
show its pixels once it has. Neither may rewrite a descriptor set that a frame in flight is
reading. Today each texture has a descriptor set of its own at set 1, and code that keeps that
set's handle, such as a mesh part, never sees a different image. Materials that sample several
textures, sprite batches that change texture between quads, and textures streamed a mip level at
a time all need a texture to change what a draw reads without the draw changing. Every 1.3
device this project targets, and the software driver CI runs, supports descriptor indexing.

## Decision

Every two-dimensional texture a context holds is a slot in one array of combined image samplers.
The array is bound once per pass in place of a set per texture, and a textured draw names its
texture by slot index. A texture's handle resolves to a slot. Filling or replacing a texture
writes its image into a free slot and points the handle there, and the old slot is retired
through the frame ring.

## Alternatives

### Swap the image behind a stable handle, and rewrite its sets
- **For**: keeps one set per texture as ADR-0008 has it, and every pipeline layout and shader as
  they are. It is the texture reference Unreal draws through.
- **Against**: each set that names a texture has to be found and written again when it fills.
  Sets that other code writes, a full-screen pass's or a material with several textures, need
  the texture to track them. A batch still breaks at every change of texture.
- **Rejected because**: the tracking grows with every kind of set that names a texture, where a
  slot index needs none.

### A pending material that draws a placeholder until the texture loads
- **For**: the smallest change: a texture resolves to nothing until it loads, and its material is
  the placeholder's until then.
- **Against**: a caller that kept the material handle draws the placeholder for ever, unless it
  asks again, which is the stale binding other engines have had to fix.
- **Rejected because**: the handles callers keep are the reason the question exists.

## Consequences

- **Gains**: a texture handle a caller holds sees its texture fill or change, in every textured
  draw, with nothing tracked. A batch of quads can change texture without a new draw. A material may
  name several textures as several indices.
- **Costs**: a device without runtime descriptor arrays, partially bound bindings and non-uniform
  indexing of sampled images is not selected. Every textured pipeline layout and shader changes,
  and a game that replaces a lit shader writes against the new layout. The array has a fixed
  size, which bounds how many textures a context holds at once. A full-screen pass keeps a set of
  its own for its sources, which mix two-dimensional, depth and volume images and are named as
  they are when the source is made.
- **Revisit when**: a device the project must support lacks descriptor indexing, or descriptor
  buffers replace descriptor sets on every target.
