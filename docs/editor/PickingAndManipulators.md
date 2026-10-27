# Picking and manipulators

How a click finds an object or one of its parts, and how a manipulator's handles move the
selection.

## Picking

A pick answers "what is under the cursor?" with a CPU ray cast against the meshes. No GPU work
or readback is involved, so a pick is answered inside the click's event handler. A camera or a
light is picked by its glyph, the lines it is drawn with.

`v3d::editor::Picker` builds a `v3d::type::geometry::Ray` with `Camera::ray()`, which
unprojects the cursor at the near and far depths. It returns a `Picker::Hit`: whether anything
was hit, the item's id, the kind of thing hit, and the component's index.

What a click tests depends on the select mask:

| Mask | Test | Winner |
|---|---|---|
| Object, Mesh, Face, on a mesh | The ray against a triangle fan over each face | Nearest along the ray |
| Vertex, Edge | Each vertex or edge projected to the screen; within 5 pixels of the cursor counts | Nearest to the camera |
| Vertex, Handle, on a curve | Each control point, or each Bezier handle, projected to the screen; within 5 pixels counts | Nearest to the camera |
| Object, Light, Camera, on a light or a camera | Each segment of its glyph projected to the screen; within 5 pixels counts | Nearest to the camera |

Vertices, edges and glyphs are drawn one pixel wide and have no area, so they are found by
screen distance instead of by the ray. A ray distance and a screen depth are compared as they
are, so under the object mask a mesh and a glyph at one point compete on a rough footing.

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

**Select masks.** `SelectMask` is one of Object, Mesh, Light, Camera, Curve, Vertex, Edge, Face
or Handle, and `SelectTool` holds it. Keys 5 to 8 set Object, Vertex, Edge and Face, the
select-mask toolbar sets the same four, and the Select Mask menu sets all nine. Changing the mask
clears the component selection.

- **Object, Mesh, Light, Camera and Curve select whole items.** Object takes every kind, and each
  of the others takes only its own kind. A mask that does not take a kind never hits it, so a light
  behind a mesh can be clicked under the light mask.
- **Vertex, Edge and Face select the parts of meshes.** Vertex also selects the control points of
  curves, of every type. Edge and Face select nothing on a curve.
- **Handle selects the handles of Bezier curves**, and nothing else. A Bezier point's left and
  right handles are selected apart from each other and from the point.

**What a curve shows.** While the mask is Vertex or Handle, a selected curve item draws a small
box at each control point, and the lines between a NURBS curve's points. While the view also
shows handles, it draws a line from each Bezier point to each of its handles, with a box at the
handle's end. `view::show::handle` therefore shows a curve's handles and a manipulator's together.
A selected point or handle is drawn in the component colour.

**What a click does.** `SelectTool` picks on the press, not the release.

- An object must be selected before any of its parts can be. Component masks test every
  selected mesh and curve, so a component selection can span several objects.
- Under a mask that selects whole items, a click on an item it takes selects that item and
  nothing else. A click on nothing clears the whole selection.
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
| Object, Mesh, on a mesh | Every corner of its bounding box projects inside |
| Object, Light, Camera, on a light or a camera | Every point of its glyph projects inside |
| Vertex | It projects inside |
| Vertex, Handle, on a curve | The control point or the handle projects inside |
| Edge | Both its ends project inside |
| Face | Every vertex of its loop projects inside |

A band selects hidden parts, as a click does, because it tests geometry and not pixels. A point
behind the camera is never inside. The band is measured in the view where it started, and is
drawn in the `ui` pass as a one-pixel outline in window pixels.

**One button, three tools.** `Pointer` routes the left button in this order:

1. The ui is offered the press first. If it takes it, the matching release goes to the ui too.
2. A press on a divider between views moves the divider, and no tool sees it.
3. Otherwise `ToolRouter` offers the press to the tools in turn, and the first that takes it has
   the gesture until the release:
   - `CameraControlTool` takes it while a camera move is in force, held by a modifier (Alt or
     Ctrl) or latched from the Camera Manipulators menu.
   - `TransformTool` takes it when the cursor is on a manipulator handle.
   - `SelectTool` takes any press, and picks.

A press is one tool's gesture from start to end. A modifier pressed in the middle of a handle
drag or a rubber band does not start a camera move, and the release ends the gesture it began.

The middle button drives the camera alone. Holding it turns the camera with the arcball, and
letting it go ends the drag, whatever the left button is doing.

### Driving a camera

`CameraControlTool` drives the camera of the view a drag starts in. Its move is chosen two ways:

- **Held.** `view::camera::dolly` (Left Alt), `view::camera::truck` (Left Ctrl) and
  `view::camera::pan` (the middle button) last while they are held. `view::camera::zoom` is a
  held command with no binding.
- **Latched.** The Camera Manipulators menu's `camera::truck`, `dolly`, `zoom`, `pan` and `pitch`
  latch a move, which lasts until the same item is chosen again or a transform mode is chosen.
  The menu checks the latched move.

A held move wins while it is held, and the latched one comes back when it is let go. Releasing
a held move ends that move only.

| Move | Perspective view | Orthographic view |
|---|---|---|
| Zoom | A drag right narrows the angle of view, from 1 to 170 degrees | A drag right shows less |
| Dolly | A drag right moves the eye forward along the view | As zoom: the picture is the same from anywhere along the view |
| Truck | Moves the eye sideways and up or down | The same, scaled to the view |
| Pan | Turns the camera with an arcball | Nothing |
| Pitch | A drag up turns the camera to look up | Nothing |

An orthographic view is not turned, because the fixed views are orthographic and a turn would
leave them showing something else.

Background: [ADR-0014](../adr/0014-editor-pick-by-cpu-ray-cast.md)

## Component selection

**An item holds whether it is selected as a whole, and the scene holds which of its parts are.**
`Scene::components()` is a `Components`: for each mesh item, by its `id()`, the indices of its
selected vertices, half edges and faces in the item's geometry, and for each curve item the
numbers of its selected control points and handles. Neither `api/brep` nor `api/curve` holds a
selection.

- **A curve's parts are numbered across all of its curves.** Control points are numbered from
  zero, the first curve's first, so a point's number is the count of points in the curves before
  it plus its place in its own curve. A point has two handle numbers, twice its own for its left
  handle and one more for its right, and only those of a Bezier point name a handle.
  `CurveParts.h` converts between a number and the curve and point it names. Adding or removing
  a curve renumbers the parts of every curve after it.

- **The selection belongs to the item, not the mesh.** Two items that show one `BRep` select its
  parts apart.
- **An edge is selected when either of its halves is.** `Components::edge()` says so. A click
  selects the half edge it hit.
- **An index names whatever part holds it now.** A change to a mesh's numbering clears the
  component selection of every item that shows the mesh, through
  `Scene::deselectComponents(geometry)`, and an operation then selects what it means to leave
  selected. A topology command's undo and redo clear it the same way.
- **An operation on a mesh two items show runs once**, with the selection of the first selected
  of them in scene order. The other items lose their component selection.
- Removing an item, and clearing the scene, drop its parts. A document does not save the
  selection.

Background: [ADR-0121](../adr/0121-editor-component-selection-belongs-to-the-item.md),
[ADR-0126](../adr/0126-editor-the-handle-mask-selects-bezier-curve-handles.md)

## Manipulators

A manipulator is the set of handles that moves, turns or resizes the selected meshes.
`TranslateManipulator`, `RotateManipulator` and `ScaleManipulator` derive from `Manipulator`.
`TransformTool` holds all three and has four modes: none (Select), translate, rotate and scale.
One mode is active at a time, and the Select mode shows no handles.

**What the handles move follows the select mask.** `Target` holds it. Under a mask that selects
whole items it is the selected items of the kinds the mask takes, and a manipulator writes their
transforms. A light mask therefore moves the selected lights and leaves a selected mesh where it
is. A camera or a light ignores its scale. In a component mode it is the
distinct vertices of the selected components: one for a vertex, two for an edge, and the loop of
a face. A vertex shared by two selected components moves once. On a curve it is the selected
control points under Vertex, and the selected handles under Handle. A component target may span
every selected object, meshes and curves together, and with no component selected there are no
handles.

**Moving components.** Each vertex is moved in world space, through its object's matrix, and
written back in the object's own space. The object's transform does not change. Every face with a
moved corner gets its normal recomputed from its points, by Newell's method; a face squashed to no
area keeps the normal it had. A mask change ends a drag under way, and the change made so far is
recorded.

**Moving a curve's parts.** A control point moves in world space as a vertex does. A Bezier
point carries its two handles with it, through the same motion, so a turn turns them about the
pivot. The curve then places its automatic and vector handles again from the moved points. A
moved handle is retyped:

- A moved vector handle becomes free.
- A moved automatic handle becomes aligned, and so does the point's other handle if it is
  automatic.
- When the moved handle and the other are both aligned, the other turns to the far side of the
  point, in line, and keeps its length.

The undo record holds every curve of each curve item it touched, whole, so undo restores a
handle's type as well as its place. A move that would leave a curve that cannot be evaluated,
such as one with a point at infinity, leaves that curve as it was.

**Where the handles are.** Handles are drawn at the pivot: the mean of the selected objects'
origins, or of the moved vertices, control points and curve handles in world space. For one object that is its own origin, where
its transform pivots. Their length is a fixed number of pixels (90), converted to world units
through the `Screen` a manipulator is given: a view's camera and its region of the window. A
handle therefore keeps its on-screen size at any distance. Handles are drawn in the handle
pass, which has no depth test.

**Moving a set.** Everything the target holds moves in one gesture, about the one pivot. A
manipulator measures a `Motion` (an offset, a turn and a stretch) and the target applies it:

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
[ADR-0096](../adr/0096-editor-handles-pivot-at-the-mean-of-the-selection.md),
[ADR-0126](../adr/0126-editor-the-handle-mask-selects-bezier-curve-handles.md)
