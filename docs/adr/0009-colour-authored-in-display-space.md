# ADR-0009: Colour — Authored In Display Space, Presented Through A UNORM Swapchain

**Date**: 2026-08-31
**Status**: accepted
**Deciders**: Joshua Farr

## Context

Phase 2 picked `VK_FORMAT_B8G8R8A8_SRGB` for the swapchain, on the usual advice that an sRGB
target is what you present through. Nothing drew anything but a clear, so nothing noticed
what that means: an `_SRGB` attachment encodes on write, which is to say it treats every
value a shader outputs as *linear* light and converts it to display space. Phase 3 put quads
on the screen and the whole frame came out washed: the `(0.06, 0.07, 0.10)` clear colour
appeared as mid slate grey, the `0.35` board grey as near white.

Nothing in this repo authors colour in linear light. Every literal in every app — pong's
paddles, tetris's pieces, the ui panel colours — is a value someone picked by looking at it.
Every texture is a PNG authored the same way. So the question is which end to move: encode
the authored values into linear on the way in, or stop the target encoding them on the way
out.

## Decision

Colour is authored in display space and written out unchanged. `Swapchain::chooseFormat`
prefers a `UNORM` 32-bit format over an `_SRGB` one, so a shader that writes `0.35` puts
`0.35` on the screen. Textures are uploaded as `UNORM` to match. Blending therefore happens
in display space, which is what every 2D UI toolkit does and what the art was authored
against.

## Alternatives Considered

### Alternative 1: UNORM swapchain, colour authored in display space — **chosen**
- **Pros**: What you write is what you see, which is the only property that matters while
  every colour in the engine is a literal someone eyeballed. One line of change. Costs
  nothing at runtime. Trivially reversible.
- **Cons**: Blending is not physically correct. Nothing is gamma correct, so when lighting
  arrives it cannot simply be added to this pipeline.
- **Why not**: n/a — chosen.

### Alternative 2: sRGB swapchain, convert content to linear
- **Pros**: Physically correct blending. The right answer for a renderer that lights
  anything. Textures get it for free by being created `_SRGB`, and vertex colours need one
  `pow` in the vertex shader.
- **Cons**: Every colour in every app becomes a value that does not mean what it looks like.
  Text antialiasing composited in linear space reads thin, which is a known and much
  complained about regression when 2D UIs move to linear blending. And it is a property that
  has to be maintained: a colour added later without the conversion is subtly wrong rather
  than obviously wrong.
- **Why not**: The engine draws no lit geometry yet. Paying the authoring cost now, for
  correctness nothing can currently exercise, gets the trade backwards — and text quality is
  the thing pong and tetris are actually judged on.

### Alternative 3: sRGB swapchain, leave the content alone
- **Pros**: None. This is what phase 2 left behind.
- **Cons**: Every colour in the engine is displayed brighter than it was written.
- **Why not**: It is the bug this record exists because of.

## Consequences

### Positive
- A colour literal, a PNG and a screenshot all agree.
- Text blending is done in the space the glyph coverage was rasterized for.
- The clear colour phase 2 chose now appears as the colour it names.

### Negative
- The renderer is not gamma correct, and says so nowhere except here.
- A device offering no `UNORM` format falls through to `formats.front()`, which may be
  `_SRGB` — and the frame would then look exactly as it did before this decision. No such
  device is known; the fallback is not silently correct, only silently unchanged.

### Risks
- **This is the decision voxel will have to revisit.** A 3D scene with lights has to blend
  and accumulate in linear space to look right, and the moment that lands, alternative 2
  stops being a cost and starts being a requirement. When that happens the conversion has to
  go in at the same time as the lighting, not after, and the 2D content needs converting with
  it — which is a mechanical change, but one that touches every colour in the tree. Expect to
  supersede this record in phase 5 rather than to keep it.
