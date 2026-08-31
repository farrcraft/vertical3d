# ADR-0001: Rendering Backend — Vulkan Replaces OpenGL Outright

**Date**: 2026-08-30
**Status**: accepted
**Deciders**: Joshua Farr

## Context

The realtime renderer was built on OpenGL through `api/gl` and GLEW, using code that in
places predates the shader era. The SDL2 to SDL3 upgrade had already disturbed the window
and context layer, which made this the moment to decide whether to modernise the GL path or
leave it. Nothing in the repository recorded why the project would move, and the choice
governs every phase of the render rewrite that follows.

## Decision

Vulkan replaces OpenGL entirely. There is no dual backend and no GL fallback; `api/gl` and
the GLEW and OpenGL dependencies are deleted once nothing references them.

## Alternatives Considered

### Alternative 1: Vulkan, GL removed — **chosen**
- **Pros**: Better performance and efficiency from explicit control over submission and
  synchronisation. A modern design rather than a decades-old global state machine. Access
  to GPU features GL does not expose and that this engine may want later.
- **Cons**: A large rewrite of everything that draws. Far more code to reach the first
  pixel. Nothing renders during the transition.
- **Why not**: n/a — chosen.

### Alternative 2: Modernise the existing OpenGL path
- **Pros**: Much the smallest change. The existing `Canvas`, `Program` and `VertexBuffer`
  abstractions survive. Something keeps rendering throughout.
- **Why not**: Leaves the engine on an API that is in maintenance across the industry, and
  forecloses the features that motivated the move.

### Alternative 3: Keep both backends behind an abstraction
- **Pros**: Retains a fallback for machines without a usable Vulkan driver.
- **Cons**: Every render feature has to be built twice, behind an abstraction that has to
  be right before either backend is finished.
- **Why not**: The codebase already demonstrates what this costs — see
  [ADR-0003](0003-one-realtime-engine.md), where a two-backend split produced an abstraction
  that does not typecheck.

### Alternative 4: SDL_GPU
- **Pros**: SDL3 is already a dependency, and `SDL_GPU` abstracts Vulkan, Metal and D3D12
  with far less hand-written backend code. Portability comes free.
- **Why not**: Not evaluated at the time the direction was set. Recorded here because a
  future reader will ask, and because it would have covered much of the same ground.

## Consequences

### Positive
- Explicit control over submission, synchronisation and memory, which is where the
  performance argument actually lives.
- Modern GPU features become available rather than being permanently out of reach.
- One backend to build every render feature against.

### Negative
- Nothing renders until the Vulkan frame loop exists, because the GL context has already
  been removed while the renderers still issue GL calls. That gap is accepted.
- "It builds" is the only available signal for the render layer until the frame loop lands.
- Every app renderer has to be rewritten, not ported.

### Risks
- The gap with nothing rendering lasts as long as it takes to reach a first pixel, and a
  long dark period makes regressions hard to attribute. Mitigated by keeping the phases
  small and pong as the pilot app.
- Losing the software fallback is a real reduction in reach, accepted in
  [ADR-0003](0003-one-realtime-engine.md) on the grounds that no target environment without
  a Vulkan driver needs supporting. That is the assumption to revisit if this ever hurts.
