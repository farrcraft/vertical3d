# The editor

These pages are for someone working on the 3D modelling editor in `vertical3d/` or on the mesh
library in `api/brep`.

The editor shows four viewports of one scene over a construction grid, an outliner of its items
and a properties editor, in areas that can be split, joined, swapped and maximised. It creates
primitive meshes, picks objects and their parts, moves them with manipulators, undoes changes,
and saves projects. The menu bar with the workspaces beside it, a tool shelf, each area's
header, the command search and a file chooser are drawn with `api/ui`, and a status bar and
notices over the same canvas.

| Page | Read it to |
|---|---|
| This page | Run the editor, find your way around its source, and see what is not built |
| [Scene.md](Scene.md) | Work with meshes, the scene items that hold them, and the viewports that show the scene |
| [PickingAndManipulators.md](PickingAndManipulators.md) | Change how clicks select things, and how handles move them |
| [CommandsAndUndo.md](CommandsAndUndo.md) | Add a command, or make a change undoable |
| [Animation.md](Animation.md) | Key item properties, and play them back with the Animate menu |
| [Rendering.md](Rendering.md) | Render the scene with moya, in the render window or as a batch of frames |
| [Files.md](Files.md) | Change the project layout, the scene document format, importing, or the RIB export |

## Running it

Build the `vertical3d` target, then run `out/build/x64-Debug/vertical3d/vertical3d.exe`. The
editor reads its data from the `data/` directory beside the executable, and its log is where
[api/engine/README.md](../api/engine/README.md#logging) says.
[contributing/Build.md](../contributing/Build.md) covers the build.

The default bindings are in `vertical3d/data/mappings.json`:

| Input | Command | Effect |
|---|---|---|
| Left mouse | `view::drag` | Pick, grab a handle, or drive the camera (see [Picking](PickingAndManipulators.md#picking)) |
| Hold Left Alt + drag | `view::camera::dolly` | Dolly a perspective view; zoom an orthographic one |
| Hold Left Ctrl + drag | `view::camera::truck` | Move the camera sideways and up or down |
| Middle mouse drag | `view::camera::pan` | Turn a perspective camera with an arcball; does nothing in an orthographic view |
| View > Navigate menu | `camera::truck`, `dolly`, `zoom`, `pan`, `pitch` | Latch a camera move, so a left drag with no modifier drives the camera (see [Driving a camera](PickingAndManipulators.md#driving-a-camera)) |
| Hold Left Shift | `select::add` | A click adds to the selection or takes away from it (see [Picking](PickingAndManipulators.md#picking)) |
| 1, 2, 3, 4 | `create::poly::cube`, `plane`, `cylinder`, `cone` | Add a primitive at the origin |
| Create menu | `create::perspective::camera`, `ortho::camera`, `light::point`, `light::distant`, `light::spot` | Add a camera or a light at the view under the cursor, facing the way it looks |
| 5, 6, 7, 8 | `select::mask::object`, `vertex`, `edge`, `face` | Choose what a click selects |
| Q, W, E, R | `transform::select`, `translate`, `rotate`, `scale` | Choose the manipulator |
| I | `transform::poly::inset_face` | Inset the selected faces (see [Modelling operations](Scene.md#modelling-operations)) |
| J | `transform::poly::merge_vertices` | Merge the selected vertices into one |
| G, M, H | `view::show::grid`, `mesh`, `handle` | Toggle what the view under the cursor draws |
| P | `view::camera::orthographic` | Switch the view under the cursor between orthographic and perspective (see [Cameras](Scene.md#viewports)) |
| F3 | `search::open` | Search the menus' commands, and run one (see [The command search](CommandsAndUndo.md#the-command-search)) |
| F12 | `view::show::statistics` | Toggle the frame statistics overlay |
| Ctrl+Z; Ctrl+Y or Ctrl+Shift+Z | `edit::undo`, `edit::redo` | Step the history |
| Ctrl+O, Ctrl+S | `project::load`, `project::save` | Open a project; save the current one |
| Ctrl+Space | `area::maximise` | Maximise the area under the cursor, or give every area its place back (see [Viewports](Scene.md#viewports)) |
| Ctrl+Page Down, Ctrl+Page Up | `workspace::next`, `previous` | Show the next or the previous workspace (see [Workspaces](Scene.md#workspaces)) |
| File > Import menu | `project::import::gltf`, `usd` | Import a glTF file's or a USD stage's scene (see [Importing](Files.md#importing)) |
| Escape | `ui::quit` | Quit |

Left Ctrl is also the truck modifier, so truck mode is on while a Ctrl chord is pressed. It
drives the camera only during a drag, so the chord is unaffected. Save As and RIB export are on
the File menu only.

The menu bar leads with the editor's mark in the accent, a menu holding the command search and
Quit, then File, Edit, Create, Select, Modify, View, Animate, Render, Window and Help. Beside it
are a tab for each workspace, together on one plate, the project's name, and a Search button.
While a menu is open, the arrow keys walk it, as
[api/ui/Components.md](../api/ui/Components.md) describes. The tool shelf down the left holds the
transform tools and the modelling operations, each with a tooltip beside it.

**Icons.** Each icon is an SVG file in `data/icons/`, drawn in white, and `data/icons.json` names
them. The renderer draws every icon into one image, each scaled to fit a square cell at the ui's
icon size in device pixels. When the scale changes, it draws them again and points the ui's
images at the new texture. Whatever draws an icon tints it: a button or menu item the colour of
its label, and the editor's own panels as they choose. An icon is named in the ui as `icon:` and
its name. Only the subset of SVG that NanoSVG draws is drawn: shapes, paths, fills, strokes and
dashes, with no text, filters or embedded images.

Background: [ADR-0139](../adr/0139-editor-icons-are-svg-drawn-at-the-ui-scale.md)

## Source layout

The editor's directories are in layers. A directory includes only the directories below it in
this table, so they have no cycle between them. A change that needs a lower directory to include
a higher one is in the wrong place.

| Path | Holds | Includes |
|---|---|---|
| `src/scene/` | The model. `item/` holds the kinds of item and their parts: which parts of each mesh are selected, and the items made from a view. The root holds the scene, its evaluation, animation and render settings, and the items an imported scene becomes | nothing of the editor's |
| `src/selection/` | What a select mask takes | `scene/` |
| `src/store/` | The project directory and the scene document it saves, with a codec for each kind of item | `scene/` |
| `src/render/` | Rendering the scene with moya: the RIB export, a shot, the render window, live render and batches | `scene/` |
| `src/view/` | The viewports and their layout, the construction plane, the wireframe and its glyphs, and the shaded pass | `scene/` |
| `src/manipulator/` | The translate, rotate and scale handles, and what they move | `scene/`, `selection/` |
| `src/command/` | The undo records, the stack that holds them, and the modelling operations that make them | `scene/`, `manipulator/` |
| `src/tool/` | The camera, select and transform tools, the router that gives each press to one of them, and picking | `scene/`, `selection/`, `view/`, `manipulator/`, `command/` |
| `src/document/` | The document: the scene, its history and its project, and every operation on them | `scene/`, `store/`, `render/`, `command/` |
| `src/control/` | The editors and panels: the controllers for playback and the Render menu, the workspaces, the command search, the icon sheet, the outliner, and the properties, timeline and graph editors. `chrome/` holds the window's furniture: the routing of the mouse, the areas' headers, the top bar, the divider hint, and what the status bar and notices say | `scene/`, `render/`, `view/`, `tool/`, `document/` |
| `src/app/` | The commands: the `Context` they act on, the `CommandTable` they register in, a class of commands for each part of the editor, the project's files and the questions asked about them, and the workspace shown | every directory above it here |
| `src/Controller.*` | The app: building the parts, the ui that shows them, the routing of input, and the command search | any |
| `src/Renderer.*` | Builds each frame's passes from the viewports and the ui | any |
| `src/main.cxx` | `main`, which calls `v3d::engine::run<Controller>` | `Controller.h` |

Beside the sources:

| Path | Holds |
|---|---|
| `shaders/` | The shaded pass's vertex and fragment stages, compiled into the editor at build time |
| `data/` | Config documents: bindings, ui, window, camera profiles, layout |
| `tests/` | The `vertical3d` Boost.Test suite, with one directory for each `src/` subdirectory, and the `vertical3d_device` suite |

Every source but `main.cxx` builds into the `v3dlib_editor` library, which the executable and
both editor suites link.

Each directory under `src/` is a namespace under `v3d::editor`: `src/scene` is
`v3d::editor::scene`, and `src/store/internal` is `v3d::editor::store::internal`. The files
directly in `src/` are in `v3d::editor`. Code names what another directory holds by that
directory, as `scene::item::Mesh` from `tool`, and a class drops its directory's name from its own:
`tool::Select`, not `tool::SelectTool`. Includes are written from the repository root,
for both the api and the editor's own headers: `#include <api/brep/BRep.h>`,
`#include <vertical3d/src/scene/Scene.h>`. A header in the same directory is included by its
bare name in quotes.

The `vertical3d` suite runs without a window or a GPU. `vertical3d_device` draws the shaded
pass on a headless device, and is skipped where there is none. Run both with
`ctest --test-dir out/build/x64-Debug -R vertical3d`.

## What is not built

[TODO.md](../TODO.md#editor) lists the open work. In short:

- Every menu command has a handler. What is open is new work, not menu items left unwired.
