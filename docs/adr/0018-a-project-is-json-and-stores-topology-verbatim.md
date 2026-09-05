# ADR-0018: Project Persistence — A Project Is JSON, And The Topology Is Stored Verbatim

**Date**: 2026-09-02
**Status**: accepted
**Deciders**: Joshua Farr

## Context

The editor makes meshes, selects them, moves them and takes any of it back, and loses all of
it when the window closes. Project persistence is item 9 of
[the rigel survey’s delete list](../audits/completed/RigelSurvey.md) and one of the two things left in
[phase 6](../plans/completed/Modernization.md) before `rigel/` can go.

Rigel had a format: `ProjectCommandSet::read`/`write` over libxml++, a `<project>` of
`<scene>`s of `<mesh>`es, each mesh carrying a transform and then flat lists of vertices,
half edges and faces addressed by index. That shape is right and survives; the encoding does
not. The XML library is gtkmm's, `vault/quantumxml` was archived when the JSON config work
replaced it, and nothing in the tree parses XML any more — `api/asset` parses JSON through
boost::json, and every config file the engine reads is one.

Two things about the model constrain what a file can carry. A `dag::Node` id comes from a
process wide counter, so it identifies a mesh within a run and not between them. And
`BRep::addFace(points, normal)` welds vertices by search and pairs edges by search, so
rebuilding a mesh through it renumbers everything it holds.

## Decision

A project is a **JSON document**, read and written by `v3d::editor::Project`, holding a
version, a name and one array of meshes. Each mesh is a placement, an array of points, an
array of half edges naming indices, and an array of faces — rigel's shape in the tree's own
encoding.

The **topology is stored as it stands** rather than as the calls that would rebuild it: the
three arrays are written and read back index for index, so a round trip renumbers nothing.
Three things are deliberately not stored:

- **Ids.** A saved id would collide with a mesh already loaded, and the two meshes would be
  the same object as far as the selection model is concerned. Meshes read from a file are new
  nodes.
- **Selection.** It is where the user is, not what the document holds, which is the same rule
  [ADR-0016](0016-undo-records-what-has-already-happened.md) applies to history.
- **A vertex's edge.** `Vertex::edge_` has no writer in any construction path and no reader
  outside its own test, so storing it would store whatever the constructor left behind. It is
  initialised to `INVALID_ID` as of this change rather than left indeterminate.

**Reading replaces the document and clears the history**, and refuses a file it does not
fully understand rather than loading as far as it gets — an index naming a vertex, edge or
face the mesh does not hold is what the wireframe and the picker would walk off the end of.
`INVALID_ID` stays legal wherever a reference may be absent, because an edge on a boundary
has no pair.

There is **no file chooser in the tree**, so `project::load` and `project::save` work on one
document at a fixed path beside the executable.

## Alternatives Considered

### Alternative 1: JSON, topology verbatim — **chosen**
- **Pros**: the one encoding the tree already parses; a round trip is the identity; the file
  is diffable and hand-editable.
- **Cons**: a document is as large as its mesh — a cube is 5KB — and the format is tied to
  the half edge representation rather than to anything portable.
- **Why**: the alternatives each give up something the editor needs today for a benefit it
  cannot yet use.

### Alternative 2: XML, as rigel had it
- **Pros**: the reader and writer already exist and could be translated line for line.
- **Cons**: needs an XML library the tree deleted; a second document encoding beside the
  JSON one every config file uses.
- **Why not**: nothing else in the repository would read it, and `api/asset` would grow a
  parser to serve exactly one file.

### Alternative 3: Store the face loops and rebuild through `addFace`
- **Pros**: a much smaller file, and one that is not tied to the half edge representation —
  it is close to what an obj or an importer would hand over.
- **Cons**: `addFace` welds and pairs by search, so the mesh that comes back is not the mesh
  that went out: indices move, and anything that names one — a saved selection, a modelling
  record, a future per-face material — names something else.
- **Why not**: the point of saving is to get the same document back. This is the right shape
  for an *import* format and the wrong one for the native file.

### Alternative 4: A binary format
- **Pros**: smaller and faster to read, and exact by construction rather than by relying on
  a float round trip.
- **Cons**: unreadable, undiffable, and versioning it is work that JSON gives away.
- **Why not**: a mesh at this scale costs nothing to parse, and every other data file in the
  repository is text a person can open.

### Alternative 5: An array of scenes, as rigel's `Project` had
- **Pros**: files written now would still read once the editor holds more than one scene.
- **Cons**: an array that is always of length one, and a nesting level every reader has to
  walk past.
- **Why not**: the editor has one `Scene` and no notion of a second. The `version` field is
  what makes adding the level cheap when there is something to put in it.

## Consequences

### Positive
- The editor opens and saves a document, which is the last of the phase 6 "done when"
  clause that was still missing, and item 9 of the survey's delete list.
- A round trip is exact: the same points, the same indices, the same placement. The suite
  pins it against a cube and a cone.
- The file is legible. `Project` writes it indented, keeping a point, a vector, an edge and a
  face each on one line, and writes numbers as floats — boost::json serializes a double as
  `0E0`, which is valid, unreadable, and impossible to hand edit.
- A malformed or dangling file is refused with a logged reason and the document in memory is
  left alone, so a failed open does not also lose the work.

### Negative
- The format is the half edge representation written down. A change to `BRep`'s members is a
  change to the file format, and the `version` field is the whole of the migration story.
- One document at a fixed path is not a document model. There is no "save as", no recent
  file, no dirty flag, and nothing warns before a load replaces unsaved work.
- Reading clears the history, so a load cannot be undone.
- A file is large for what it holds, because every half edge is four indices of its own.

### Risks
- **The float round trip.** Points are written through `std::to_chars` at float precision and
  read back as doubles narrowed to float, which is exact for every value the editor writes.
  A hand-written file carrying a value that is not representable as a float is silently
  rounded — the escape hatch is that the reader is the one place the narrowing happens.
- **Validation is structural, not semantic.** The reader rejects an index that names nothing;
  it does not check that a face's loop closes or that a pair is mutual. A file that is
  consistent but not manifold loads, and whatever walks it decides what that means.
- **The fixed path.** `project::load` overwrites the scene from `project.json` beside the
  executable with no prompt, which is a surprising thing for a key press to do. It is
  deliberate placeholder behaviour, like the create and select mask keys, and goes when the
  menus of survey item 1 land with a file chooser behind them.
