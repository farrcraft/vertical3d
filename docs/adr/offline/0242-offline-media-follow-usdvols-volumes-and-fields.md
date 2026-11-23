# ADR-0242: Offline: media follow UsdVol's volumes and fields

**Status**: accepted
**Date**: 2026-11-16
**Tags**: offline, volumes, interchange
**Documented in**: [offline/Scene.md](../../offline/Scene.md)

## Context

moya's scene names surfaces and their materials, and nothing that fills the space between them.
A medium is either a grid read from a file, such as smoke, or a constant inside a closed surface,
such as fog in a box or the liquid in a glass. OpenPBR describes coloured transmission and
subsurface as a medium below a dielectric surface, so one object can hold both a surface and the
medium inside it. Three clients fill the scene: RIB, which names a grid through RenderMan's
`Volume` primitive and a medium through a volume `Bxdf`, the editor, and a Hydra delegate, which
hands over UsdVol's volumes. USD, Karma and Cycles hold fields apart from the volume that reads
them, and read each field in the shader by a name of the volume's choosing.

## Decision

A volume is a geometry that names its fields, each an object of a file and a grid, by the names
its shader reads them by, as UsdVol does. A material's volume terminal holds a MaterialX volume
shader, which reads a field through `geompropvalue` and fills the volume or closed surface it is
given to. A closed surface whose
material has no volume terminal takes the medium its OpenPBR transmission depth and subsurface
inputs give, and a volume with no material reads its `density` field as its density.

## Alternatives

### RenderMan's volumes
- **For**: RIB reads as written: a `Volume` primitive names its file and its grids, and its
  `Bxdf` is a volume shader.
- **Against**: A material holds one shader, a surface or a volume, so a glass of coloured liquid
  takes two surfaces, and OpenPBR's medium below its surface has no place. Fields live inside the
  primitive, so a changed grid rebuilds the whole volume, and a Hydra delegate folds UsdVol's
  fields into it.
- **Rejected because**: OpenPBR's own transmission and subsurface are a surface and a medium on
  one object.

### pbrt-v4's media
- **For**: Nothing is inferred: every instance names the medium inside it and outside it, as
  `MediumInterface` does.
- **Against**: A medium is an object of parameters, not a MaterialX graph, so patterns cannot
  drive it, against ADR-0190. Neither USD nor RIB says what each surface separates, so every
  client has to work it out.
- **Rejected because**: It moves media out of the materials every client already fills.

## Consequences

- **Gains**:
  - A Hydra delegate passes UsdVol's volumes and fields through as they are.
  - One material holds a surface and the medium inside it, as OpenPBR and Cycles do.
  - A volume's parameters run in the node program, so a pattern can drive them.
  - A changed file or grid rebuilds that field alone.
- **Costs**:
  - The scene gains a volume kind, a field kind and a third terminal.
  - The RIB client turns one `Volume` request into a volume and its fields, and a volume `Bxdf`
    into a terminal.
  - A field's name in the shader and its grid's name in the file are two names, as UsdVol keeps
    them, which a user can confuse.
  - Which medium fills a closed surface is a rule moya states, since USD leaves it to each
    renderer.
- **Revisit when**: USD adopts a schema of media that bind to surfaces, or MaterialX defines how
  a surface and a volume shader share an object.
