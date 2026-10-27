# Meshes, the scene and viewports

The editor's data: half-edge meshes from `api/brep`, the scene items that place them, and the
four viewports that draw it.

## Meshes

`api/brep` holds `v3d::brep::BRep`, a half-edge mesh. It is the only mesh type the editor
models with.

**Parts and indices.** A `BRep` stores three arrays: vertices, half edges and faces. Each part
is named by a `brep::Index`, a `uint32_t` offset into its array. A half edge stores four
indices:

- `vertex`: the vertex the half edge ends at
- `face`: the face it borders
- `pair`: the half edge running the other way along the same edge
- `next`: the next half edge around its face

A face stores its normal and one of its half edges.

**Winding.** A face's loop runs clockwise when seen from outside, and its stored normal points
outward. Every primitive is built that way, and a normal recomputed from a face's points follows
the same rule, so a move never turns a face inside out. An importer of a counter-clockwise format
reverses each face's loop. Background:
[ADR-0097](../adr/0097-editor-faces-wind-clockwise-seen-from-outside.md)

`brep::INVALID_ID` (`1 << 31`) marks a
reference to nothing, such as the `pair` of an edge on an open boundary. **Do not change
`INVALID_ID`.** Scene documents store it, so a new value breaks every saved project.

**Building.** `BRep::addFace(points, normal)` adds a face from a list of points. It reuses an
existing vertex at the same position and pairs each new half edge with an existing one by
search. Because of that search, rebuilding a mesh through `addFace` can number its parts
differently. The lower-level `addVertex`, `addEdge` and `addFace` overloads that take a
`Vertex`, `HalfEdge` or `Face` append exactly what they are given.

**Building from a file's faces.** `brep::build()` in `api/brep/Builder.h` makes a whole mesh
from `Polygons`: a list of points, and each face as a run of indices into it. Use it for
anything larger than a primitive, because `addFace` searches the mesh for each vertex and pair,
and `build()` finds them through hash tables.

- Points at exactly the same position become one vertex, and -0 counts as 0.
- `BuildOptions::winding` says which way the input's faces run seen from outside. A
  counter-clockwise face is reversed.
- `BuildOptions::joinTriangles` joins pairs of triangles back into quads. A pair is joined
  across the edge they share when that edge is the longest of each, the quad is convex, and the
  normals differ by no more than `joinAngle`. Where a triangle could join either of two
  neighbours, the flatter pair is joined first. A flat grid of triangulated squares gives back
  its squares, never one large face.
- A face with fewer than three distinct corners is left out and counted in `Built::skipped`.
- The input is refused, with `Built::problem` saying why, when an index or a count is out of
  range, a point is not finite, a face visits one vertex twice, or two faces run along one edge
  in the same direction. The last happens where three or more faces meet at an edge, or where
  neighbouring faces disagree about which side is outside.

**Validation.** `BRep::validate(&problem)` checks that every reference points at a part the
mesh holds: a half edge's vertex always, its face, pair and next unless they are `INVALID_ID`,
and a face's edge always. A `next` chain that does not close is allowed, because an unfinished
modelling operation can leave one. A scene document is read against this rule.

**Strict validation.** `BRep::validateStrict(&problem)` is the check a topology change is tested
against. It runs `validate()` first, then requires that:

- every pair is mutual, and no half edge is its own pair
- every face's `next` chain returns to the face's first edge
- every half edge in a face's loop names that face
- no two half edges of one face run between the same two vertices

An edge with no pair passes, because an open mesh has a boundary. The problem names the edge or
face at fault. Every primitive the editor creates passes it. It does not check that a surface is
manifold at a vertex.

**Walking a mesh.** Use the functions in `api/brep/Topology.h` instead of following `next()`
yourself:

| Function | Returns |
|---|---|
| `faceLoop(mesh, face)` | The half edges of a face in order, each at most once. A chain that never returns to the face's first edge yields the edges walked before it repeats one or ends, which is an open path rather than a ring |
| `loopSegment(mesh, loop, entry)` | The two endpoints of one entry of a loop, or nothing when either cannot be resolved |
| `ownsEdge(mesh, edge)` | Whether this half edge represents its edge. The lower-numbered half of a pair owns it; an unpaired half edge owns itself |
| `center(mesh, face)` | The average of a face's vertices |
| `faceUV(mesh, face, &u, &v)` | Two perpendicular unit vectors in the plane of a face. `u` runs along the first edge with a length, and the first edge after it that is not parallel sets the plane. A face of fewer than two edges, or whose vertices all lie on a line, leaves `u` and `v` unchanged |
| `edgesFrom(mesh, vertex)` | The half edges leaving a vertex, in index order |
| `facesAround(mesh, vertex)` | The faces meeting at a vertex, each once, in index order |
| `newellNormal(mesh, face)` | A face's unit normal by Newell's method, which holds for a face that is not quite planar. It points outward, the side the loop runs clockwise when seen from. Zero for a face with no area |

`edgesFrom` and `facesAround` scan every half edge, because nothing keeps a vertex's own `edge`
current. Each costs time in proportion to the whole mesh.

**Snapshots.** `BRep::snapshot()` copies the three arrays as they stand, and `BRep::restore()`
puts a snapshot back whole. A topology command keeps one from before an operation and one from
after it.

**Counting changes.** `BRep::changes()` counts the mesh's changes, and never goes down. Adding a
part and `restore()` each count one, so every function in `Operations.h` counts, because they work
through them. A part changed in place, through the pointer `vertex()`, `edge()` or `face()`
returns, is not seen by the mesh, so the code that changes it calls `BRep::touch()`. The editor's
component move (`Target::move`) and a vertex command's undo and redo do.

**Changing topology.** The functions in `api/brep/Operations.h` change which parts a mesh holds.
Each leaves a mesh that passes `validateStrict()`. Each appends its new parts, so no existing
index changes, apart from `mergeVertices`. It compacts the arrays, so every part after a removed
one is renumbered.

| Function | Does |
|---|---|
| `splitEdge(mesh, edge)` | Adds a vertex at the middle of an edge and splits both its halves, keeping the pairs mutual. A split half edge keeps its index and ends at the new vertex; the part beyond it is a new half edge that follows it. Returns the new vertex |
| `splitFace(mesh, face, first, second)` | Joins two corners of a face with a new edge, making two faces. The face keeps the corners from the second round to the first, and the new face holds the rest. Both keep the winding and get a normal from their points. Refuses corners that are the same or next to each other. Returns the new face |
| `extrudeFaces(mesh, faces)` | Moves each face onto copies of the corners on its outline, and joins the copies to the corners left behind with a ring of quads. Faces that share an edge extrude as one region, so only the region's outline gains a ring. The copies sit where the corners are, so the ring's quads have no area and a zero normal until the faces are moved. Each face keeps its index and its half edges. Returns the ring's faces |
| `insetFaces(mesh, faces, fraction)` | Makes a smaller copy of each face inside it, joined to its outline by a ring of quads. Each face is inset on its own. Each corner of a copy is `fraction` of the way to the face's centre. Each face keeps its index and becomes the copy. Returns the rings' faces |
| `mergeVertices(mesh, vertices)` | Merges vertices into one at their centre. Edges that lose their length go, faces left with fewer than three corners go, and vertices nothing uses go. Edges that now run between the same two vertices are paired, so a merge can join two open boundaries. Refuses a merge that would leave an edge shared by more than two faces, a face that meets itself, or nothing at the merged point, and then changes nothing. Returns the merged vertex |

**Geometry only.** A `BRep` has no id and no placement. It is described about its own origin,
and the scene item that holds it places it. `v3dlib_brep` depends on `v3dlib_type` alone. A
`BRep` can be copied, and the copy is a separate mesh with the same parts. A `BRep` holds no
selection; [PickingAndManipulators.md](PickingAndManipulators.md#component-selection) describes
where it lives.

## Modelling operations

The Transform menu's Poly items act on the selection. Each records one `TopologyCommand`, and
does nothing, with a log line, on a selection it cannot use.

- **Split Edge** (`transform::poly::split_edge`) works under the edge mask. It splits every
  selected edge of every selected mesh at its middle, and selects the two parts of each, so it
  can be repeated.
- **Split Face** (`transform::poly::split_face`) works under the vertex mask. Exactly two vertices
  must be selected, in one mesh, on one face and not next to each other on it. It joins them with
  a new edge, and the two vertices stay selected.
- **Extrude Face** (`transform::poly::extrude_face`) works under the face mask. It extrudes the
  selected faces of every selected mesh, and faces that share an edge extrude as one region. The
  faces stay selected and have not moved, so the translate manipulator pulls them out. Moving
  them gives the ring its normals, as any component move does.
- **Inset Face** (`transform::poly::inset_face`, the I key) works under the face mask. It insets
  each selected face on its own, moving its corners a quarter of the way to its centre. The
  smaller faces stay selected. The handles pivot at a face's centre, so the scale manipulator
  changes how far it is inset.
- **Merge Vertices** (`transform::poly::merge_vertices`, the J key) works under the vertex mask.
  At least two vertices must be selected, in one mesh. They become one at their centre, which
  stays selected. A merge that would leave an edge shared by more than two faces, or a face that
  meets itself, is refused. Merging the two corners across a face is refused for both reasons.

The functions that do the work are in `src/command/Modelling.h`, apart from the controller, so they
can be tested without it.

## The scene

`v3d::editor::Scene` is the document being edited. It belongs to the editor, not to the api.
It holds scene items of every kind in one list, finds them by id, and keeps them in insertion
order. `Scene::add` appends. It also holds the hierarchy, which [Hierarchy](#hierarchy) describes.

**Items.** A scene item is a `v3d::editor::Item`. It has an id unique within one run of the
editor, a placement that is a `type::Transform`, and the object selection flag. There are four
kinds:

| Kind | Class | Holds |
|---|---|---|
| Mesh | `Mesh` | A shared `brep::BRep`, which its transform places. Two mesh items may share one `BRep`, and then share its component selection too |
| Camera | `Camera` | A `Lens`: perspective or orthographic, the angle of view in degrees, the orthographic half height, and the clipping distances. It looks along its own +z axis with +y up, as a camera profile with no rotation does |
| Light | `Light` | An `Emission`: a point, distant or spot light, its colour and intensity, and a spot light's cone and soft edge in radians, with RI's standard shaders' defaults. A distant or spot light shines along its own +z axis |
| Group | `Group` | Nothing of its own. It is a parent where no other item is to be one, and is drawn and picked as a box `GROUP_REACH` from its origin along each axis |

- **Identity.** Each item has two ids.
  - `Node` gives each item an id from a process-wide counter. It is unique within one run, is
    never reused, and means nothing in another run. A selection names an item by it.
  - The document id, `Item::documentId()`, is unique within one document and is saved with it.
    `Scene::add` gives an item without one the scene's next document id. An item keeps its id
    after a removal, so an undone creation comes back with it. Ids are never reused within a
    document, so the scene keeps the next one to give, and the document stores it. Anything
    saved that refers to an item names it by its document id.

  An item cannot be copied, because a copy would share both ids.
- **Placement.** `placement()` is a translation, a rotation and a scale, measured in the space of
  the item's parent. A command undoes a gesture by setting all three at once. `matrix()` composes
  them as translation × rotation × scale, which is the item's local matrix. Where the item is in
  the world comes from the scene's evaluation. `translation(v)` sets the position, and
  `translate(v)` adds an offset to it. A camera and a light ignore their scale.
- **Revision.** `Item::revision()` moves whenever anything a document saves of the item changes:
  its placement, its lens or emission, and a mesh's geometry. It is the item's own count of
  changes, which every placement setter and every setter of what its kind holds adds one to, and
  for a mesh `BRep::changes()` as well. A setter that writes what is already there counts too.
  Each count only goes up, so the sum moves exactly when one of them does. A change
  of selection does not move it. A save writes an item again only when its revision has moved.
- **Kinds.** `Item::kind()` names an item's class, and `as<T>(item)` casts to that class, or
  gives an empty pointer for any other kind. `Scene::mesh(id)` is the typed lookup for a mesh,
  and `Scene::item(id)` finds any kind.
- **Selection.** `Scene::selection()` returns every selected item of every kind in insertion
  order, as `Items`. `Scene::selected<T>()` returns the selected items of one kind, so
  `selected<Mesh>()` gives the `Meshes` an operation on components works through. Component
  selection lives inside those meshes and may span several of them. A modelling operation runs
  once on a `BRep` that two selected items share.
- **Visitors.** Code that reads the whole scene implements `SceneVisitor` and is passed to
  `Scene::accept()`, which calls the `visit` for each item's kind with where one evaluation of
  the scene puts it. Every `visit` is pure, so a new kind does not compile until each visitor
  handles it. The walk is depth first: an item, then everything beneath it, then `leave()` for
  it. `WireframeVisitor`, `RIBExportVisitor` and `Picker` work this way. The wireframe draws a
  camera, a light or a group as its glyph, the picker picks it by that glyph, and the export
  writes each light.
- **Not the views' cameras.** Each viewport owns its camera. `engine::CameraProfiles` holds the
  camera profiles read from `data/cameras.json`, and the layout names one profile per view. A
  camera item is part of the document.
- **Making a camera or a light.** The Create menu's Camera and Light items make one at the
  active view's eye, facing the way the view looks, through a `CreateCommand`, so the new item
  is the selection and one undo removes it. `create_camera()` and `create_light()` in
  `src/scene/CreateItem.h` do the placing. A camera takes the view's lens, and the default lens
  when the view's would not be valid. A light takes RI's default emission for its type.

Background: [ADR-0099](../adr/0099-editor-scene-items-own-identity-and-placement.md),
[ADR-0103](../adr/0103-editor-items-have-persistent-document-ids.md)

### Hierarchy

Any item may be the parent of others, and each item has at most one parent, so the items form a
set of trees. One item can only appear in one place. Geometry appears twice when two mesh items
share one `BRep`.

- **The links.** The scene holds each link, naming both ends by document id. Every link has both
  ends in the scene. `Scene::parent(child, parent)` sets one, and a parent of 0 clears it. It
  refuses an item that is not in the scene, an item as its own parent, and a parent that is
  beneath the child. `Scene::parent(id)`, `children(id)` and `ancestors(id)` read the links.
  `children(0)` gives the items with no parent. Siblings come in the scene's order.
- **The evaluation.** `Scene::evaluate()` composes where every item is. An item's local matrix
  is its placement, after any curve on it, which [Animation.md](Animation.md#evaluating-a-frame)
  describes. Its world matrix is its parent's world matrix times its local matrix, and an item
  with no parent has the identity as its parent's. The result is an `Evaluation`, which gives
  each item's `Placed`: the parent's world matrix, the local matrix and the world matrix. It
  also gives each item's `Pose`: its placement, and a camera's lens or a light's emission. It is
  a copy, so an edit made after it shows only in a later evaluation.
- **Every reader takes placement from an evaluation.** The wireframe, the shaded pass, the
  picker, the manipulators, the RIB export and `profile_of()` all do. None of them reads an
  item's own matrix as where it is, or a camera's or a light's own lens or emission. A visitor
  is handed the lens and the emission with the placement. A glyph is drawn through its world
  matrix without the scale, through `glyphPlacement()`, so it keeps its size.
- **Composition is the whole matrix.** A child takes its parent's scale as well as its rotation,
  as glTF, USD and RI do. Under a parent that is rotated and scaled unevenly, a child can be
  sheared. `orientation()` takes the rotation out of such a matrix, keeping the x axis.
- **Moving.** A manipulator measures a drag in world space and writes it back into the
  item's parent's space, so an item beneath another moves as the drag says.
  `Scene::uppermost()` leaves out every selected item that is beneath another selected item, so
  an item selected with its parent moves once, with the parent.
- **Removing.** `Scene::remove` takes the item and everything beneath it, at every depth. The
  links among them go too, so adding the items back does not restore the hierarchy.
- **Grouping.** Create > Group (`create::group`) gathers the uppermost of the selection beneath
  a new group, through a `GroupCommand`, and the group becomes the selection. The group goes at
  the mean of the items' origins, with no rotation or scale. When the items all have the same
  parent the group goes beneath it, and has no parent otherwise. Each item's placement changes so
  that it stays where it was in the world. That is exact for an item that keeps its parent's
  space. An item taken from beneath a sheared parent loses the shear. Undo moves the items back
  to their parents and placements, removes the group, and leaves the items selected.

Background: [ADR-0104](../adr/0104-editor-the-scene-evaluates-a-hierarchy-of-items.md)

The four primitives in `api/brep/Primitives.h`, `brep::cube()`, `plane()`, `cylinder()` and
`cone()`, are each one unit across and centred on the origin. Each is geometry, and the mesh
item that holds it places it. The cylinder and the cone are open: they have no cap faces.

## Viewports

A viewport is a camera, a rectangle of the window, and a set of display flags. It owns no
device state.

**Layout.** `data/layout.json` is a tree of vertical and horizontal splits. Each leaf names a
camera profile. `ViewLayout` turns the tree into one region per leaf, and names each view
after its profile. A profile shown twice names its second view "Perspective 2" and so on, so
every view has a pass of its own. A name, camera or split
that is not a string makes `load()` return false, and so does a split with no children. The
default layout is Front, Top, Left and Perspective. The menu bar and the toolbars take strips
along two edges of the window, and the views divide the rest.

**Shares.** A split divides its area between its children by their shares. A node's `share` is
a number above zero, a weight against the other children of its split, and a node with none
has a share of one. A split whose children name no share divides evenly, as the default layout
does. A share that is not a number above zero makes `load()` return false.

**Dragging a divider.** A press within `ViewLayout::dividerReach` (3 pixels) of the line
between two views takes hold of it before any tool sees the press, and the drag moves it. The
two views beside it trade weight, so every other view stays as it was. Each keeps at least
`ViewLayout::minimumShare` (5%) of its split. Where two dividers meet, the outer split's is
taken. The shares last as long as the editor runs, and are not written back to
`data/layout.json`.

**Drawing.** Each frame, the renderer fills two line canvases for every view:

- the scene canvas, which `ViewPort::draw()` fills with the construction grid and every mesh
  as a wireframe
- the handle canvas, which the active manipulator fills when something is selected and the view
  shows handles. The renderer hands the manipulator a `Screen`, the view's camera and region,
  so neither the view nor the manipulator includes the other

The renderer turns each view into two passes, then adds one more pass for the ui:

| Pass | Region | Depth | Clears |
|---|---|---|---|
| Scene pass, named after the view, which no other view shares. It draws the meshes' shaded surfaces first and the scene canvas over them | The view's region | Tested | Its own region, colour and depth |
| Handle pass | The view's region | Not tested | Nothing; it draws over the scene pass |
| `ui` | The whole window | Not tested | Nothing; it also draws the rubber band |

All views share one depth buffer. The handle pass has no depth test so that the manipulator is
always visible, including where it lies in the same plane as the grid's axis lines.

**Display flags.** `ViewPort::SHOW_GRID`, `SHOW_MESH`, `SHOW_CAMERA`, `SHOW_LIGHT` and
`SHOW_HANDLE` all start on. The show commands (`view::show::grid`, `mesh`, `camera`, `light` and
`handle`) toggle them on the *active view*, which is the view under the cursor.

**Glyphs.** A camera or a light is drawn in lines, in its own space, on the scene canvas. A glyph
is placed by its item's translation and rotation, and not its scale. It is `GLYPH_LENGTH` (one
unit) long along the item's +z axis, so it has a size in the scene rather than on the screen. An
unselected glyph is drawn in its own colour, and a selected one in the selection's. The shapes
are in `src/view/Glyphs.h`:

| Item | Glyph |
|---|---|
| Perspective camera | A pyramid from the eye to a picture one unit away, as tall as the angle of view and 4:3 wide, with a triangle above the picture that points up |
| Orthographic camera | A box twice the extent tall, 4:3 wide and one unit deep, with the same triangle |
| Point light | A star of three lines through the light |
| Distant light | Three parallel lines along +z, the middle one with an arrowhead |
| Spot light | A cone from the light, one unit along each side, with a ring at its edge and a ring where its soft edge begins |

A camera does not hold the shape of its picture, which a render's settings give, so its glyph
draws 4:3.

**The construction grid.** `ConstructionPlane` draws evenly spaced lines, with every nth line
and the two axis lines in stronger colours. An orthographic view gets a grid in the plane of
the two axes it looks across. A perspective view gets the ground plane.

**Active view.** The view under the cursor becomes active as the mouse moves. It does not
change while a drag is under way, so a drag that crosses a border keeps its camera. The camera,
select and transform tools all work in the active view. The cursor over the menu bar or a
toolbar does not change it, so a menu item acts on the view the cursor was last over.

**Cameras.** The View menu's Camera items show a fixed view in the active view: Front, Back,
Left, Right, Top, Bottom and Perspective, each a profile in `data/cameras.json`. The view keeps
its region and its camera object, so the camera tools keep driving it. The menu marks the item
for the profile the active view shows. A view laid out from a profile the menu does not offer
marks none. `Controller::fixedViews()` lists each command and its profile.

**Orthographic** (`view::camera::orthographic`, the P key) switches the active view's camera
between an orthographic and a perspective projection, and leaves the camera where it is. The
menu item is checked while the camera is orthographic.

A selected face is drawn as its outline and a selected vertex as a small box, on the wireframe
over the surface. The outline covers every edge of the face, including the edges it shares with
a neighbour.

## Shaded views

A view that shows meshes draws their surfaces in its scene pass, before its wireframe.
`ShadedPass` in `src/view/ShadedPass.h` does it, with the editor's own pipelines and shaders in
`vertical3d/shaders/`.

**Triangles.** `brep::triangulate()` in `api/brep/Triangulation.h` cuts each face into
triangles by ear clipping in the face's plane, so a concave face is cut inside its outline.
`brep::triangulateFace()` cuts one face. A face ear clipping cannot cut, one whose points lie on
a line, is fanned from its first corner. Each triangle winds counter-clockwise seen from
outside, the reverse of the face's loop.

- **Flat shading** asks for `brep::Normals::Face`, which gives each face its own copies of its
  corners, lit by the face's normal.
- **Smooth shading** asks for `brep::Normals::Vertex`, which gives each vertex one copy, lit by
  the average of the normals of the faces around it.
- A face whose stored normal is zero, as an extrude's ring is until it is moved, takes its
  normal from its points, and +z when those have no area.

**Uploads.** A mesh is uploaded once for each shading a view draws it in. It is uploaded again
only when its `BRep::changes()` has moved, so a move of the item uploads nothing: its placement
reaches the shader as a push constant at each draw. An upload no view drew in a frame is let go,
through the frame ring, once the frames that read it are done.

**The light** is at the eye. A surface facing the view is drawn in its colour, and one seen edge
on keeps 30% of it. Either side of a surface is lit alike, so a back face is not black.

**Display.** Each view keeps its own `ViewPort::Display`. The View menu sets the active view's:

| Command | Sets | Default |
|---|---|---|
| `view::shade::flat`, `smooth` | The shading | Flat |
| `view::cull::none`, `front`, `back`, `both` | Which side of the triangles is left out | Back |
| `view::rendermode::front::points`, `lines`, `fill` | How the front of a triangle is drawn | Fill |
| `view::rendermode::back::points`, `lines`, `fill` | How the back of a triangle is drawn | Fill |

The front is the side a face's outward normal points to. A view draws each mesh twice, once
keeping the front in the front's fill and once keeping the back in the back's, and the cull
leaves out the side it names. The default fills the front and leaves the back out. A closed
surface looks the same either way, and its back is never drawn.

**Depth.** Every surface is drawn at a depth bias that pushes it back, so the wireframe drawn
over it at the same depth stays in front. A point or line fill needs a device feature,
`fillModeNonSolid`. On a device without it a side asked for that way is drawn filled, and the
log says so once at startup.
