# ADR-0149: Text: colour glyphs from every OpenType colour table

**Status**: accepted
**Date**: 2026-11-02
**Tags**: text
**Amends**: [ADR-0145](0145-text-coverage-on-the-pixel-grid-curves-when-scaled.md)
**Documented in**: [api/Fonts.md](../../api/Fonts.md#colour-glyphs)

## Context

Emoji and other colour glyphs reach text through the fallback chain, and an emoji font stores
them in one of five OpenType tables. COLR v0 and v1 hold layers of outlines filled with palette
colours and gradients, SVG holds an SVG document for each glyph, and CBDT and sbix hold PNG
images at fixed sizes. Browsers and systems split over which they read: Safari has no COLR v1,
Apple's emoji font is sbix, Android's is CBDT, and some fonts ship only SVG. Text on the pixel
grid has to be sharp at its exact size, and text that is scaled or in the world at any size and
angle. A glyph in a table the api does not read is drawn as a missing glyph, or not in colour.

## Decision

Every colour table is read: COLR v0, COLR v1, SVG, CBDT and sbix. A COLR or SVG glyph is drawn
as text is. On the pixel grid it is painted at its device size into an RGBA page, and when scaled
or in the world it is drawn from layers of filled curves. A CBDT or sbix glyph is the image from its
nearest strike, scaled, in both modes.

## Alternatives

### COLR v0 and v1 only
- **For**: every colour glyph is drawn sharp both ways, through HarfBuzz's paint interface,
  with no further library.
- **Against**: Apple's and Android's emoji fonts, and SVG-only fonts, draw as missing glyphs.
- **Rejected because**: a game or the editor loads fonts the api does not choose, and a glyph
  drawn soft is better than none.

### COLR, CBDT and sbix, with no SVG
- **For**: covers the system emoji fonts with no SVG renderer, since FreeType decodes PNG.
- **Against**: an SVG-only font draws no colour.
- **Rejected because**: SVG glyphs are in use in design tools and Apple's ecosystem, and SDL_ttf
  and Godot read them for that reason.

### Bitmap tables only
- **For**: one glyph kind, an image, and the simplest store.
- **Against**: soft at any size the font did not bake, no palettes, and no COLR font draws in
  colour.
- **Rejected because**: it gives up the sharpness ADR-0145 promises for all other text.

## Consequences

- **Gains**:
  - Any emoji font a game ships draws in colour.
  - COLR and SVG glyphs are sharp on the grid and in the world, as other text is.
  - A palette, and the text's colour as a glyph's foreground, apply to COLR glyphs.
- **Costs**:
  - Three glyph kinds beside coverage: an RGBA image at a size, layered colour curves, and a
    scaled bitmap.
  - CBDT and sbix glyphs are soft when scaled past their largest strike.
  - The curve shader grows layers, clips and gradients, and costs more per pixel for them.
  - An SVG renderer joins the dependencies.
- **Revisit when**: one format displaces the others in the fonts games ship, or COLR v1 painting
  on the device proves too slow for a screen of emoji.
