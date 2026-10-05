# ADR-0011: Rendering: lines as a world-space primitive

**Status**: accepted
**Date**: 2026-09-01
**Documented in**: [api/Rendering.md](../api/Rendering.md)

## Context

The batched quad ([ADR-0005](0005-2d-one-batched-quad-pipeline.md)) serves the games, but a 3D
modeller is mostly lines: a construction grid, an axis marker, wireframe display, highlighted
edges and the manipulator handles. All of these are 3D geometry in the scene, often drawn in
several viewports at once. A line can be faked with two triangles per segment, but those have to
be rebuilt whenever the camera moves to keep facing the viewer at a constant width. The pipeline
builder (`pipeline::Builder`) can already make a line-list pipeline; what is missing is a way to
produce line geometry and a decision about what space it is in.

## Decision

Lines are a second primitive, not quads in disguise. A `LineCanvas` collects segments in world
space, `renderer::Line` draws it as a line list through the camera its pass binds at set 0, and
lines are one pixel wide.

## Alternatives

### Expand every segment into quads and keep one primitive
- **For**: one pipeline, one vertex format and one upload path, and lines batch and order with
  everything else. Line width in pixels comes free, and so do antialiased or dashed lines later.
- **Against**: three times the vertex data. Each quad has to face the viewer at a constant
  screen width, so a world-space grid is projected on the CPU every frame, in every viewport.
- **Rejected because**: it turns a static grid into per-frame CPU work, to keep a uniformity the
  vertex format breaks anyway: a line quad needs its own width and direction attributes.

### A line list in the quad canvas's pixel space
- **For**: one canvas type, and the UI could use lines for borders immediately.
- **Against**: every 3D line has to be projected by the app first, per viewport. Depth testing a
  wireframe becomes impossible once the geometry is flattened.
- **Rejected because**: the primitive exists for the editor, and everything the editor draws
  with it is in the scene rather than on top of it.

### Enable the `wideLines` device feature and carry a width per segment
- **For**: a thicker origin line on the grid, and heavier highlighting for a selected edge.
- **Against**: `wideLines` is optional, so a device without it either fails to start or needs
  the quad expansion after all. Line width is pipeline or dynamic state, so a width per segment
  breaks the batch the way a texture change breaks a quad batch.
- **Rejected because**: colour gives the same emphasis without a device requirement or a
  batching rule.

## Consequences

- **Gains**:
  - The grid, axis marker, wireframe, edge highlight and manipulators are one kind of geometry.
  - An unclipped `LineCanvas` is one draw item, with no texture, material or index buffer.
  - The pass decides occlusion. In a pass with depth, lines are hidden by solid geometry in
    front of them; in a pass without depth, they draw on top.
  - Several viewports cost several passes, not several copies of the geometry.
- **Costs**:
  - A second pipeline and a second canvas type to keep in step with the quad.
  - One pixel wide on every display, which looks thin at high resolution.
  - A `LineCanvas` in a pass whose camera was never set draws in clip space and shows nothing,
    with no error.
  - Segments are not indexed, so a dense mesh wireframe uploads about twice the vertices a
    shared-vertex form would.
- **Revisit when**: the editor needs line width for something colour cannot express, or dense
  wireframes dominate the line count and need an indexed path.
