# ADR-0198: Offline: the working space is OpenColorIO's scene-linear role

**Status**: accepted
**Date**: 2026-11-07
**Tags**: offline, colour
**Documented in**: [offline/Colour.md](../../offline/Colour.md)

## Context

moya carries light as spectra, but a scene gives most of its colours as RGB, and a compositor
reads the film's outputs as RGB. Every RGB value needs a space: its primaries and white point.
Blender works in linear Rec. 709 by default and offers ACEScg, Redshift moved its default from
linear Rec. 709 to ACEScg, and pipelines that run on OpenColorIO name the space in their
configuration's scene-linear role. The RGB-to-spectrum table is built at build time for one
gamut, and an RGB value outside it cannot be matched by a smooth reflectance.

## Decision

moya's working space is the scene-linear role of the OpenColorIO configuration it runs with,
which is ACEScg in the ACES 2.0 configuration moya uses when given none. An output is written in
the working space unless it names another, and an RGB value that names no space is read in it.
The RGB-to-spectrum table is built for ACEScg, and every RGB value is converted into ACEScg
before its spectrum is found.

## Alternatives

### Linear Rec. 709, fixed
- **For**: The gamut most textures are authored in, and Blender's default.
- **Against**: Colours outside sRGB's gamut, from wide-gamut textures, measured lights or an
  ACES pipeline, are clipped or negative. A studio's configuration is ignored.
- **Rejected because**: It limits every colour moya reads or writes to sRGB's gamut, which is
  narrower than the light moya simulates.

### ACEScg, fixed
- **For**: One space everywhere, wide enough for Rec. 2020 and DCI-P3.
- **Against**: A pipeline whose configuration names another scene-linear space has to convert
  every input and every output around moya.
- **Rejected because**: A renderer that plugs into other pipelines takes their working space
  from their configuration, as OpenColorIO intends.

### A spectrum table for each working space
- **For**: The table's gamut always matches the working space.
- **Against**: The table is fitted at build time, which takes too long to repeat when a space is
  named at run time.
- **Rejected because**: ACEScg's gamut holds the display spaces, Rec. 2020 and DCI-P3 among them,
  so one table serves the working spaces pipelines use.

## Consequences

- **Gains**:
  - moya renders in the space its host's pipeline already uses, with no conversion around it.
  - With no configuration, a picture is in ACEScg, and a compositor reading it as ACES gets it
    without conversion.
  - One table, built once, finds the spectrum of an RGB value from any working space a pipeline
    uses.
- **Costs**:
  - An RGB input in another space is converted into ACEScg before its spectrum is found. The
    film holds ACEScg, which is converted into another working space once, when the picture is
    written or displayed.
  - A colour outside ACEScg's gamut gets the nearest spectrum the table holds, not its own. Some
    camera gamuts, such as ARRI Wide Gamut and S-Gamut3, reach beyond it.
  - A RIB scene's colours are read as ACEScg when they name no space, so a scene made for moya's
    old core renders more saturated unless its client names Rec. 709 for them.
- **Revisit when**: a working space wider than ACEScg is common in the configurations moya runs
  with.
