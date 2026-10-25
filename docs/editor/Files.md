# Projects, scene documents and RIB export

How a project is laid out, how its scene is saved and loaded, and how a scene is exported for
the offline renderer.

## Projects

A project is a directory, held by `v3d::editor::Project`:

```
<project>/
  project.json     the manifest
  scenes/          scene documents; the first one listed is the one the editor edits
  assets/          textures and imported source files, as ordinary files
  cache/           data that can be rebuilt; safe to delete
  renders/         pictures the renderer writes
  autosave/        recovery data
```

The manifest is a JSON document, written atomically by `asset::writeDocument`:

```json
{
  "version": 1,
  "format": "v3d-project",
  "name": "untitled",
  "scenes": ["scenes/main.v3d"]
}
```

- `version` is the manifest's format version, `Project::VERSION`. An older manifest is
  migrated forward one version at a time by `asset::readForward`, and a newer one is refused.
- `format` must be `v3d-project`, so a JSON file that is not a manifest is refused.
- `scenes` lists at least one scene, each a path relative to the project directory. A path that
  is absolute, or that climbs out of the directory with `..`, is refused.

A file the project refers to is named by its path relative to the project directory. A save
makes every directory of the layout that is missing, then writes the scene, then the manifest,
so a manifest never names a scene that was not written.

Background: [ADR-0100](../adr/0100-editor-a-project-is-a-directory-with-a-manifest.md)

## Scene documents

A scene document is a SQLite database, read and written by `v3d::editor::SceneStore` through
`v3dlib_database`. The extension is `.v3d`.

**The header.** The database's `application_id` is `0x56334453` ("V3DS"), so a database another
program wrote is refused. Its `user_version` is the version of the tables,
`SceneStore::SCHEMA_VERSION`, which is 1.

**The tables.** All three are `STRICT`, so a value of the wrong type is refused as it is written.

| Table | Column | Holds |
|---|---|---|
| `meta` | `key`, `value` | `next_id`: the document id the next new item is given |
| `items` | `id` | The item's document id, the primary key |
| | `position` | The item's place in the scene's order, from 0 |
| | `kind` | `mesh`, `camera` or `light` |
| | `type_version` | The version of the kind's payload and blobs |
| | `tx`, `ty`, `tz` | The translation |
| | `rx`, `ry`, `rz`, `rw` | The rotation, a quaternion |
| | `sx`, `sy`, `sz` | The scale |
| | `payload` | A JSON object of what the kind adds. The table checks that it is JSON |
| `buffers` | `item` | The item the blob belongs to. Deleting the item deletes its blobs |
| | `role` | What the blob is |
| | `count` | How many entries it holds |
| | `data` | The entries, as little-endian arrays |

**The kinds**, each at version 1:

| Kind | Payload | Blobs |
|---|---|---|
| `mesh` | `{}` | `vertices`: 3 floats each, the point. `edges`: 4 unsigned 32-bit integers each, the vertex, face, pair and next. `faces`: 3 floats and an unsigned 32-bit integer each, the normal and the first half edge |
| `camera` | `projection` (`perspective` or `orthographic`), `fov` in degrees, `extent`, `clipping` as `[near, far]` | None |
| `light` | `type` (`point`, `distant` or `spot`), `colour` as three numbers, `intensity`, `cone` and `delta` in radians | None |

**Each mesh item stores its own geometry.** Two items that share a `BRep` are written with a copy
each, and read back as two meshes.

**Topology is stored index for index.** The three blobs are the mesh's own arrays, so a save and
an open renumber nothing. `BRep::INVALID_ID` is stored as it is.

**Not stored:**

- **The run's ids.** An item's `dag::Node` id means nothing in another run. Its document id is
  stored instead.
- **Selection.** It describes what the user is doing, not what the document holds.
- **A vertex's edge.** Nothing in the editor sets it.

**Reading.** `SceneStore::read` replaces the whole scene, in the saved order and with the saved
document ids, and nothing comes back selected. It refuses the document, and leaves the scene
untouched, if:

- the file is not a database, or its `application_id` is not the editor's
- its schema version, or any item's type version, is newer than this build's, or is one no
  build wrote
- an item's kind is not one this build reads
- a payload is missing a field its version defines, has one it does not define, or has a value
  of the wrong type or out of range
- a mesh does not have exactly its three blobs, a blob's `count` does not fill it exactly, a
  number in it is not finite, or the mesh fails `BRep::validate()`
- a blob belongs to an item that is not a mesh
- `next_id` is missing or is not above every id the document holds

**Writing.** `SceneStore::write` writes in one transaction. A write that fails at any point leaves
the document as the last save left it, and a document the write was making is removed. A
database that is not a scene document, or that this build could not read, is refused rather than
overwritten.

**A save writes only what changed.** A `SceneStore` remembers the document it last wrote or read,
and for each item in it the item itself, its revision and its place in the order. A write to that
document:

- writes the row and the blobs of each item that is new, or whose revision has moved
- gives a new `position` to each item whose place alone changed, and writes nothing else of it
- deletes the row of each item the scene no longer holds, which takes its blobs with it
- writes `next_id`

A write to any other path, a document the store has neither written nor read, is written whole:
everything it held is deleted and every item is written. The next write after a write that
failed is written whole too, because the document may no longer hold what the store remembers.
`SceneStore::written()` says what the last write did.

An item's revision is `Item::revision()`, which [Scene.md](Scene.md#the-scene) describes. A change
that does not move it is not saved.

**Versions.** Two versions are kept apart:

- The schema version covers the tables. A change to a table is a new schema version, with a step
  that migrates the version before it.
- Each kind's type version covers its payload and its blobs. Any change to what a kind stores is a
  new version of that kind, with a step that migrates the version before it. A reader refuses a
  field its version does not define, so an older build refuses a newer document rather than
  dropping the fields it cannot read.

No step exists yet, because the schema and every kind are at version 1.

Background: [ADR-0101](../adr/0101-editor-a-scene-document-is-a-sqlite-database.md),
[ADR-0102](../adr/0102-editor-scene-records-are-rows-with-json-payloads.md),
[ADR-0103](../adr/0103-editor-items-have-persistent-document-ids.md)

## Open, Save and Save As

- The current project starts as the directory `project` beside the executable. A save makes it.
- **Open** (O, or Project > Load) shows the file chooser in open mode, filtered to `.json`, to
  pick a project's manifest. A successful open makes that project current and clears the history.
- **Save** (S, or Project > Save) writes to the current project without asking.
- **Save As** (Project > Save As) shows the chooser in save mode. The name typed is a new project
  directory, which must not exist yet. A successful write makes it the current project.

The chooser is `ui::shell::FileChooser`. It drives the `chooser` container laid out in
`data/vgui.json`: a file list, a name box, and Accept and Cancel buttons. Those send
`project::chooser::pick`, `project::chooser::accept` and `project::chooser::cancel`.

**The dirty flag.** The project is dirty when the history is anywhere but where it was at the last
save or open. The window caption names the project directory, and ends in ` *` when it is dirty.
[Commands and undo](CommandsAndUndo.md#the-saved-position) states the rule.

**Asking before discarding.** Open, the quit command (Escape, or the menu) and closing the window
all ask first when the project is dirty. A prompt offers three answers:

- **Save** writes the project to its current directory and then continues. A write that fails
  stops the open or the quit, and the project stays dirty.
- **Discard** continues without writing.
- **Cancel** does nothing.

A clean project does not ask. A close that arrives while the prompt is up is ignored. The prompt
is `ui::shell::Prompt`, driving the `prompt` container laid out in `data/vgui.json`. Its three
buttons send `project::prompt::answer0` to `answer2`. The tools still take clicks beside it, as
they do beside the file chooser.

## RIB export

Project > Export > rib (`project::export::rib`) writes the scene as RenderMan RIB to
`export.rib` beside the executable, replacing any file already there. The export is one-way:
nothing reads it back into the editor. The scene document stays the editor's own format, and
the offline renderer reads RIB.

`RIBExportVisitor` writes:

- a header, then a `Format` the size of the active view's camera profile
- the camera as a `Projection` (orthographic or perspective), `ScreenWindow`, `Clipping` and
  `Transform`. When exactly one camera item is selected the export renders from it, through
  `profile_of()`, which gives it the active view's picture size. Otherwise it renders from the
  active view
- inside `WorldBegin` and `WorldEnd`, every light first, as a `LightSource` of RI's standard
  shader for its type: `pointlight`, `distantlight` or `spotlight`. Each has its `intensity`,
  its `lightcolor`, and a `from` in world space where the light is. A distant or spot light has a
  `to` one unit along its +z axis, and a spot light its `coneangle` and `conedeltaangle`
- then for each mesh: an `AttributeBegin` block with an identifier name, a `ConcatTransform` of
  the mesh's matrix, and one `Polygon` per face

RI lights only what is declared after a light, so the lights come before every mesh whatever
the scene's order. A camera item is never written as an item. The export writes no materials,
so the result renders in one flat colour. RI treats a
polygon as planar and convex without checking. moya dices only the first four vertices of a
polygon, so a face with more than four vertices renders as a quad.
[offline/](../offline/README.md) covers what moya does with the file.

Background: [ADR-0023](../adr/0023-offline-rib-is-the-scene-format.md)
