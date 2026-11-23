# ADR-0252: Offline: a walk leaves its surface by a Lambertian lobe

**Status**: accepted
**Date**: 2026-11-20
**Tags**: offline, volumes, lighting
**Amends**: ADR-0246
**Documented in**: [offline/Integrator.md](../../offline/Integrator.md)

## Context

A path walking the medium below a subsurface leaves where it meets the surface from inside. A
walk's own events take no light sample, so the light a walk carries out is found only by the
light sample taken where it leaves, through the lobe it leaves by. Through the surface's own
refraction, a smooth surface admits no light sample of a point, spot or distant light. A glossy
one admits so few that its light arrives as rare bright samples that do not converge.
Marble and skin are smooth or glossy, and lit by such lights, more often than not.

## Decision

A walk ends where it meets its surface from inside, and the path leaves by a Lambertian lobe
that goes through the surface. That lobe takes its light sample as any diffuse bounce does, and
gives back the radiance the surface's index took from light as it refracted in. The walk still
enters by the subsurface's dielectric, but no Fresnel factor applies where it leaves.

## Alternatives

### Leave by the subsurface's refraction
- **For**: The exact medium below a dielectric that OpenPBR defines, reciprocal, with the
  surface's roughness shaping the light that leaves.
- **Against**: A smooth subsurface is lit by no light of zero size, and a glossy one converges
  only by rare bright samples, which also spill onto what the walk lights.
- **Rejected because**: It fails the common case of a smooth or glossy subsurface under a key
  light, and a unidirectional path tracer has no other way to find that light.

### Leave by Lambertian transmission or a refraction of roughness one, chosen by a coin
- **For**: Keeps some of a refraction's shape where light leaves, as Cycles' Principled BSDF
  does.
- **Against**: Half its exits are a refraction, which is lit less evenly than a Lambertian lobe,
  and its roughness of one is a constant OpenPBR does not name.
- **Rejected because**: It adds noise and a constant for a shape that a walk's many events have
  already all but erased.

## Consequences

- **Gains**:
  - A smooth or glossy subsurface is lit by every light and converges as a diffuse surface does,
    as in Chiang, Kutz and Burley's walk and in Cycles'.
  - The exit is an ordinary diffuse bounce, so light samples, MIS and the depth limits treat it
    as they treat any other.
- **Costs**:
  - The walk is no longer reciprocal: light enters by a refraction and leaves diffusely.
  - The surface's roughness, anisotropy and Fresnel factor no longer shape the light that leaves
    a walk, and a walk is never reflected back in by its boundary. A thin feature lets out
    somewhat more light than the medium below a dielectric would.
- **Revisit when**: moya gains a way to connect a light through a refraction, such as a
  bidirectional or guided method, or OpenPBR names how a walk leaves its surface.
