# ADR-0041: Writing A File — A Document Is Written Whole Or Not At All, And Readably

**Date**: 2026-09-07
**Status**: accepted
**Deciders**: Joshua Farr

## Context

`api/asset` reads documents and does not write one. The tree has one writer —
`editor/scene/Project::write` — and it opens its target with `std::ios::trunc`, so the previous
project is destroyed before a byte of the new one exists and a crash, a full disk or a lost
drive in between leaves the player with neither. Beside it sits 133 lines of pretty-printer in
an anonymous namespace, reachable from that translation unit only, written because
`boost::json::serialize` puts a document on one line.

Two more writers are coming: a settings overlay under `engine::userPath()`, and a save format
that chose JSON over SQLite specifically because a person can read and diff it. Both would
otherwise copy both halves, and both would copy the truncation with them.

## Decision

**A document is written to a temporary file beside its target and renamed onto it, so the
previous document survives every failure except the rename itself.** The library writes bytes
and serializes a `boost::json::value`; what the document holds stays the caller's, exactly as
reading works today.

**Readable output is the format, not an option.** There is no one-line mode and no flag,
because every document this tree writes is one a person will open.

## Alternatives Considered

### Alternative 1: A temp file beside the target, then rename — **chosen**
- **Pros**: The rename is the only step that can lose the old document, and on Windows and on
  POSIX alike it is the one step the filesystem makes atomic against a crash. Beside the target
  rather than in the system temp directory, because a rename across volumes is a copy and a copy
  is not atomic — which is the whole point. A failed write leaves the old file untouched and the
  caller told; a failed rename additionally removes the partial temp file, so the directory is
  left as it was found.
- **Cons**: Needs write permission in the target's directory, not just to the target. Leaves a
  visible sibling file for the duration of the write, which a directory watcher will see.
- **Why not**: n/a — chosen.

### Alternative 2: Truncate and write in place, as the editor does today
- **Pros**: One open, no rename, no temp file, and no permission needed beyond the file itself.
- **Cons**: It is the defect. The window between the truncate and the flush is the whole write,
  and everything in it costs the player both copies.
- **Why not**: It is what this record exists to stop.

### Alternative 3: Write in place, but keep a `.bak` of the previous document first
- **Pros**: The old document is recoverable, and a player can see that it is.
- **Cons**: Two windows instead of one — a crash during the backup copy, and a crash during the
  write — and recovery is manual, so it only helps a player who knows the convention. It also
  doubles what the directory holds forever.
- **Why not**: It converts a lost file into a file the player has to know how to restore. The
  rename makes the failure invisible instead.

### Alternative 4: A write mode flag, compact or indented
- **Pros**: A caller with a large document could skip the printer, and `boost::json::serialize`
  is faster than walking the value.
- **Cons**: Every caller then decides, and a document written compact once is a document nobody
  can diff at the moment they need to. No writer in the tree or on the roadmap wants the compact
  form: a project, a settings overlay and a save file are all read by people.
- **Why not**: A knob with one correct setting is not a knob.

### Alternative 5: `std::filesystem` for the rename rather than `boost::filesystem`
- **Pros**: Standard, no dependency, and `std::filesystem::rename` has the same replace-on-
  Windows semantics.
- **Cons**: `api/asset` already links `Boost::filesystem` publicly for `Manager::path_`, and two
  path types in one small library is a conversion at every boundary between them.
- **Why not**: Consistency inside the library it lands in. This is reversible in a way the rest
  of this record is not.

## Consequences

### Positive
- The editor's save stops being able to eat a project, and it gets there by deleting code rather
  than by adding any.
- The printer becomes the library's: scalars and vectors kept on a line, records compacted,
  floats through `std::to_chars` so a coordinate does not print as `0.10000000149011612`. The
  next two writers get it for free, which is what stops a third copy.
- A caller that already holds a `boost::json::value` needs one call, and one that holds bytes of
  its own — a text format, an image — needs the same call one level down.

### Negative
- A directory the process cannot create a file in is now a directory it cannot write to at all,
  where truncating in place would have worked. That is the trade being bought.
- The temp file is visible while the write runs, so a tool watching the directory sees a create
  and a rename rather than a modify.
- Nothing here makes the write durable against power loss: the rename is atomic with respect to
  a crash, and neither the data nor the directory entry is fsynced. A player who loses power
  mid-save may find either document, and will not find a half of one.

### Risks
- **A rename onto an open file fails on Windows.** If the target is held by another process the
  write fails cleanly and the caller is told, which is correct but is a failure mode the
  truncating path did not have. The escape hatch is the caller's: report it and let the player
  close whatever holds the file.
- A temp file left behind by a process killed between the write and the rename is not cleaned up
  by anything. It is named from the target, so the next successful write to that path overwrites
  it rather than accumulating.
