# Editor Modelling

What the editor needs to make a model rather than place one. Today it creates primitive meshes,
picks an object or one of its parts, moves whole objects with manipulators, undoes those moves,
and saves and exports a project. It cannot change a mesh's shape, select more than one thing, or
warn before discarding work.

The gaps are each listed in [TODO.md](../TODO.md#editor). This roadmap orders them.
[The editor](../editor/README.md) describes the app.

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
- **[`SelectTool`](../../vertical3d/src/tool/SelectTool.h)** selects by clicking, under a mask of
  object, vertex, edge or face. One object, and one component of a kind, is selected at a time.
  `Scene::selection()` returns a single mesh.
- **Component modes select a part, and the manipulator then moves the whole object.**
- **[`ViewLayout`](../../vertical3d/src/view/ViewLayout.h)** divides the window into a fixed tree
  of splits. Each split divides its area evenly.
- **Most menu commands have no handler.** They fall into five groups:
  - the fixed camera views and the projection switch
  - the importers
  - animation playback
  - face culling and shading modes
  - the masks and show toggles for cameras, lights, curves and handles

  A camera or a
  light has a flag on the view, but nothing draws either.

## What it needs

### 1. A dirty flag

- The stack records the position it was at when the project was last saved or opened.
- The project is dirty when the stack is at any other position.
- The stack is bounded, so a save marker can fall off its far end. Once it has, the project stays
  dirty until the next save.
- Load and quit ask before discarding a dirty project.

This comes first because every later step adds more ways to lose work.

### 2. Multiple selection

- Shift-click adds to or removes from the selection, for objects and for components.
- A rubber band selects every object or component inside a screen rectangle, through the same
  pick queries the click uses.
- `Scene::selection()` returns a set. The manipulators act on the set about a shared pivot.
- Selection is not part of the undo history, and stays so.

### 3. Modelling operations

- The first operation moves the selected vertices, edges or faces. The manipulator moves the
  components rather than the whole object.
- A component move is recorded as a command that holds the moved vertices' old and new positions.
- Later operations change topology: extrude, inset, split and merge. They need `BRep` operations
  that keep the mesh valid, each tested with `BRep::validate`.
- Whether a topology change records the whole mesh before and after, or the operation and its
  inverse, needs an ADR.

### 4. The missing command handlers

Take each group from [What exists](#what-exists) as one piece of work, in any order:

- **Camera views and projection.** These need nothing new.
- **Shading and culling modes.** These are view state.
- **Cameras and lights in the scene.** Drawing them comes first. Their masks and show toggles
  follow.
- **Importers.** Each format is separate. An importer creates meshes through the command stack, so
  an import can be undone.
- **Playback.** This needs animation in the scene, which nothing in the editor has. It waits on a
  decision about what the editor animates.

### 5. Resizable panes

A split records each child's share, and dragging a divider changes it. Whether the shares are
written back to the layout's config is for the step to decide. This has no order against the
rest.

## The ordering

```
1 Dirty flag ──> 2 Multiple selection ──> 3 Modelling operations
                                                │
4 Command handlers   (camera, shading: any time; importers after 1; playback undecided)
5 Resizable panes    (no order)
```

## What this needs decided

- **How a topology change is recorded for undo**: step 3, an ADR.
- **What the editor animates**: before the playback commands, an ADR.

## Verification

- The dirty flag, the selection set and every `BRep` operation are headless, and are tested in the
  vertical3d and brep suites.
- A modelling command is tested by applying it, undoing it, and comparing the mesh with the
  original.
- Drawing and dragging are checked by running the editor with the validation layer on.

## Triggers

There is no consumer outside the editor. The roadmap is due when the editor is picked up again.
