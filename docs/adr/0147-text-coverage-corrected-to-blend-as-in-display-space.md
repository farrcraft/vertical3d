# ADR-0147: Text: coverage corrected to blend as in display space

**Status**: accepted
**Date**: 2026-11-01
**Documented in**: [api/rendering/Canvas.md](../api/rendering/Canvas.md#text-weight)

## Context

The ui draws into a UNORM swapchain, where coverage blends in display space, and a lit scene
draws into an _SRGB target, where the hardware blends in linear light. The same coverage then
looks a different weight in each: dark text on a light ground turns thin and light text on a
dark ground turns bold. Text on the pixel grid is hinted to whole pixels, so anything that
thickens its stems blurs them. Light text on a dark ground also needs a way to look heavier.

## Decision

Text's coverage is corrected in the shader so that, in an _SRGB target, it blends to the colour
it would have in display space over a ground of the opposite luminance. A style's contrast
strengthens the coverage before the correction, in every target. The correction applies to
coverage, curve and distance field glyphs alike, and to nothing that is not text.

## Alternatives

### Linear blending with stem darkening
- **For**: blending in linear light is exact over any ground, and FreeType's guidance, Adobe
  and macOS pair it with darkening small text.
- **Against**: a UNORM target cannot blend linearly, so every ui quad would move to an _SRGB
  target and its gradients and shadows would blend differently too. Darkening moves hinted stems
  off whole pixels, and FreeType darkens only through drivers that do not read TrueType faces,
  so each glyph format would need its own thickening.
- **Rejected because**: it blurs the hinted text that is most of what the tree draws, and
  changes how every ui quad blends.

## Consequences

- **Gains**:
  - Text looks one weight in a UNORM and an _SRGB target, and as the editor's text looks now.
  - One term in the shaders serves all three glyph formats.
  - Contrast is one number on a style.
- **Costs**:
  - The correction assumes a ground of the opposite luminance, so over a mid-tone or busy ground
    it is approximate.
  - Text carries a value in each vertex that marks it as text.
- **Revisit when**: text is drawn mostly over mid-tone grounds, or the ui moves to an _SRGB
  target for another reason.
