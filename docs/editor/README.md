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
| [Animation.md](Animation.md) | Key item properties, and play them back with the Timeline menu |
| [Rendering.md](Rendering.md) | Render the scene with moya, in the render window or as a batch of frames |
| [Files.md](Files.md) | Change the project layout, the scene document format, or the RIB export |

## Running it

Build the `vertical3d` target, then run `out/build/x64-Debug/vertical3d/vertical3d.exe`. The
editor reads its data from the `data/` directory beside the executable and logs to `v3d.log`
in the same place. [contributing/Build.md](../contributing/Build.md) covers the build.

The default bindings are in `vertical3d/data/mappings.json`:

| Input | Command | Effect |
|---|---|---|
| Left mouse | `view::drag` | Pick, grab a handle, or drive the camera (see [Picking](PickingAndManipulators.md#picking)) |
| Hold Left Alt + drag | `view::camera::dolly` | Dolly a perspective view; zoom an orthographic one |
| Hold Left Ctrl + drag | `view::camera::truck` | Move the camera sideways and up or down |
| Middle mouse drag | `view::camera::pan` | Turn a perspective camera with an arcball; does nothing in an orthographic view |
| Camera Manipulators menu | `camera::truck`, `dolly`, `zoom`, `pan`, `pitch` | Latch a camera move, so a left drag with no modifier drives the camera (see [Driving a camera](PickingAndManipulators.md#driving-a-camera)) |
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
| `src/view/` | The viewports and their layout, the construction plane, and the shaded pass |
| `shaders/` | The shaded pass's vertex and fragment stages, compiled into the editor at build time |
| `src/scene/` | The scene and its kinds of item, animation and render settings, the primitives and modelling operations, wireframe drawing, RIB export, and the project files |
| `src/command/` | The undo records and the stack that holds them |
| `src/tool/` | The camera, select and transform tools, and picking |
| `src/manipulator/` | The translate, rotate and scale handles, and what they move |
| `src/render/` | Rendering the scene with moya: a shot, the render window, live render and batches |
| `data/` | Config documents: bindings, ui, window, camera profiles, layout |
| `tests/` | The `vertical3d` Boost.Test suite, with one directory for each `src/` subdirectory, and the `vertical3d_device` suite |

Everything is in the namespace `v3d::editor`. Includes are written from the repository root,
for both the api and the editor's own headers: `#include <api/brep/BRep.h>`,
`#include <vertical3d/src/scene/Scene.h>`. A header in the same directory is included by its
bare name in quotes.

The `vertical3d` suite runs without a window or a GPU. `vertical3d_device` draws the shaded
pass on a headless device, and is skipped where there is none. Run both with
`ctest --test-dir out/build/x64-Debug -R vertical3d`.

## What is not built

[TODO.md](../TODO.md#editor) lists the open work. In short:

- Six menu items have no handler, and log themselves as unregistered: the four importers and
  the curve and handle select masks.
