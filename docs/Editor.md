# The Editor

`vertical3d/` is the modelling tool and the most complete app in the tree: four viewports of a
scene over a construction grid, with manipulators, undo, a command directory, menus, two
toolbars and project persistence.

## Layout

Sources group into `src/view/`, `src/scene/`, `src/command/`, `src/tool/` and
`src/manipulator/`. `Controller`, `Renderer` and `main` stay at the root of `src/` as the app
shell, and `tests/` mirrors the five subdirectories. Includes are relative, so a file under one
of them reaches the api as `../../../api/`.

## Meshes

`api/brep` holds one mesh representation, `BRep`, which is half-edge and is what the editor
models with.

A mesh names its own parts with `brep::Index`, one `uint32_t` for a vertex, a half edge or a
face, since all three are offsets into a `BRep`'s arrays. `INVALID_ID` is `1 << 31`, and **its
value is part of the project file format**, so changing it would invalidate documents already
written.

**The mesh keeps its own rules.** `BRep::validate()` says whether every reference a mesh holds is
to something it holds, which is what the project loader refuses a document over. Walking a face
is `brep::faceLoop()`, which ends rather than spins on a ring that does not close, and what a
segment of it draws, which half of a pair stands for the edge and whether an edge is selected are
`api/brep/Topology.h` too - const, and by `Index`. The editor walks a mesh through them rather than
along `next()` itself.

## What the ADRs settle

Read the record rather than inferring the rule from the code.

| Area | Record |
|---|---|
| A mesh is a dag node with a transform; the scene belongs to the editor | [ADR-0013](adr/0013-mesh-is-a-dag-node.md) |
| Picking is a CPU ray cast, with screen space proximity for components | [ADR-0014](adr/0014-picking-is-a-cpu-ray-cast.md) |
| Manipulators write the object transform, and are an overlay pass | [ADR-0015](adr/0015-manipulators-write-the-object-transform.md) |
| Undo records what has already happened; one gesture is one command | [ADR-0016](adr/0016-undo-records-what-has-already-happened.md) |
| A command is a name in a context; the directory is the editor's | [ADR-0017](adr/0017-a-command-is-a-name-in-a-context.md) |
| A project is JSON, and stores topology verbatim | [ADR-0018](adr/0018-a-project-is-json-and-stores-topology-verbatim.md) |
| The ui is laid out by what draws it, and hit tested against those bounds | [ADR-0019](adr/0019-the-ui-is-laid-out-by-what-draws-it.md) |
| A theme is data, and the app resolves the images it names | [ADR-0020](adr/0020-a-theme-is-data-and-the-app-resolves-its-images.md) |

## Three things the ADRs do not say

- **Multiple viewports are several passes over one frame.** `Renderer` builds one `Pass` per
  `ViewPort`, each with its own region, camera and clear. `data/layout.json` decides the split.
- **A project is chosen through `ui::shell::FileChooser`.** `project::load` and
  `project::saveAs` put up the `chooser` container `data/vgui.json` lays out, and
  `project::save` writes to whichever project was last opened or saved as. That starts as
  `project.json` beside the executable. `project::export::rib` still writes a fixed
  `export.rib` beside it.
- **`Tool` stays in the editor.** No game holds a gesture open across events.

## RIB export

One way only, per [ADR-0023](adr/0023-rib-is-the-offline-scene-description.md).
`RIBExportVisitor` writes topology and a placement per mesh from the active view's camera, and
nothing reads it back. The scene has no lights and no materials, so what it produces renders in
one flat colour. [OfflineRenderers.md](OfflineRenderers.md) covers what the offline renderer
does with the file.

## What is unbuilt

[TODO.md](TODO.md#editor) carries it: 55 of the menu's 75 commands log themselves and do
nothing, there is no modelling operation, and selection is one object at a time.
