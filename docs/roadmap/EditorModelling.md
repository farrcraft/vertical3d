# Editor Modelling

What the editor needs to make a model rather than place one. Today it creates primitive meshes,
picks an object or one of its parts, moves whole objects with manipulators, undoes those moves,
and saves and exports a project. It cannot change a mesh's shape, select more than one thing, or
warn before discarding work.

The gaps are each listed in [TODO.md](../TODO.md#editor). This roadmap orders them.
[The editor](../editor/README.md) describes the app.

The [EditorModelling plan](../plans/EditorModelling.md) takes up sections 1, 2, 3 and 5, and the
camera views of section 4. The rest of section 4 stays here.

## What exists

- **[`CommandStack`](../../vertical3d/src/command/CommandStack.h)** holds up to 64 completed
  changes, and undoes and redoes them. A command records a change after it is made, and has no
  `execute()`. Opening a project clears the stack.
  [Commands and undo](../editor/CommandsAndUndo.md) states the rules.
- **The only undoable changes are creating a mesh and moving an object.**
  [`TransformCommand`](../../vertical3d/src/command/TransformCommand.h) records an object's
  placement. Nothing records a change to a mesh's vertices.
- **[`brep::BRep`](../../api/brep/BRep.h)** is a half-edge mesh. It can add vertices, edges and
  faces, validate itself and report its bound. It has no operation that changes an existing mesh.
  - `BRep::validate` checks only that each reference is in range. It does not check that pairs
    are mutual, that a `next` chain closes, or that a loop's half edges name its face.
  - A `BRep` cannot be copied, because a `dag::Node` cannot.
  - A face stores its normal, and nothing recomputes it when a vertex moves.
- **Left Shift is the camera's pan modifier.** While a camera modifier is held, a press drives the
  camera and never reaches `SelectTool`. The bindings cannot express a chord.
- **The engine quits without asking the app.** It registers `ui::quit` itself, and a closed window
  calls `Engine::quit()` directly. `api/ui/shell` has no prompt to ask a question with.
- **[`SelectTool`](../../vertical3d/src/tool/SelectTool.h)** selects by clicking, under a mask of
  object, vertex, edge or face. One object, and one component of a kind, is selected at a time.
  `Scene::selection()` returns a single mesh.
- **Component modes select a part, and the manipulator then moves the whole object.**
- **[`ViewLayout`](../../vertical3d/src/view/ViewLayout.h)** divides the window into a fixed tree
  of splits. Each split divides its area evenly.
- **Most menu commands have no handler.** They fall into nine groups:
  - the fixed camera views and the projection switch
  - the importers
  - animation playback
  - face culling and shading modes
  - the masks and show toggles for cameras, lights, curves and handles
  - the modelling operations: `transform::poly::extrude_face`, `split_edge` and `split_face`
  - the Render menu: `render::view`, `render::batch` and `render::settings`
  - `create::group`, `create::ortho::camera` and `create::perspective::camera`
  - the camera toolbar's `camera::*` buttons

  A camera or a light has a flag on the view, but nothing draws either. Every fixed view already
  has a camera profile in `data/cameras.json`.
- **The editor draws lines only.** There is no shaded draw of a mesh.

## What it needs

### 1. A dirty flag

- The stack records the position it was at when the project was last saved or opened.
- The project is dirty when the stack is at any other position.
- The stack is bounded, so a save marker can fall off its far end. Once it has, the project stays
  dirty until the next save.
- Load and quit ask before discarding a dirty project. That needs a prompt in `api/ui/shell` with
  three answers, and an engine whose every path to quitting asks the app first.

This comes first because every later step adds more ways to lose work.

### 2. Multiple selection

- Shift-click adds to or removes from the selection, for objects and for components. The pan
  modifier moves off Left Shift first, and Shift becomes a held command of its own.
- A rubber band selects every object or component inside a screen rectangle, through the same
  pick queries the click uses.
- `Scene::selection()` returns a set. The manipulators act on the set about a shared pivot. Where
  that pivot is moves the handles off the object's origin, which ADR-0015 forbids, so it needs an
  ADR.
- Selection is not part of the undo history, and stays so.

### 3. Modelling operations

- The first operation moves the selected vertices, edges or faces. The manipulator moves the
  components rather than the whole object.
- A component move is recorded as a command that holds the moved vertices' old and new positions.
  It recomputes the normals of the faces around each moved vertex.
- Later operations change topology: extrude, inset, split and merge. They need `BRep` operations
  that keep the mesh valid. `BRep::validate` is too weak to test them against, so a strict check
  comes first: mutual pairs, closed loops, and each loop's half edges naming its face.
- How a topology change is recorded for undo needs an ADR. The undo must restore the mesh index
  for index, because a later command can hold indices into it. A snapshot cannot be a copy of the
  `BRep`, which cannot be copied.

### 4. The missing command handlers

Take each group from [What exists](#what-exists) as one piece of work, in any order:

- **Camera views and projection.** These need nothing new.
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

### 5. Resizable panes

A split records each child's share, and dragging a divider changes it. Whether the shares are
written back to the layout's config is for the step to decide. This has no order against the
rest.

## The ordering

```
1 Dirty flag ──> 2 Multiple selection ──> 3 Modelling operations
                                                │
4 Command handlers   (camera: any time; shading after a shaded draw; importers after 1; playback undecided)
5 Resizable panes    (no order)
```

## What this needs decided

- **Where the handles pivot for a set and for components**: step 2, an ADR amending ADR-0015.
- **How a topology change is recorded for undo**: step 3, an ADR.
- **What the editor animates**: before the playback commands, an ADR.

## Verification

- The dirty flag, the selection set and every `BRep` operation are headless, and are tested in the
  vertical3d and brep suites.
- A modelling command is tested by applying it, checking the result with the strict check,
  undoing it, and comparing the mesh with the original index for index.
- Drawing and dragging are checked by running the editor with the validation layer on.

## Triggers

There is no consumer outside the editor. The roadmap is due when the editor is picked up again.
