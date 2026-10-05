# ADR-0066: Lighting: light in linear, draw to sRGB

**Status**: accepted
**Date**: 2026-10-03
**Amends**: [ADR-0009](0009-colour-display-space-unorm-swapchain.md)
**Documented in**: [api/rendering/ColourAndPost.md](../api/rendering/ColourAndPost.md)

## Context

[ADR-0009](0009-colour-display-space-unorm-swapchain.md) has every colour authored in display
space and written unchanged through a `UNORM` target, and names lighting as what that cannot
accommodate. Lighting adds and multiplies light, which is only correct in linear values.
[ADR-0049](0049-swapchain-caller-picks-the-format.md) lets an app present through an `_SRGB`
swapchain, which encodes linear output on store. Textures are authored in display space, so a
lit renderer has to decode them or they enter the lighting maths undecoded. glTF defines a base
colour texture as sRGB and a base colour factor as linear.

## Decision

The lit pipelines compute in linear and draw into an `_SRGB` target, either a swapchain chosen
under ADR-0049 or an offscreen target, which encodes on store. A lit model's albedo is uploaded
in an sRGB format so that sampling decodes it to linear, and the texture factory's default
encoding stays display space, so ADR-0009 holds for everything that is not lit. A base colour
factor is linear, as glTF defines it.

## Alternatives

### Upload the albedo as `UNORM`, like every other texture
- **For**: One way to upload a texture, lit or not.
- **Against**: Every texture is lit as if it were already linear, so mid tones come out dark and
  oversaturated. The renderer would be linear everywhere except in the input most of the
  picture comes from.
- **Rejected because**: It is wrong for every lit model, and every later app would inherit it.

### Light in display space through a `UNORM` target
- **For**: The same targets and textures as the rest of the tree.
- **Against**: Adding two lights in display space does not give the sum of the two lights, and
  cel band thresholds tuned against one display curve are wrong against another.
- **Rejected because**: It is what ADR-0009 says lighting cannot do.

## Consequences

- **Gains**:
  - Lights, fills and shadows combine correctly, and textures match what glTF says they are.
- **Costs**:
  - A lit scene needs an `_SRGB` target. A 2D overlay drawn into the same target reads
    lighter, because the quad pipeline does not encode its colours; it has to draw to another
    target or accept that.
  - Two texture encodings in one tree, chosen at upload by what will sample the texture.
  - An albedo uploaded as `UNORM` by mistake looks dark rather than failing. `MeshRegistry` is
    the one place that uploads a lit albedo, so the choice is made there only.
- **Revisit when**: the 2D pipelines move to linear colour, so that one encoding serves every
  texture and target.
