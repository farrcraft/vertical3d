# Editor Modelling

What the editor still needs to make a model rather than place one. It now asks before it
discards work, selects sets of objects and components, moves components, and splits, extrudes,
insets and merges with undo. Its views switch between the fixed cameras and its panes resize.
What is left is the menu commands that still have no handler.

The gaps are each listed in [TODO.md](../TODO.md#editor). This roadmap orders them.
[The editor](../editor/README.md) describes the app. The
[EditorModelling plan](../plans/completed/EditorModelling.md) built the rest, and records why it
was ordered as it was.

## What exists

- **[`CommandStack`](../../vertical3d/src/command/CommandStack.h)** holds up to 64 completed
  changes, knows the position the project was last saved at, and undoes and redoes. A creation, an
  object move, a component move and a topology change are each a command.
  [Commands and undo](../editor/CommandsAndUndo.md) states the rules.
- **[`brep::BRep`](../../api/brep/BRep.h)** is a half-edge mesh with a strict check, walks around
  a vertex, and snapshots. `api/brep/Operations.h` splits, extrudes, insets and merges.
- **The scene holds meshes only.** A camera or a light has a flag on the view, but nothing draws
  either, and the scene cannot hold one.
- **The editor draws lines only.** There is no shaded draw of a mesh.
- **45 of the menu's 80 commands have no handler.** They fall into six groups:
  - the importers
  - animation playback
  - face culling and shading modes
  - the masks and show toggles for cameras, lights, curves and handles
  - the Render menu: `render::view`, `render::batch` and `render::settings`
  - `create::group`, `create::ortho::camera` and `create::perspective::camera`, and the camera
    toolbar's `camera::*` buttons

## What it needs

Take each group as one piece of work, in any order:

- **Shading and culling modes.** These need a shaded draw of the mesh, which the editor's renderer
  does not have. The mode itself is view state.
- **Cameras and lights in the scene.** Drawing them comes first. Their masks and show toggles
  follow.
- **Importers.** Each format is separate. An importer creates meshes through the command stack, so
  an import can be undone.
- **Playback.** This needs animation in the scene, which nothing in the editor has. It waits on a
  decision about what the editor animates.
- **The Render menu.** This hands the scene to moya. The RIB export already writes it.
- **Groups and scene cameras.** `create::group` needs a scene that holds a hierarchy, and a camera
  in the scene needs the scene to hold something other than meshes.

## What this needs decided

- **What the editor animates**: before the playback commands, an ADR.

## Verification

- Anything headless is tested in the vertical3d suite.
- Drawing is checked by running the editor with the validation layer on.

## Triggers

There is no consumer outside the editor. The roadmap is due when the editor is picked up again.
