# ADR-0224: Offline: a ray's footprint is a cone

**Status**: accepted
**Date**: 2026-11-11
**Tags**: offline, textures
**Documented in**: [plans/MoyaMaterials.md](../../plans/MoyaMaterials.md)

## Context

A texture is filtered over the area a pixel's sample covers where its ray meets a surface, and
bump mapping takes a height's differences over the same area. The area has to follow a path
through mirrors and glass, where it is no longer the camera's. Every method approximates it
after a rough bounce, which most bounces are. Each float a path carries costs memory and
bandwidth on a GPU. A footprint seen at a grazing angle is long and narrow, and filtering it as a
circle blurs the texture.

## Decision

A ray carries its footprint as a cone, a width and a spread angle, set by the camera's pixel and
carried through reflection and refraction. At each hit the cone grows by the surface's curvature,
its spread widens by the sampled lobe's roughness, and it is projected onto the surface as an
ellipse. A lookup filters over the ellipse by a bounded number of trilinear probes along its
major axis.

## Alternatives

### Ray differentials carried through every bounce
- **For**: An exact, anisotropic footprint through specular chains, as RenderMan and pbrt carry
  one.
- **Against**: Four vectors a path, six times a cone's state, and computation at every bounce.
  After a rough bounce they are approximated as a cone's spread is.
- **Rejected because**: Their exactness holds only on the specular chains, and the projected
  cone gives back the anisotropy at the hit.

### Camera differentials alone, approximated at each hit
- **For**: Nothing is carried in the path, as pbrt-v4 approximates later hits.
- **Against**: A hit seen in a mirror or through glass takes a footprint measured from the camera,
  not along the path.
- **Rejected because**: Textures seen through specular surfaces would filter at the wrong width.

### EWA filtering over the ellipse
- **For**: The highest-quality filter over an elliptical footprint, as pbrt uses.
- **Against**: Its cost grows with the ellipse's size at its level, with no bound a GPU can plan
  for.
- **Rejected because**: Probes along the major axis give nearly its quality at a bounded cost.

### One trilinear lookup
- **For**: The cheapest lookup.
- **Against**: A narrow footprint is filtered as a circle, so a grazing texture blurs.
- **Rejected because**: The ellipse is already known at the hit.

## Consequences

- **Gains**:
  - A path carries two floats for its footprint on every device.
  - Textures seen in mirrors and through glass filter at the width the path gives them.
  - Bump mapping and texture lookups share one footprint.
- **Costs**:
  - The cone is round, so a pixel stretched along one axis is not captured until it is projected.
  - Widening by roughness is an approximation, tuned rather than derived.
  - A lookup at a grazing angle costs several trilinear probes.
- **Revisit when**: A footprint after rough bounces that is both principled and as small as a
  cone, such as path differentials, becomes practical on a GPU.
