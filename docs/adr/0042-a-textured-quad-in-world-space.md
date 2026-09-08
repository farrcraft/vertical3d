# ADR-0042: 2D Drawing — A Textured Quad In World Space Is A Third Primitive, Ordered By Its Caller

**Date**: 2026-09-07
**Status**: accepted
**Deciders**: Joshua Farr

## Context

The realtime renderer has two primitives and they divide the space between them the wrong way
for anything drawn in a projection. `realtime::Canvas` is textured and is in canvas pixels,
through an orthographic projection it pushes itself and that ignores the pass camera.
`realtime::LineCanvas` is in world space through the pass camera at set 0, and is lines. So a
rectangle can be textured or it can be in the world, and not both.

Two consumers want the missing one. `grid::Overlay` can outline a tile and cannot fill one,
which `TODO.md` has carried since the tile grid landed. And a game built against this api has
settled on a fixed 3/4 isometric orthographic camera and a stylized 2D look, which makes every
tree, cabin and character a textured quad at a world position: today it would project world to
screen itself, per sprite per frame, and hand the result to a canvas with its own idea of what
the projection is.

[ADR-0005](0005-one-batched-quad-primitive.md) is why this is a decision rather than an
addition. It made the batched quad the one primitive for 2D and has held well —
[ADR-0036](0036-text-is-a-distinct-kind-of-quad.md) amended it for distance field glyphs
without splitting it — but its vertex format carries a `vec2` position, and a world position is
not one.

## Decision

**A third primitive.** `realtime::WorldCanvas` accumulates textured quads whose vertices carry
a `vec3` world position, and `vulkan::WorldRenderer` draws them through the camera the pass
carries at set 0 — the arrangement [ADR-0011](0011-lines-are-the-second-primitive.md) settled
for lines, with the quad pipeline's fragment stage rather than the line pipeline's.

**The caller supplies the order.** A world canvas draws its quads in the order they were added,
and what a quad's depth means is the caller's: in an isometric projection it is a function of
the world position and the sprite's footprint, which is the game's knowledge and not the
renderer's. The depth-tested pipeline **tests and does not write**, so solid scene geometry in
front of a quad hides it while two blended quads do not cut holes in each other.

**ADR-0005 is narrowed again rather than amended.** As with 0011, the primitive it describes is
now the primitive for content in canvas pixels.

## Alternatives Considered

### Alternative 1: A third canvas and renderer, world space, camera from set 0 — **chosen**
- **Pros**: A `vec3` position stays out of the ui vertex, which is every glyph of every label.
  The pass camera is already bound at set 0 for lines, so multiple viewports cost four passes
  rather than four copies of the geometry, and `grid::Overlay` hands quads out through a sink
  the way it hands out segments — nothing in `api/grid` names a renderer. The batching rule,
  the material machinery and the fragment shader are the quad pipeline's already.
- **Cons**: A third canvas type, and the api now has three answers to "how do I draw
  something". Two vertex formats that differ only in the width of one field.
- **Why not**: n/a — chosen.

### Alternative 2: `Canvas` learns that a batch is world space
- **Pros**: One canvas type and one place an app fills. Everything already built — the
  transform stack, the clip, the text path, `rect`, `circle`, `arc`, `ring` — works in world
  space for free.
- **Cons**: The position has to widen to `vec3` for every vertex including every glyph, or the
  class grows a second vertex stream and stops being one thing. `Canvas` currently *is* the
  definition of screen space in this tree — `clip()` resolves a rectangle through the modelview
  because a canvas draws in the pixels a scissor is measured in, and that stops being true for
  half of it. Its `translate` and `scale` take a `vec2`.
- **Why not**: It is a larger change to a class four apps depend on, in exchange for avoiding a
  type. The claim that it is smaller does not survive the vertex format.

### Alternative 3: Sort inside the renderer, by distance from the camera
- **Pros**: A caller adds quads in any order and gets a correct picture, which is what somebody
  expects the first time.
- **Cons**: The renderer would have to decide what a quad's distance is — its centre, its
  nearest corner, its base — and every answer is wrong for some projection. In a 3/4 isometric
  view a tree behind a cabin is not the one further from the camera; it is the one whose feet
  are further up the ground plane. Sorting also has to happen after the transform stack has
  been applied, so it cannot be done as quads arrive.
- **Why not**: It would be quietly wrong for the second app, which is worse than being
  obviously the caller's.

### Alternative 4: Depth buffer, tested and written
- **Pros**: No ordering question at all, and correct against the scene in both directions.
- **Cons**: A blended quad that writes depth cuts a hole the shape of its whole rectangle,
  including the transparent parts, so the sprite behind it disappears where the two overlap.
  Alpha to coverage or an alpha cutout avoids that and gives hard edges, which is exactly what
  a stylized 2D look does not want.
- **Why not**: It trades the ordering problem for a worse one. Testing without writing keeps
  what the depth buffer is good for and drops what it is not.

### Alternative 5: A sort key on the batch, ordered by `Pass::sort()`
- **Pros**: The machinery exists — a `DrawItem` already carries a key with a layer, and a pass
  can be told to record in key order.
- **Cons**: The key sorts by pipeline and material within a layer, which is what makes it useful
  for a scene pass and what makes it wrong here: two quads at the same depth with different
  textures would be reordered against each other. A batch is also cut by texture change, so the
  unit being sorted is not the unit the caller reasons about.
- **Why not**: It is the same answer as alternative 3 wearing a different hat, with the sort in
  a place that cannot see what is being sorted.

## Consequences

### Positive
- `grid::Overlay` can fill a tile, through a quad sink beside the line sink it already has, and
  still names no renderer.
- A sprite at a world position is drawn by handing four world corners to a canvas. No per frame
  projection on the cpu, no camera reimplemented in an app, and a second viewport of the same
  world costs a second pass.
- The fragment stage is the quad pipeline's, so a world quad samples an atlas exactly as a ui
  sprite does and `image::TextureAtlas` serves both.
- Batching is ADR-0005's rule unchanged: a new draw where the bound texture changes, an
  untextured quad against the renderer's white texture, painter order preserved with no sort.

### Negative
- Three canvas types, and which one to use is a question an app now has to answer. The rule is
  the space: pixels on the screen is `Canvas`, a position in the world is `WorldCanvas` or
  `LineCanvas`.
- The caller sorts. An app that adds quads in entity order gets a picture with the trees in
  front of the cabins, and nothing warns it.
- No clip. `Canvas` and `LineCanvas` both carry ADR-0037's scissor and this does not, because
  neither consumer asks for one and a world space stream has no transform that would carry a
  rectangle to the screen. Adding it is additive.
- A world quad is a rectangle in the world, so a quad standing upright is only square-on from
  one direction. Nothing here billboards.

### Risks
- **A pass whose camera was never set draws a world canvas in clip space**, which is the same
  trap ADR-0011 recorded for lines: the identity is the 2D default, so a caller mixing world
  quads into a ui pass gets silence rather than an error.
- Painter ordering across two canvases in one pass is submission order, so an app drawing a
  world canvas and a line canvas that should interleave by depth cannot express it. The escape
  hatch is the layer argument both submissions take, which is coarse.
- Nothing culls back faces, so a quad wound either way is visible from both sides. That is what
  a tile highlight and a sprite both want and it means a caller cannot use winding to hide one.
