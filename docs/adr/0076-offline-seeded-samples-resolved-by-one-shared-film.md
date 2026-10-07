# ADR-0076: Offline: seeded samples resolved by one shared film

**Status**: accepted
**Date**: 2026-10-04
**Documented in**: [offline/CamerasAndSampling.md](../offline/CamerasAndSampling.md)

## Context

Supersampling, the RI pixel filters, depth of field and motion blur all need more than one
sample per pixel, and something has to turn those samples into pixels. Every reference image is
compared at a tolerance of one 8-bit step. That tolerance absorbs float rounding across
compilers but not noise that changes between runs. moya's two hiders visit pixels in different
orders: the reyes hider renders a bucket at a time and the ray hider a pixel at a time. Anything
that depends on render order would let them drift apart.

## Decision

A sample is a point in a pixel, a time within the shutter and a point on the lens. Every hider
produces samples, and one shared `offline::Film` filters them into pixels; the pixel filter is
applied there and nowhere else. A pixel's samples are stratified, jittered and seeded by its
column and row, so a frame is the same on every run, with every standard library and in any
order of pixels or buckets.

## Alternatives

### Each hider filters its own samples
- **For**: Each hider keeps its own loop, and the reyes hider could filter per bucket with no
  whole-image state.
- **Against**: Two filters have to be kept in agreement, and a reference per hider pins each one
  separately. A bucket filtering alone also mishandles the samples at its edges.
- **Rejected because**: The hiders must agree on what a pixel is, and two implementations of one
  formula is how they stop agreeing.

### Random samples from one global stream
- **For**: Simpler than a seed per pixel, with no stratification.
- **Against**: Bucket order, a crop window or a thread changes the picture. A reference then pins
  one render order rather than a scene.
- **Rejected because**: A reference compared at one 8-bit step does not survive noise that
  moves.

### A box filter only
- **For**: Exact, and easy to check by hand.
- **Against**: The RI default is a gaussian two pixels wide, and a box filter aliases visibly on
  an edge.
- **Rejected because**: A scene that names no filter should render the way the RI standard says.

## Consequences

- **Gains**:
  - One filter implementation, so the hiders cannot filter differently.
  - Depth of field and motion blur are each a coordinate of a sample, not a new pass.
  - Determinism is a property of the design, which leaves room for threads that do not change a
    picture.
  - One sample under a one-pixel box reproduces a render taken at pixel centres exactly.
- **Costs**:
  - A sample near a pixel edge contributes to every pixel its filter reaches. The film holds a
    weighted sum per pixel for the whole frame, a second set of planes the size of the image.
  - The RI defaults are two by two samples under a gaussian, so a scene that names nothing is
    four times slower than one sample per pixel. References that pin hiding or shading name one
    sample and a one-pixel box.
  - Adaptive sampling (`PixelVariance`) is honoured only by the ray hider. The reyes hider
    samples a whole bucket at once and takes the count `PixelSamples` names.
  - A pixel's seed is its position, so every frame of an animation uses the same sample
    pattern. A moving scene shows this as fixed-pattern noise.
- **Revisit when**: animation is rendered, at which point the frame number joins the seed in the
  sampler alone.
