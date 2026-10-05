# ADR-0012: Camera: projection targets Vulkan clip space

**Status**: amended
**Date**: 2026-09-01
**Amended by**: [ADR-0052](0052-camera-selectable-handedness.md)
**Documented in**: [api/Types.md](../api/Types.md)

## Context

`v3d::type::Camera` is the api's one camera class, and its projection was written against
OpenGL's clip space: y up, and depth from -1 at the near plane to 1 at the far plane. Vulkan,
the only renderer ([ADR-0001](0001-rendering-replace-opengl-with-vulkan.md)), clips with y down and
depth in [0, 1], so a scene drawn through an OpenGL projection comes out mirrored vertically
with the near half of the frustum clipped away. The voxel app already works around this with a
private camera of its own. The editor needs orthographic and perspective views, profile loading,
and `project()` and `unproject()` for picking, all of which the api camera has.

## Decision

`v3d::type::Camera` builds Vulkan clip space for both perspective and orthographic projections:
y points down, and depth runs from 0 at the near plane to 1 at the far plane. `project()` and
`unproject()` measure against the same convention. The camera's basis vectors keep their meaning,
so the projection is the one place the camera's axes meet Vulkan's.

## Alternatives

### Keep OpenGL clip space and correct it in each pass
- **For**: `api/type` is untouched, and one correction matrix in the recorder serves every pass.
- **Against**: `projection()` returns a matrix that cannot be drawn with, and `project()` and
  `unproject()` stay in the old space, so picking applies the correction by hand in reverse. A
  correction matrix cannot change the depth range without changing what the near and far planes
  mean.
- **Rejected because**: it moves the mismatch rather than removing it.

### Each app carries its own camera
- **For**: already working in the voxel app, and an app is free to choose its own convention or a
  reversed depth buffer.
- **Against**: every app reimplements projection maths, and the api camera becomes code that
  only its tests use.
- **Rejected because**: the editor needs exactly what the api camera already has, and another
  copy of the projection maths costs more than fixing the one.

### Vulkan clip space with reversed depth
- **For**: reversed depth (near at 1, far at 0) spends floating-point precision where it is
  needed, and is close to standard practice for large scenes.
- **Against**: it changes the depth clear value and the comparison of every depth-tested
  pipeline, none of which the editor needs.
- **Rejected because**: it is separable. It can be added later to the same matrices and the
  pipelines, when a scene is deep enough to need it.

## Consequences

- **Gains**:
  - Every consumer gets a projection it can draw with directly, orthographic profiles included.
  - `project()` and `unproject()` are inverses of each other, which picking needs.
  - One camera convention matches the one renderer.
- **Costs**:
  - Readers who know `glOrtho` and `glFrustum` will find the signs surprising.
  - Negating y reverses the winding a front face presents, so a culling pipeline that draws
    through this camera declares clockwise front faces.
  - The voxel app's private camera remains, with the same convention and a different interface.
- **Revisit when**: a scene is deep enough that depth precision suffers, which is the case for
  reversed depth.
