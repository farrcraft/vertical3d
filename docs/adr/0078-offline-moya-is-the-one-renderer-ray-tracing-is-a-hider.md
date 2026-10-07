# ADR-0078: Offline: moya is the one renderer, ray tracing is a hider

**Status**: accepted
**Date**: 2026-10-04
**Amends**: [ADR-0022](0022-offline-shared-library-with-no-vulkan.md)
**Supersedes**: [ADR-0077](0077-offline-one-shared-ray-tracer.md)
**Documented in**: [offline/RayTracing.md](../offline/RayTracing.md)

## Context

A RenderMan renderer has a hider: the stage that decides what is visible at each sample. moya
has a reyes hider, and needs ray tracing both for shadows and reflections and for primary rays.
A second, separate ray tracing renderer would hold its own copy of the RI graphics state: the
attribute and transform stacks, motion blocks, light handles and the frame aspect and screen
window chain. Two renderers would mean two drivers and two RIB handlers, and every RI request
implemented twice.

## Decision

moya is the only offline renderer, and ray tracing is a hider it selects with
`Hider "raytrace"`; the reyes hider stays the default under RI's name `"hidden"`. Both hiders
read one graphics state and one camera, and share one ray tracer in `api/render/offline/trace`.
That tracer holds its scene in world space, and each primitive carries the lights that were on
when it arrived, so a traced hit is lit as the camera sees the same surface.

## Alternatives

### A separate ray tracing renderer over the shared tracer
- **For**: Each renderer stays small and simple to read, and the tracer is still shared.
- **Against**: Two drivers, two RIB handlers and two render contexts, each with its own RI
  graphics state. A scene has to be pointed at two executables, and the two state machines can
  disagree.
- **Rejected because**: Every RI request would land twice, and the duplicate state would be most
  of the second renderer.

### Move the RI graphics state into `api/render/offline` and keep two renderers
- **For**: Each renderer shrinks to its hider.
- **Against**: Two drivers, two RIB handlers and two render contexts remain over one shared
  state, sized for exactly two consumers.
- **Rejected because**: Once the state is shared, what remains of the second renderer is a
  hider, and this decision makes it a hider.

### Shadow maps in moya instead of a tracer
- **For**: The standard reyes answer, with no second copy of the geometry.
- **Against**: A depth pass per light, a `shadow()` built-in and its own filtering, and still no
  reflections or refraction.
- **Rejected because**: Sharing one tracer is less work and covers reflections as well.

## Consequences

- **Gains**:
  - A request is implemented once, in one graphics state and one RIB handler.
  - A scene renders under either hider by changing one request, so the two can be compared on
    the same file.
  - The ray hider casts primary rays from moya's own camera coordinate systems, so it accepts any
    camera the reyes hider does, including an off-centre `ScreenWindow`.
  - Spheres render under the ray hider, and the reyes hider gains traced shadows and reflections.
- **Costs**:
  - The render context has two paths through `render()`.
  - The ray hider uses a primitive's colour, not a varying `"Cs"`, so the same scene can differ in
    colour between hiders. The reyes hider still does not draw spheres.
  - Under the reyes hider every primitive is stored twice, in a bucket and in the traced scene.
    Its traced rays see the scene at shutter open, so a moving occluder's shadow is sharp.
  - Adaptive sampling (`PixelVariance`) applies only under the ray hider, so a scene moved between
    hiders can change its noise. The hider is named in the file, so this is visible.
  - Brute-force intersection tests every primitive for every ray.
- **Revisit when**: a scene is too slow to trace, which calls for an acceleration structure inside
  `offline::trace`, or a third hider is wanted, which tests whether the `moya::Hider` interface is
  the right shape.
