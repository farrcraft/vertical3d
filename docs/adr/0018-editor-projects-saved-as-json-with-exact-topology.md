# ADR-0018: Editor: projects saved as JSON with exact topology

**Status**: amended
**Date**: 2026-09-02
**Amended by**: [ADR-0041](0041-files-write-documents-atomically.md)
**Documented in**: [editor/](../editor/README.md)

## Context

The editor needs to save and open its document. Every data file in the tree is JSON, read
through `api/asset` with boost::json, and nothing parses XML. A `dag::Node` id identifies a mesh
only within one run. `BRep::addFace()` reuses vertices and pairs edges by search, so rebuilding a
mesh through it can renumber its parts. Anything that names a part by index, such as a selection
or a future per-face material, needs those numbers to survive a save.

## Decision

A project is a JSON document, read and written by `v3d::editor::Project`, holding a version, a
name and a list of meshes, each with its placement and its vertex, half-edge and face arrays.
The topology is stored index for index, so a save and an open renumber nothing, and mesh ids
and selection are not stored. Reading refuses a file it does not fully understand and leaves
the open document untouched.

## Alternatives

### XML
- **For**: a known shape for a mesh file, and a reader and writer could be translated directly.
- **Against**: it needs an XML library the tree does not have, and a second document encoding.
- **Rejected because**: nothing else in the repository would read XML.

### Store face loops and rebuild through `addFace`
- **For**: a much smaller file, not tied to the half-edge representation, and close to what an
  importer produces.
- **Against**: the mesh that comes back is numbered differently, so anything that names a part
  names something else.
- **Rejected because**: the native file must give back the same document. This shape suits an
  import format.

### A binary format
- **For**: smaller and faster to read, and exact without relying on a float round trip.
- **Against**: unreadable and not diffable, and versioning it is extra work.
- **Rejected because**: a mesh at this scale costs nothing to parse, and every other data file in
  the repository is text.

### An array of scenes
- **For**: files written now would still read if the editor later held several scenes.
- **Against**: an array always of length one, and an extra level every reader walks past.
- **Rejected because**: the editor has one scene. The version field makes adding the level cheap
  later.

## Consequences

- **Gains**:
  - A save and an open give back the same points, indices and placement.
  - The file is readable, diffable and editable by hand.
  - A malformed file is refused with a logged reason, and the open document is kept.
- **Costs**:
  - The format is the half-edge representation written down, so a change to `BRep`'s members is
    a format change, handled by version migration per
    [ADR-0073](0073-files-migrate-old-documents-one-version-at-a-time.md).
  - A file is large for what it holds, because every half edge stores four indices.
  - Opening a project clears the undo history.
  - Validation is structural: a file whose indices are valid but whose surface is not manifold
    still loads.
- **Revisit when**: the editor holds more than one scene, or files grow large enough for size or
  parse time to matter.
