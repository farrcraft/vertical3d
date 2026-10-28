# ADR-0093: Offline: derivatives from neighbouring shading points

**Status**: amended
**Date**: 2026-10-18
**Amended by**: ADR-0113, ADR-0114
**Documented in**: [offline/ShadingLanguage.md](../offline/ShadingLanguage.md)

## Context

SL's `Du`, `Dv`, `Deriv`, `area` and `calculatenormal` are rates of change across the surface,
and bump mapping and displacement need them. The machine runs a shader over a batch of points,
and the reyes hider's batch is a grid with a neighbour on every side. The ray hider shades one
hit at a time, and the hits of a scanline lie on different primitives and are cast in
different adaptive passes. A derivative computed one way under one hider and another way under
the other gives a bumped surface two looks.

## Decision

A derivative is a difference across neighbouring shading points in the batch, divided by the
step in u or v between them. The reyes hider's neighbours are its grid's next vertex along u or
v, or the previous one on the last row or column. The ray hider shades each hit with two
neighbours a pixel's width away along the primitive's u and v, as a batch of three.

## Alternatives

### Ray differentials
- **For**: each ray carries its rate of change across the image, so a derivative is exact for
  the ray's footprint, and texture filtering can use it too.
- **Against**: every built-in needs its derivative written by hand, rather than differencing the
  values the shader computed. The reyes hider would still difference its grid, so the two
  hiders would compute derivatives in two ways.
- **Rejected because**: it doubles the library's work, and the two hiders would still differ.

### A scanline's hits as the batch
- **For**: one run of the machine for many hits, which is also faster.
- **Against**: neighbouring hits can lie on different primitives, or on the same primitive at
  different times under adaptive sampling, so their difference is not a derivative.
- **Rejected because**: it gives wrong derivatives at every edge and in every adaptive pixel.

### Derivatives under the reyes hider only
- **For**: nothing changes in the ray hider, and a hit keeps a batch of one.
- **Against**: bump mapping and `calculatenormal` work under one hider and return zero under the
  other.
- **Rejected because**: a scene would render differently under the two hiders.

## Consequences

- **Gains**: the machine computes a derivative one way under both hiders, from the shader's own
  values, so every built-in and every user function has a derivative for free.
- **Costs**: the ray hider runs the machine over three points for every hit, about three times
  the shading work. A stencil point a step past a primitive's edge lies off the primitive, on
  its plane extended. A difference is first order, so a derivative on a curved surface is only
  as good as the step is small.
- **Revisit when**: texture filtering needs a ray's footprint, which differentials would give,
  or shading three points per hit shows in a ray hider profile.
