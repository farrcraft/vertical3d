# ADR-0114: Offline: a hit's neighbours are shaded only for a derivative

**Status**: accepted
**Date**: 2026-10-25
**Amends**: ADR-0093
**Documented in**: [offline/RayTracing.md](../offline/RayTracing.md)

## Context

A traced hit is shaded with two neighbours a pixel away, so that a derivative is a difference
across the batch as it is on a grid. The neighbours are shaded by the surface shader and by every
light on it, so a hit costs three points of shading where one would do for a shader that never
reads a derivative. None of the standard shaders reads one. Whether a shader reads a derivative
is known when it is compiled, because its own functions are inlined.

## Decision

The compiler records whether a program differentiates: it calls `Du`, `Dv`, `Deriv`, `area` or
`calculatenormal`, or reads `du` or `dv`. A hit is shaded with its neighbours only when its
surface shader or a light shining on it differentiates, and as one point otherwise.

## Alternatives

### Always shade the neighbours
- **For**: every shader sees the same batch layout at a hit, whether or not it uses it.
- **Against**: three times the shading of a hit for the shaders that never read a derivative,
  which are most of them.
- **Rejected because**: a derivative nobody reads does not change the picture.

### Ray differentials
- **For**: a derivative from the ray's own footprint, with no neighbours to shade at all.
- **Against**: every built-in needs its derivative written by hand, and the two hiders would
  compute derivatives two ways.
- **Rejected because**: ADR-0093's reasons for rejecting it still hold.

### Shade the neighbours on demand, at the first derivative a run reaches
- **For**: a shader that reads a derivative only on some branch pays for it only there.
- **Against**: the neighbours' values up to that point would have to be computed again, which
  means running the shader's earlier instructions over them part way through a run.
- **Rejected because**: the machine runs a batch whose width is fixed when it starts.

## Consequences

- **Gains**: a hit under a shader that never differentiates is one point of shading, and with
  ADR-0113's batches that cuts machine time again on the standard shaders.
- **Costs**: a shader that reads a derivative on a branch it seldom takes pays for the
  neighbours at every hit. A light that differentiates puts every surface it lights back on the
  stencil.
- **Revisit when**: shaders that read a derivative on a rarely taken branch are common, or the
  machine can widen a batch part way through a run.
