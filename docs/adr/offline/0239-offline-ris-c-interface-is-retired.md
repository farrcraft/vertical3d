# ADR-0239: Offline: RI's C interface is retired

**Status**: accepted
**Date**: 2026-11-15
**Tags**: offline, interchange
**Documented in**: [offline/README.md](../../offline/README.md)

## Context

moya declares RI's C entry points, such as `RiBegin` and `RiSphere`, and turns each call into the
request a RIB file would carry. The retained scene is moya's API, and RIB is one of its clients.
RI's calls form a stream with no handles, which cannot change a scene once it is made. The entry
points have C++ linkage inside a static library, so no C program or foreign binding can link them,
and nothing in the tree but their own tests calls them. RenderMan kept RI for batch work alone once
its interactive rendering and its Hydra delegate moved to a scene interface of handles.

## Decision

RI's C entry points are deleted, with the converter and the stack of handlers behind them. moya's
API is the retained scene in C++, and RIB is a file the scene client reads. A C interface for other
programs, if moya needs one, is a decision of its own over the retained scene.

## Alternatives

### An adapter over the scene client
- **For**: A C program renders through the new core, and moya still implements RI.
- **Against**: It keeps a stream with no handles and no edits as an API, beside the RIB file that
  carries the same stream. It is not yet a C ABI, and would need C linkage and a shared library.
- **Rejected because**: It serves no caller the scene interface does not serve better.

### A RIB writer
- **For**: RI's own reading of `RiBegin` given a file name, as Pixar's and Aqsis's libraries
  gave it.
- **Against**: It belongs to a RIB library, not to a renderer, and the editor already writes RIB
  through the C++ writer.
- **Rejected because**: No caller needs a second way to write RIB.

## Consequences

- **Gains**:
  - One API, the retained scene, and one way in from RIB.
  - The request to give RI's objects a handle table falls away with the interface.
- **Costs**:
  - moya no longer implements the RenderMan Interface as a C API.
  - Nothing drives moya from C until a scene interface in C is decided.
- **Revisit when**: Another program, or a binding in another language, needs to build moya's
  scene.
