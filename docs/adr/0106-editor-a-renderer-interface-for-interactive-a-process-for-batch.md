# ADR-0106: Editor: a renderer interface for interactive, a process for batch

**Status**: accepted
**Date**: 2026-10-25
**Documented in**: [editor/Rendering.md](../editor/Rendering.md)

## Context

The editor renders a view while the user works, and later re-renders as edits arrive, both in a
render window and in a rendered viewport for look development. It also renders ranges of frames
from its cameras, which a render farm or another RenderMan renderer may run instead of moya.
An interactive render needs pictures as they improve, a way to stop, and the scene's changes as
they happen. A batch render needs a scene file and a program that turns it into an image, with
nothing of the editor in memory.

## Decision

The editor drives an interactive render through an interface of its own, which keeps a render up
to date with the scene and reports its progress and pictures. moya is the first renderer behind
it, linked into the editor and run off the main thread. A batch render writes a RIB file per
frame and runs `moya.exe` on each.

## Alternatives

### Link moya for everything
- **For**: one path, no executable to find, and the picture never passes through a file.
- **Against**: a batch holds the whole render in the editor's memory, nothing outside the
  editor can run the frames, and the editor depends on moya's render context.
- **Rejected because**: batch and farm rendering need a scene file and a standalone renderer.

### Run moya.exe for everything
- **For**: a renderer failure never takes the editor down, and cancelling is ending a process.
- **Against**: an interactive render needs a protocol for progress, pictures and edits before it
  gives anything more than a finished file.
- **Rejected because**: interactive rendering is defined by what passes between the editor and
  the renderer, which an interface states and a command line does not.

## Consequences

- **Gains**:
  - Render View, a later live render and a later rendered viewport all speak to one interface.
  - A renderer can be swapped, or moved into a process of its own, behind that interface.
  - Batch frames are files that a farm or another renderer can render, in parallel.
- **Costs**:
  - Two ways into moya to keep: the linked library and the executable.
  - A renderer failure during an interactive render takes the editor down with it.
  - The editor finds `moya.exe` beside itself, so the build has to put it there.
- **Revisit when**: an interactive render has to survive a renderer failure, or a farm needs a
  scene format other than RIB.
