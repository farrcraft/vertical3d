# ADR-0001: Rendering: replace OpenGL with Vulkan

**Status**: accepted
**Date**: 2026-08-30
**Documented in**: [internals/realtime/](../internals/realtime/README.md)

## Context

The realtime renderer was built on OpenGL through GLEW, with code that in places predates
programmable shaders. The move from SDL2 to SDL3 already disturbs the window and context layer.
The graphics API underneath governs every part of the render layer, so it has to be settled
before that layer is rebuilt.

## Decision

Vulkan replaces OpenGL entirely. There is one backend, with no OpenGL fallback, and the OpenGL
and GLEW dependencies leave the tree.

## Alternatives

### Modernise the existing OpenGL path
- **For**: much the smallest change. The existing canvas, shader program and vertex buffer
  abstractions survive, and something keeps rendering throughout.
- **Against**: OpenGL is in maintenance across the industry, and it does not expose GPU
  features the engine may want later.
- **Rejected because**: it keeps the engine on an API with no future and rules out the
  features that motivated the move.

### Keep both backends behind an abstraction
- **For**: a fallback remains for machines without a usable Vulkan driver.
- **Against**: every render feature is built twice, behind an abstraction that has to be right
  before either backend is finished.
- **Rejected because**: the tree already showed the cost. Two backends behind one interface
  produced an operation type whose two halves did not agree
  ([ADR-0003](0003-rendering-one-engine-for-2d-and-3d.md)).

## Consequences

- **Gains**:
  - Explicit control over submission, synchronisation and memory. The performance gain comes
    from this control.
  - Modern GPU features are within reach.
  - Every render feature is built once, against one backend.
- **Costs**:
  - Every app renderer is rewritten rather than ported.
  - Far more code is needed to reach the first pixel than OpenGL needs.
  - Machines without a Vulkan driver are not supported, and there is no software fallback.
- **Revisit when**: a target environment without a Vulkan driver has to be supported.
