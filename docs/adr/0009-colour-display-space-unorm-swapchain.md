# ADR-0009: Colour: display space, UNORM swapchain

**Status**: amended
**Date**: 2026-08-31
**Amended by**: [ADR-0049](0049-swapchain-caller-picks-the-format.md), [ADR-0066](0066-lighting-light-in-linear-draw-to-srgb.md)
**Documented in**: [api/rendering/ColourAndPost.md](../api/rendering/ColourAndPost.md)

## Context

An `_SRGB` attachment treats every value a shader writes as linear light and encodes it to
display space on write. Every colour in the apps, from paddle literals to UI panels, was picked
by eye in display space, and every texture is a PNG authored the same way. Drawn into an
`_SRGB` target, all of them appear brighter than written. Either the authored values are
converted to linear on the way in, or the target stops encoding them on the way out.

## Decision

Colour is authored in display space and written out unchanged. The swapchain prefers a 32-bit
`UNORM` format over an `_SRGB` one, and textures are uploaded as `UNORM` to match, so a shader
that writes 0.35 puts 0.35 on the screen. Blending therefore happens in display space, as it
does in most 2D UI toolkits.

## Alternatives

### sRGB swapchain, with content converted to linear
- **For**: physically correct blending, which is the right answer for a renderer that lights
  anything. Textures convert for free by being created `_SRGB`, and vertex colours need one
  `pow` in the vertex shader.
- **Against**: every colour literal stops meaning what it looks like. Text antialiasing blended
  in linear space looks thin, a known regression when 2D UIs move to linear blending. A colour
  added later without the conversion is subtly wrong rather than obviously wrong.
- **Rejected because**: nothing draws lit geometry yet. Paying the authoring cost for
  correctness nothing exercises gets the trade backwards, and text quality is what the 2D apps
  are judged on.

## Consequences

- **Gains**:
  - A colour literal, a PNG and a screenshot all agree.
  - Text is blended in the space its coverage was rasterized for.
  - No runtime cost, and the choice is easy to reverse.
- **Costs**:
  - Blending is not gamma correct, so lighting cannot simply be added to this pipeline.
  - A surface that offers no `UNORM` format falls back to the first format it offers, which may
    be `_SRGB`. Colours then come out too bright, with no error.
- **Revisit when**: a scene is lit. Lighting has to accumulate in linear space, and the
  conversion has to arrive with the lighting rather than after it.
  [ADR-0066](0066-lighting-light-in-linear-draw-to-srgb.md) does this for lit scenes.
