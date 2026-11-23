# ADR-0213: Offline: a light's shape is a primitive of the hierarchy

**Status**: accepted
**Date**: 2026-11-09
**Tags**: offline, lighting
**Documented in**: [offline/Lights.md](../../offline/Lights.md)

## Context

A path that samples its next direction from a BSDF can meet a light, and MIS needs that light's
emission and density at the point it met. UsdLux's rect, disk, sphere and cylinder lights each
have a shape that no geometry in the scene holds. A mesh light's shape is a prototype the
hierarchy already holds. Scenes with many lights are expected, since the integrator picks among
them with a light tree. UsdLux names no camera visibility for a light, and renderers differ in
whether a camera ray sees one.

## Decision

Each light with a shape is an instance of a light prototype in the world's hierarchy, and a hit
on it names the light. A light is hidden from a kind of ray by the visibility that instances
already carry. A light's shape is hidden from camera rays unless the scene sets its camera
visibility, and a dome light is seen by them.

## Alternatives

### Each light's shape tested beside the traversal
- **For**: The hierarchy holds no light, so an edit to a light rebuilds nothing. Cycles tests its
  point, spot and area lights in a loop after each trace.
- **Against**: Every ray pays for every light, whatever its distance, and lights need a
  visibility check of their own.
- **Rejected because**: Its cost grows with the count of lights, and the light tree exists for
  scenes with many.

### A hierarchy of the lights' shapes alone
- **For**: A light edit rebuilds only a small hierarchy, and the cost grows slowly with the count
  of lights.
- **Against**: Every ray is traced twice, through the scene and through the lights. On a GPU
  that is a second trace call for each ray.
- **Rejected because**: It doubles the traces to save a rebuild of the hierarchy over instances,
  which is cheap.

### A light's shape seen by camera rays
- **For**: A light in the frame looks as bright as it is, with no setting to find.
- **Against**: A light placed in front of the camera blocks the view. Arnold and RenderMan hide
  a light from the camera by default for that reason.
- **Rejected because**: Lighting a shot places lights in front of the camera far more often than
  it shows one.

## Consequences

- **Gains**:
  - One traversal finds geometry and lights, and its cost grows with the hierarchy alone.
  - Sphere, disk and cylinder lights are met by the quadric tests that geometry uses, and a GPU
    back end intersects them as the custom primitives it already needs.
  - Light linking, shadow linking and visibility act on lights as they act on instances.
- **Costs**:
  - An edit to a light rebuilds the hierarchy over the world's instances.
  - A light's shape lies in the scene's hierarchy, so a shadow ray towards a light has to end
    before its shape.
  - A light hidden from camera rays shows no shape in a render until the scene sets it.
- **Revisit when**: a scene's lights change on every frame in an interactive render and the
  rebuild of the world's hierarchy shows in its cost.
