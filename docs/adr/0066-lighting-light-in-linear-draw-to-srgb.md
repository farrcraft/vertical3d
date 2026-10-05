# ADR-0066: Lighting: light in linear, draw to sRGB

**Date**: 2026-10-03
**Status**: accepted
**Deciders**: Joshua Farr

## Context

[ADR-0009](0009-colour-display-space-unorm-swapchain.md) has every colour in the tree authored in
display space and written out unchanged through a `UNORM` chain, and it named lighting as the
thing that could not simply be added to that. Lighting is arithmetic on light, which is only
right in linear. [ADR-0049](0049-swapchain-caller-picks-the-format.md) lets a consumer ask
for an `_SRGB` chain, and retcon does. Its lit tier writes linear and lets the target encode it,
but it uploads its albedo as `UNORM`. So a texture authored in display space enters the lighting
undecoded, and retcon's reference capture has that baked in.
[LitScene](../plans/completed/LitScene.md) brings that tier into the api and has to say which of the two it
keeps.

## Decision

**The lit pipelines compute in linear and are drawn into an `_SRGB` target**, a swapchain named
under ADR-0049 or an offscreen target in an `_SRGB` format, which encodes on store. **A lit
model's albedo is uploaded as `_SRGB`**, so it is decoded to linear when it is sampled.
`TextureFactory` takes an encoding whose default stays `UNORM`, so ADR-0009 is unchanged for
everything that is not lit. A base colour factor is linear, as glTF defines it.

## Alternatives Considered

### Alternative 1: Keep retcon's `UNORM` albedo
- **Pros**: retcon's reference capture is unchanged when it adopts the tier.
- **Cons**: Every texture is lit as if it were already linear, so mid tones come out dark and
  saturated. The tier would be linear everywhere except the one input most of the picture comes
  from, and every later consumer would inherit that.
- **Why not**: It copies a defect so that one picture holds.

### Alternative 2: Light in display space through a `UNORM` target
- **Pros**: The same target and textures as everything else in the tree.
- **Cons**: Adding two lights in display space is not adding two lights, and banding thresholds
  tuned against one display curve are wrong against another.
- **Why not**: It is the thing ADR-0009 said lighting could not do.

### Alternative 3: Linear lighting, an sRGB target, decoded albedo — **chosen**
- **Pros**: Correct throughout, and it is what glTF's colour space says a base colour texture is.
- **Cons**: retcon's capture moves once on adoption, by exactly the albedo's decoding. A lit
  scene and a 2D ui drawn into one `_SRGB` target need the ui's colours encoded, which the quad
  pipeline does not do.
- **Why not**: n/a — chosen.

## Consequences

### Positive
- A light, a fill and a shadow combine as light does.
- The difference retcon's capture shows on adoption is named in advance rather than hunted for.

### Negative
- A consumer drawing a lit scene picks an `_SRGB` format for that target, and a 2D overlay over
  it draws into another target or accepts that its colours read lighter.
- Two encodings of texture in one tree. A texture's encoding has to be chosen at upload, by
  what samples it.

### Risks
- A lit model's albedo registered `UNORM` by mistake looks dark rather than failing. The
  `MeshRegistry` is the one place that uploads a lit albedo, so the choice is made there and
  nowhere else.
