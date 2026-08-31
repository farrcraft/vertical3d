# ADR-0003: Realtime Engine — One Vulkan Engine Rather Than Separate 2D And 3D

**Date**: 2026-08-30
**Status**: accepted
**Deciders**: Joshua Farr

## Context

`api/render/realtime` held two engines built on different technologies: `Engine2D` over
`SDL_Renderer`, and `Engine3D` moving to Vulkan by [ADR-0001](0001-vulkan-replaces-opengl.md).
The split looks dimensional but is not — pong and tetris are both 2D games and both already
run on the *3D* engine, and odyssey is the only consumer of the 2D one. Deciding this before
the frame loop is written matters, because one engine and two engines want different frame
abstractions.

## Decision

`Engine2D` and `Engine3D` merge into one Vulkan engine that handles both 2D and 3D work. The
unit of variation is the render pass, not the engine: a frame is a list of passes, each with
its own target, camera, depth configuration and ordering.

## Alternatives Considered

### Alternative 1: One engine, passes as the unit of variation — **chosen**
- **Pros**: What differs between 2D and 3D drawing — camera and projection, depth testing,
  sort order, pipeline state — is per-pass, not per-engine. A 2D game can then use a 3D or
  post-process pass for effects without switching engines. Every render feature is built
  once. `Scene::collect()` returning a `Frame` already has this shape.
- **Cons**: Loses the `SDL_Renderer` software fallback, and its free render-target and
  logical-presentation handling, all of which must be rebuilt on Vulkan.
- **Why not**: n/a — chosen.

### Alternative 2: Keep both engines
- **Pros**: `SDL_Renderer` is written, debugged, portable and has a software fallback.
  Odyssey keeps working through the whole Vulkan transition at no cost.
- **Cons**: Every feature built twice — text, sprites, render targets, compositing. The 2D
  engine is already half-finished: `Scene2D::collect()` returns an empty frame.
- **Why not**: The tax is already visible in the tree. `Operation::run` takes
  `shared_ptr<Context>` while `Operation2D::run` takes `shared_ptr<Context2D>`, so nothing
  overrides the pure virtual and every 2D operation is abstract. Two backends cracked the
  shared abstraction.

### Alternative 3: One engine with explicit 2D and 3D modes chosen at initialisation
- **Pros**: Simpler than passes, and closer to how the split reads today.
- **Why not**: A mode is chosen once, which makes mixed use — 2D content with a 3D or
  compositing pass — a special case again. Passes make it ordinary.

## Consequences

### Positive
- One `Context`, which makes the `run(Context)` versus `run(Context2D)` mismatch
  unwriteable rather than merely fixed.
- `Window2D` and `Window3D` collapse into one `Window`; they already differ only by the
  Vulkan flag and a logical-size field.
- Compositing and post-processing stop being special cases.

### Negative
- The `SDL_Renderer` software fallback is lost. Confirmed acceptable: no target environment
  without a Vulkan driver needs supporting.
- Odyssey cannot port until the Vulkan path has textured quad batching, so it stays on
  `SDL_Renderer` in the interim and the old path outlives the decision by several phases.
- Logical presentation and stretch-to-fit must be rebuilt as an offscreen target plus a
  scaled blit.

### Risks
- Losing the software fallback also removes the free route to headless render testing, which
  is what forces the approach in [ADR-0007](0007-ci-rendering-tests.md).
- If a target without a Vulkan driver ever matters, this is the decision to reopen, and the
  cost of reopening rises with every feature built only on the Vulkan path.
