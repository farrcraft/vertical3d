# Meshes, the scene and viewports

The editor's data: half-edge meshes from `api/brep`, the scene that holds them, and the four
viewports that draw it.

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

A face stores its normal and one of its half edges. `brep::INVALID_ID` (`1 << 31`) marks a
reference to nothing, such as the `pair` of an edge on an open boundary. **Do not change
`INVALID_ID`.** Project files store it, so a new value breaks every saved project.

**Building.** `BRep::addFace(points, normal)` adds a face from a list of points. It reuses an
existing vertex at the same position and pairs each new half edge with an existing one by
search. Because of that search, rebuilding a mesh through `addFace` can number its parts
differently. The lower-level `addVertex`, `addEdge` and `addFace` overloads that take a
`Vertex`, `HalfEdge` or `Face` append exactly what they are given.

**Validation.** `BRep::validate(&problem)` checks that every reference points at a part the
mesh holds: a half edge's vertex always, its face, pair and next unless they are `INVALID_ID`,
and a face's edge always. A `next` chain that does not close is allowed, because an unfinished
modelling operation can leave one. Validation does not check that a pair is mutual or that a
surface is manifold.

**Walking a mesh.** Use the functions in `api/brep/Topology.h` instead of following `next()`
yourself:

| Function | Returns |
|---|---|
| `faceLoop(mesh, face)` | The half edges of a face in order. It stops after the mesh's edge count, so an unclosed chain ends rather than looping forever |
| `loopSegment(mesh, loop, entry, &from, &to)` | The two endpoints of one entry of a loop |
| `ownsEdge(mesh, edge)` | Whether this half edge represents its edge. The lower-numbered half of a pair owns it; an unpaired half edge owns itself |
| `edgeSelected(mesh, edge)` | Whether either half of the edge is selected |
| `center(mesh, face)` | The average of a face's vertices |
| `faceUV(mesh, face, &u, &v)` | Two perpendicular unit vectors in the plane of a face. `u` runs along the first edge with a length, and the first edge after it that is not parallel sets the plane. A face of fewer than two edges, or whose vertices all lie on a line, leaves `u` and `v` unchanged |

**Identity and placement.** `BRep` derives from `v3d::dag::Node` and `v3d::dag::Transform`, so
`v3dlib_brep` links `v3dlib_dag` publicly.

- `Node` gives each mesh an id from a process-wide counter. Ids are unique within one run, are
  never reused, and mean nothing in another run. A `Node`, and so a `BRep`, cannot be copied.
- `Transform` holds a translation, a rotation and a scale. `matrix()` composes them as
  translation × rotation × scale, so geometry is described about the mesh's own origin.
  `translation(v)` sets the position; `translate(v)` adds an offset to it.

**Selection flags.** `BRep::selected()` is the object selection. Each `Vertex`, `HalfEdge` and
`Face` carries its own flag for component selection. `BRep::deselectComponents()` clears the
component flags and leaves the object flag alone.

Background: [ADR-0013](../adr/0013-editor-a-mesh-is-a-dag-node.md)

## The scene

`v3d::editor::Scene` is the document being edited. It belongs to the editor, not to the api.
It holds meshes and nothing else, finds them by their `Node` id, and keeps them in insertion
order. `Scene::add` appends.

- **No cameras.** Each viewport owns its camera. `config::CameraProfiles` holds the camera
  profiles read from `data/cameras.json`, and the layout names one profile per view.
- **One selected object.** `Scene::selection()` returns the first selected mesh. The editor
  keeps at most one mesh selected, and component selection happens inside that mesh.
- **Visitors.** Code that reads the whole scene implements `SceneVisitor` and is passed to
  `Scene::accept()`. `WireframeVisitor`, `RIBExportVisitor`, `Picker` and the project writer
  all work this way.

The four primitives in `src/scene/CreatePoly.h` are each one unit across and centred on the
origin, so a mesh is placed by its transform. The cylinder and the cone are open: they have
no cap faces.

## Viewports

A viewport is a camera, a rectangle of the window, and a set of display flags. It owns no
device state.

**Layout.** `data/layout.json` is a tree of vertical and horizontal splits. Each leaf names a
camera profile. `ViewLayout` turns the tree into one region per leaf. The default layout is
Front, Top, Left and Perspective. The menu bar and the toolbars take strips along two edges of
the window, and the views divide the rest.

**Drawing.** Each frame, every `ViewPort` fills two line canvases:

- the scene canvas: the construction grid and every mesh as a wireframe
- the handle canvas: the manipulator, if one is active and something is selected

The renderer turns each view into two passes, then adds one more pass for the ui:

| Pass | Region | Depth | Clears |
|---|---|---|---|
| Scene pass, named after the view | The view's region | Tested | Its own region, colour and depth |
| Handle pass | The view's region | Not tested | Nothing; it draws over the scene pass |
| `ui` | The whole window | Not tested | Nothing |

All views share one depth buffer. The handle pass has no depth test so that the manipulator is
always visible, including where it lies in the same plane as the grid's axis lines.

**Display flags.** `ViewPort::SHOW_GRID`, `SHOW_MESH` and `SHOW_HANDLE` start on. The show
commands toggle them on the *active view*, which is the view under the cursor. `SHOW_CAMERA`
and `SHOW_LIGHT` exist, but nothing draws cameras or lights, so no command toggles them.

**The construction grid.** `ConstructionPlane` draws evenly spaced lines, with every nth line
and the two axis lines in stronger colours. An orthographic view gets a grid in the plane of
the two axes it looks across. A perspective view gets the ground plane.

**Active view.** The view under the cursor becomes active as the mouse moves. It does not
change while a drag is under way, so a drag that crosses a border keeps its camera. The camera,
select and transform tools all work in the active view.

The editor draws lines only. A shaded display mode is not built, which is why a selected face is
drawn as its outline and a selected vertex as a small box.
