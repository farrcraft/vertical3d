# ADR-0253: Offline: a render is a session that takes edits between passes

**Status**: accepted
**Date**: 2026-11-20
**Tags**: offline, editor
**Documented in**: [offline/Interactive.md](../../offline/Interactive.md)

## Context

The editor renders while the user works, and each edit should reach the picture within a moment.
A render builds its hierarchies, materials, textures, volumes, lights and film from nothing, and
for a scene of any size that build takes longer than a pass. The scene already gathers edits into
commits and stamps what each commit changed, so a consumer can rebuild only that. Cycles keeps one
`Session` that resets its buffers on a scene update between samples, and Arnold interrupts a
render session, takes node edits and restarts it. RenderMan's live render keeps its scene loaded
and restarts the image after each edit, and each of the three renders a batch the same way.

## Decision

A render is a session that keeps every structure it builds across versions of the scene. Between
two passes it applies the batches committed since the last, rebuilds only what they changed, and
starts the film again unless nothing a picture shows was changed. `render()` is a session that
takes no edits and runs to its end, so a batch and an interactive render take one path.

## Alternatives

### A render started again for each edit, with structures the caller keeps
- **For**: `render()` keeps its shape, and the build of the unchanged structures is still saved.
- **Against**: Every edit still cancels the render, stops its threads and makes its film again.
  Each caller holds the kept structures and has to pass them to the right render, so the editor
  and a Hydra delegate would each hold a session in all but name.
- **Rejected because**: The kept state needs one owner, and that owner is a session.

### A render started again from nothing, with each structure built in parallel
- **For**: Nothing is kept, so nothing can be kept wrongly, and the oracle holds by construction.
- **Against**: The time from an edit to a picture grows with the scene, however many threads build
  it. A camera move rebuilds the whole scene, which the user sees as a pause.
- **Rejected because**: An interactive render must not cost a full build for an edit that touched
  one object.

## Consequences

- **Gains**:
  - An edit costs what it touched: a camera move rebuilds nothing but the film.
  - moya.exe, the tests and the editor render through one path, so the reference renders hold the
    interactive render to the same bytes.
  - A Hydra render delegate maps onto a session directly, since Hydra syncs dirty prims into a
    renderer that lives across edits.
- **Costs**:
  - Every structure needs an update from a change set as well as a build from nothing, and a kept
    structure that disagrees with a built one is a bug only a comparison test finds.
  - A session holds its structures in memory for as long as the editor renders.
  - A checkpoint describes the version a session is rendering, so the passes before an edit are
    not in it.
- **Revisit when**: a structure's build from nothing becomes faster than a pass for the scenes the
  editor renders, so that keeping it gains nothing.
