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

- An object must be selected before any of its parts can be. Component masks test every
  selected mesh, so a component selection can span several objects.
- In Object mode, a click on a mesh selects it and nothing else. A click on nothing clears the
  whole selection.
- In a component mode, a click on a part selects it and clears every other part of every mesh.
  A click on a part that is already selected clears it along with the rest. A click on nothing
  clears the components and keeps the objects selected.
- While Left Shift is held (`select::add`, which `SelectTool::adding()` follows), a click
  toggles the object or part under the cursor and leaves the rest of the selection alone. A
  click on nothing changes nothing.

**A rubber band.** A press on nothing is applied as a click at once, and then held. If the
cursor moves more than 4 pixels along either axis before the release, the drag is a rubber band,
and the release selects everything inside the rectangle under the select mask. A smaller
movement is a click. A band adds to what the click left, so it replaces the selection, or adds
to it while Left Shift is held. A band never deselects.

`Picker::inside()` decides what a rectangle holds, through the same screen projection a vertex
or edge pick uses:

| Mask | Inside when |
|---|---|
| Object | Every corner of its bounding box projects inside |
| Vertex | It projects inside |
| Edge | Both its ends project inside |
| Face | Every vertex of its loop projects inside |

A band selects hidden parts, as a click does, because it tests geometry and not pixels. A point
behind the camera is never inside. The band is measured in the view where it started, and is
drawn in the `ui` pass as a one-pixel outline in window pixels.

**One button, three tools.** `Controller::drag()` routes the left button in this order:

1. The ui is offered the press first. If it takes it, the matching release goes to the ui too.
2. If a camera modifier (Alt or Ctrl) is held, the drag drives the camera. A release
   with a modifier held still goes to `TransformTool` and `SelectTool` as well, so a handle drag
   or a rubber band that a modifier pressed mid-way ends.
3. Otherwise, if the cursor is on a manipulator handle, `TransformTool` takes the press.
4. Otherwise `SelectTool` picks.

The middle button drives the camera alone. Holding it turns the camera with the arcball, and
letting it go ends the drag, whatever the left button is doing.

Background: [ADR-0014](../adr/0014-editor-pick-by-cpu-ray-cast.md)

## Manipulators

A manipulator is the set of handles that moves, turns or resizes the selected meshes.
`TranslateManipulator`, `RotateManipulator` and `ScaleManipulator` derive from `Manipulator`.
`TransformTool` holds all three and has four modes: none (Select), translate, rotate and scale.
One mode is active at a time, and the Select mode shows no handles.

**A manipulator writes each selected mesh's transform and never its vertices.** It acts on
whole objects even when a face or a vertex is what is selected. Moving a part of a mesh needs a
modelling operation, and there is none yet.

**Where the handles are.** Handles are drawn at the pivot, the mean of the selected objects'
origins. For one object that is its own origin, where its transform pivots. Their length is a
fixed number of pixels (90), converted to world units through the view. A handle therefore keeps
its on-screen size at any distance. Handles are drawn in the handle pass, which has no depth
test.

**Moving a set.** Every selected object moves in one gesture, about the one pivot:

- A translate moves every object by the same offset.
- A rotate turns each object, and carries its origin round the pivot by the same turn.
- A scale multiplies each object's own scale, and scales its distance from the pivot: along the
  handle's direction for an axis handle, and all three ways for the centre handle.

A rotate or a scale of several objects therefore changes each one's translation as well. A drag
keeps the set it grabbed, so a selection change mid-gesture does not change what moves.

**Grabbing.** A handle is picked by projecting it to the screen and measuring the cursor's
distance from its segments; within 6 pixels counts. The handle under the cursor is highlighted
before it is grabbed. `Manipulator::Axis` is X, Y, Z or None; None is the centre handle.

**Dragging.** `apply()` receives the cursor positions before and after one motion event. It
measures the change and adds it to the transform. It never writes an absolute value.

| Manipulator | Axis handle | Centre handle |
|---|---|---|
| Translate | Moves along the axis by the drag's component along the handle's projection on screen | Moves in the plane of the screen |
| Rotate | Turns by the angle the cursor swept around the pivot on screen; the sign depends on whether the axis points into or out of the screen | Tumbles about the camera's axes |
| Scale | Multiplies that axis's scale by the drag along the handle, relative to the handle's length | Scales all three axes; dragging right or up grows the object |

More rules:

- A handle pointing straight at the viewer projects to a point and contributes nothing.
- A rotate ring that is nearly edge-on to the view cannot be grabbed. In an axis view, only the
  ring facing the viewer can be turned.
- The scale manipulator always uses the object's own axes, whatever the coordinate space. For a
  set, the handles follow the first selected object's axes, and each object scales along its
  own. Each scale factor is kept at 0.01 or above, and so is the factor a distance from the
  pivot scales by, so a drag cannot turn an object or a set inside out. Mirroring is not
  possible.
- In local space, the handles line up with the first selected object's axes.
- `Manipulator::Space` is Global or Local and starts as Global. No command changes it yet.
- Changing the mode ends any drag under way, and the change made so far is recorded for undo.

Background: [ADR-0015](../adr/0015-editor-manipulators-edit-the-object-transform.md),
[ADR-0096](../adr/0096-editor-handles-pivot-at-the-mean-of-the-selection.md)
