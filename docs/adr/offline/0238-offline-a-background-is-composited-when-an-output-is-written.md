# ADR-0238: Offline: a background is composited when an output is written

**Status**: accepted
**Date**: 2026-11-15
**Tags**: offline, compositing
**Documented in**: [offline/Output.md](../../offline/Output.md), [offline/Rib.md](../../offline/Rib.md)

## Context

RI's `background` imager fills the pixels geometry does not cover with a colour, and leaves every
pixel opaque. moya's beauty carries alpha, and its film, checkpoints, channels, Cryptomatte layers,
deep samples and the denoiser's input are all built from the film's sums. A dome light the camera
sees leaves its pixels at zero alpha. RenderMan replaced imagers with display filters, which run
on the finished pixels and never write back to the film, and its background filter composites the
beauty over a colour.

## Decision

A background is a filter of an output: when an output is written, its beauty is composited over
the colour by its alpha, and its alpha becomes one. `Imager "background"` and RenderMan's
`DisplayFilter "PxrBackgroundDisplayFilter"` both name it. Any other imager or display filter is
reported once and changes nothing.

## Alternatives

### A dome light the camera alone sees
- **For**: How Arnold, Cycles and 3Delight show an environment behind a render.
- **Against**: Alpha stays zero where the colour shows, the opposite of the imager. It needs
  light linking to keep it from lighting anything.
- **Rejected because**: It is an environment, not a plate, and gives a different picture.

### Reported and dropped
- **For**: What RenderMan's path tracer does with an imager.
- **Against**: Old scenes, moya's installed one among them, lose their backgrounds.
- **Rejected because**: A filter at output costs the core nothing.

## Consequences

- **Gains**:
  - The film and every channel stay the raw render, so a compositor still gets alpha from them.
  - The background lights nothing and appears in no reflection, as the imager's did not.
  - Other display filters, such as a plate image, have a place to go.
- **Costs**:
  - The beauty an output writes differs from the film's, and a checkpoint holds the film's.
  - A denoised beauty is composited after denoising, so the denoiser never sees the colour.
- **Revisit when**: The editor gains a backplate, and it needs a plate image in the same stage.
