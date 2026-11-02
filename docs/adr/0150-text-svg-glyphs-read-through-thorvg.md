# ADR-0150: Text: SVG glyphs read through ThorVG

**Status**: accepted
**Date**: 2026-11-02
**Documented in**: [api/Fonts.md](../api/Fonts.md#colour-glyphs)

## Context

A colour glyph from an OpenType SVG table is an SVG document, which FreeType parses and hands
to an outside renderer. Colour glyphs are painted at their device size on the pixel grid, and
drawn from layers of filled curves when scaled or in the world. An SVG glyph is therefore wanted
both as pixels and as paths with fills. Gradients, transforms and shared definitions through
`use` appear in real emoji fonts. The tree's nanosvg cannot resolve `use`.

## Decision

SVG glyphs are read through ThorVG. Its scene of shapes, each a path with a solid or gradient
fill, is rasterized for the pixel grid and walked into layers of curves for scaled and world
text, as a COLR glyph is.

## Alternatives

### plutosvg
- **For**: a small C library with FreeType's SVG hooks built in. SDL_ttf and Dear ImGui use it.
- **Against**: it renders only to pixels, so an SVG glyph cannot be drawn from its curves.
- **Rejected because**: SVG glyphs would be the one vector colour format soft when scaled.

### lunasvg
- **For**: fuller SVG and CSS support than plutosvg, from the same author.
- **Against**: it also renders only to pixels.
- **Rejected because**: the same as plutosvg.

### nanosvg, already in the tree
- **For**: no new dependency, and it gives paths with fills.
- **Against**: no `use`, partial gradients and no CSS, so many emoji draw wrongly or not at all.
- **Rejected because**: emoji fonts depend on what it lacks.

## Consequences

- **Gains**:
  - SVG glyphs are drawn sharp in both modes, as COLR glyphs are.
  - One library reads SVG for text, and its Lottie reader is available to the api.
- **Costs**:
  - ThorVG is a larger dependency than plutosvg.
  - It reads the SVG Tiny profile, so an SVG feature outside it is not drawn.
  - FreeType's SVG hooks and ThorVG's scene are joined by the api's own code.
- **Revisit when**: an emoji font draws wrongly for a feature ThorVG lacks, or FreeType gains an
  SVG renderer of its own.
