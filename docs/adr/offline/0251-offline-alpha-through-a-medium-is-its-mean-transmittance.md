# ADR-0251: Offline: alpha through a medium is its mean transmittance

**Status**: accepted
**Date**: 2026-11-19
**Tags**: offline, compositing, volumes, colour
**Documented in**: [offline/Integrator.md](../../offline/Integrator.md)

## Context

A render of fog over nothing composites over a plate only when its alpha is one less the share of
light the fog lets through. A path carries four wavelengths, and a chromatic medium lets each
through by a different share, while alpha is one number. Tracking already weighs a camera path
that leaves the scene by what each wavelength passed. Cycles makes a volume's alpha one less the
mean of the throughput such a path keeps.

## Decision

A camera sample whose path leaves the scene without meeting anything has an alpha of one, less
the mean over its wavelengths of the throughput it keeps. A path that meets a surface, a light or
a collision has an alpha of one, so the expected alpha is one less the mean transmittance along
the camera ray.

## Alternatives

### The transmittance's luminance
- **For**: Alpha then follows how dense the medium looks.
- **Against**: It weighs green over red and blue, and four sampled wavelengths give a luminance
  only by a weighting of their own, which makes its estimate noisier.
- **Rejected because**: The mean is unbiased from the throughput as it stands, and matches Cycles.

## Consequences

- **Gains**:
  - Fog over nothing composites over a plate, and grey fog does so exactly.
  - Alpha needs no tracking of its own, since the path's own weights estimate it.
- **Costs**:
  - A chromatic medium composited by one alpha tints the plate behind it greyly, not by its
    colour.
  - Thin fog's alpha is as noisy as the camera's collisions in it.
- **Revisit when**: A compositor asks for an alpha in each channel.
