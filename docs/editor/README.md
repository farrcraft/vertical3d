# The editor

These pages are for someone working on the 3D modelling editor in `vertical3d/` or on the mesh
library in `api/brep`.

The editor shows four viewports of one scene over a construction grid. It creates primitive
meshes, picks objects and their parts, moves them with manipulators, undoes changes, and saves
projects. Menus, two toolbars and a file chooser are drawn with `api/ui`.

| Page | Read it to |
|---|---|
| This page | Run the editor, find your way around its source, and see what is not built |
| [Scene.md](Scene.md) | Work with meshes, the scene items that hold them, and the viewports that show the scene |
| [PickingAndManipulators.md](PickingAndManipulators.md) | Change how clicks select things, and how handles move them |
| [CommandsAndUndo.md](CommandsAndUndo.md) | Add a command, or make a change undoable |
| [Files.md](Files.md) | Change the project layout, the scene document format, or the RIB export |

## Running it

Build the `vertical3d` target, then run `out/build/x64-Debug/vertical3d/vertical3d.exe`. The
editor reads its data from the `data/` directory beside the executable and logs to `v3d.log`
in the same place. [contributing/Build.md](../contributing/Build.md) covers the build.

The default bindings are in `vertical3d/data/mappings.json`:

| Input | Command | Effect |
|---|---|---|
| Left mouse | `view::drag` | Pick, grab a handle, or drive the camera (see [Picking](PickingAndManipulators.md#picking)) |
| Hold Left Alt + drag | `view::camera::zoom` | Zoom an orthographic view; dolly a perspective one |
| Hold Left Ctrl + drag | `view::camera::truck` | Move the camera sideways and up or down |
| Middle mouse drag | `view::camera::pan` | Turn a perspective camera with an arcball; does nothing in an orthographic view |
| Hold Left Shift | `select::add` | A click adds to the selection or takes away from it (see [Picking](PickingAndManipulators.md#picking)) |
| 1, 2, 3, 4 | `create::poly::cube`, `plane`, `cylinder`, `cone` | Add a primitive at the origin |
| Create menu | `create::perspective::camera`, `ortho::camera`, `light::point`, `light::distant`, `light::spot` | Add a camera or a light at the view under the cursor, facing the way it looks |
| 5, 6, 7, 8 | `select::mask::object`, `vertex`, `edge`, `face` | Choose what a click selects |
| Q, W, E, R | `transform::select`, `translate`, `rotate`, `scale` | Choose the manipulator |
| I | `transform::poly::inset_face` | Inset the selected faces (see [Modelling operations](Scene.md#modelling-operations)) |
| J | `transform::poly::merge_vertices` | Merge the selected vertices into one |
| G, M, H | `view::show::grid`, `mesh`, `handle` | Toggle what the view under the cursor draws |
| P | `view::camera::orthographic` | Switch the view under the cursor between orthographic and perspective (see [Cameras](Scene.md#viewports)) |
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
| `src/scene/` | `Scene`, its items `Item`, `Mesh`, `Camera` and `Light`, `SceneVisitor`, the primitives in `CreatePoly`, the operations in `Modelling`, `WireframeVisitor`, `RIBExportVisitor`, `Project` and `SceneStore` |
| `src/command/` | `Command`, `CommandStack`, `CreateCommand`, `TransformCommand`, `VertexCommand`, `TopologyCommand`, `Placement` |
| `src/tool/` | `Tool`, `CameraControlTool`, `SelectTool`, `TransformTool`, `Picker`, `SelectMask` |
| `src/manipulator/` | `Manipulator` and its translate, rotate and scale subclasses, and `Target`, what they move |
| `data/` | Config documents: bindings, ui, window, camera profiles, layout |
| `tests/` | The `vertical3d` Boost.Test suite, with one directory for each `src/` subdirectory |

Everything is in the namespace `v3d::editor`. Includes are written from the repository root,
for both the api and the editor's own headers: `#include <api/brep/BRep.h>`,
`#include <vertical3d/src/scene/Scene.h>`. A header in the same directory is included by its
bare name in quotes.

The tests run without a window or a GPU. Run them with
`ctest --test-dir out/build/x64-Debug -R vertical3d`.

## What is not built

[TODO.md](../TODO.md#editor) lists the open work. In short:

- Most menu items have no handler and log themselves as unregistered.
