# ADR-0144: Text: glyph pages of plots, reused least recently used first

**Status**: accepted
**Date**: 2026-11-01
**Tags**: text
**Documented in**: [api/Fonts.md](../../api/Fonts.md#the-glyph-store)

## Context

Glyphs are made the first time they are drawn, at each size, step of a pixel and mode a text
needs, so the set an app draws is not known up front and keeps growing. An editor or a game
shows many sizes and scripts over a session, and device memory for glyphs has to stay bounded.
A frame that is being built names glyphs it will sample, so those cannot be overwritten before
it is submitted. A region of a texture can be replaced while earlier frames still read it.

## Decision

Glyphs are packed into pages of square plots, and each plot is emptied and reused as one. A full
store adds a page up to a limit, then empties the plot used longest ago that the frame being
built does not use. When the frame being built uses every plot, a page is added past the limit
and let go once a frame uses nothing in it.

## Alternatives

### One atlas, cleared when full
- **For**: the simplest store, bounded, with no record of use, as Unreal's Slate flushes its font
  atlas.
- **Against**: every glyph is made again after a clear, in one burst, and a clear while the frame
  being built uses the atlas loses glyphs it needs.
- **Rejected because**: it stalls and can drop text at the moment the atlas fills.

### Pages that only grow
- **For**: nothing is ever made twice.
- **Against**: memory grows with every size, step and script shown, without bound.
- **Rejected because**: a long session in the editor would hold every glyph it ever drew.

### Evicting single glyphs
- **For**: the finest reuse, taking back exactly what is no longer drawn.
- **Against**: glyphs of mixed sizes leave holes that a new glyph rarely fits, and finding a hole
  needs a full packer.
- **Rejected because**: Skia empties whole plots of its atlas for the same reason.

## Consequences

- **Gains**:
  - Glyph memory is bounded by the page limit, unless one frame needs more.
  - No frame draws without a glyph it asked for, and filling the store never makes everything
    again at once.
  - A plot is the unit both of reuse and of upload, so a frame copies only the plots it changed.
- **Costs**:
  - Emptying a plot forgets every glyph in it, used recently or not, so some are made again.
  - A glyph larger than a plot is not stored, so large text has to be drawn another way.
  - The store records which frame used each plot, and a renderer has to begin each frame on it.
- **Revisit when**: glyphs larger than a plot are drawn from the store, or pages of several
  formats are packed together.
