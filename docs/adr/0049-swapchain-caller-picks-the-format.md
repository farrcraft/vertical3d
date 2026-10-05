# ADR-0049: Swapchain: caller picks the format

**Status**: accepted
**Date**: 2026-09-08
**Amends**: [ADR-0009](0009-colour-display-space-unorm-swapchain.md)
**Documented in**: [internals/RealtimeRenderer.md](../internals/RealtimeRenderer.md)

## Context

[ADR-0009](0009-colour-display-space-unorm-swapchain.md) has colour authored in display space
and presented through a `UNORM` swapchain, on the condition that nothing draws lit geometry. An
app that lights its scene writes linear light and needs the target to encode it to sRGB, so
that blending happens in linear space before the encode. Through a `UNORM` chain such an app
has to encode in its own shaders, which moves every alpha blend into encoded space. The apps in
this tree still author their colours in display space.

## Decision

`Swapchain` takes a preferred colour format and uses it where the surface offers it in a
non-linear sRGB colour space, and `Context3D` passes one through. The default,
`VK_FORMAT_UNDEFINED`, keeps ADR-0009's choice. A preference the surface does not offer falls
back to that choice rather than failing.

## Alternatives

### Reverse ADR-0009 and present through sRGB for every app
- **For**: One convention, and physically correct blending everywhere.
- **Against**: Every colour literal in the 2D apps and the editor would stop meaning what it
  looks like, and text antialiasing blended in linear space reads thin. ADR-0009 weighed this
  and nothing about it has changed.
- **Rejected because**: The apps in this tree still author in display space, and one app's
  lighting does not make their colours wrong.

### The lit app keeps its own swapchain and presenter
- **For**: No change to the api.
- **Against**: The swapchain, the acquire and present loop and their synchronisation would be
  duplicated outside the api, though they are the least app-specific part of the device layer.
  `Presenter` takes a `Swapchain`, so it would be unusable too.
- **Rejected because**: An api that a lit app cannot present through is incomplete, and the fix
  belongs in the api.

## Consequences

- **Gains**:
  - An app that lights its scene presents through the format its shaders assume, and its
    blending stays linear.
  - Every app in the tree passes nothing and gets ADR-0009's format as before.
  - `Swapchain::chooseFormat` is a pure function, so the rule is tested without a GPU.
- **Costs**:
  - Two presentation conventions exist in one api. `Swapchain::format()` is the only reliable
    answer to what the chain is, and a pipeline built against a guess is wrong.
  - A preference offered only in another colour space silently falls back to the default. An
    app whose shaders rely on the encode is then wrong everywhere; a warning is logged, and
    `format()` reports what was chosen.
- **Revisit when**: the apps in this tree move to linear colour, which would reverse ADR-0009
  and make sRGB the default.
