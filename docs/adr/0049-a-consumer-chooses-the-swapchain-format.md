# ADR-0049: Presentation — A Consumer Names The Swapchain Format, And UNORM Stays The Default

**Date**: 2026-09-08
**Status**: accepted
**Deciders**: Joshua Farr

## Context

[ADR-0009](0009-colour-authored-in-display-space.md) settled that colour is authored in
display space and presented through a `UNORM` chain, and `Swapchain::chooseFormat` has
preferred one ever since. That record named the condition it rests on: *"the engine draws no
lit geometry yet"*, and paying the authoring cost for correctness nothing can exercise gets
the trade backwards.

An external consumer now draws lit geometry. retcon — the first app on this api that is not
in this tree ([ADR-0027](0027-the-api-is-consumed-as-source.md)) — cel-shades a scene from a
directional light, renders a shadow map, and composites through a colour-grading LUT. It
presents through `B8G8R8A8_SRGB` and writes linear light into it, so the target's encode is
part of its pipeline rather than an accident of it. Its ui layer converts to linear on write
for the same reason, and its translucent panels blend in linear space because the hardware
blends before it encodes.

A `UNORM` chain is not a re-tuning for such an app. Both its composite and its ui would have
to apply the transfer function themselves, which moves every alpha blend into encoded space —
the outcome ADR-0009's own alternative 2 weighs and declines, for an engine that draws what
this one now does not.

## Decision

`Swapchain` takes a preferred colour format and honours it where the surface offers one in a
non-linear sRGB colour space; `Context3D` passes one through. The parameter defaults to
`VK_FORMAT_UNDEFINED`, which is ADR-0009's rule unchanged, and a preference the surface does
not offer falls back to it rather than failing.

## Alternatives Considered

### Alternative 1: A preferred format the caller may name — **chosen**
- **Pros**: ADR-0009 is unamended for every app in this tree, which passes nothing and gets
  what it got before. The decision moves to the layer that knows whether its colours are
  linear, which is the app. `chooseFormat` stays a pure function and is now testable without
  a device.
- **Cons**: Two presentation conventions exist in one api, so a pipeline cannot assume the
  chain's format. It never could — dynamic rendering already builds against
  `Swapchain::format()`.
- **Why not**: n/a — chosen.

### Alternative 2: Reverse ADR-0009 and present through sRGB again
- **Pros**: One convention. Physically correct blending everywhere.
- **Cons**: Every colour literal in pong, tetris, voxel and the editor becomes a value that
  does not mean what it looks like, and text antialiasing composited in linear space reads
  thin. ADR-0009 measured all of that and it has not changed.
- **Why not**: The apps in this tree still author in display space. Nothing about a second
  consumer's needs makes their colours wrong.

### Alternative 3: The consumer keeps its own swapchain and presenter
- **Pros**: No change here at all.
- **Cons**: A swapchain, an acquire/present loop and its synchronization are duplicated in
  the consumer forever — the part of a device tier that is least app-specific and most worth
  sharing. It also leaves `Presenter` unreachable, since it takes a `Swapchain`.
- **Why not**: The api being unusable by its second consumer is a fault in the api, not in
  the consumer.

## Consequences

### Positive
- An app that lights and grades its scene can present through the format its shaders were
  written against, and its blending stays in linear space.
- `chooseFormat` is public and pure, so the rule is covered by a test on a machine with no
  gpu.

### Negative
- `Swapchain::format()` is now the only answer to what the chain is, rather than something a
  reader can infer from ADR-0009. Anything building a pipeline against a guess is wrong, in a
  way it would not have been before.

### Risks
- A caller naming a format the surface offers only in a different colour space silently gets
  the default instead, and an app whose shaders depend on the encode would then be wrong
  everywhere rather than obviously broken in one place. Mitigated by matching on the colour
  space as well as the format, by `format()` reporting what was settled on rather than what
  was asked for, and by a warning when a preference was asked for and not met — silence means
  it was.
