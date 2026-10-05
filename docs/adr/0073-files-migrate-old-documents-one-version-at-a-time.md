# ADR-0073: Files: migrate old documents one version at a time

**Status**: accepted
**Date**: 2026-10-04
**Documented in**: [api/Assets.md](../api/Assets.md)

## Context

Every document a game writes, such as settings, a save or a project, outlives the build that
wrote it. A later build has to read an older document, refuse a newer one, and never leave a
document half migrated. Walking a document from its version to the current one is the same in
every game; only the individual changes differ. Documents are written atomically by a rename onto
the same path ([ADR-0041](0041-files-write-documents-atomically.md)).

## Decision

A JSON document carries an integer `"version"` at its root and is read forward by a chain of
steps, each taking one version to the next. `asset::readForward()` walks a copy of the document
and replaces the caller's only when every step has succeeded. What to do with the result, such as
writing a migrated document back or refusing to overwrite a newer one, is the caller's.

## Alternatives

### A migration from each old version straight to the current one
- **For**: one step per document, and no step undoes another's work.
- **Against**: every new version rewrites every existing migration, so the work grows with the
  square of the number of versions.
- **Rejected because**: the cost lands on every release, not only the one that changes the format.

### The version in the file name
- **For**: a build can see which documents it can read without parsing them, and an old document
  stays beside its migrated copy.
- **Against**: a migration becomes a write under a new name and a delete of the old one, and a
  crash between the two leaves both. A player's directory fills with versions.
- **Rejected because**: it gives up the atomic write for something a root key already says.

## Consequences

- **Gains**:
  - A new version costs one step that knows only the version before it.
  - A document from any earlier version is read by the same steps in order, and is never left half
    migrated.
  - A document with no version, or a version that is not a whole number, is refused rather than
    read as it is.
- **Costs**:
  - A document many versions old runs every step in between, including steps that undo each
    other's work.
  - The chain only grows. A step cannot be deleted while a document of the version before it might
    exist.
  - A step is plain code over a `boost::json::object`, with no schema to check it. A test per step
    is the only guard.
  - A step that changes anything other than the document it was handed escapes the copy, and the
    walk cannot detect it.
  - Refusing to overwrite a newer document is each caller's job.
- **Revisit when**: callers keep overwriting newer documents, which would move that refusal into
  the writer itself.
