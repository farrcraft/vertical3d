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
| `edgeSelected(mesh, edge)` | Whether either half of the edge is selected |
| `center(mesh, face)` | The average of a face's vertices |
| `faceUV(mesh, face, &u, &v)` | Two perpendicular unit vectors in the plane of a face. `u` runs along the first edge with a length, and the first edge after it that is not parallel sets the plane. A face of fewer than two edges, or whose vertices all lie on a line, leaves `u` and `v` unchanged |
| `edgesFrom(mesh, vertex)` | The half edges leaving a vertex, in index order |
| `facesAround(mesh, vertex)` | The faces meeting at a vertex, each once, in index order |
| `newellNormal(mesh, face)` | A face's unit normal by Newell's method, which holds for a face that is not quite planar. It points outward, the side the loop runs clockwise when seen from. Zero for a face with no area |

`edgesFrom` and `facesAround` scan every half edge, because nothing keeps a vertex's own `edge`
current. Each costs time in proportion to the whole mesh.

**Snapshots.** `BRep::snapshot()` copies the three arrays as they stand, and `BRep::restore()`
puts a snapshot back whole, selection flags included. A topology command keeps one from before an
operation and one from after it.

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
`BRep` can be copied, and the copy is a separate mesh with the same parts and the same flags.

**Selection flags.** Each `Vertex`, `HalfEdge` and `Face` carries its own flag for component
selection. `BRep::deselectComponents()` clears them all. The object selection is on the scene
item, not on the mesh.

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

The functions that do the work are in `src/scene/Modelling.h`, apart from the controller, so they
can be tested without it.

## The scene

`v3d::editor::Scene` is the document being edited. It belongs to the editor, not to the api.
It holds scene items of every kind in one list, finds them by id, and keeps them in insertion
order. `Scene::add` appends.

**Items.** A scene item is a `v3d::editor::Item`. It derives from `v3d::dag::Node` and
`v3d::dag::Transform`, and carries the object selection flag. There are three kinds:

| Kind | Class | Holds |
|---|---|---|
| Mesh | `Mesh` | A shared `brep::BRep`, which its transform places. Two mesh items may share one `BRep`, and then share its component selection too |
| Camera | `Camera` | A `Lens`: perspective or orthographic, the angle of view in degrees, the orthographic half height, and the clipping distances. It looks along its own +z axis with +y up, as a camera profile with no rotation does |
| Light | `Light` | An `Emission`: a point, distant or spot light, its colour and intensity, and a spot light's cone and soft edge in radians, with RI's standard shaders' defaults. A distant or spot light shines along its own +z axis |

- **Identity.** Each item has two ids.
  - `Node` gives each item an id from a process-wide counter. It is unique within one run, is
    never reused, and means nothing in another run. A selection names an item by it.
  - The document id, `Item::documentId()`, is unique within one document and is saved with it.
    `Scene::add` gives an item without one the scene's next document id. An item keeps its id
    after a removal, so an undone creation comes back with it. Ids are never reused within a
    document, so the scene keeps the next one to give, and the document stores it. Anything
    saved that refers to an item names it by its document id.

  An item cannot be copied, because a copy would share both ids.
- **Placement.** `Transform` holds a translation, a rotation and a scale. `matrix()` composes
  them as translation × rotation × scale. `translation(v)` sets the position; `translate(v)`
  adds an offset to it. A camera and a light ignore their scale.
- **Kinds.** `Item::kind()` names an item's class, and `as<T>(item)` casts to that class, or
  gives an empty pointer for any other kind. `Scene::mesh(id)` is the typed lookup for a mesh,
  and `Scene::item(id)` finds any kind.
- **Selection.** `Scene::selection()` returns every selected item of every kind in insertion
  order, as `Items`. `Scene::selected<T>()` returns the selected items of one kind, so
  `selected<Mesh>()` gives the `Meshes` an operation on components works through. Component
  selection lives inside those meshes and may span several of them. A modelling operation runs
  once on a `BRep` that two selected items share.
- **Visitors.** Code that reads the whole scene implements `SceneVisitor` and is passed to
  `Scene::accept()`, which calls the `visit` for each item's kind. Every `visit` is pure, so a
  new kind does not compile until each visitor handles it. `WireframeVisitor`,
  `RIBExportVisitor`, `Picker` and the project writer all work this way. The wireframe draws a
  camera or a light as its glyph, the picker picks it by that glyph, and the export writes each
  light.
- **Not the views' cameras.** Each viewport owns its camera. `config::CameraProfiles` holds the
  camera profiles read from `data/cameras.json`, and the layout names one profile per view. A
  camera item is part of the document.
- **Making a camera or a light.** The Create menu's Camera and Light items make one at the
  active view's eye, facing the way the view looks, through a `CreateCommand`, so the new item
  is the selection and one undo removes it. `create_camera()` and `create_light()` in
  `src/scene/CreateItem.h` do the placing. A camera takes the view's lens, and the default lens
  when the view's would not be valid. A light takes RI's default emission for its type.

Background: [ADR-0099](../adr/0099-editor-scene-items-own-identity-and-placement.md),
[ADR-0103](../adr/0103-editor-items-have-persistent-document-ids.md)

The four primitives in `src/scene/CreatePoly.h` are each one unit across and centred on the
origin. Each is geometry, and the mesh item that holds it places it. The cylinder and the cone
are open: they have no cap faces.

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

**Drawing.** Each frame, every `ViewPort` fills two line canvases:

- the scene canvas: the construction grid and every mesh as a wireframe
- the handle canvas: the manipulator, if one is active and something is selected

The renderer turns each view into two passes, then adds one more pass for the ui:

| Pass | Region | Depth | Clears |
|---|---|---|---|
| Scene pass, named after the view, which no other view shares | The view's region | Tested | Its own region, colour and depth |
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
are in `src/scene/Glyphs.h`:

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

The editor draws lines only. A shaded display mode is not built, so a selected face is drawn as
its outline and a selected vertex as a small box. The outline covers every edge of
the face, including the edges it shares with a neighbour.
