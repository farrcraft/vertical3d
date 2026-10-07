# Project files and RIB export

How a project is saved and loaded, and how a scene is exported for the offline renderer.

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

Background: [ADR-0018](../adr/0018-editor-projects-saved-as-json-with-exact-topology.md),
[ADR-0041](../adr/0041-files-write-documents-atomically.md)

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
[offline/](../offline/README.md) covers what moya does with the file.

Background: [ADR-0023](../adr/0023-offline-rib-is-the-scene-format.md)
