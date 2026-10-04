# ADR-0075: A 2D Game Space — A Canvas May Draw In A Space Of Its Own, And Its Projection Stays A Push Constant Per Submit

**Date**: 2026-10-04
**Status**: accepted
**Deciders**: Joshua Farr

## Context

A `Canvas` draws in pixels. pong's court is the window in pixels with 800 by 600 written into its
rules: the paddle travel, where the right paddle stands, and the ball's speeds. A FIXME asks for
variables. tetris fits its well to the window by hand, and odyssey picks through a hard-coded
tile width. [RenderingPipeline.md](../RenderingPipeline.md#what-is-not-built-yet) also records
that the 2D pass does not read set 0, and the roadmap took the two to be one change. They are
not. Set 0 is a camera per pass. The quad projection is a push constant per submit, and so per
canvas. pong draws its court and its menu in one pass, and the menu is in pixels.

## Decision

**A canvas may be given a space, a size in the game's own units with its origin at the top left,
and a fit: stretched to the canvas, or contained in it at its own aspect with bars either side.**
`projection()` then maps the space into its viewport, and `toSpace()` maps a pixel back. A clip
rectangle is mapped out to pixels, because a scissor is in pixels. A canvas with no space draws
in pixels, exactly as before. **The 2D pass keeps taking its projection from the canvas, and does
not read set 0.**

## Alternatives Considered

### Alternative 1: A space on the canvas, through the push constant it already has — **chosen**
- **Pros**: A projection per submit is a projection per canvas, which is what a game space is. A
  court and a menu share one pass as two canvases. No shader or pipeline changes, and the
  mapping is headless, so a test asserts it.
- **Cons**: A space is a scale and an offset, not a camera, so it does not pan, zoom or rotate.
  A 2D game that scrolls translates what it draws, as it does today.
- **Why not**: n/a — chosen.

### Alternative 2: The quad pipeline reads set 0's camera, as the other pipelines do
- **Pros**: One way for every pipeline to be told where it looks, and a 2D camera could pan and
  zoom like any other.
- **Cons**: A camera per pass puts the court and the menu in separate passes, or the menu in
  court units. It changes every quad shader and every app's submit, to buy a camera that no 2D
  app here moves.
- **Why not**: It costs a pass per space, and nothing in this tree needs what it buys.

### Alternative 3: An orthographic `type::camera::Camera` the app hands to the canvas
- **Pros**: The camera classes already build orthographic projections, and a profile could carry
  the space.
- **Cons**: `Camera::orthographic` is centred and symmetric, with y up, so it cannot say "800 by
  600 from the top left" without a new mode. It also brings a view matrix and a depth range that
  a canvas has no use for.
- **Why not**: It is a third way to say what a size and a fit say.

### Alternative 4: Each game maps its own coordinates
- **Pros**: Nothing to decide, and a game keeps whatever arithmetic it likes.
- **Cons**: That is the FIXME. Each game also maps the cursor back by hand, and a clip drawn in
  game units would be scissored in the wrong place.
- **Why not**: Every 2D game writes the same scale and offset, and a clip is easy to get wrong.

## Consequences

### Positive
- pong's rules are written against its court, and a window of any shape shows the court whole.
- A cursor reaches a game in its own units through `toSpace()`.
- The set 0 item in RenderingPipeline.md is closed as decided, rather than left open.

### Negative
- Something drawn outside the space lands in the bars, because nothing clips to the viewport.
  A game that draws past its court clips there itself.
- A contained space scales text with it, so text in a space grows and shrinks with the window,
  where the ui's stays at its size.

### Risks
- **A 2D game that wants a camera** — one that scrolls a world larger than the screen and zooms
  it — would want alternative 2. That is a camera on the quad pipeline. A space need not be
  undone for it, because a canvas with no space would read the camera instead.
