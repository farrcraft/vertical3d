# ADR-0076: Offline Sampling — A Pixel Is A Filtered Set Of Seeded Samples, Resolved By One Film Both Renderers Share

**Date**: 2026-10-04
**Status**: accepted
**Deciders**: Joshua Farr

## Context

talyn and moya both take one sample per pixel centre and write it straight into
`offline::FrameBuffer`'s planes. Phase 4 of [the offline rendering
roadmap](../roadmap/OfflineRendering.md) asks for supersampling, the five RI pixel filters, depth
of field and motion blur, and [OfflineRenderingPhases4To6](../plans/OfflineRenderingPhases4To6.md)
takes it up. Two constraints shape the answer. Every reference in both suites is compared at one
8-bit step, which absorbs float rounding across compilers and not noise. And moya renders a bucket
at a time while talyn renders a pixel at a time, so anything that depends on render order would
let the two hiders drift apart.

## Decision

**A sample is a point in a pixel, a time across the shutter and a point on the lens. Both renderers
produce samples, and one shared `offline::Film` turns them into pixels: the filter is applied
there and nowhere else.** A pixel's samples are stratified and jittered from a `type::Random`
seeded by its column and row, so a frame is the same on every run, every standard library and
any order of pixels or buckets. A pixel's colour is the weighted sum of the samples within the
filter's width, divided by the sum of the weights, and its coverage is the same sum over hit or
miss. Opacity is held per sample. The imager runs after the film resolves, on pixels, as it does
now.

## Alternatives Considered

### Alternative 1: A shared film, fed seeded samples — **chosen**
- **Pros**: One filter implementation, so the two renderers cannot filter differently. A seed per
  pixel keeps every reference reproducible and leaves room for threads that do not change a
  picture. One sample under a one pixel box gives the sample back exactly, so the existing
  references survive unchanged.
- **Cons**: A sample near a pixel edge is splatted into every pixel its filter reaches, so the
  film holds whole-image sums rather than finishing a pixel and forgetting it.
- **Why not**: n/a — chosen.

### Alternative 2: Each renderer filters its own samples
- **Pros**: Each renderer keeps its own loop, and moya could filter per bucket with no
  whole-image state.
- **Cons**: Two filters to keep in agreement, and a reference per renderer pins each separately.
  A bucket filtering alone also mishandles the samples at its edge.
- **Why not**: The renderers are meant to agree on what a pixel is, and two implementations of
  the same formula is how they stop agreeing.

### Alternative 3: Random samples from one global stream
- **Pros**: Simpler than a seed per pixel, and needs no stratification.
- **Cons**: Bucket order, crop window or a thread changes the picture. A reference then pins one
  render order rather than a scene.
- **Why not**: A reference compared at one 8-bit step does not survive noise that moves.

### Alternative 4: A box filter only
- **Pros**: Exact and easy to test by hand.
- **Cons**: The RI default is a gaussian two pixels wide, and a box at two by two aliases visibly
  on an edge.
- **Why not**: A scene that names no filter should render the way RenderMan says it does.

## Consequences

### Positive
- moya's `Ri*Filter` functions and both renderers' sampling become calls into one library.
- Depth of field and motion blur are each a coordinate of a sample rather than a new pass.
- Determinism is a property of the design rather than of a test fixture.

### Negative
- The RI defaults are two by two samples under a gaussian, so a scene that names nothing renders
  four times slower and differently from before. The existing references name `PixelSamples 1 1`
  and a one pixel box to keep their pictures.
- A film is a second set of planes the size of the image, held for the whole frame.
- moya does not adapt its sample count, because a reyes hider samples a bucket at once.

### Risks
- A pixel's seed is its position, so two frames of an animation share their sample patterns.
  That is what a reference wants and what a moving scene shows as fixed-pattern noise; the
  escape hatch is folding the frame number into the seed, in the sampler alone.
