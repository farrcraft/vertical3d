# ADR-0014: Editor: pick by CPU ray cast

**Status**: accepted
**Date**: 2026-09-02
**Documented in**: [editor/](../editor/README.md)

## Context

The editor needs to answer "what is under the cursor?" for a mesh, a face, an edge or a vertex.
It draws wireframe lines, and edges and vertices are one pixel wide, so some targets have no
area on screen. A manipulator drag needs a ray and a distance along it as well as a hit.
Everything below the renderer should be testable without a window or a GPU, per
[ADR-0007](0007-ci-render-tests-on-software-vulkan.md). `v3d::type::Camera` can project and
unproject consistently, per [ADR-0012](0012-camera-projection-targets-vulkan-clip-space.md).

## Decision

Picking is a CPU ray cast against the brep, done in the editor rather than in the api. The
`v3d::type::geometry::Ray` is built from the cursor by the camera, and `v3d::editor::Picker`
returns a hit naming the mesh, the kind of part and the part's index. Targets with area (objects
and faces) are tested against the ray; targets without area (vertices and edges) are tested by
screen distance from the cursor.

## Alternatives

### An id-buffer pass read back to the CPU
- **For**: cost does not grow with the scene, and it picks exactly what the user sees.
- **Against**: it needs an integer attachment, a readback buffer and a fence wait, which either
  stalls the frame or answers a frame late. A one-pixel line is hard to hit without drawing it
  wider. None of it is testable without a GPU, and the pick moves out of the event handler into
  the frame loop.
- **Rejected because**: its advantages matter at a scene size the editor does not reach, and a
  manipulator drag needs an answer at once.

### A ray cast for objects, an id buffer for components
- **For**: each method is used where it is strongest.
- **Against**: two mechanisms to write and keep in agreement. Vertices and edges still have no
  area, so the GPU half would need dedicated point and thick-line pipelines just for picking.
- **Rejected because**: it puts the hardest half of the problem on the GPU and doubles the code.

### Screen-space distance for everything
- **For**: one rule instead of two, and no ray.
- **Against**: a face would be picked by clicking near its outline, so the middle of a large face
  could not be picked. Depth order between overlapping faces becomes a guess. A manipulator drag
  still needs a ray.
- **Rejected because**: it gives the wrong answer for targets that have area.

## Consequences

- **Gains**:
  - A pick is answered inside the click's event handler, with no GPU work or readback.
  - Picking is testable without a window or a device.
  - The ray, in `api/type`, is reusable by manipulators, plane drops and future collision
    queries.
- **Costs**:
  - Cost is linear in the triangles of every mesh whose bounding box the ray meets.
  - Faces are split into fans, which is correct only for planar convex faces.
  - Picking tests geometry, not pixels, so a hidden mesh is still hit once anything is drawn
    solid.
  - Users learn two rules: a face is picked by clicking on it, and a vertex by clicking near it.
  - One thing is selected at a time.
- **Revisit when**: scenes are large enough for the linear cost to show, the editor draws solid
  geometry, or selection needs more than one item.
