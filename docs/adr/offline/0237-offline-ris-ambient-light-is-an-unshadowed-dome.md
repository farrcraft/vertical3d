# ADR-0237: Offline: RI's ambient light is an unshadowed dome

**Status**: accepted
**Date**: 2026-11-15
**Tags**: offline, lighting, compositing
**Documented in**: [offline/Rib.md](../../offline/Rib.md), [offline/Lights.md](../../offline/Lights.md)

## Context

RI's `ambientlight` lights every point by the same amount, unshadowed, which no physical light
does. Most RIB scenes written for RI name one beside their other lights. moya's integrator samples
every light it gathers, and a light path expression and a light group can only collect light that
a light event names. A light that shadows nothing is already expressible, through shadow linking.
On a diffuse surface, a constant dome that nothing shadows gives the surface's albedo times its
radiance, which is RI's ambient term.

## Decision

`LightSource "ambientlight"` becomes a dome light of constant radiance, its `intensity` times its
`lightcolor`, whose shadow link names no instance. Camera rays do not see it.

## Alternatives

### A dome light that casts shadows
- **For**: The physical light an ambient term stands for, with occlusion.
- **Against**: Every corner and contact darkens, so no old scene keeps its look.
- **Rejected because**: It renders a different light from the one the scene asked for.

### A term added at each hit, not sampled
- **For**: RI's ambient term exactly, on every lobe.
- **Against**: It has no light event, so no expression or light group can hold it. It lies
  outside importance sampling and outside the energy a furnace test checks.
- **Rejected because**: It would be the one light the integrator does not own.

### Reported and dropped
- **For**: What RenderMan's path tracer does with an RSL light.
- **Against**: Old scenes lose a share of their light, and only a log line says so.
- **Rejected because**: The unshadowed dome keeps the scene's light at no cost to the core.

## Consequences

- **Gains**:
  - An old scene keeps its diffuse look, and its ambient light is a light group of its own.
  - Light linking and `Illuminate` turn it on and off as they do any light.
- **Costs**:
  - It lights glossy and specular lobes too, which RI's ambient term did not.
  - Its light reaches inside closed rooms, as RI's did, which a physical light never would.
- **Revisit when**: A scene needs RI's ambient term on diffuse lobes alone.
