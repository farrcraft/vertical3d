# ADR-0011: Line Drawing — A Second Primitive, In World Space, Through The Pass Camera

**Date**: 2026-09-01
**Status**: accepted
**Deciders**: Joshua Farr

## Context

[ADR-0005](0005-one-batched-quad-primitive.md) made the batched quad the one primitive the
engine draws, and four games have been happy with it: a rectangle, a sprite, a glyph and a
menu panel are all the same textured quad. The editor of phase 6 is not a game. A modeller is
mostly lines — a construction grid, an axis decoration in the corner, wireframe and
shaded-wireframe display, a highlighted edge, and the shafts and rings of the translate,
rotate and scale manipulators — and [RigelSurvey.md](../RigelSurvey.md) calls the absence of
any way to draw one the largest single api gap in the tree.

A line can be faked with quads: two triangles per segment, expanded on the cpu or in a
geometry pass. That is what a renderer wanting anti-aliased, screen-space-width lines does,
and it is not free. The question is whether the engine grows a second primitive or bends the
first one, and it has to be settled before the editor is written rather than after, because
whichever way it goes is what the construction plane, the manipulators and the wireframe are
all built on.

`vulkan::PipelineBuilder` already takes `topology()` and `polygon()`, so a line-list pipeline
costs a chained call. What it does not have is anything producing line geometry, a place to
put it, or a decision about what space it is in.

## Decision

Lines are a second primitive, not quads in disguise. `realtime::LineCanvas` accumulates
segments on the cpu and `vulkan::LineRenderer` draws the whole of one as a single
non-indexed `VK_PRIMITIVE_TOPOLOGY_LINE_LIST`. Positions are in **world space** and are
transformed by the camera the pass carries at set 0 — unlike the quad pipeline, which pushes
its own orthographic projection. Lines are one pixel wide.

## Alternatives Considered

### Alternative 1: A second primitive, world space, camera from set 0 — **chosen**
- **Pros**: A line list is the hardware primitive; one segment is two vertices and no
  indices, against six vertices, six indices and a cpu expansion per segment as quads. The
  grid, the wireframe and the manipulators are all 3D, so world space is what they are
  already in — a screen-space primitive would make every one of them project its own
  geometry. Reading set 0 is what [ADR-0008](0008-binding-by-update-frequency.md) says a per
  frame camera is for, and it means multiple viewports cost four passes rather than four
  copies of the geometry.
- **Cons**: A second pipeline pair to keep in step with the first, and a second canvas type
  an app has to know about. Width is fixed at one pixel.
- **Why not**: n/a — chosen.

### Alternative 2: Expand every segment into quads and keep one primitive
- **Pros**: ADR-0005 stands unamended. One pipeline, one vertex format, one upload path, and
  lines batch and order with everything else already drawn. Width in pixels comes free, and
  so does the road to anti-aliased or dashed lines later.
- **Cons**: Three times the vertex data and an expansion that has to be redone whenever the
  camera moves, because the quad has to face the viewer and be a constant width on screen —
  which for a world-space grid means projecting every vertex on the cpu every frame. A
  construction grid is a few thousand segments and there are four viewports of it.
- **Why not**: It turns a static grid into per-frame cpu work proportional to the camera, and
  it does so to preserve a uniformity that the vertex format breaks anyway — a line quad
  needs its own attributes for width and direction.

### Alternative 3: A line list, but in the quad canvas's pixel space
- **Pros**: One canvas type. An app that already fills a `Canvas` gets lines with no new
  concept, and the ui could use them for borders and separators immediately.
- **Cons**: Every 3D line has to be projected by the app before it can be drawn, which is the
  cost of alternative 2 without its benefit, and the projection has to be redone per viewport.
  A depth-tested wireframe becomes impossible — there is nothing left to test against once
  the geometry is flattened.
- **Why not**: The primitive exists for the editor, and everything the editor draws with it is
  in the scene rather than on top of it.

### Alternative 4: Ask for the `wideLines` device feature and carry a width per segment
- **Pros**: The thicker origin line rigel's `ConstructionPlane` wanted, and heavier
  highlighting for a selected edge.
- **Cons**: `wideLines` is an optional feature; a device without it fails the check and the
  engine either refuses to start or needs a fallback path, which is alternative 2 built after
  all. `lineWidth` is also pipeline or dynamic state rather than vertex data, so a width per
  segment cuts the draw the way a texture change cuts a quad batch.
- **Why not**: It buys emphasis, which colour also buys, at the cost of a device requirement
  and a batching rule. Revisit it if a modeller turns out to need width for something colour
  cannot express.

## Consequences

### Positive
- The editor has its primitive, and the grid, the axis decoration, the wireframe, the
  selected-edge highlight and all three manipulators are one type of geometry rather than five
  special cases.
- A whole `LineCanvas` is one `DrawItem`. There is no texture, no material and no index
  buffer, so the renderer is a third the size of `QuadRenderer`.
- Lines are the first thing in `api/render` to read set 0 — the quad pipeline still pushes its
  own projection. That makes the line pipeline the worked example for the per-pass camera the
  quad pipeline is meant to move onto, and for the multiple viewports of phase 6.
- The pass model does the choosing between occluded and overlaid lines. The pipeline built for
  a pass with a depth attachment tests and writes depth, so a wireframe is hidden by the solid
  geometry in front of it; a manipulator that must stay visible goes in a pass without depth.
  No flag, no third pipeline.

### Negative
- ADR-0005's "one primitive" is now "one primitive for 2D". The claim was always about the 2D
  content four games draw, but it read as a claim about the engine, and it no longer is.
- One pixel wide, everywhere, on every display. A 4K editor will look thin.
- A `LineCanvas` in a pass whose camera was never set draws in clip space and looks like
  nothing at all, because an unset pass camera is the identity. That is the 2D default, so a
  caller mixing lines into a ui pass gets silence rather than an error.
- Two canvas types, and the api now has two answers to "how do I draw something".

### Risks
- The device half has been compiled and linked but never run — no app draws lines yet, so the
  pipelines have not been through the validation layer. The first consumer is where that gets
  found, which is the same position `vulkan::Mesh` was in before voxel's port.
- Segments are not indexed, so a grid of n lines uploads 2n vertices where a shared-vertex
  representation would upload roughly n. That is the right trade for a manipulator and the
  wrong one for a dense wireframe of a mesh whose edges all meet; if wireframe display turns
  out to dominate, an indexed path can be added without changing anything decided here.
