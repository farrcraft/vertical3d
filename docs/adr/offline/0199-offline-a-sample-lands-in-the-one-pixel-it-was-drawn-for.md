# ADR-0199: Offline: a sample lands in the one pixel it was drawn for

**Status**: accepted
**Date**: 2026-11-07
**Tags**: offline
**Documented in**: [offline/Film.md](../../offline/Film.md)

## Context

A pixel filter wider than a pixel blends each sample into its neighbours, either by adding the
sample to every pixel under the filter or by drawing the sample's position from the filter.
moya's film is written by many threads in tiles, and a picture must be the same under any thread
count. The denoiser moya will run assumes that each pixel's noise is independent of its
neighbours'. RenderMan's denoiser requires the importance-sampled mode for this reason, and
3Delight added one so that Open Image Denoiser reads its pictures correctly. pbrt-v4 and Cycles
draw positions from the filter, and RenderMan offers both modes.

## Decision

Each camera sample's position within its pixel is drawn from the pixel filter. The sample's
value is added to that one pixel alone, with the weight the filter's value over its density
gives. RI's five filters are sampled exactly where they have an inverse, and from a table where
they do not.

## Alternatives

### Splat each sample into every pixel under the filter
- **For**: Exact for every filter, negative lobes included. Each sample informs several pixels,
  which lowers variance at a given sample count for a wide filter. The old film already works
  this way.
- **Against**: Tiles write into one another's pixels, so a tile needs borders that are merged
  in a fixed order, or atomic adds, which change the sum's rounding with the thread count.
  Neighbouring pixels share samples, so their noise is correlated.
- **Rejected because**: Correlated noise defeats the denoiser, and shared pixels work against a
  picture that is the same under any thread count.

### Splat, with the filter fixed to a one-pixel box
- **For**: Splatting and importance sampling coincide, and every pixel is independent.
- **Against**: RI's other four filters, and any filter wider than a pixel, are lost.
- **Rejected because**: A scene names its filter, and the box aliases more than any of the others.

## Consequences

- **Gains**:
  - A tile writes only its own pixels, so tiles need no borders, locks or atomics, on the CPU or a
    GPU.
  - A pixel's value depends only on its own samples, so the sum is in sample order under any
    thread count, and the denoiser sees independent noise.
  - Drawing a sample's position costs the same however wide the filter is.
- **Costs**:
  - A filter with negative lobes, such as Catmull-Rom or sinc, gives samples of either sign, so its
    pictures are noisier than splatting the same samples would give.
  - A wide filter blurs without lowering noise, since each sample still informs one pixel.
  - Filters without an analytic inverse, Gaussian among them, are tabulated for sampling, and a
    table's resolution limits how closely the drawn positions follow the filter.
- **Revisit when**: moya's denoiser no longer needs independent pixel noise, or a filter is
  wanted that importance sampling cannot reproduce.
