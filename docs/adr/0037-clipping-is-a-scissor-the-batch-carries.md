# ADR-0037: Clipping — A Clip Rectangle Is Batch State, And The Device Scissors The Draw

**Date**: 2026-09-06
**Status**: accepted
**Deciders**: Joshua Farr

## Context

[ADR-0034](0034-a-component-has-children-and-a-box.md) gave a component children and a box and
listed what it did not do: "nothing checks that a child's box stays inside its parent's, and
nothing clips one that does not". [ADR-0035](0035-an-immediate-mode-layer-over-the-same-canvas.md)
has the same gap from the other side — a panel longer than the window it is in runs off the
bottom of it.

Both want the same thing and neither can be finished without it. A scroll view is a box that
shows part of what it holds, which is a translation and a clip; a `Scrollbar` cannot be more
than a declaration until something can cut a child off at its parent's edge. A dropped menu
already overflows the strip it came from deliberately, so the answer cannot be to clip
everything.

`Canvas` accumulates quads with a modelview stack and cuts a batch where the texture or the
text flag changes ([ADR-0005](0005-one-batched-quad-primitive.md),
[ADR-0036](0036-text-is-a-distinct-kind-of-quad.md)). A batch becomes one `DrawItem`, and the
recorder sets viewport and scissor once per pass and never per item.

## Decision

**A clip rectangle is canvas state on a stack beside the transform, it is carried by the batch
the way a texture is, and the recorder turns it into a dynamic scissor on the draw.** Clipping
is per draw and not per vertex: a quad that straddles the edge is drawn whole and the device
keeps the part inside.

The rectangle is intersected with whatever is already clipped, so a clip inside a clip can only
take room away. A component clips its children when it asks to — `Component::clip(true)` — for
the same reason `pickable` is opt-in: most of a hud draws inside its box already, and a menu
that drops out of its strip must keep doing so.

## Alternatives Considered

### Alternative 1: A clip on the batch, scissored by the recorder — **chosen**
- **Pros**: The mechanism the canvas already has. A clip changes what the batch records, the
  stream cuts, and the extra cost of a clipped subtree is one draw call at each edge of it,
  which is what a texture change costs today. A scissor is free in the rasteriser — no fragment
  is shaded outside it — and it is pixel exact, so a glyph is cut mid-stem rather than dropped.
  Scissor is already dynamic state, so nothing recompiles a pipeline.
- **Cons**: A scissor is axis aligned in framebuffer pixels, so a clip cannot be rotated, and
  cannot be rounded to match a panel's corner radius. The canvas maps to the framebuffer one to
  one, which holds while a pass draws into a target the size of the canvas and would not
  survive a ui drawn into a smaller viewport.
- **Why not**: n/a — chosen.

### Alternative 2: Clip the geometry on the cpu as it is added
- **Pros**: No device state at all, no batch cut, and the clip could be any shape the clipper
  can handle.
- **Cons**: Every primitive needs its own clipping code — a rect is easy, an arc is polygon
  clipping, and a glyph needs its texture coordinates interpolated to the cut so that half a
  letter samples half the atlas. All of it is paid every frame on the cpu for geometry that is
  rebuilt every frame anyway.
- **Why not**: A large amount of arithmetic to avoid a state the device sets for nothing.

### Alternative 3: Discard in the fragment shader against a rectangle in the push block
- **Pros**: Costs no more draws than Alternative 1 does — a batch already carries push constants
  of its own, so the rectangle rides along in them. Unlike a scissor it could be rounded, which
  would let a clip follow a panel's corner radius.
- **Cons**: Pays per fragment for what the rasteriser can do for free, and the fragments outside
  the clip are still shaded far enough to be discarded. It also puts a ui concern in the one
  shader every 2D thing in the engine draws with.
- **Why not**: The same batching cost with a worse per pixel one. Worth revisiting only if a
  rounded clip is wanted.

### Alternative 4: A stencil buffer
- **Pros**: The general answer — any shape, nested to the depth of the stencil's bit count.
- **Cons**: The 2D pass has no stencil attachment and neither quad pipeline tests one, so this
  is a format change, two more pipelines, and a mask drawn before the content of every clipped
  box.
- **Why not**: Out of proportion to a rectangle inside a rectangle.

## Consequences

### Positive
- `Immediate` clips a window to its body and keeps a scroll offset per window, so a panel longer
  than the window it is in scrolls rather than running off the bottom of it.
- `Scrollbar` is a component rather than a declaration, and a `Panel` that asks to clip is a
  scroll view once something translates what it holds.
- The clip is on the canvas rather than on the ui, so anything drawing 2D can use it — a
  minimap cut to a circle's bounding box, a sprite cut to a frame — without going through
  `api/ui`.

### Negative
- A clipped subtree costs draws. Two clipped boxes side by side cannot merge into one batch
  however alike they are, because the scissor between them differs.
- The rectangle is resolved against the transform at the moment it is pushed, so translating
  after a clip moves what is drawn and not what it is cut to. That is what a scroll view wants
  and it is the opposite of what a naive reading expects.
- **A clip only reaches the quad primitive.** `LineCanvas` builds its own draw items
  ([ADR-0011](0011-lines-are-the-second-primitive.md)) and carries no clip, so a ui drawn in
  lines is not cut. Nothing in the tree draws one.

### Risks
- The canvas is in pixels and a scissor is in framebuffer pixels, and the two agree only because
  the ui is drawn into a pass covering the whole target. A ui drawn into a pass with a viewport
  offset would be clipped in the wrong place, and nothing detects it.
- A clip that closes to nothing is a scissor of zero extent, which draws nothing rather than
  everything. The intersection has to be clamped rather than left inverted, and an inverted
  rectangle reaching the device is a validation error rather than a wrong picture.
