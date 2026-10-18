# ADR-0094: Offline: area lights sampled per shading point

**Status**: proposed
**Date**: 2026-10-18
**Documented in**: [offline/ShadingLanguage.md](../offline/ShadingLanguage.md)

## Context

An RI area light is the geometry declared after `AreaLightSource` in the same attribute block,
and its light shader runs at points on that geometry. A soft shadow needs many such points for
each shading point, each with its own shadow ray. The reyes hider shades a grid's vertices
before any film sample reaches them, and the ray hider shades a hit inside a sample. A picture
must not change with the order of buckets or pixels, or between two runs.

## Decision

The light shader runs at points sampled on the area light's geometry, and the illuminance loop
sees each sample as a separate light. The samples are seeded by a hash of the light's handle and
the shading point's position, so a grid vertex and a ray hit at one point get the same samples.
The number of samples is an attribute, `Attribute "light" "integer nsamples"`, with a default of
16.

## Alternatives

### Seeded from the film's sample
- **For**: the samples are stratified against the pixel's, which lowers the noise of a soft
  shadow under the ray hider.
- **Against**: the reyes hider shades a vertex before any film sample exists, so it would need
  a seed of its own, and the two hiders would sample a light differently.
- **Rejected because**: the two hiders would draw the same soft shadow two ways.

### A fixed pattern for every shading point
- **For**: no seed and no hash, and the samples are the same everywhere.
- **Against**: every point sees the light through the same few points, so a soft shadow bands
  into copies of the occluder's edge.
- **Rejected because**: a viewer reads bands as structure, where noise reads as softness.

### An area light as many point lights
- **For**: placed once when the world ends, it needs nothing new in either hider.
- **Against**: it bands the same way a fixed pattern does, and the number of lights a scene
  holds grows by the sample count.
- **Rejected because**: it has the fixed pattern's banding and costs as much.

## Consequences

- **Gains**: both hiders shade an area light the same way at the same point. A picture is the
  same on every run and in any bucket or pixel order. A light's shader is unchanged: it runs at
  a point as it runs at a placement's origin now.
- **Costs**: samples are not stratified across a pixel, so a soft shadow is noisier than a
  stratified one at the same count. Each sample is a run of the light shader and a shadow ray,
  so the cost of a light grows with its sample count. A hash of a position changes when the
  position does, so the noise moves with a moving surface.
- **Revisit when**: a soft shadow's noise needs fewer samples than independent ones give, which
  stratifying per shading point could do without the film.
