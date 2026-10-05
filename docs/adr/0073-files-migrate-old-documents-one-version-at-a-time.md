# ADR-0073: Files: migrate old documents one version at a time

**Date**: 2026-10-04
**Status**: accepted
**Deciders**: Joshua Farr

## Context

[ADR-0041](0041-files-write-documents-atomically.md) settled how a document is written
and left reading to each caller. Every document a game writes outlives the build that wrote it,
and the tree reads its two versioned documents two different ways. `engine::Settings` refuses a
newer file, but it reads an older or unversioned one as it is, and it skips the check silently
when the version is not a number. The editor's project refuses anything but its own version.
Neither has anything to say about an old document a later build could still read.

cozy has written that walk for its save, under its own ADR-0003, as a chain of migrations with
twenty cases behind it. Nothing in its walk is specific to cozy. retcon's design states the same
rule for its Boost.Serialization saves, which are not JSON and stay its own.

## Decision

**A JSON document carries an integer `"version"` at its root, and is read forward by a chain in
which step `n` takes version `n + 1` to `n + 2`.** `asset::readForward()` walks a copy, stamps
the version after each step, and replaces the caller's document only when every step succeeded.
It answers one of four readings, and what to do with each is the caller's:

* **current**: the document is already at this build's version;
* **migrated**: it was walked forward, and may be written back;
* **newer**: a later build wrote it. Nothing is read, and the caller must not overwrite it;
* **refused**: it has no version, the version is not a whole number or is below one, the chain is
  missing a step, or a step failed. The document is untouched.

## Alternatives Considered

### Alternative 1: A chain of single steps, walked on a copy — **chosen**
- **Pros**: A new version costs one step that knows only the version before it. A document from
  any earlier version is read by the same steps, run in order, and the walk cannot leave it half
  migrated, because the copy is only assigned back at the end. It is cozy's shape, and cozy's
  cases come across as they are.
- **Cons**: A document many versions old runs every step between, including steps that undo
  each other's work. The chain only grows, and a step cannot be deleted while any document of the
  version before it might exist.
- **Why not**: n/a — chosen.

### Alternative 2: A migration from each old version straight to the current one
- **Pros**: One step per document, and no step to undo another's work.
- **Cons**: Every new version rewrites every existing migration, so the work grows with the
  square of the versions.
- **Why not**: The cost lands on every release, not just the one that changes the format.

### Alternative 3: The version in the file name
- **Pros**: A build can see which documents it can read without parsing any of them, and an old
  document stays beside a migrated one.
- **Cons**: [ADR-0041](0041-files-write-documents-atomically.md)'s rename replaces one
  path with itself, so a migration becomes a write under a new name and a delete under the old
  one, and a crash between the two leaves both. A player's directory fills with versions.
- **Why not**: It gives up the atomic write for something the root key already says.

### Alternative 4: Leave the walk to each game
- **Pros**: Nothing to decide here, and each game keeps exactly the rules it wants.
- **Cons**: `Settings` is in this tree and is a versioned document with no walk. Every game would
  write the same twenty cases, and the first to get the copy wrong would leave a player with a
  half-migrated save.
- **Why not**: The walk has no part that differs between games. What differs is the steps.

## Consequences

### Positive
- `Settings` refuses an unversioned document, and a version that is not a number, instead of
  reading either as it is.
- The editor's project takes a migration the day it needs one, with nothing to restructure.
- cozy replaces its walk with this one and keeps its steps.

### Negative
- A missing version is now refused. Every settings file this tree has written carries one, so
  only a file edited by hand is affected, and it falls back to defaults.
- A step is a function over a `boost::json::object`, so a migration that renames a key or moves a
  section is plain code with no schema to check it against. A test per step is the only guard.

### Risks
- **A step that mutates something other than the document it was handed** escapes the copy. The
  walk cannot detect that, so a step must read and write only its argument.
- **A newer document that a build overwrites anyway** loses what the later build stored. The
  reading says *newer*, but refusing the write is still the caller's job, as `Settings`'s
  `writable()` already does. The escape hatch, if a caller keeps getting it wrong, is for the
  writer to refuse a newer document itself, which would move the rule into `writeDocument` and
  out of the callers.
