# Picking and manipulators

How a click finds an object or one of its parts, and how a manipulator's handles move the
selection.

## Picking

A pick answers "what is under the cursor?" with a CPU ray cast against the meshes. No GPU work
or readback is involved, so a pick is answered inside the click's event handler.

`v3d::editor::Picker` builds a `v3d::type::geometry::Ray` with `Camera::ray()`, which
unprojects the cursor at the near and far depths. It returns a `Picker::Hit`: whether anything
was hit, the mesh's `Node` id, the kind of thing hit, and the component's index.

What a click tests depends on the select mask:

| Mask | Test | Winner |
|---|---|---|
| Object, Face | The ray against a triangle fan over each face | Nearest along the ray |
| Vertex, Edge | Each vertex or edge projected to the screen; within 5 pixels of the cursor counts | Nearest to the camera |

Vertices and edges are drawn one pixel wide and have no area, so they are found by screen
distance instead of by the ray.

Rules the picker follows:

- The ray is moved into each mesh's own space by the inverse of the mesh's matrix. The
  geometry is not moved. `Ray::transformed()` does not renormalise the direction, so distances
  found in different meshes stay comparable.
- A mesh whose bounding box the ray misses is skipped.
- A face is split into a fan from its first vertex. This is only correct for planar convex
  faces.
- Picking tests geometry, not pixels. A mesh hidden behind another is still hit if the ray
  reaches it.
- The view's camera matrices are rebuilt before the cast, so a pick does not depend on a frame
  having been drawn since the camera moved.

**Select masks.** `SelectMask` is one of Object, Vertex, Edge or Face, and `SelectTool` holds
it. Keys 5 to 8, the Select Mask menu and the select-mask toolbar all set it. Changing the mask
clears the component selection.

**What a click does.** `SelectTool` picks on the press, not the release.

- An object must be selected before any of its parts can be. Component masks test only
  selected meshes.
- In Object mode, a click on a mesh selects it and nothing else. A click on nothing clears the
  whole selection.
- In a component mode, a click on a part toggles it and clears the other parts of the same
  kind. A click on nothing clears the components and keeps the object selected.
- One thing is selected at a time. There is no rubber band and no shift-click.

**One button, three tools.** `Controller::drag()` routes the left button in this order:

1. The ui is offered the press first. If it takes it, the matching release goes to the ui too.
2. If a camera modifier (Alt, Ctrl or Shift) is held, the drag drives the camera.
3. Otherwise, if the cursor is on a manipulator handle, `TransformTool` takes the press.
4. Otherwise `SelectTool` picks.

Background: [ADR-0014](../adr/0014-editor-pick-by-cpu-ray-cast.md)

## Manipulators

A manipulator is the set of handles that moves, turns or resizes the selected mesh.
`TranslateManipulator`, `RotateManipulator` and `ScaleManipulator` derive from `Manipulator`.
`TransformTool` holds all three and has four modes: none (Select), translate, rotate and scale.
One mode is active at a time, and the Select mode shows no handles.

**A manipulator writes the mesh's transform and never its vertices.** It acts on the whole
object even when a face or a vertex is what is selected. Moving a part of a mesh needs a
modelling operation, and there is none yet.

**Where the handles are.** Handles are drawn at the mesh's own origin, because that is where
its transform pivots. Their length is a fixed number of pixels (90), converted to world units
through the view. A handle therefore keeps its on-screen size at any distance. Handles are drawn
in the handle pass, which has no depth test.

**Grabbing.** A handle is picked by projecting it to the screen and measuring the cursor's
distance from its segments; within 6 pixels counts. The handle under the cursor is highlighted
before it is grabbed. `Manipulator::Axis` is X, Y, Z or None; None is the centre handle.

**Dragging.** `apply()` receives the cursor positions before and after one motion event. It
measures the change and adds it to the transform. It never writes an absolute value.

| Manipulator | Axis handle | Centre handle |
|---|---|---|
| Translate | Moves along the axis by the drag's component along the handle's projection on screen | Moves in the plane of the screen |
| Rotate | Turns by the angle the cursor swept around the origin on screen; the sign depends on whether the axis points into or out of the screen | Tumbles about the camera's axes |
| Scale | Multiplies that axis's scale by the drag along the handle, relative to the handle's length | Scales all three axes; dragging right or up grows the object |

More rules:

- A handle pointing straight at the viewer projects to a point and contributes nothing.
- A rotate ring that is nearly edge-on to the view cannot be grabbed. In an axis view, only the
  ring facing the viewer can be turned.
- The scale manipulator always uses the object's own axes, whatever the coordinate space. Each
  scale factor is kept at 0.01 or above, so a drag cannot turn an object inside out. Mirroring is
  not possible.
- `Manipulator::Space` is Global or Local and starts as Global. No command changes it yet.
- Changing the mode ends any drag under way, and the change made so far is recorded for undo.

Background: [ADR-0015](../adr/0015-editor-manipulators-edit-the-object-transform.md)
