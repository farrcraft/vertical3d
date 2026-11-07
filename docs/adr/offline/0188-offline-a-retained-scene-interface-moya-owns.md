# ADR-0188: Offline: a retained scene interface moya owns

**Status**: accepted
**Date**: 2026-11-06
**Tags**: offline, editor, interchange
**Documented in**: [roadmap/MoyaRebuild.md](../../roadmap/MoyaRebuild.md)

## Context

moya reads a scene from RIB, from the editor while a user works, and from other pipelines,
which describe scenes in USD. The editor sends a scene, edits it, renders again and cancels.
RIB describes a scene as a stream of requests against a graphics state, and USD as a stage of
prims with time samples. Neither holds what an edit must rebuild: an acceleration structure, a
compiled material or a light tree. Motion and instancing have to be carried whatever the source.

## Decision

moya's public interface is a retained scene of its own: geometry prototypes and their instances,
materials, lights, cameras and render outputs, each created, modified and deleted by handle.
Transformations and geometry carry samples over the shutter. RIB, the editor and a Hydra render
delegate are each a client that translates into it.

## Alternatives

### Hydra and USD as the internal model
- **For**: USD input, motion samples and instancers come with it, as in Karma.
- **Against**: Hydra delivers a scene to a renderer and does not hold its rendering data, so
  Karma also translates into structures of its own. moya would follow OpenUSD's versions, and a
  render delegate must match its host's USD version exactly.
- **Rejected because**: moya would still need its own scene, and would also be bound to USD.

### Translate RIB into USD and read only USD
- **For**: One input path.
- **Against**: RIB's attribute stack and its shader binding have no USD schema. Interactive
  edits still need a protocol beneath USD.
- **Rejected because**: It loses RIB's meaning and still leaves the scene interface to design.

### Keep the RIB handler interface as the API
- **For**: It exists, and the editor already drives it.
- **Against**: It is a stream of requests with no handles, so nothing can be edited after it
  is sent. It describes the RenderMan graphics state, not moya's scene.
- **Rejected because**: An interactive render needs to modify one object without resending the
  scene.

## Consequences

- **Gains**:
  - An edit rebuilds only what it touched, so the editor's renders restart quickly.
  - A third-party pipeline drives moya through the same interface as the editor, or through
    Hydra.
  - RIB keeps working, as a client that tracks RI's graphics state.
- **Costs**:
  - moya designs and keeps stable a scene schema of its own.
  - Each source needs a translator, and a Hydra delegate needs the USD port built with Hydra
    and without OpenGL.
  - ADR-0023 and ADR-0025 describe RIB as the scene format, and are amended.
- **Revisit when**: USD gains a standard interface for retained renderer scenes that pipelines
  adopt.
