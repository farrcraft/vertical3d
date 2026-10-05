# The editor

This document is for someone working on the 3D modelling editor in `vertical3d/` or on the mesh
library in `api/brep`. It covers how the editor is put together and the rules each part follows.

The editor shows four viewports of one scene over a construction grid. It creates primitive
meshes, picks objects and their parts, moves them with manipulators, undoes changes, and saves
projects. Menus, two toolbars and a file chooser are drawn with `api/ui`.

- [Running it](#running-it)
- [Source layout](#source-layout)
- [Meshes](#meshes)
- [The scene](#the-scene)
- [Viewports](#viewports)
- [Picking](#picking)
- [Manipulators](#manipulators)
- [Undo](#undo)
- [Commands](#commands)
- [Project files](#project-files)
- [RIB export](#rib-export)
- [What is not built](#what-is-not-built)

## Running it

Build the `vertical3d` target, then run `out/build/x64-Debug/vertical3d/vertical3d.exe`. The
editor reads its data from the `data/` directory beside the executable and logs to `v3d.log`
in the same place. [contributing/Build.md](contributing/Build.md) covers the build.

The default bindings are in `vertical3d/data/mappings.json`:

| Input | Command | Effect |
|---|---|---|
| Left mouse | `view::drag` | Pick, grab a handle, or drive the camera (see [Picking](#picking)) |
| Hold Left Alt + drag | `view::camera::zoom` | Zoom an orthographic view; dolly a perspective one |
| Hold Left Ctrl + drag | `view::camera::truck` | Move the camera sideways and up or down |
| Hold Left Shift + drag | `view::camera::pan` | Turn a perspective camera with an arcball; does nothing in an orthographic view |
| 1, 2, 3, 4 | `create::poly::cube`, `plane`, `cylinder`, `cone` | Add a primitive at the origin |
| 5, 6, 7, 8 | `select::mask::object`, `vertex`, `edge`, `face` | Choose what a click selects |
| Q, W, E, R | `transform::select`, `translate`, `rotate`, `scale` | Choose the manipulator |
| G, M, H | `view::show::grid`, `mesh`, `handle` | Toggle what the view under the cursor draws |
| F3 | `view::show::statistics` | Toggle the frame statistics overlay |
| Z, Y | `edit::undo`, `edit::redo` | Step the history |
| O, S | `project::load`, `project::save` | Open a project; save the current one |
| Escape | `ui::quit` | Quit |

The bindings cannot express a key chord such as Ctrl+Z, so undo and redo use bare keys. Left
Ctrl is also the truck modifier. Save As and RIB export are on the Project menu only.

## Source layout

| Path | Holds |
|---|---|
| `src/main.cxx` | `main`, which calls `v3d::engine::run<Controller>` |
| `src/Controller.*` | The app: startup, command registration, mouse routing, project actions |
| `src/Renderer.*` | Builds each frame's passes from the viewports and the ui |
| `src/view/` | `ViewLayout`, `ViewPort`, `ConstructionPlane` |
| `src/scene/` | `Scene`, `SceneVisitor`, the primitives in `CreatePoly`, `WireframeVisitor`, `RIBExportVisitor`, `Project` |
| `src/command/` | `Command`, `CommandStack`, `CreateCommand`, `TransformCommand`, `Placement`, `CommandDirectory` |
| `src/tool/` | `Tool`, `CameraControlTool`, `SelectTool`, `TransformTool`, `Picker`, `SelectMask` |
| `src/manipulator/` | `Manipulator` and its translate, rotate and scale subclasses |
| `data/` | Config documents: bindings, ui, window, camera profiles, layout |
| `tests/` | The `vertical3d` Boost.Test suite, with one directory for each `src/` subdirectory |

Everything is in the namespace `v3d::editor`. Includes are written from the repository root,
for both the api and the editor's own headers: `#include <api/brep/BRep.h>`,
`#include <vertical3d/src/scene/Scene.h>`. A header in the same directory is included by its
bare name in quotes.

The tests run without a window or a GPU. Run them with
`ctest --test-dir out/build/x64-Debug -R vertical3d`.

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
| `faceUV(mesh, face, &u, &v)` | Two perpendicular unit vectors in the plane of a face |

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

Background: [ADR-0013](adr/0013-editor-a-mesh-is-a-dag-node.md)

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

Background: [ADR-0014](adr/0014-editor-pick-by-cpu-ray-cast.md)

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

Background: [ADR-0015](adr/0015-editor-manipulators-edit-the-object-transform.md)

## Undo

`v3d::editor::Command` is a record of a change that has **already been made**. It has
`undo()`, `redo()` and `name()`, and no `execute()`. `CommandStack::push` stores a command and
never applies it.

**One gesture is one command.** A manipulator drag changes the mesh on every motion event so
the user sees it move. `TransformTool` records one `TransformCommand` when the drag ends,
holding the mesh's `Placement` (translation, rotation and scale) from the start and the end of
the gesture. Undo puts the whole starting placement back.

The rules:

- **The first do goes through `redo()`** where the change is not interactive.
  `Controller::createPoly` builds a `CreateCommand`, calls `redo()` to add the mesh, and then
  pushes it. Creating and redoing use the same code.
- **Every path out of a drag records it.** `TransformTool::commit()` runs on release and on a
  mode change. It pushes only if the placement actually changed, so a handle grabbed and
  released without moving records nothing.
- **A command holds the mesh, not its id.** A mesh removed by an undo stays alive inside the
  command and returns with the same id. Later commands in the history still refer to it.
- **Selection is not history.** Undo does not restore what was selected. `CreateCommand::undo`
  clears the selection flag of the mesh it removes, so that at most one mesh stays selected.
- **The stack has a capacity**, 64 by default. When it is full, the oldest command is dropped.
  A mesh held by a dropped command is freed then.
- **A new push discards the redo side.**
- **Opening a project clears the history**, so an open cannot be undone. It also cancels any
  drag under way without recording it.

A future modelling operation that changes topology will need its own command type that records
the topology it changed. `TransformCommand` does not fit that case.

Background: [ADR-0016](adr/0016-editor-undo-records-completed-changes.md)

## Commands

A command is anything the user can ask the editor to do: create a cube, change the select mask,
undo, quit. It is identified by a context and a name written together as `"context::name"`, for
example `create::poly::cube` or `transform::rotate`. This is the string `event::Event::str()`
returns.

**The directory.** `v3d::editor::CommandDirectory` maps each command string to a handler.
`Controller::registerCommands()` registers every handler at startup:

- `addPress(name, handler)` registers a handler that runs on the press only. A release of the
  same binding is still accepted, and does nothing.
- `add(name, handler)` registers a handler that receives the whole event, so it can tell a
  press from a release. The camera modifiers use it, because a camera mode lasts while its key
  is held.

`Controller::handleEvent` calls `invoke()` and does nothing else. After every command it calls
`syncUi()`, which sets the check marks on menu items and toolbar toggles from the editor's
state: the active view's show flags, the select mask and the transform mode.

**How a command arrives.** Three sources produce the same `event::Event`, so all three reach
the same handler:

- **A key or mouse binding** in `data/mappings.json`. The `source` names the key; the
  `destination` names the command's context and name.
- **A menu item** in `data/vgui.json`, with `context` and `command` fields.
- **A toolbar button** in `data/vgui.json`, with the same two fields.

**The order of events.** The engine handles each SDL event in this order:

1. `Controller::onEvent()` receives the raw SDL event first. The editor passes keyboard events
   to `ui::shell::Keyboard`, so a focused text box (such as the file chooser's name box) takes
   its keys. An event it takes never reaches the bindings.
2. A key or button is published as an `event::Source` to every listener on `sink<Source>`. The
   editor does not listen there.
3. Unless a listener consumed the source, the event engine sends the commands bound to it on
   `sink<Event>`. The editor's `handleEvent` listens there, so it receives commands and never
   raw keys.

Mouse motion and window resizes reach the editor as `event::kind::MouseMotion` and
`event::kind::WindowResize` on their own sinks, without bindings.

**Rules.**

- An unregistered command is logged as a warning: "no command is registered as ...". A menu
  item for a feature that does not exist reports itself this way.
- A second registration of the same name is refused, and the first handler stays. The
  controller logs it as an error.
- Startup logs how many commands are registered and how many of them are on a menu.
- Command names follow the editor's original UI definition, so menu items and bindings use the
  same strings. `ui::quit` uses the `ui` context, as every app's application-level commands do.
  `edit::undo` and `edit::redo` were added for the history.

**Adding a command:**

1. Register a handler in `Controller::registerCommands()` with `press()` or `hold()`.
2. Bind it in `data/mappings.json`, add it to a menu or toolbar in `data/vgui.json`, or both,
   with the same context and name.
3. If it describes on/off state, mark it in `syncUi()`.

**Tools.** `v3d::editor::Tool` is the interface for something that keeps receiving mouse input
while it is active: `activate(name)`, `deactivate(name)`, `motion(position)` and
`button(button, pressed, position)`. `CameraControlTool`, `SelectTool` and `TransformTool`
implement it. `Tool` lives in the editor because no other app holds a gesture open across
events.

`CommandDirectory`, `CommandStack` and `Tool` are all in the editor rather than the api. No
other app has a document to undo or several ways to invoke one command.

Background: [ADR-0081](adr/0081-input-key-events-and-commands-are-separate.md)

## Project files

A project is a JSON document read and written by `v3d::editor::Project`.

```json
{
  "version": 1,
  "name": "untitled",
  "meshes": [
    {
      "transform": {
        "translation": [0.0, 0.0, 0.0],
        "rotation": [0.0, 0.0, 0.0, 1.0],
        "scale": [1.0, 1.0, 1.0]
      },
      "vertices": [[-0.5, -0.5, 0.5], ...],
      "edges": [{ "vertex": 1, "face": 0, "pair": 7, "next": 1 }, ...],
      "faces": [{ "normal": [0.0, 0.0, 1.0], "edge": 0 }, ...]
    }
  ]
}
```

The rotation is a quaternion written as x, y, z, w.

**Topology is stored index for index.** The three arrays are written and read back exactly, so
a save and an open renumber nothing. The reader does not rebuild faces through
`BRep::addFace`, because that would renumber the parts.

**Not stored:**

- **Mesh ids.** A saved id could collide with a mesh already loaded. Meshes read from a file
  get new ids.
- **Selection.** It describes what the user is doing, not what the document holds.
- **A vertex's edge.** Nothing in the editor sets it.

**Reading.** `Project::read` replaces the whole scene. It refuses the file, and leaves the
scene untouched, if:

- the file cannot be parsed
- a required field is missing
- any mesh fails `BRep::validate()`
- the version is newer than `Project::VERSION`

An older version is migrated forward one version at a time by `asset::readForward`. There are
no migration steps yet, because the format is still at version 1. Each refusal is logged with
its reason.

**Writing.** `Project::write` calls `asset::writeDocument`. It writes the document to a
temporary file beside the target and renames it over the target. If the write fails, the
previous file survives. The output is indented, and floats are written at float precision so
values stay readable.

**Open, Save and Save As.**

- The current project path starts as `project.json` beside the executable.
- **Open** (O, or Project > Load) shows the file chooser in open mode, filtered to `.json`. A
  successful read makes the chosen file the current project and clears the history.
- **Save** (S, or Project > Save) writes to the current project path without asking.
- **Save As** (Project > Save As) shows the chooser in save mode. A successful write makes the
  new file the current project.

The chooser is `ui::shell::FileChooser`. It drives the `chooser` container laid out in
`data/vgui.json`: a file list, a name box, and Accept and Cancel buttons. Those send
`project::chooser::pick`, `project::chooser::accept` and `project::chooser::cancel`.

There is no dirty flag. Nothing warns before an open or a quit discards unsaved work.

Background: [ADR-0018](adr/0018-editor-projects-saved-as-json-with-exact-topology.md),
[ADR-0041](adr/0041-files-write-documents-atomically.md)

## RIB export

Project > Export > rib (`project::export::rib`) writes the scene as RenderMan RIB to
`export.rib` beside the executable, replacing any file already there. The export is one-way:
nothing reads it back into the editor. The project file stays the editor's own format, and the
offline renderer reads RIB.

`RIBExportVisitor` writes:

- a header, then a `Format` the size of the active view's camera profile
- the active view's camera as a `Projection` (orthographic or perspective), `ScreenWindow`,
  `Clipping` and `Transform`
- for each mesh, inside `WorldBegin` and `WorldEnd`: an `AttributeBegin` block with an
  identifier name, a `ConcatTransform` of the mesh's matrix, and one `Polygon` per face

The scene has no lights or materials, so the result renders in one flat colour. RI treats a
polygon as planar and convex without checking. moya dices only the first four vertices of a
polygon, so a face with more than four vertices renders as a quad.
[OfflineRenderer.md](OfflineRenderer.md) covers what moya does with the file.

Background: [ADR-0023](adr/0023-offline-rib-is-the-scene-format.md)

## What is not built

[TODO.md](TODO.md#editor) lists the open work. In short:

- Most menu items have no handler and log themselves as unregistered.
- There is no modelling operation. A component mode selects a part, and the manipulator then
  moves the whole object.
- One thing is selected at a time.
- There is no dirty flag.
- The viewport panes cannot be resized by dragging.
