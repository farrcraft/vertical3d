# ADR-0037: 2D: clip with a per-batch scissor

**Status**: accepted
**Date**: 2026-09-06
**Documented in**: [api/rendering/Canvas.md](../api/rendering/Canvas.md)

## Context

A scroll view shows part of what it holds, which needs a translation and a clip, and a panel
longer than its window has to be cut off at the window's edge
([ADR-0034](0034-ui-layout-is-resolved-while-drawing.md),
[ADR-0035](0035-ui-immediate-mode-beside-the-retained-tree.md)). Some content, such as a dropped
menu, overflows its parent on purpose, so clipping cannot be universal. The 2D canvas already
cuts its quad stream into batches where the texture or text flag changes
([ADR-0005](0005-2d-one-batched-quad-pipeline.md),
[ADR-0036](0036-text-sdf-glyphs-through-the-quad-shader.md)), and each batch becomes one draw item.

## Decision

A clip rectangle is canvas state, kept on a stack beside the transform and intersected with any
enclosing clip. Each batch carries its rectangle the way it carries a texture, and the recorder
applies it as a dynamic scissor on that draw. A quad that straddles the edge is drawn whole and
the device keeps only the part inside.

## Alternatives

### Clip the geometry on the CPU as it is added
- **For**: no device state and no batch break, and the clip could be any shape the clipper
  handles.
- **Against**: every primitive needs its own clipping code. A rectangle is easy, an arc needs
  polygon clipping, and a glyph needs its texture coordinates interpolated at the cut. All of it
  runs every frame on the CPU.
- **Rejected because**: it is a large amount of arithmetic to avoid state the device sets for
  free.

### Discard in the fragment shader against a rectangle in the push constants
- **For**: no more draws than a scissor, since the rectangle rides in the push constants a batch
  already has. Unlike a scissor, the rectangle could have rounded corners.
- **Against**: pays per fragment for what the rasteriser does for free, since fragments outside
  the clip are still shaded up to the discard. It also puts a UI concern into the shader every
  2D draw uses.
- **Rejected because**: it has the same batching cost and a worse per-pixel cost.

### A stencil buffer
- **For**: the general answer: any shape, nested as deep as the stencil's bit count allows.
- **Against**: the 2D pass has no stencil attachment and the quad pipelines do not test one. It
  needs a format change, more pipelines, and a mask drawn before the content of every clipped
  box.
- **Rejected because**: it is out of proportion to a rectangle inside a rectangle.

## Consequences

- **Gains**:
  - A scissor costs nothing in the rasteriser, needs no pipeline change, and cuts exactly at a
    pixel, so a glyph is cut mid-stroke rather than dropped.
  - The clip belongs to the canvas, so any 2D drawing can use it, not only the UI.
  - A clipped region costs about one extra draw at each of its edges, the same as a texture
    change.
- **Costs**:
  - Two clipped boxes side by side cannot share a batch, however alike they are.
  - A scissor is axis aligned, so a clip cannot be rotated or follow rounded corners.
  - The rectangle is fixed through the transform when it is pushed. Translating afterwards moves
    what is drawn, not the clip, which suits scrolling but surprises a first reader.
  - Canvas pixels match framebuffer pixels only when the pass covers the whole target. A canvas
    drawn into an offset viewport is clipped in the wrong place, and nothing detects it.
- **Revisit when**: a clip has to follow rounded corners. The fragment-shader option then becomes
  worth its cost, with the scissor kept as the outer bound.
