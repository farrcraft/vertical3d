# ADR-0042: Rendering: world-space sprites

**Status**: amended
**Date**: 2026-09-07
**Amends**: [ADR-0005](0005-2d-one-batched-quad-pipeline.md)
**Amended by**: [ADR-0082](0082-textures-owned-by-the-device-context.md)
**Documented in**: [api/rendering/LinesAndWorldQuads.md](../api/rendering/LinesAndWorldQuads.md)

## Context

The realtime renderer has two primitives. `realtime::Canvas` draws textured quads in canvas
pixels, through an orthographic projection it pushes itself, and ignores the pass camera.
`realtime::LineCanvas` draws lines in world space through the pass camera at set 0. Nothing
draws a textured rectangle at a world position, which a filled grid tile and a sprite in an
isometric game both need. [ADR-0005](0005-2d-one-batched-quad-pipeline.md) made the batched
quad the one 2D primitive, but its vertex carries a 2D position.

## Decision

A third primitive, `realtime::WorldCanvas`, collects textured quads whose vertices carry a 3D
world position, and `vulkan::renderer::World` draws them through the pass camera at set 0, as
[ADR-0011](0011-rendering-lines-as-a-world-space-primitive.md) does for lines, with the quad
pipeline's fragment stage. Quads are drawn in the order they were added, and the caller decides
that order. In a pass with a depth buffer the pipeline tests depth and does not write it.

## Alternatives

### `Canvas` draws some batches in world space
- **For**: One canvas type for an app to fill. The transform stack, the clip, text and the
  shape helpers would all work in world space.
- **Against**: The position widens to 3D for every vertex, including every glyph of every
  label, or the class grows a second vertex stream. `Canvas` defines screen space: its clip is a
  scissor measured in its pixels, and its `translate` and `scale` take 2D values.
- **Rejected because**: It is a larger change to a class every 2D app depends on, made to avoid
  adding one type.

### The renderer sorts quads by distance from the camera
- **For**: A caller can add quads in any order and get a correct picture.
- **Against**: The renderer has to choose what a quad's distance is (its centre, its nearest
  corner, its base), and every choice is wrong for some projection. In a 3/4 isometric view
  the sprite behind is the one whose base is further up the ground plane, not the one further
  from the camera. The sort also has to run after the transform stack is applied.
- **Rejected because**: It would be silently wrong for some apps. The caller knows its
  projection; the renderer does not.

### Depth tested and written
- **For**: No ordering question, and correct against the rest of the scene in both directions.
- **Against**: A blended quad that writes depth hides whatever is behind its whole rectangle,
  transparent parts included. An alpha cutout or alpha to coverage avoids that but gives hard
  edges, which a stylised 2D look does not want.
- **Rejected because**: It replaces the ordering problem with a worse one.

## Consequences

- **Gains**:
  - `grid::Overlay` can fill a tile through a quad sink beside its line sink, and still names
    no renderer.
  - A sprite at a world position needs no projection on the CPU, and a second viewport of the
    same world costs a second pass.
  - A world quad samples an atlas exactly as a 2D sprite does, and batching follows ADR-0005's
    rule unchanged.
- **Costs**:
  - Three canvas types. The choice is by space: screen pixels is `Canvas`, a world position is
    `WorldCanvas` or `LineCanvas`.
  - The caller sorts. Quads added in entity order draw in the wrong order and nothing warns.
    `DepthOrder` is one way to sort them.
  - No clip ([ADR-0037](0037-2d-clip-with-a-per-batch-scissor.md)), no billboarding, and no
    back-face culling, so an upright quad is square-on from one direction only and its winding
    cannot hide it.
  - A pass whose camera was never set draws world quads in clip space with no error, as it
    does lines.
  - A world canvas and a line canvas in one pass draw in submission order and cannot
    interleave by depth. The layer argument is the only control, and it is coarse.
- **Revisit when**: an app needs world quads clipped, billboarded, or interleaved by depth with
  other primitives in one pass.
