# ADR-0244: Offline: media are tracked by null scattering with spectral MIS

**Status**: accepted
**Date**: 2026-11-16
**Tags**: offline, volumes
**Documented in**: [offline/Integrator.md](../../offline/Integrator.md)

## Context

A path in a medium has to find where it scatters or is absorbed, and a shadow ray what passes,
where the medium's density varies along the ray. A path carries four wavelengths, and a medium
such as skin or coloured smoke has a different density at each. A light sample through a medium
is noisy unless it can be weighed against a tracked path by multiple importance sampling.
Tracking meets an unknown number of collisions, so its random numbers cannot all take fixed
places among the sampler's dimensions, and each device must still give one picture on every run.

## Decision

A path is tracked by null scattering at its hero wavelength, and the other wavelengths are
weighed by spectral MIS, as pbrt-v4 runs Miller, Georgiev and Jarosz's formulation. A shadow ray
takes the transmittance by ratio tracking inside the same MIS, and a scattering event follows
Henyey and Greenstein's phase function. A scattering event's direction, light sample and roulette
draw from a bounce's dimensions, and the distances between collisions from a hash of the pixel,
the sample, the bounce and the collision.

## Alternatives

### Spectral tracking
- **For**: Proven in production at Disney and in RenderMan, and simpler: free paths are drawn so
  that every wavelength stays unbiased.
- **Against**: The chance of a tracked distance is unknown, so a light sample through a medium is
  weighed against the phase function by direction alone. Emitting and thin media are noisier.
- **Rejected because**: Volumetric lights and path guiding build on light samples weighed across
  the whole path.

### Delta tracking at the hero wavelength alone
- **For**: The simplest tracker: a chromatic medium keeps one wavelength, as pbrt-v4's simple
  integrator does.
- **Against**: A chromatic medium renders with colour noise, from one wavelength in four.
- **Rejected because**: It gives up the colour accuracy ADR-0189 chose four wavelengths for.

### Ray marching at a fixed step
- **For**: Cheap to reason about, and what Arnold and the old Cycles do.
- **Against**: Biased by the step, and its cost follows the step rather than the medium's
  density.
- **Rejected because**: moya is an unbiased path tracer, as ADR-0186 decides.

## Consequences

- **Gains**:
  - Every wavelength of a chromatic medium stays unbiased.
  - Light samples through a medium are weighed against tracked paths over distance and direction.
  - Volumetric lights and path guiding build on the same formulation.
  - The picture is the same on every run, though a path's collisions vary in number.
- **Costs**:
  - The path state carries two more spectra, the rescaled chances.
  - A collision's weight needs care, or the variance grows in thin, bright media.
  - The distances between collisions come from a hash, not from the sampler, so they do not
    gain from the sampler's stratification.
- **Revisit when**: Tracking's random numbers can be drawn from a sampler of unbounded
  dimensions without losing its stratification, or a production renderer shows an unbiased
  tracker with lower variance in chromatic media.
