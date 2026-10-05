# ADR-0075: 2D: a canvas may have its own coordinate space

**Status**: accepted
**Date**: 2026-10-04
**Documented in**: [api/rendering/Canvas.md](../api/rendering/Canvas.md)

## Context

A `Canvas` draws in window pixels. A 2D game wants its own units: a court of fixed size, shown
whole in a window of any shape, with the cursor mapped back into the same units. Without help,
each game writes the same scale and offset and the reverse mapping, and a clip drawn in game
units is scissored in the wrong place. The 2D quad pipeline takes its projection from a push
constant per submit, which is per canvas, while set 0 holds one camera per pass. A game often
draws its play area and its menu in one pass, and the menu is in pixels.

## Decision

A canvas may be given a space: a size in the game's units with its origin at the top left, and
a fit, either stretched to the canvas or contained at its own aspect ratio with bars either
side. `projection()` maps the space into the viewport, `toSpace()` maps a pixel back, and a clip
rectangle is mapped to pixels for the scissor. A canvas with no space draws in pixels, and the
2D pass keeps taking its projection from the canvas rather than reading set 0.

## Alternatives

### The quad pipeline reads set 0's camera, as the other pipelines do
- **For**: One way for every pipeline to be told where it looks, and a 2D camera could pan and
  zoom.
- **Against**: One camera per pass puts a play area and its menu in separate passes, or the menu
  in game units. It changes every quad shader and every app's submit.
- **Rejected because**: It costs a pass per space, for a camera no 2D app here moves.

### An orthographic `type::camera::Camera` handed to the canvas
- **For**: The camera classes already build orthographic projections.
- **Against**: `Camera`'s orthographic projection is centred and symmetric with y up, so it
  cannot express "800 by 600 from the top left" without a new mode. It also brings a view matrix
  and depth range a canvas does not use.
- **Rejected because**: It is a third way to say what a size and a fit say.

### Each game maps its own coordinates
- **For**: Nothing in the api changes, and each game keeps its own arithmetic.
- **Against**: Every 2D game writes the same scale and offset and the same cursor mapping, and
  each gets the clip wrong in its own way.
- **Rejected because**: The mapping is the same in every game and belongs in one place.

## Consequences

- **Gains**:
  - A game's rules are written against its own court, and a window of any shape shows it
    whole.
  - The cursor reaches a game in its own units through `toSpace()`.
  - No shader or pipeline changes, and the mapping is tested without a GPU.
- **Costs**:
  - A space is a scale and an offset, not a camera, so it cannot pan, zoom or rotate. A
    scrolling game translates what it draws.
  - Anything drawn outside the space lands in the bars, because nothing clips to the viewport.
  - Text in a contained space scales with the window, while the UI's text stays its own size.
- **Revisit when**: a 2D game scrolls and zooms a world larger than the screen. That needs a
  camera on the quad pipeline, and a canvas with no space could read the camera without undoing
  this.
