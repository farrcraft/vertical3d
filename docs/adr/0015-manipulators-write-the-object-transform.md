# ADR-0015: Manipulators — Handles Write The Object Transform, And Are An Overlay Pass

**Date**: 2026-09-02
**Status**: accepted
**Deciders**: Joshua Farr

## Context

The editor draws four viewports of a scene, creates meshes in it and selects them, and
nothing can be moved. [ADR-0013](0013-mesh-is-a-dag-node.md) gave a mesh a `dag::Transform`
so that it has somewhere to be moved to, and [ADR-0014](0014-picking-is-a-cpu-ray-cast.md)
gave a click something to select — and said, in passing, that "the manipulators will be
picked by their own analytic test against the ray rather than by reserving a range of the
same integer". This is the record of what that test turned out to be, and of the two larger
questions the port had to answer first: what a handle writes, and where it is drawn.

Rigel has three manipulators — translate, rotate and scale — described in
[RigelSurvey.md](../audits/completed/RigelSurvey.md) item 6. None of it is portable: each draws itself with
`glBegin`/`glEnd` and a `glLoadName` per axis, and each turns a mouse delta into a transform
by way of `Window::instance()->activeView()`. What comes across is the shape of the thing: a
base holding an axis constraint and a coordinate space, three subclasses, and one live
manipulator at a time chosen by a tool mode on Q, W, E and R.

Two defects in that code are part of the context rather than of the port.
`TranslateManipulator::transform` assigns rather than accumulates in its unconstrained
branch, so a free drag discards its horizontal component; and it then calls
`_selection->translation(t)`, writing one frame's delta as an absolute position, so a drag
snaps the object to the origin and jitters. `ScaleManipulator` and `RotateManipulator` both
get the accumulation right, which is what makes the translate case a defect and not a
convention. `TransformManipulator`'s constructor also leaves `_coordinateSpace`
uninitialised.

## Decision

A manipulator **writes the mesh's `dag::Transform` and never its geometry**, is **drawn at
the object's own origin**, and is **picked by projecting its handles to the screen** and
measuring the cursor's distance from them.

`v3d::editor::Manipulator` is the base: an `Axis` constraint of none, x, y or z — none being
the centre handle rather than the absence of one — a `Space` of global or local, both
initialised, and a `Placement` saying where the handles sit, what they are aligned to, and
how long they are in world units. The length is a constant number of pixels converted
through the view, so a handle keeps its size on screen however far away the object is; it is
measured by unprojecting a one pixel step, which makes the perspective and orthographic
cases one piece of code rather than two.

`TranslateManipulator`, `RotateManipulator` and `ScaleManipulator` are the three, and
`TransformTool` is the third `Tool`: it holds all three, one is in force at a time, and a
fourth mode holds none.

Three things follow from writing the transform:

- **A drag is measured, not read.** `apply()` is given the two cursor positions either side
  of one motion event and adds what it measures — `translate()` by an offset,
  `rotation()` composed on the left, `scale()` multiplied. This is the fix for the rigel
  defect above, and it is also what makes the three consistent with each other.
- **An axis drag is the gesture's component along the projected handle.** The handle is
  projected to the screen, the drag is dotted with it, and the result converted back to
  world units by the handle's own length in both. A handle pointing at the viewer projects
  to nothing and answers zero rather than dividing by it. Rigel instead asked whether the
  axis was more horizontal or more vertical on screen and used that whole component, which
  is the same answer only for an axis aligned view.
- **A rotate ring turns by how far the cursor swept round the origin on screen**, signed by
  whether the axis runs into the screen or out of it. That is the gesture the ring invites,
  and it needs no ray-plane intersection to find where on the ring the cursor is.

The scale manipulator's handles are always the object's own axes whatever the coordinate
space says, because a scale is a vector in those axes: there is no global scale that could
be written back through the transform.

Handles are drawn in a **second pass per viewport, with no depth attachment**, over what the
scene pass left — an overlay is a pass without depth, per
[ADR-0011](0011-lines-are-the-second-primitive.md). A handle shares its plane with the
construction grid's own axis lines, which the depth test then decides between arbitrarily.

## Alternatives Considered

### Alternative 1: Handles write the object transform, drawn at the object origin — **chosen**
- **Pros**: The transform is the only thing a mesh has that describes where it is, and it is
  what [ADR-0013](0013-mesh-is-a-dag-node.md) put there for this. Drawing at the origin
  tells the truth about the pivot: `Transform::matrix()` composes about the object's own
  origin, so a rotate ring drawn anywhere else would turn the object away from the ring the
  user is dragging. It is testable without a window or a device — the whole of a manipulator
  is arithmetic over a camera and a transform. And project persistence has one thing per
  mesh to write out rather than a changed vertex list.
- **Cons**: A component mode selects a face or a vertex and then moves the whole object,
  which is not what a modeller does. Moving a component means editing geometry, and that is
  a modelling operation rather than a placement.
- **Why not**: n/a — chosen.

### Alternative 2: Handles write the geometry, moving the selected vertices
- **Pros**: One mechanism covers object and component modes — an object drag is every vertex
  and a vertex drag is one. It is what the component select masks appear to promise.
- **Cons**: It throws away the transform, so a mesh's placement stops being a thing that can
  be written out, undone or shared between instances, and a rotate would have to bake a
  rotation into every point. Two meshes could never share geometry. And `splitEdge`,
  `extrudeFace` and `splitFace` all rebuild the vertex array, so a drag under way would be
  holding indices into an array a modelling operation could reallocate.
- **Why not**: It collapses placement into geometry, which is exactly the distinction
  ADR-0013 was written to draw.

### Alternative 3: Handles drawn at the centre of the selected components
- **Pros**: What rigel did — the manipulator follows the face or the vertices the user is
  working on, which is where the attention is.
- **Cons**: It is a lie for two of the three manipulators. Rotation and scale pivot at the
  object's origin, so a gizmo drawn at a face's centre would turn the object about a point
  it is not drawn at. Rigel drew it there and applied the transform at the origin anyway.
- **Why not**: The handle has to be where the pivot is, or the drag does not do what the
  drawing says it will.

### Alternative 4: Handles in the scene pass, offset toward the camera to beat the grid
- **Pros**: One pass per viewport rather than two, and no second canvas.
- **Cons**: The offset is a magic number that has to hold for an orthographic camera at any
  zoom and a perspective one at any distance, and it buys a handle that is still occluded by
  any geometry in front of it — which is the case a modeller most needs to grab it in. It is
  also the flag ADR-0011 declined to add.
- **Why not**: The overlay is what the pass model already expresses, and it costs a pass.

## Consequences

### Positive
- The selection can be moved, turned and resized. Verified against a run on 2026-09-02: an x
  handle drag moves a cube in the three views that can show the move and not in the one that
  cannot, a quarter sweep of the z ring turns a cone's apex from +y to -x, an x scale handle
  dragged its own length doubles the cube along x, the centre handle moves in the plane of
  the screen and tumbles about the camera's axes, and the validation layer is silent across
  eight passes a frame.
- The rigel defects are fixed rather than ported: a free drag keeps both of its components,
  every drag accumulates, and the coordinate space is initialised.
- A handle highlights when the cursor is over it, before it is grabbed, because a hover and
  a grab ask the same question of the same code.
- One button now drives three tools. A modifier held means the drag is driving a camera;
  otherwise a handle takes the press if the cursor is on one, and a press no handle took is
  what picks — so a click still selects with a manipulator on screen.
- Ten cases cover the manipulators and the tool, over the existing suite.

### Negative
- A component mode selects a face and then moves the object, because the manipulator writes
  the transform. Moving a component needs a modelling operation, and there is none.
- A ring seen within about eight degrees of edge on is not offered to a click at all. Its
  projection is a line through the middle of the manipulator whose ends land exactly on the
  rim of the ring facing the camera, so it would take clicks meant for that rim. In an axis
  view this is what a user expects — only the ring facing them can be turned — but it is a
  threshold, and a ring just past it is grabbable along a line that crosses everything.
- A rotate pick walks all three rings as the segments they are drawn as, which is 144
  projections per motion event. That is nothing beside the pick it shares a button with, and
  it is per event rather than per frame, but it is not free.
- The handle pass exists for every viewport whether or not anything is selected. It records
  nothing when the canvas is empty, but it is a pass in the frame either way.

### Risks
- Nothing is undoable. A drag writes the transform on every motion event, so an undo model
  will have to decide what one undoable unit is — the whole gesture, not the event — and
  the tool is where the gesture's beginning and end are known. That is a change to
  `TransformTool` rather than to the manipulators.
- The scale manipulator clamps each factor to a small positive minimum so that a drag cannot
  turn the object inside out and leave no handle to drag back. A user who wants a mirrored
  object has no way to ask for one.
- The tool modes take q, w, e and r, which is what gui.xml binds them to. The select masks
  had e, and moved to the digits 5 to 8. Both sets are placeholders for the menus and
  toolbars that are still untranslated.
