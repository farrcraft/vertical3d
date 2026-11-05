# ADR-0146: Text: effects drawn from a distance field

**Status**: accepted
**Date**: 2026-11-01
**Tags**: text
**Amends**: [ADR-0145](0145-text-coverage-on-the-pixel-grid-curves-when-scaled.md)
**Documented in**: [api/Fonts.md](../../api/Fonts.md#effect-fields)

## Context

A style may give text an outline, a shadow and a glow. Games want them on text on the pixel
grid, such as a HUD, and on text that is scaled or stands in the world. Text on the grid is
drawn as a coverage mask and scaled text from its curves, and neither format says how far a
pixel is from the glyph's edge. A glow and a soft shadow need that distance, and an outline is
the set of pixels within a width of the edge.

## Decision

A glyph with an effect is also kept as a one channel signed distance field, made by the glyph
store when an effect first asks for it, with its spread wide enough for the effect. Each effect
is drawn from the field beneath the glyph, and the glyph's face is still drawn as coverage or
from its curves. A field serves every size it is drawn at.

## Alternatives

### Effects from the curves
- **For**: exact at any width and angle, and nothing more is stored.
- **Against**: the shader finds a distance to each curve near the pixel, which costs a cubic
  solve per curve over bands widened by the effect. Text on the pixel grid would have to be
  drawn from its curves, unhinted, to have an effect. No established renderer does this.
- **Rejected because**: effects would not reach hinted text on the grid, and the cost per pixel
  is the highest of the three.

### A stroked glyph
- **For**: FreeType's stroker gives an outline as a glyph, hinted on the grid or read as curves,
  with round, mitred or bevelled corners. Godot and Unreal outline bitmap text this way, and
  Slug keeps a widened outline for each font.
- **Against**: a shadow can only be the glyph again, offset. A glow or a soft shadow is a
  blurred mask, so it cannot reach scaled or world text. Each width is its own glyph at each
  size and quarter pixel step. The stroker's sharp joins are known to move the outline off the
  glyph.
- **Rejected because**: the same effects would not reach both kinds of text.

## Consequences

- **Gains**:
  - Outline, shadow, soft shadow and glow are drawn the same way on the pixel grid and in the
    world, each from one sample of the field.
  - The width of an effect is limited only by the spread its field is made with.
  - Text with no effect makes no field.
- **Costs**:
  - The store makes, keeps and draws a third format of glyph.
  - An outline's corners are always round.
  - A field is made on the CPU for each variable font instance and each spread.
  - A field drawn much larger than it was made blurs the effect's edge, though not the face.
- **Revisit when**: effects need sharp corners, or fields for many instances crowd the store.
