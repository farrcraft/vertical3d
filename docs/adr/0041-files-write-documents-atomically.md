# ADR-0041: Files: write documents atomically

**Status**: accepted
**Date**: 2026-09-07
**Amends**: [ADR-0018](0018-editor-projects-saved-as-json-with-exact-topology.md)
**Documented in**: [api/Assets.md](../api/Assets.md)

## Context

Games and the editor write documents a player cares about: projects, settings and saves. A writer
that truncates its target before writing destroys the previous document first, so a crash or a
full disk during the write leaves neither copy. Each writer that solves this for itself also writes
its own JSON pretty-printer, because `boost::json::serialize` puts a document on one line. Every
document the tree writes is one a person will open and compare.

## Decision

`api/asset` writes a document to a temporary file beside its target and renames it onto the
target, so the previous document survives every failure except the rename itself. The library
writes bytes and serializes a `boost::json::value`, and what the document holds stays the
caller's. JSON is always written indented, with no compact mode.

## Alternatives

### Keep a `.bak` of the previous document, then write in place
- **For**: the old document is recoverable, and a player can see that it is.
- **Against**: two windows for a crash instead of one, during the backup copy and during the
  write. Recovery is manual, so it helps only a player who knows the convention, and the
  directory holds two copies of everything.
- **Rejected because**: it turns a lost file into one the player has to know how to restore. The
  rename leaves nothing to restore.

### A temporary file in the system temp directory
- **For**: no extra file appears beside the target while it is written.
- **Against**: the temp directory may be on another volume, and a rename across volumes is a copy,
  which is not atomic.
- **Rejected because**: atomicity is the reason for the rename.

### `std::filesystem` for the rename rather than `boost::filesystem`
- **For**: standard, with the same replace-on-rename behaviour on Windows.
- **Against**: `api/asset` already takes `boost::filesystem` paths in its public interface, and two
  path types in one small library means a conversion at every boundary.
- **Rejected because**: consistency inside the library. This part is easy to reverse.

## Consequences

- **Gains**:
  - A failed write leaves the old document untouched and the caller told.
  - Every writer shares one pretty-printer, so the next writer does not copy one.
- **Costs**:
  - The process needs permission to create files in the target's directory, not only to write
    the target.
  - A tool watching the directory sees a create and a rename rather than a modify.
  - Nothing is flushed to disk, so the write is safe against a crash but not against power loss.
  - On Windows a rename onto a file another process holds open fails, where truncating might have
    worked.
  - A temporary file left by a process killed before the rename is not cleaned up until the next
    write to that path.
  - A caller cannot ask for compact output, even for a large document.
- **Revisit when**: a document must survive power loss, or a writer needs output nobody will read.
