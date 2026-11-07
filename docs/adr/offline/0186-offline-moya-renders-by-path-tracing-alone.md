# ADR-0186: Offline: moya renders by path tracing alone

**Status**: accepted
**Date**: 2026-11-06
**Tags**: offline, lighting, shaders
**Documented in**: [roadmap/MoyaRebuild.md](../../roadmap/MoyaRebuild.md)

## Context

moya renders as RenderMan did before 2016. A reyes hider dices and shades grids, a ray hider
traces Whitted rays, and shaders written in the RenderMan Shading Language gather light in
`illuminance` loops. Pixar removed reyes in RenderMan 21 and deprecated that language. Every
current production renderer is a path tracer in which the integrator gathers light and a
material only describes how a surface scatters it. Light gathered inside a shader cannot be
importance sampled against the material, so soft lighting, glossy reflection and bounced light
stay noisy or absent.

## Decision

moya is rebuilt as a unidirectional path tracer with next event estimation and multiple
importance sampling. The reyes hider, the ray hider and the RenderMan Shading Language are
retired, with no compatibility mode. RIB stays as one way to describe a scene.

## Alternatives

### Keep reyes and the shading language as a legacy mode beside a path tracer
- **For**: Existing RSL shaders and RIB scenes render as they do now.
- **Against**: Two shading models, two ways of finding visibility and two sets of lights. Every
  feature is designed twice or works under one mode only.
- **Rejected because**: A shader that computes its own colour cannot take part in light
  sampling, so the legacy mode could never share the new mode's quality.

### Extend the ray hider into a path tracer and keep the shading language
- **For**: The RIB handler, the graphics state and the shaders carry over.
- **Against**: The shading language returns a colour, not a description of scattering. Light
  shaders compute light at a point and cannot be sampled by area or solid angle.
- **Rejected because**: The integrator would still not own light sampling, which is the change
  that path tracing needs.

### Bidirectional path tracing or vertex connection and merging as the core
- **For**: Better on caustics and on light that reaches the camera through small openings.
- **Against**: Production renderers keep these as options at most. They complicate every
  material, light and sampler, and suit screen-space samplers poorly.
- **Rejected because**: Path guiding and manifold next event estimation cover the same cases on
  a unidirectional core.

## Consequences

- **Gains**:
  - Materials, lights and the integrator separate as they do in every current renderer.
  - Area lights, environments and bounced light are sampled, and converge.
  - Volumes, subsurface scattering, path guiding and denoising build on one core.
- **Costs**:
  - Every RSL shader and the six built-in surface shaders stop working.
  - The reyes hider's displacement and its record of a grid's derivatives are lost, and
    displacement moves to tessellation.
  - ADR-0026, ADR-0078, ADR-0093, ADR-0094, ADR-0095, ADR-0113, ADR-0114 and ADR-0125 describe
    code this retires, and are superseded as it goes.
- **Revisit when**: never, for the core. A stylised or legacy mode can be added as a material
  or a display filter on this core.
