# ADR-0101: Editor: a scene document is a SQLite database

**Status**: accepted
**Date**: 2026-10-24
**Supersedes**: ADR-0018
**Amends**: ADR-0041
**Documented in**: [editor/Files.md](../editor/Files.md#scene-documents)

## Context

A scene will hold large uniform data as well as structure: topology, per-corner attributes, skin
weights, keys and caches. Saving a large scene whole on every save makes autosave slow, and an
autosave that rewrites the document is a risk to it. The document must save only what changed,
survive a crash at any point of a save, and be read by tools other than the editor. At that size a
document is not read or compared by hand, so a stable specification and a reader in every
language matter more than a text encoding.

## Decision

A scene document is a SQLite database. A save writes the changes in one transaction, and the
document is the state of the last save. Autosave writes to a separate recovery database in the
project's autosave directory, never to the document. SQLite is reached through one api library,
`v3dlib_database`.

## Alternatives

### A JSON document, rewritten whole and renamed into place
- **For**: readable and diffable, every other document in the tree is JSON, and the atomic write
  and the migration chain exist.
- **Against**: a save costs the whole document, so autosave grows with the scene. Bulk data as
  text is several times its binary size, and a parse tree in memory is several times more.
- **Rejected because**: it cannot save incrementally.

### LMDB
- **For**: a copy-on-write B-tree that survives a crash without a log, and very fast reads.
- **Against**: a bare key-value store with no schema, no query and no constraint, a file that
  never shrinks, and a format defined only by its implementation.
- **Rejected because**: SQLite gives the same safety with a specified format and a schema.

### LevelDB or RocksDB
- **For**: fast incremental writes through a log, and compression.
- **Against**: the store is a directory that compacts itself in the background, a write is atomic
  only within one batch, and nothing but the library reads it.
- **Rejected because**: they are built for servers, not for a document a user copies.

### A chunked file of our own, appended to and compacted
- **For**: full control of the layout, and no dependency.
- **Against**: crash safety, compaction and a specification would all be ours to build and keep
  right, and no other tool would read the file.
- **Rejected because**: SQLite already is that, specified and tested.

## Consequences

- **Gains**:
  - A save writes only what changed, and a crash at any point leaves the last save whole.
  - The file format is specified and stable, and every language can read it.
  - Constraints and foreign keys in the schema check a document as it is written.
- **Costs**:
  - The document is binary, so it cannot be read or compared without a SQLite tool.
  - SQLite is a dependency of the tree.
  - "Save" and the document's file are no longer one rename, so a save in progress has to hold a
    write transaction, and only one process may write a document at a time.
- **Revisit when**: the scene's bulk data outgrows a single file, or two writers need to edit one
  document at the same time.
