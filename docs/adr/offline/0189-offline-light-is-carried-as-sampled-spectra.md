# ADR-0189: Offline: light is carried as sampled spectra

**Status**: accepted
**Date**: 2026-11-06
**Tags**: offline, colour
**Documented in**: [roadmap/MoyaRebuild.md](../../roadmap/MoyaRebuild.md)

## Context

A renderer that multiplies RGB triples gives results that depend on the RGB space it works in.
The error grows under narrow-band lights and coloured interreflection. Dispersion and measured
light and camera data need light carried by wavelength. Textures, OpenColorIO configurations and
compositing are RGB. Adding spectra to an RGB renderer touches every colour value, its kernels
and its GPU state, and Cycles' attempt was never merged.

## Decision

Each path carries four wavelengths, sampled by hero wavelength sampling, and every radiance and
reflectance value is a sampled spectrum. RGB inputs are converted to smooth spectra when they are
read, and the film converts to its RGB working space. The colour type is fixed when moya is
compiled, so an RGB build stays possible for comparison.

## Alternatives

### RGB transport
- **For**: Inputs and outputs need no conversion, and a path carries three values with no
  wavelengths.
- **Against**: Colour accuracy depends on the working space. Dispersion needs a separate
  mechanism. Changing to spectra later is the hard case.
- **Rejected because**: moya aims for the highest quality, and the decision is cheapest to take
  before anything is built.

### More than four wavelengths per path
- **For**: Less colour noise per sample.
- **Against**: More state per path, which costs most on a GPU.
- **Rejected because**: Four wavelengths with hero sampling is the balance pbrt-v4 and Manuka
  use.

## Consequences

- **Gains**:
  - Colour does not depend on the working space, and coloured lights mix correctly.
  - Dispersion and spectral lights need no special case.
- **Costs**:
  - Colour noise from the sampled wavelengths, which more samples remove.
  - Every RGB texture and parameter goes through a conversion table.
  - A path carries eight more floats than an RGB path.
- **Revisit when**: never for the core. The count of wavelengths can change at compile time.
