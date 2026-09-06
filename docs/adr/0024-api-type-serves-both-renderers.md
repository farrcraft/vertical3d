# ADR-0024: Shared Types — `api/type` Serves Both Renderers, And A Convention Is A Parameter Rather Than A Fork

**Date**: 2026-09-04
**Status**: accepted
**Deciders**: Joshua Farr

## Context

[ADR-0012](0012-camera-builds-vulkan-clip-space.md) made `v3d::type::Camera` build Vulkan clip
space — y down, depth in [0, 1]. Every consumer it had was realtime, and the convention was
chosen for the swapchain.

The offline renderers are consumers now. `moya` links `v3dlib_type` already and its
`RenderContext` half-implements the reyes screen and raster spaces, which are specified
independently of any graphics API. `talyn` needs a primary ray through a pixel, which is
`Camera::ray()` and `type::Ray::intersects` — both written for the editor's picker
([ADR-0014](0014-picking-is-a-cpu-ray-cast.md)) and both tested.

So the roadmap's question is whether the offline renderers use the api types at all. Using them
unchanged means inheriting a clip convention chosen for a swapchain neither renderer touches;
not using them means a third and fourth camera in a tree where ADR-0012 already records the
second as a duplication to collapse.

## Decision

`api/type` is the shared geometric vocabulary for both the realtime and the offline renderers.
Where a type carries a convention only one of them needs, **the convention becomes a parameter
of the type rather than grounds for a second copy** — `Camera` is told which clip space to
build. Vulkan clip space stays the default and stays what the realtime stack gets; ADR-0012 is
narrowed, not reversed.

## Alternatives Considered

### Alternative 1: One set of types, with the convention selectable — **chosen**
- **Pros**: One implementation of the projection, unprojection and intersection maths. The
  offline renderers keep `api/type` honest: they link no Vulkan, so anything realtime that
  leaks into it fails to build for them before it reaches CI.
- **Cons**: `Camera` grows a mode, and a mode is a thing to get wrong. Its tests double for the
  paths that differ.
- **Why not**: n/a — chosen.

### Alternative 2: The offline renderers carry their own camera and geometry types
- **Pros**: The reyes screen and raster spaces are well specified and moya half-implements them
  already. Each renderer stays free to change its own.
- **Cons**: A third camera class, and a fourth once talyn needs one — ADR-0012 rejected this
  same argument for voxel. talyn would also re-derive the ray/AABBox and ray/triangle
  intersections that `type::Ray` already has under test.
- **Why not**: The duplication is the projection maths, which is the part that is hard to get
  right and easy to get subtly wrong.

### Alternative 3: `api/type` stays Vulkan-shaped and each offline renderer corrects at its boundary
- **Pros**: Nothing in `api/type` changes, and the correction is one matrix.
- **Cons**: This is the alternative ADR-0012 rejected at the pass, and it is worse offline.
  `projection()` would return a matrix a consumer may not use, and `project()`/`unproject()`
  would stay in the wrong space — in a reyes renderer raster space *is* the working space, not
  an output of it.
- **Why not**: It moves the mismatch into the renderer that can least afford it.

### Alternative 4: A second camera class in `api/type` for offline use
- **Pros**: Neither class has a mode, and each reads as one convention throughout.
- **Cons**: Two classes with the same interface. Pan, tilt, dolly, truck, zoom, the arcball
  rotation and profile loading are identical in both, and the difference is a handful of signs
  in one matrix.
- **Why not**: The shared surface is far larger than the difference.

## Consequences

### Positive
- talyn's phase 1 is a scene class, a loop and a shading constant: the camera, the ray and the
  intersection all exist and are tested.
- `api/type` acquires the consumers that prove it is API-agnostic. It was written as generic
  geometry and has only ever been used by one backend.
- A convention that is a parameter can be read off the call site. A convention baked into a
  class can only be recovered by reading its matrix.

### Negative
- ADR-0012's simplicity is gone: a reader can no longer answer "what space does this camera
  build" without knowing how it was constructed.
- Some of `api/type` will need widening that only the offline side uses — the reyes spaces are
  not a subset of what `Camera` builds today. Each such widening is a cost paid by a library
  every app links.

### Risks
- The default hides the parameter. Offline code that forgets to set it gets Vulkan clip space,
  and the image comes out vertically mirrored with the far half of the scene clipped — which
  looks like a renderer bug, not a configuration one. Mitigation is a phase 1 test that renders
  a known triangle and compares against a reference, which catches exactly this.
- If reyes turns out to need spaces `Camera` cannot express, the answer is that
  `moya::RenderContext` keeps its own raster space and uses `Camera` for the view and projection
  only. The decision is that the class is not forked, not that every space runs through it.
