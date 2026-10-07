# ADR-0003: Rendering: one engine for 2D and 3D

**Status**: accepted
**Date**: 2026-08-30
**Documented in**: [api/rendering/FramesAndTargets.md](../api/rendering/FramesAndTargets.md)

## Context

`api/render/realtime` held two engines on different technologies: a 2D engine over
`SDL_Renderer`, and a 3D engine moving to Vulkan under
[ADR-0001](0001-rendering-replace-opengl-with-vulkan.md). The split was not really by dimension,
since pong and tetris are 2D games that ran on the 3D engine. What differs between 2D and 3D
drawing is the camera and projection, depth testing, sort order and pipeline state. One engine
and two engines need different frame structures, so the choice has to come before the frame
loop.

## Decision

There is one Vulkan engine for both 2D and 3D work. The render pass, not the engine, is what
varies: a frame is a list of passes, each with its own target, camera, depth configuration and
ordering.

## Alternatives

### Keep both engines
- **For**: `SDL_Renderer` is written, debugged and portable, and has a software fallback. The 2D
  app keeps working throughout the move to Vulkan.
- **Against**: every feature is built twice: text, sprites, render targets and compositing. The
  2D engine was already incomplete.
- **Rejected because**: the cost was already visible. The 2D and 3D operation types took
  different context types, so no 2D operation could implement the shared interface.

### One engine with a 2D or 3D mode chosen at start-up
- **For**: simpler than passes, and closer to how the old split read.
- **Against**: a mode is chosen once, so 2D content with a 3D or compositing pass becomes a
  special case.
- **Rejected because**: passes make mixed use ordinary rather than special.

## Consequences

- **Gains**:
  - One context type and one window type, so the 2D and 3D interfaces cannot drift apart.
  - A 2D game can add a 3D or post-processing pass without switching engines.
  - Compositing, offscreen targets and several viewports are all more passes over one frame.
- **Costs**:
  - The `SDL_Renderer` software fallback is gone, on the grounds that no target environment
    lacks a Vulkan driver.
  - Losing it also removes the free route to headless render tests.
    [ADR-0007](0007-ci-render-tests-on-software-vulkan.md) is needed because of this.
  - Logical presentation and stretch-to-fit, free under `SDL_Renderer`, have to be rebuilt as
    an offscreen target and a scaled draw.
- **Revisit when**: a target without a Vulkan driver has to be supported. The cost of reopening
  this grows with every feature built only on Vulkan.
