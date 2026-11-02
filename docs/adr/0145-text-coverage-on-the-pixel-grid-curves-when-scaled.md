# ADR-0145: Text: coverage on the pixel grid, curves when scaled

**Status**: amended
**Date**: 2026-11-01
**Amends**: [ADR-0036](0036-text-sdf-glyphs-through-the-quad-shader.md)
**Amended by**: [ADR-0146](0146-text-effects-drawn-from-a-distance-field.md), [ADR-0149](0149-text-colour-glyphs-from-every-opentype-colour-table.md)
**Documented in**: [api/Fonts.md](../api/Fonts.md#coverage-glyphs), [api/ui/README.md](../api/ui/README.md#text-is-supplied-by-the-app)

## Context

Most text is drawn on the screen's pixel grid, in the editor's panels and a game's menus, where
it has to be sharp at small sizes. A distance field built at one size and scaled down cannot be
hinted to the grid it lands on, so its stems and x-height fall between pixels. Glyphs are now
made when they are drawn, at each size, so one atlas serving every size no longer needs a
field. Text that is scaled, turned or placed in the world still has to stay sharp at any size
and angle, with sharp corners.

## Decision

Text on the pixel grid is rasterized at its exact device size, hinted vertically, at a quarter
pixel's horizontal step, and drawn as a coverage mask through the quad shader. Text that is
scaled, turned or in the world is drawn from the glyphs' quadratic curves in a fragment shader,
as the Slug algorithm does, at any size and angle.

## Alternatives

### Coverage beside a multi-channel distance field
- **For**: a field is cheap to sample, outlines, glows and shadows are offsets in it, and Unreal
  and Godot scale text this way.
- **Against**: it is exact only within its distance range, its corners can break, thin strokes
  are weak, and each glyph's field is built on the CPU for every variable font instance.
- **Rejected because**: curves are exact at every size and angle, and read the instanced outline
  directly.

### Coverage, curves and a distance field
- **For**: each mode does what it is best at, effects through the field.
- **Against**: three glyph formats to make, store and draw.
- **Rejected because**: the field is kept only if effects need it, and that is a separate
  decision.

### Distance fields for all text
- **For**: one format, as ADR-0036 chose, and one atlas for every size.
- **Against**: small text cannot be hinted, so it is soft on the pixel grid.
- **Rejected because**: the editor's 12 pixel text is the most common text the api draws.

## Consequences

- **Gains**:
  - Text on the pixel grid is sharp: its baseline is on a whole pixel and its stems are hinted.
  - Scaled and world text is exact at any size, with no range to outgrow.
  - A change of ui scale rasterizes glyphs at the new size rather than scaling old ones.
- **Costs**:
  - Every size and quarter pixel step drawn is its own glyph in the store.
  - A curve glyph costs more per pixel than a field sample.
  - Outlines, glows and shadows are not free with curves.
- **Revisit when**: effects need a distance field, or text on the grid is drawn at so many sizes
  that the store thrashes.
