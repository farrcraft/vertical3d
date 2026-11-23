# ADR-0249: Offline: a collision in a medium is a first hit

**Status**: accepted
**Date**: 2026-11-19
**Tags**: offline, compositing, volumes
**Amends**: ADR-0230
**Documented in**: [offline/Integrator.md](../../offline/Integrator.md)

## Context

A camera sample's first-hit channels, its Cryptomatte IDs and its deep record each say what the
sample met first. A camera ray in fog or smoke may collide in the medium before it reaches any
surface, and a collision has a depth and a place but no surface normal, primitive or `st`. A
denoiser reads an albedo and a normal at the first point that spreads light, and a scattering
event in a medium spreads it. Cycles writes no first-hit value at a volume and lets the surface
behind show through, and gives its denoiser the medium's albedo and the view direction there.

## Decision

A camera ray's first collision in a medium, before any surface, is its first hit for every
first-hit channel and for what the sample covers. Its depth and position are the collision's, its
instance is the volume's or the enclosing surface's, and the values only a surface has are their
defaults. A denoiser reads the medium's single-scattering albedo there, times what the path
carried, and a normal back along the ray, and nothing past it.

## Alternatives

### Surfaces only, as Cycles
- **For**: A plate's depth and position show through fog, and no channel holds a value of a
  point that is not on a surface.
- **Against**: A sample's depth and position then describe a surface its Cryptomatte ID and its
  deep record do not name, and its alpha comes from the fog while its depth does not.
- **Rejected because**: Every output of one camera sample should describe the same thing.

### Nothing for a denoiser at a volume
- **For**: The simplest rule, as the renderer did before.
- **Against**: Fog reads as black to the denoiser, which then smooths it against the surfaces
  behind it.
- **Rejected because**: A denoiser guides by an albedo that fog has.

## Consequences

- **Gains**:
  - Depth, position, instance, Cryptomatte and deep output agree for every camera sample.
  - A denoiser sees fog by its colour, as Cycles' does.
- **Costs**:
  - A depth channel through thin fog is noisy, since each sample stops at its own collision.
  - The plate's depth behind fog takes a render of its own, without the fog.
- **Revisit when**: A compositor needs the depth behind a volume beside the volume's own, as a
  channel of its own.
