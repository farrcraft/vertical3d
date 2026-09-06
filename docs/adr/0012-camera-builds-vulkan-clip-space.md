# ADR-0012: Camera Convention — `v3d::type::Camera` Builds Vulkan Clip Space

**Date**: 2026-09-01
**Status**: accepted
**Deciders**: Joshua Farr

## Context

`v3d::type::Camera` is the repository's one camera class. Its `createProjection()` was
written against `glFrustum` and `glOrtho`, whose signatures the comments around it quoted, and
so it produced OpenGL clip space: y pointing up and depth running from -1 at the near plane to
1 at the far one.

Nothing had ever drawn through it. [ADR-0001](0001-vulkan-replaces-opengl.md) deleted OpenGL
from the tree, and Vulkan clips y downward and depth against [0, 1]. A scene drawn through
an OpenGL projection under Vulkan comes out vertically mirrored with everything in the near
half of the frustum clipped away. Nothing could observe that while the class had no
consumer.

Voxel hit this first and answered it privately: `voxel/src/engine/Camera.cxx` is a second
camera class of the app's own, whose `perspective()` negates the vertical scale and maps
depth to [0, 1] with a comment explaining both. The convention was therefore already settled in
practice, but not anywhere the api could see it.

Phase 6's editor is what forces the question. It draws four viewports of one scene through
four `v3d::type::Camera`s — six of the eight profiles in rigel's `gui.xml` are orthographic —
and it reads back through `project()` and `unproject()` for picking. Carrying a private
camera the way voxel does would leave the api camera with no consumer at all.

## Decision

`v3d::type::Camera` builds **Vulkan clip space**: y points down, and depth runs from zero at
the near plane to one at the far plane. Both the perspective and the orthographic paths do,
and `project()` and `unproject()` measure against the same convention — y downward from the
top of the viewport, depth already in [0, 1].

The camera looks along its own `direction`, which the profile documents as +z of the basis
its three normals define, so a point in front of the camera has a positive view z and w is
that z rather than its negation. The projection had it the other way round, which put the
default profile's own subject behind the camera.

The camera's axes are otherwise unchanged. `eye`, `up`, `right` and `direction` are the right
handed, y up basis they always were, and a profile loaded out of `gui.xml` still means what
it meant. The projection is the one place the two conventions meet.

## Alternatives Considered

### Alternative 1: The camera builds Vulkan clip space — **chosen**
- **Pros**: One camera, one convention, matching the one renderer. Every future consumer gets
  a projection it can draw with rather than one it has to correct. `project()` and
  `unproject()` become inverses of each other, which picking needs.
- **Cons**: A silent behavioural change to a class whose matrices some future reader may
  expect to match `glOrtho`. The negated y also reverses the winding a front face presents,
  so a pipeline that culls has to call its front faces clockwise.
- **Why not**: n/a — chosen.

### Alternative 2: Leave the camera in OpenGL space and correct it at the pass
- **Pros**: `api/type` stays untouched, and the correction is one matrix multiply in the
  recorder that every pass gets for free.
- **Cons**: The camera's `projection()` would then be a matrix nobody may draw with, which is
  a trap rather than an interface. `project()` and `unproject()` would still be in the old
  space, so picking would need the correction applied by hand in the opposite direction. And
  a correction matrix cannot fix the depth range without also changing what the near and far
  planes mean, so the precision distribution stays the OpenGL one.
- **Why not**: It moves the mismatch rather than removing it, and leaves the class's most
  obvious method returning something unusable.

### Alternative 3: Every app carries its own camera, as voxel does
- **Pros**: Already working, and it is what the tree does today. An app that wants a different
  convention or a reversed-depth buffer is free to have one.
- **Cons**: The editor needs orthographic projections, `unproject` for picking, arcball
  rotation and profile loading — all of which `v3d::type::Camera` has and voxel's does not.
  Duplicating it is a second implementation of the projection maths, and `api/type::Camera`
  becomes dead code that only the tests exercise.
- **Why not**: The phase 6 editor is the app that most needs what the api camera already
  has, and a fourth implementation of the projection maths is a worse cost than the fix.

### Alternative 4: Vulkan clip space and a reversed depth buffer
- **Pros**: Reversed depth — near at one, far at zero — spends floating point precision where
  it is needed and is close to standard practice for a large scene.
- **Cons**: It is a change to the depth clear value, the comparison op of every depth-testing
  pipeline, and `vulkan::Recorder`, none of which the editor's grid needs. `Recorder` clears
  depth to one today and says in a comment that it does so because the projections here are
  not reversed.
- **Why not**: Out of scope for this decision and separable from it. Nothing forecloses it —
  it is a change to the same two matrices plus the pipelines, and it can be made when a scene
  is deep enough to suffer without it.

## Consequences

### Positive
- The editor's four viewports draw through the api camera directly, orthographic profiles
  included, with no per-app correction.
- `project()` and `unproject()` round trip. They did not before: `project` measured y downward
  and `unproject` measured it upward, so only points on the horizontal centre line survived
  the trip — a defect the test suite had recorded as expected behaviour. Picking depends on
  the round trip holding.
- `project()` now performs the perspective divide, which it never did. It was correct only
  for orthographic cameras, where w is one.

### Negative
- A reader who knows the `glOrtho` and `glFrustum` forms will find the signs surprising. The
  comments around the matrices give the derivation of what is built rather than of what those
  two functions build.
- Two camera classes remain in the tree with the same convention and different interfaces —
  voxel's and the api's. That is a duplication to collapse later, not a decision made here.
- The convention is no longer the class's only one.
  [ADR-0024](0024-api-type-serves-both-renderers.md) makes it a parameter so the offline
  renderers can use the same camera; Vulkan clip space stays the default and stays what the
  realtime stack gets.

### Risks
- Anything that stored a projection built by the old code and compares against a new one will
  disagree. Nothing does: the class had no consumer outside its own tests.
- The winding reversal is latent. The line pipeline culls nothing, so the editor does not feel
  it, but the first pipeline to cull faces while drawing through a `v3d::type::Camera` has to
  declare clockwise front faces, exactly as voxel's does.
