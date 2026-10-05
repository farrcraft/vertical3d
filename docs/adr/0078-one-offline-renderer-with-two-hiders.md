# ADR-0078: Offline Renderers — moya Is The One Offline Renderer, And Ray Tracing Is A Hider It Selects

**Date**: 2026-10-04
**Status**: accepted
**Deciders**: Joshua Farr

## Context

[ADR-0077](0077-one-ray-tracer-both-renderers-reach.md) moved talyn's scene, intersection and hit
shading into `api/render/offline/trace` and left talyn a driver over it. Its Alternative 3, one
renderer with two hiders, was set aside as larger than either renderer needed. What talyn has left
is a primary ray caster with adaptive sampling, about 270 lines, beside a second copy of the RI
graphics state that moya already holds. That state covers the attribute and transform stacks,
motion blocks, light handles, and the frame aspect and screen window chain. talyn also refuses
any camera `type::camera::Profile` cannot hold: an off-centre screen window, or a world to camera
matrix that is not a rotation and a translation.

## Decision

**talyn is retired, and ray tracing is a hider moya selects with `Hider "raytrace"`.** The reyes
hider stays the default, under RI's name `"hidden"`. Both hiders read one graphics state, one
camera and one shared traced scene. The ray hider casts its primary rays from moya's own camera
coordinate systems rather than from a `Profile`, so it takes any camera moya's reyes hider takes.

## Alternatives Considered

### Alternative 1: Merge talyn into moya as a hider — **chosen**
- **Pros**: One graphics state, one RIB handler, one C API and one driver. A scene renders under
  either hider by changing one request, so the two can be pinned against each other on the same
  file. The ray hider gains every camera moya accepts.
- **Cons**: moya's render context grows a second path through `render()`. talyn's code-built
  test scenes have to be rebuilt through moya's context, because they placed a `Profile`
  directly.
- **Why not**: n/a — chosen.

### Alternative 2: Move the RI graphics state into `api/render/offline` and keep both renderers
- **Pros**: Each renderer shrinks to its hider, and talyn stays a small renderer to read.
- **Cons**: Two drivers, two RIB handlers and two render contexts remain over one shared state,
  so the same scene still has to be written for two executables. The shared state would also be
  sized for exactly two consumers.
- **Why not**: Once the state is shared, what is left of talyn is a hider, which is what this
  decision makes it.

### Alternative 3: Leave talyn as it is
- **Pros**: No work.
- **Cons**: Every RI request still lands twice, and the ADR-0077 comment that the two renderers'
  aspect chains are "the same chain" stays a promise rather than something enforced.
- **Why not**: The duplicate is the larger part of what talyn now is.

## Consequences

### Positive
- One RIB handler and one graphics state, so a request is implemented once.
- The ray hider renders an off-centre `ScreenWindow` and any world to camera matrix, which talyn
  refused.
- moya renders spheres under the ray hider. The reyes hider still does not dice them.
- `trace::Scene` no longer holds a `type::camera::Camera`, since nothing casts primary rays from
  it.

### Negative
- talyn's references become moya references under `Hider "raytrace"`. The focus picture moves,
  because talyn started its rays on the near plane and so put its lens there. The ray hider puts
  the lens at the eye, where RI and the reyes hider put it.
- The ray hider takes a primitive's colour, not a varying `"Cs"`, because that is what the traced
  scene holds. The reyes hider interpolates `"Cs"`, so the same scene can differ in colour
  between the two hiders.
- Each primitive given to the ray hider is stored once, in the traced scene. Under the reyes hider
  it is stored twice, as ADR-0077 already accepted.

### Risks
- **An adaptive sample count is a ray hider property.** `PixelVariance` is honoured under
  `"raytrace"` and ignored under `"hidden"`, per
  [ADR-0076](0076-a-pixel-is-a-filtered-set-of-seeded-samples.md). A scene moved between hiders
  can therefore change its noise. The hider is named in the file, so this is never silent.
- **A `Hider` request the renderer does not know is reported and leaves the current hider in
  place**, as an unknown `PixelFilter` does, so a scene written for another renderer still
  renders.
