# ADR-0014: Picking — A CPU Ray Cast Against The Brep, With Screen Space Proximity For Components

**Date**: 2026-09-02
**Status**: accepted
**Deciders**: Joshua Farr

## Context

The editor draws four viewports of a scene and creates meshes in it, and nothing can be
selected. [ADR-0013](0013-mesh-is-a-dag-node.md) landed the two things selection needed
underneath it — a mesh has a `dag::Node` id, and all four of `Vertex`, `HalfEdge`, `Face`
and `BRep` carry a `selected()` flag — and left the mechanism open. Everything phase 6 has
left is downstream of it: the manipulators act on a selection, the modelling operations act
on a selected component, and project persistence writes out what a selection names.

Rigel's `ViewPort::selection_hit_test` is `glRenderMode(GL_SELECT)` with a name stack and a
hand-rolled `gluPickMatrix`, described in [RigelSurvey.md](../audits/completed/RigelSurvey.md) item 3.
`GL_SELECT` was deprecated in GL 3 and has no Vulkan equivalent, so the mechanism has to be
rebuilt whatever else happens. What it wrapped is worth keeping: an object must be selected
before any of its components may be, a click that hits nothing deselects, clicking a
component twice toggles it, and one component of a kind is selected at a time.

Two things constrain the choice. The editor draws **lines and nothing else** —
[ADR-0011](0011-lines-are-the-second-primitive.md) gave the api a line primitive and there is
no triangle one, so there is no pipeline that could rasterise a mesh into an id buffer
without one being written first. And a wireframe's edges and vertices are drawn one pixel
wide, so whatever answers a click has to answer for a target with no area.

`v3d::type::Camera` is ready for the cast half: `project()` and `unproject()` became
inverses of each other on 2026-09-01 with
[ADR-0012](0012-camera-builds-vulkan-clip-space.md), which is exactly the round trip a ray
cast is built on.

## Decision

Picking is a **CPU ray cast against the brep**, in the editor rather than in the api.

`v3d::type::Ray` is an origin, a direction, and intersection against an `AABBox` and against
a triangle; `Camera::ray()` builds one from a screen point by unprojecting the near and far
depths of the pixel, which gives a perspective camera a fan and an orthographic one a set of
parallels without either being a case of its own. `v3d::editor::Picker` walks a scene with
it and returns a `Hit`: whether anything was found, the `dag::Node` id of the mesh, the kind
of thing, and the index of the component within that mesh.

What it casts against depends on what is being picked, because they are not the same shape
of question:

- **An object or a face has area on the screen**, and is answered by the ray meeting a
  triangle of a fan over the face's half edge loop. Nearest along the ray wins.
- **A vertex or an edge does not**, and is answered by screen space proximity: the geometry
  is projected and anything drawn within a few pixels of the cursor is a candidate. Among
  candidates, nearest to the camera wins, which is what rigel's depth sorted hit buffer did.

The ray is moved into each mesh's own space by the inverse of its matrix rather than the
geometry being moved into the world, so a pick costs one matrix inverse per mesh instead of a
transform per vertex. `Ray::transformed()` deliberately does not renormalise, which is what
makes a distance found in a scaled mesh's space comparable with one found in another's.

Rigel's integer name-space encoding — objects below 16,777,216, the next 32 for manipulator
axes, everything above a component index — is **not** ported. It existed because a GL name
stack carries one `GLuint`; a `Hit` is a struct and can say what kind of thing it holds. The
manipulators will be picked by their own analytic test against the ray rather than by
reserving a range of the same integer.

The select mask is the editor's, not the api's: `v3d::editor::SelectMask` is one of object,
vertex, edge or face, held by `SelectTool` and bound to keys because there are no menus yet.
Rigel's gui.xml also lists curve, mesh, light, camera and handle masks; none of them has a
node type to select, so they are left out rather than stubbed.

## Alternatives Considered

### Alternative 1: A CPU ray cast against the brep — **chosen**
- **Pros**: No GPU work, no readback, and no pipeline stall waiting for one — a pick is
  answered inside the click's own event handler rather than a frame later. It is testable
  without a window or a device, which is what [ADR-0007](0007-ci-rendering-tests.md) says
  everything below the recorder has to be until CI can render. It needs no triangle
  primitive, which the tree does not have. It gives a distance and a point along the ray,
  which is what a manipulator drag needs next and what an id buffer would not provide. And
  it answers for a vertex and an edge, which have no area to rasterise.
- **Cons**: Cost is linear in the scene's triangles per click, with only a bounding box to
  reject a mesh early. A face fan assumes planar convex faces. And it picks the geometry
  rather than the picture: a mesh hidden behind another is still hit if the ray reaches it,
  so what is picked and what is drawn can disagree once there is anything but wireframe.
- **Why not**: n/a — chosen.

### Alternative 2: An id buffer pass, read back to the CPU
- **Pros**: Cost is independent of scene complexity, and it is exact by construction —
  whatever the pick reads is what the user was looking at, including through any shader that
  moved a vertex. It is what a large scene eventually wants.
- **Cons**: There is no triangle primitive to rasterise ids with, so it starts by writing
  one. It needs an `R32_UINT` attachment, a host visible readback buffer, and a fence wait —
  which either stalls the frame or answers the click one frame late, and a manipulator drag
  wants an answer now. It cannot answer for a one pixel line without drawing that line fat
  enough to hit, and `wideLines` is an optional device feature this device is not asked for.
  None of it is testable without a GPU. And a pick would have to happen inside the frame
  loop rather than in the event handler, which is a change to how the editor is shaped for a
  scene of a few thousand triangles.
- **Why not**: It is the right answer at a scale the editor is nowhere near, and it is
  blocked on the primitive that phase 6 does not need for anything else.

### Alternative 3: Ray cast for objects, id buffer for components
- **Pros**: Each mechanism used where it is strongest — the ray for the coarse question and
  the rasteriser for the fine one, where a component's index is exactly what an id buffer
  writes.
- **Cons**: Two mechanisms to write, two to keep agreeing about what is selectable, and the
  fine one is the half that is hardest to do on the GPU: a vertex and an edge still have no
  area, so it would need point and thick line pipelines drawing handles purely to be picked.
  Everything against alternative 2 applies to it, for the harder half only.
- **Why not**: It doubles the mechanism to make the worse half of the problem GPU shaped.

### Alternative 4: Screen space proximity for everything, including objects and faces
- **Pros**: One rule rather than two, and it is what the vertex and edge cases have to do
  anyway. The picker would need no ray at all.
- **Cons**: A face is picked by clicking anywhere on it, not by clicking near its boundary —
  proximity to the projected outline would make the middle of a large face unpickable and
  the space just outside a small one pickable. Depth ordering between two faces under the
  cursor stops being a distance along a ray and becomes a guess. And a manipulator drag still
  needs a ray, so it would be written later anyway.
- **Why not**: It answers the wrong question for the two kinds that have area.

## Consequences

### Positive
- A mesh can be selected, and so can one of its faces, edges or vertices. Verified against a
  run on 2026-09-02: a click selects a cube, a click off it deselects, a second click on the
  same face toggles it back off, the mask keys switch what a click looks for, and the
  validation layer is silent throughout.
- `v3d::type::Ray` is in `api/type` rather than in the editor, because ray against box and
  ray against triangle are geometry rather than editor policy. The manipulators, the
  construction plane's ray-plane drop and any future collision query get it for free.
- Everything is testable without a window or a device. Twelve cases cover the picker and the
  tool, and four more the ray, over the existing suites.
- The wireframe shows what is selected: a selected object recolours, a selected face draws
  its boundary in the component colour, and a selected vertex draws a small box — there being
  no filled primitive to shade either with.
- One button drives two tools. A drag with a camera modifier held moves the camera and a bare
  click picks, which is how rigel divided them.

### Negative
- A pick walks every triangle of every mesh whose bounding box the ray meets. That is
  nothing at four primitives and will be something at a real model; a spatial index is the
  answer when it is, and nothing about this decision blocks one.
- `Picker::surface` triangulates a face as a fan from its first vertex, which is only correct
  for planar convex faces. The four primitives produce nothing else and no modelling
  operation in the tree does either, but `splitFace` and `extrudeFace` are one bad
  argument away from a face that a fan gets wrong.
- Picking sees geometry, not pixels. A mesh occluded by another is still hit if the ray
  reaches it — invisible in wireframe, where everything is see-through anyway, and wrong the
  day anything is drawn solid.
- The vertex and edge tolerance is a constant in pixels rather than something the user can
  set, and there is no ui to set it from.

### Risks
- Two rules mean two behaviours a user has to learn: a face is picked by clicking on it and a
  vertex by clicking near it. That is what every modeller does, but it is a rule the code
  does not enforce anywhere — a fifth mask added without thought would have to pick one.
- Only one thing is selected at a time. Rigel had the same limit and the manipulators were
  written against it, so a rubber band or a shift-click is a change to the selection model
  and not just to the tool.
