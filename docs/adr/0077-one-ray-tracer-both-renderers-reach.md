# ADR-0077: Offline Ray Tracing — One Ray Tracer In The Shared Library, Which talyn Drives And moya's Shaders Reach

**Date**: 2026-10-04
**Status**: accepted
**Deciders**: Joshua Farr

## Context

Phase 6 of [the offline rendering roadmap](../roadmap/completed/OfflineRendering.md) asks whether talyn and
moya unify. [OfflineRenderingPhases4To6](../plans/completed/OfflineRenderingPhases4To6.md) answers it here,
after its step 11 made talyn's `trace()` re-entrant and step 12 gave it transparency, reflection,
refraction and spheres. moya keeps no scene: primitives go into buckets in camera space and are
diced once, so `GridShader` answers neither `trace()` nor `transmission()`, and moya has no
shadows. Both renderers already share the RIB reader, the shading language, the film and the
sampler through `api/render/offline`, per [ADR-0022](0022-offline-rendering-shares-an-api-library.md).

## Decision

**talyn's scene, its intersection and its hit shading move into `api/render/offline/trace`, under
`offline::trace`, and both renderers reach it.** talyn becomes a driver that casts primary rays
into the shared scene. moya keeps its reyes hider for what the camera sees, adds every primitive it
is given to a shared scene as well, and answers a shader's `trace()` and `transmission()` from it.
A renderer is then its hider, and everything after a hit is shared.

- **The shared scene is in world space**, as talyn's is. moya adds a primitive as it arrives, before
  it is split or diced, by its object-to-world placement and its motion; its shaders ask from camera
  space, so `GridShader` carries a ray into world space and nothing back out but a colour.
- **A primitive carries the lights that were on when it arrived**, as moya's `Shading` does, held
  as one shared set per change of lights rather than a copy each. A traced hit is shaded against its
  own primitive's lights, so a mirror in moya shows a surface lit as the camera sees it.

## Alternatives Considered

### Alternative 1: One ray tracer in the shared library — **chosen**
- **Pros**: One intersection and one hit shader, so a reflection and a shadow are the same in both
  renderers. moya gains shadows without a depth pass. talyn's suite keeps testing the tracer, and
  the offline suite gains the cases that test it without either renderer.
- **Cons**: moya holds every primitive twice, once in its buckets and once in the scene.
  `api/render/offline` grows a scene and an intersector, which is more than "the code both
  renderers parse and shade with".
- **Why not**: n/a — chosen.

### Alternative 2: moya links `libtalyn` and uses talyn as its ray tracing component
- **Pros**: Nothing moves, and the step is small.
- **Cons**: One renderer linking another makes talyn's driver state, its options and its sampling
  part of moya's build, and the dependency runs between two peers rather than down into the api.
- **Why not**: [ADR-0022](0022-offline-rendering-shares-an-api-library.md) puts what both renderers
  use in `api/render/offline`, and this is that.

### Alternative 3: One renderer with two hiders
- **Pros**: A RenderMan renderer with a ray traced hider is a real design, and one options and
  attribute state would serve both. talyn as an executable retires.
- **Cons**: A larger change than either renderer needs: moya's context, buckets and C API would have
  to absorb talyn's driver, and talyn is the simpler of the two to read and to test against.
- **Why not**: The tracer is what moya lacks; its hider and its state are not.

### Alternative 4: Leave them apart, and moya traces nothing
- **Pros**: No work, and no second copy of moya's geometry.
- **Cons**: moya never has shadows. A reyes renderer's own answer is a shadow map, which is a depth
  pass per light, a `shadow()` built-in and a filter of its own.
- **Why not**: Sharing the tracer is less work than a shadow map and gives reflections besides.

### Alternative 5: The scene holds one light list, as talyn's does
- **Pros**: Simpler; talyn's lights are already one list, fixed at the first primitive.
- **Cons**: moya honours `Illuminate` per primitive for what the camera sees, so a traced hit would
  be lit by a different set from the same surface seen directly.
- **Why not**: A renderer that disagrees with itself across a mirror is a worse fault than a pointer
  per primitive.

## Consequences

### Positive
- moya has ray traced shadows, and `shinymetal` and `glass` work in it.
- A traced ray is one implementation, so the two renderers' reflections can be pinned against each
  other the way `textured.rib` pins their textures.
- talyn's picture does not change: its scene moves rather than being rewritten.

### Negative
- moya's memory holds each primitive twice, and its time adds a ray per shading point per light for
  every shadow, through talyn's brute-force intersection.
- moya shades a grid once for all of its samples, so its shadows and traced rays see the scene at
  shutter open. talyn's see each sample's time. A moving occluder's shadow is sharp in moya.
- `api/render/offline` stops being only a parser, a language and a film.

### Risks
- **A ray leaving a micropolygon may meet the polygon it was diced from.** The shared scene holds
  the exact polygon and moya shades a bilinear grid of it, which only agree when the polygon is
  planar. The offset along `Ng` absorbs a planar one; a warped quad would show acne, and the
  escape hatch is a larger offset for a ray that starts on a moya grid, in `GridShader` alone.
- **talyn's per-scene light list becomes per primitive.** No scene in either suite switches a light
  after its geometry, so no reference moves; one that does will now render as RI describes rather
  than as talyn did.
- **Brute force will not survive moya's scene sizes.** The acceleration structure the plan holds
  waits for a scene that takes a second, and moya tracing is the likeliest way to produce one; it
  goes inside `offline::trace` and neither renderer changes.
