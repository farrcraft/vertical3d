# ADR-0077: Offline: one shared ray tracer

**Status**: superseded
**Date**: 2026-10-04
**Superseded by**: [ADR-0078](0078-offline-moya-is-the-one-renderer-ray-tracing-is-a-hider.md)
**Documented in**: [offline/RayTracing.md](../offline/RayTracing.md)

## Context

Two offline renderers share the RIB reader, the shading language, the film and the sampler
through `api/render/offline` ([ADR-0022](0022-offline-shared-library-with-no-vulkan.md)). One is
a ray tracer with a world-space scene, intersection and hit shading. The other, moya, is a reyes
renderer that keeps no scene: primitives go into buckets in camera space and are diced once. It
therefore cannot answer a shader's `trace()` or `transmission()`, and has no shadows or
reflections.

## Decision

The ray tracer's scene, intersection and hit shading move into `api/render/offline/trace`, under
`offline::trace`, and both renderers use them. moya keeps its reyes hider for what the camera
sees, also adds every primitive to the shared scene, and answers `trace()` and `transmission()`
from it. The scene is in world space, and each primitive carries the lights that were on when it
arrived, held as one shared set per change of lights.

## Alternatives

### moya links the ray tracer's renderer library
- **For**: Nothing moves, and the change is small.
- **Against**: One renderer linking another makes the second renderer's driver state, options
  and sampling part of moya's build. The dependency runs between two peers rather than down into
  the api.
- **Rejected because**: ADR-0022 puts what both renderers use in `api/render/offline`.

### moya traces nothing
- **For**: No work, and no second copy of moya's geometry.
- **Against**: moya never has shadows. A reyes renderer's own answer is a shadow map: a depth pass
  per light, a `shadow()` built-in and its own filtering.
- **Rejected because**: Sharing the tracer is less work than shadow maps and gives reflections as
  well.

### The scene holds one light list
- **For**: Simpler, and the existing tracer already used one list.
- **Against**: moya honours `Illuminate` per primitive for what the camera sees, so a traced hit
  would be lit by a different set of lights than the same surface seen directly.
- **Rejected because**: A renderer that disagrees with itself across a mirror is a worse fault
  than one pointer per primitive.

## Consequences

- **Gains**:
  - moya has ray traced shadows, and `shinymetal` and `glass` work in it.
  - A reflection and a shadow are one implementation in both renderers.
  - The tracer is tested in the offline suite without either renderer.
- **Costs**:
  - moya holds every primitive twice, in its buckets and in the scene, and every shadow adds a
    ray per shading point per light through a brute-force intersection.
  - moya shades a grid once for all its samples, so its traced rays see the scene at shutter
    open, and a moving occluder's shadow is sharp.
  - A ray leaving a micropolygon may hit the exact polygon it was diced from when that polygon
    is not planar.
  - `api/render/offline` grows a scene and an intersector.
- **Revisit when**: brute-force intersection is too slow for moya's scenes, which calls for an
  acceleration structure inside `offline::trace`.
